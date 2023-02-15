#pragma once

#include "netTypes.h"

#include "eventReceiver.hpp"
#include "packetProcedure.hpp"


namespace mln::net {

	struct ServiceParams {
		boost::asio::io_context& ioc_;
		EventReceiver& receiver_;
		PacketProcedure::CustomPacketParser packetParser_;
		PacketManipulator* manip_;
		size_t serviceUpdateTimeMs_ = 0;
		size_t keepAliveTimeMs_ = 0;
	};

	struct AcceptorUserParams {
		std::string addr;
		uint16_t port;
		size_t workerThreadsCount;
		bool usePortReuseChecking = false;
		SessionType sessionType = SessionType::TCP;
	};

	struct ConnectorUserParams {
		std::string addr;
		uint16_t port;
		SessionType sessionType = SessionType::TCP;
	};

}//namespace mln::net {