#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>

#include "netTypes.h"
#include "session.hpp"
#include "packetProcedure.hpp"
#include "eventReceiver.hpp"
#include "netCommonObjects.hpp"

namespace mln::net {
	class NetServiceAcceptorTcp
	{
	public:
		NetServiceAcceptorTcp(
			ServiceParams& svcParams
			, AcceptorUserParams& acceptorParams
		)
			: _netObj(svcParams)
			, _acceptorSocket(svcParams.ioc_)
		{
		}

		bool acceptWait(const std::string& addr, const uint16_t port
			, size_t workerThreadsCount
		){
			if (0 == workerThreadsCount) {
				workerThreadsCount = boost::thread::hardware_concurrency();
			}

			using TCP = boost::asio::ip::tcp;

			auto session = Session::create(
				SessionType::TCP
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

			_acceptorSocket.open(endpoint.protocol(), ec);

			_acceptorSocket.set_option(TCP::no_delay(true));
			_acceptorSocket.set_option(TCP::acceptor::reuse_address(true));
			_acceptorSocket.set_option(boost::asio::socket_base::linger(true, 0));
			_acceptorSocket.bind(endpoint, ec);

			if (ec) {
				LOGE(ec.message());
				return false;
			}

			_acceptorSocket.listen(boost::asio::socket_base::max_connections, ec);
			if (ec) {
				LOGE(ec.message());
				return false;
			}

			_acceptorSocket.async_accept(session->socket()
				, boost::asio::bind_executor(_netObj._strand, boost::bind(
					&NetServiceAcceptorTcp::handleAccept, this, boost::asio::placeholders::error, session)));


			_netObj.expireTimerReady();

			LOGD("starting tcpsocket-listen({}/{})...", ipAddr, port);

			return true;
		}

		void handleAccept(const boost::system::error_code& ec, Session::sptr session) {
			if (!ec) {
				[[likely]]
				_netObj._eventReceiver.onAccept(session);

				session->startAccept();

				auto newSession = Session::create(SessionType::TCP
					, _netObj._ioc
					, std::bind(&PacketProcedure::dispatch, _netObj._packetProc.get(), std::placeholders::_1, std::placeholders::_2)
					, _netObj._packetProc->getManip()
					, &_netObj._eventReceiver
					, _netObj._keepAliveTimeMs, 0);
				newSession->setServiceID(_serviceIndex);

				_acceptorSocket.async_accept(newSession->socket()
					, boost::asio::bind_executor(_netObj._strand, boost::bind(
						&NetServiceAcceptorTcp::handleAccept, this, boost::asio::placeholders::error, newSession)));
			}
			else {
				[[unlikely]]
				LOGE("failed handleAccept(). code:{}, msg:{}", ec.value(), ec.message());
			}
		}

	public:
		NetCommonObjects _netObj;

	private:
		boost::asio::ip::tcp::acceptor	_acceptorSocket;

		size_t _serviceIndex;

		//------------------------------------

		
	};
}//namespace mln::net {