#pragma once

#include <string>
#include <boost/asio.hpp>
#include "netTypes.h"
#include "session.hpp"
#include "packetProcedure.hpp"
#include "eventReceiver.hpp"
#include "netCommonObjects.hpp"

namespace mln::net {

	class NetServiceConnectorTcp
	{
	public:
		NetServiceConnectorTcp(
			ServiceParams& svcParams
			, ConnectorUserParams& connectorParams
		)
			: _netObj(svcParams)
		{
			setTargetServer(connectorParams.addr, connectorParams.port);
		}

		void setTargetServer(const std::string& addr, const uint16_t& port) {
			_targetAddr = addr;
			_targetPort = port;
		}

		Session::sptr connect() {
			return connectSync(_targetAddr, _targetPort);
		}

		void connectWait(
			const uint16_t sessionCnt = 1
			/*, size_t workerThreadsCount = 1*/
			, const size_t connectionID = 0
		) {
			/*if (0 < workerThreadsCount) {
				NetService::addWorkerThreads(workerThreadsCount, _ios);
			}*/

			connectAsync(_targetAddr, _targetPort, sessionCnt, connectionID);
		}

	private:
		Session::sptr connectSync(const std::string& addr, const uint16_t port, const size_t connectionID = 0) {

			if (false == setEndPoint(addr, port)) {
				[[unlikely]]
				return nullptr;
			}

			try {
				auto session = createConnectSession(connectionID);

				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				session->socket().connect((*_iterEndPoint)->endpoint());
				if (false == handleConnectSync((*_iterEndPoint), session)) {
					LOGE("failed makeSession. connectionID : {}", connectionID);
					return nullptr;
				}
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				_netObj.expireTimerReady();

				return session;
			}
			catch (std::exception& e) {
				LOGE("Connection failed. Exception: {}", e.what());
				return nullptr;
			}
		}

		bool connectAsync(
			const std::string& addr, const uint16_t port
			, const uint16_t sessionCnt, const size_t connectionID = 0
		){
			assert(0 < sessionCnt);

			if (false == setEndPoint(addr, port)) {
				[[unlikely]]
				return false;
			}

			try {
				auto session = createConnectSession(connectionID);

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				session->socket().async_connect((*_iterEndPoint)->endpoint()
					, boost::asio::bind_executor(_netObj._strand, boost::bind(
						&NetServiceConnectorTcp::handleConnectAsync
						, this
						, boost::asio::placeholders::error
						, (*_iterEndPoint), session, sessionCnt - 1)));
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				_netObj.expireTimerReady();

			}//try
			catch (std::exception& e)
			{
				LOGE("Connection failed. Exception:{}", e.what());
				return false;
			}

			return true;

		}

		bool setEndPoint(const std::string& addr, const uint16_t port) {
			
			using TCP = boost::asio::ip::tcp;

			std::string ipAddr;
			if (true == addr.empty()) {
				ipAddr = "127.0.0.1";
			}
			else {
				ipAddr = addr;
			}

			try {
				if (!_resolver) {
					_resolver = std::make_unique<TCP::resolver>(_netObj._ioc);
					_query = std::make_unique<TCP::resolver::query>(ipAddr.c_str(), std::to_string(port));
					_iterEndPoint = std::make_unique<TCP::resolver::iterator>();
					*_iterEndPoint = (*_resolver).resolve(*_query);
				}

				if (TCP::resolver::iterator() == *_iterEndPoint) {
					assert(false && "There are no more endpoints to try. Shut down the client.");
					return false;
				}
			}
			catch (std::exception& e)
			{
				LOGE("setEndPoint failed. Exception: {}", e.what());
				return false;
			}

			auto endPoint = (*_iterEndPoint)->endpoint();
			LOGD("set endPoint. {}/{}", endPoint.address().to_string(), endPoint.port());
			return true;
		}

		bool handleConnectAsync(
			const boost::system::error_code& ec
			, boost::asio::ip::tcp::resolver::iterator iterEndpoint
			, Session::sptr session
			, const uint16_t remainCnt
		) {
			// The async_connect() function automatically opens the socket at the start
			// of the asynchronous operation. If the socket is closed at this time then
			// the timeout handler must have run first.
			if (false == session->socket().is_open()) {
				LOGD("Connecting timed-out");
				session->socket().close();
				_netObj._eventReceiver.onConnectFailed(session);
				return false;
			}

			if (ec) {// Check if the connect operation failed
				LOGE("Connect error: {}", ec.message());
				session->socket().close();
				_netObj._eventReceiver.onConnectFailed(session);
				return false;
			}

			auto endPoint = (*_iterEndPoint)->endpoint();
			LOGD("Connected to {}/{}", endPoint.address().to_string(), endPoint.port());
			session->saveEndPoint();

			session->startConnect();
			_netObj._eventReceiver.onConnect(session);

			if (0 < remainCnt) {
				using TCP = boost::asio::ip::tcp;

				try {
					session = createConnectSession(0);
					session->socket().async_connect(iterEndpoint->endpoint()
						, boost::asio::bind_executor(_netObj._strand
							, boost::bind(
								&NetServiceConnectorTcp::handleConnectAsync
								, this
								, boost::asio::placeholders::error
								, iterEndpoint, session, remainCnt - 1)));
				}
				catch (std::exception& e) {
					LOGE("Connection failed. Exception: {}", e.what());
					return false;
				}
			}
			return true;
		}

		bool handleConnectSync(
			boost::asio::ip::tcp::resolver::iterator iterEndpoint
			, Session::sptr session
		){
			auto endPoint = (iterEndpoint)->endpoint();
			LOGD("Connected to {}/{}", endPoint.address().to_string(), endPoint.port());

			session->saveEndPoint();
			session->startConnect();

			_netObj._eventReceiver.onConnect(session);

			return true;
		}
		
		Session::sptr createConnectSession(const size_t connectionID) {
			using TCP = boost::asio::ip::tcp;

			auto session = Session::create(
				SessionType::TCP
				, _netObj._ioc
				, std::bind(&PacketProcedure::dispatch, _netObj._packetProc.get(), std::placeholders::_1, std::placeholders::_2)
				, _netObj._packetProc->getManip()
				, &_netObj._eventReceiver
				, _netObj._keepAliveTimeMs
				, connectionID
			);

			session->setServiceID(_netObj._index);

			session->socket().open((*_iterEndPoint)->endpoint().protocol());

			session->socket().set_option(boost::asio::socket_base::linger(true, 0));
			session->socket().set_option(TCP::no_delay(true));
			session->socket().set_option(TCP::acceptor::reuse_address(true));

			return session;

		}

	public:
		NetCommonObjects _netObj;

	private:
		std::string _targetAddr;
		uint16_t _targetPort;

		std::unique_ptr< boost::asio::ip::tcp::resolver> _resolver;
		std::unique_ptr< boost::asio::ip::tcp::resolver::query> _query;
		std::unique_ptr< boost::asio::ip::tcp::resolver::iterator> _iterEndPoint;
	};
};//namespace mln::net {