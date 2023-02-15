#pragma once

#include "netTypes.h"
#include "session.hpp"
#include "packetProcedure.hpp"
#include "eventReceiver.hpp"
#include "netCommonObjects.hpp"

namespace mln::net {
	class NetServiceAcceptor
	{
	public:
		NetServiceAcceptor(
			ServiceParams& svcParams
			, AcceptorUserParams& acceptorParams
		)
			: _netObj(svcParams)
			, _acceptorSocketTcp(svcParams.ioc_)
			, _acceptorSocketWeb(svcParams.ioc_)
		{
		}

		bool acceptWait(const std::string& addr
			, const uint16_t portTcp
			, const uint16_t portWebsocket
			, size_t workerThreadsCount
		){
			if (0 == workerThreadsCount) {
				workerThreadsCount = boost::thread::hardware_concurrency();
			}

			using TCP = boost::asio::ip::tcp;

			auto createSession = [&](
				 const SessionType sessionType
				, const uint16_t port
				, TCP::acceptor& acceptorSocket
				) ->std::optional<Session::sptr> {

				auto session = Session::create(
					sessionType
					, _netObj._ioc
					, std::bind(&PacketProcedure::dispatch, _netObj._packetProc.get(), std::placeholders::_1, std::placeholders::_2)
					, _netObj._packetProc->getManip()
					, &_netObj._eventReceiver
					, _netObj._keepAliveTimeMs
					, 0
				);
				session->setServiceID(_serviceIndex);

				TCP::resolver resolver(_netObj._ioc);

				std::string ipAddr;
				if (true == addr.empty()) {
					ipAddr = "0.0.0.0";
				}
				else {
					ipAddr = addr;
				}

				boost::system::error_code ec;

				TCP::resolver::query query(ipAddr.c_str(), std::to_string(port));
				TCP::endpoint endpoint = *resolver.resolve(query, ec);

				acceptorSocket.open(endpoint.protocol(), ec);

				acceptorSocket.set_option(TCP::no_delay(true));
				acceptorSocket.set_option(TCP::acceptor::reuse_address(true));
				acceptorSocket.set_option(boost::asio::socket_base::linger(true, 0));
				acceptorSocket.bind(endpoint, ec);

				if (ec) {
					LOGE(ec.message());
					return std::nullopt;
				}

				acceptorSocket.listen(boost::asio::socket_base::max_connections, ec);
				if (ec) {
					LOGE(ec.message());
					return std::nullopt;
				}

				return session;
			};

			// handle tcp socket
			auto sessionTcpOpt = createSession(SessionType::TCP, portTcp, _acceptorSocketTcp);
			if (false == sessionTcpOpt.has_value()) {
				LOGE("failed create AcceptSession. portTcp:{}", portTcp);
				return false;
			}
			_acceptorSocketTcp.async_accept(sessionTcpOpt.value()->socket()
				, boost::asio::bind_executor(_netObj._strand, boost::bind(
					&NetServiceAcceptor::handleAccept, this, boost::asio::placeholders::error, sessionTcpOpt.value())));


			// handle web socket
			auto sessionWebOpt = createSession(SessionType::WEBSOCKET, portWebsocket, _acceptorSocketWeb);
			if (false == sessionWebOpt.has_value()) {
				LOGE("failed create AcceptSession. portWebsocket:{}", portWebsocket);
				return false;
			}
			_acceptorSocketWeb.async_accept(sessionWebOpt.value()->socket()
				, boost::asio::bind_executor(_netObj._strand, boost::bind(
					&NetServiceAcceptor::handleWebsocketLayer, this, boost::asio::placeholders::error, sessionWebOpt.value())));


			_netObj.expireTimerReady();

			LOGD("starting server. ip:{}, portTcp:{}, portWebsocket:{}", addr, portTcp, portWebsocket);

			return true;
		}

		void handleWebsocketLayer(const boost::system::error_code& ec, Session::sptr session) {

			using namespace boost::beast;

			if (!ec) {
				auto& websocket = session->websocket();
				
				if (! _netObj._keepAliveTimeMs) {
					get_lowest_layer(websocket).expires_never();
				}
				else {
					get_lowest_layer(websocket).expires_after(
						std::chrono::milliseconds(_netObj._keepAliveTimeMs)
					);
				}

				websocket.set_option(websocket::stream_base::timeout::suggested(role_type::server));
				websocket.set_option(websocket::stream_base::decorator(
					[](websocket::response_type& res) {
					res.set(http::field::server,
						std::string(BOOST_BEAST_VERSION_STRING) +
						" websocket-server");
				}));

				websocket.async_accept(
					boost::asio::bind_executor(_netObj._strand
						, boost::bind(
							&NetServiceAcceptor::handleAccept
							, this
							, boost::asio::placeholders::error
							, session)));
			}
			else {
				[[unlikely]]
				LOGE("failed handleWebsocketLayer(). code:{}, msg:{}", ec.value(), ec.message());
			}
		}

		void handleAccept(const boost::system::error_code& ec, Session::sptr session) {
			if (!ec) {
				[[likely]]

				session->saveEndPoint();

				_netObj._eventReceiver.onAccept(session);

				session->startAccept();

				auto newSession = Session::create(session->_sessionType
					, _netObj._ioc
					, std::bind(&PacketProcedure::dispatch, _netObj._packetProc.get(), std::placeholders::_1, std::placeholders::_2)
					, _netObj._packetProc->getManip()
					, &_netObj._eventReceiver
					, _netObj._keepAliveTimeMs, 0);
				newSession->setServiceID(_serviceIndex);


				if (SessionType::TCP == newSession->_sessionType) {
					_acceptorSocketTcp.async_accept(newSession->socket()
						, boost::asio::bind_executor(_netObj._strand, boost::bind(
							&NetServiceAcceptor::handleAccept, this, boost::asio::placeholders::error, newSession)));
				}
				else {
					_acceptorSocketWeb.async_accept(newSession->socket()
						, boost::asio::bind_executor(_netObj._strand, boost::bind(
							&NetServiceAcceptor::handleWebsocketLayer, this, boost::asio::placeholders::error, newSession)));
				}
				
			}
			else {
				[[unlikely]]
				LOGE("failed handleAccept(). code:{}, msg:{}", ec.value(), ec.message());
			}
		}

	public:
		NetCommonObjects _netObj;

	private:
		boost::asio::ip::tcp::acceptor	_acceptorSocketTcp;
		boost::asio::ip::tcp::acceptor	_acceptorSocketWeb;

		size_t _serviceIndex;

		//------------------------------------

		
	};
}//namespace mln::net {