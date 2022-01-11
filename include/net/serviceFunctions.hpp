#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "eventReceiver.hpp"
#include "logger.hpp"
#include "packetProcedure.hpp"
#include "netService.hpp"
#include "serviceParamTypes.h"

namespace mln::net {

	

	/*struct ServiceParams {
		boost::asio::io_context& ioc_;
		EventReceiver& receiver_;
		PacketProcedure::CustomPacketParser packetParser_;
		PacketManipulator* manip_;
		size_t serviceUpdateTimeMs_ = 0;
		size_t keepAliveTimeMs_ = 0;

		ServiceParams(boost::asio::io_context& ioc
			, EventReceiver& receiver
			, PacketProcedure::CustomPacketParser packetParser
			, PacketManipulator* manip
			, size_t serviceUpdateTimeMs = 0
			, size_t keepAliveTimeMs = 0
		)
			: ioc_(ioc)
			, receiver_(receiver)
			, packetParser_(packetParser)
			, manip_(manip)
			, serviceUpdateTimeMs_(serviceUpdateTimeMs)
			, keepAliveTimeMs_(keepAliveTimeMs)
		{
		}

	};*/

	

	

	namespace service {
		

		

		

		//static std::shared_ptr<NetServiceConnector> createConnector(
		//	const size_t connectorIdx
		//	, ServiceParams& svcParams
		//	, ConnectorUserParams& userParams
		//) {
		//	if (s_connectors.size() <= connectorIdx) {
		//		s_connectors.resize(connectorIdx + 1);
		//	}

		//	if (nullptr == s_connectors[connectorIdx]
		//		|| s_connectors[connectorIdx]->GetSocketType() != userParams.socketType
		//		) {
		//		switch (userParams.socketType) {
		//		case SocketType::TCP:
		//			s_connectors[connectorIdx] = std::make_shared<NetServiceConnectorTcp>(svcParams, userParams);
		//			break;
		//		case SocketType::WEBSOCKET:
		//			/*s_connectors[connectorIdx] = std::make_shared<NetServiceConnectorWebsocket>(svcParams, userParams);*/
		//			break;
		//		default:
		//			LOGC("socket type is invalid, socket type: {}", userParams.socketType);
		//			throw std::runtime_error("invalid socketType");
		//		}
		//		s_connectors[connectorIdx]->setIndex(connectorIdx);
		//	}
		//	return s_connectors[connectorIdx];
		//}

		/*static std::shared_ptr<NetServiceConnector> createConnector(
			ServiceParams& svcParams
			, ConnectorUserParams& userParams) {
			return createConnector(s_connectors.size(), svcParams, userParams);
		}*/

		static std::optional<ConnectorPtrType> getConnector(const size_t connectorIdx) {
			assert(s_connectors.size() > connectorIdx);
			if (s_connectors.size() <= connectorIdx) {
				return std::nullopt;
			}
			return s_connectors[connectorIdx];
		}
		
		


	}//namespace service {

}//namespace mln::net {
