#pragma once

#include <atomic>
#include <memory>
#include "eventReceiver.hpp"
#include "netServiceAcceptorTcp.hpp"
#include "netServiceConnectorTcp.hpp"
//#include "serviceFunctions.hpp"
#include "serviceParamTypes.h"
#include "packetJson/packetParser.hpp"

namespace mln::net {

	class NetServiceConnector;

	class NetService
	{
	public:
		using PostStartCallbackType = std::function<void()>;

		using AcceptorPtrType = std::shared_ptr<NetService>;
		using ConnectorPtrType = std::shared_ptr<NetService>;

		using AcceptorContainerType = std::vector< AcceptorPtrType >;
		using ConnectorContainerType = std::vector< ConnectorPtrType >;

		inline static AcceptorContainerType s_acceptors;
		inline static ConnectorContainerType s_connectors;
		inline static boost::asio::io_context* s_ioc = nullptr;
		inline static std::vector<std::shared_ptr<boost::thread>> s_threads;

	public:
		NetService(ServiceParams& svcParams, AcceptorUserParams& acceptorParams
			, const size_t connectorIdx = 0
		) {
			_accepterTcp = std::make_unique<NetServiceAcceptorTcp>(
				svcParams
				, acceptorParams
				);
			_accepterTcp->_netObj.setIndex(connectorIdx);
		}


		NetService(ServiceParams& svcParams, ConnectorUserParams& connectorParam
			, const size_t connectorIdx = 0
		) {
			_connectorTcp = std::make_unique<NetServiceConnectorTcp>(
				svcParams
				, connectorParam
				);

			_connectorTcp->_netObj.setIndex(connectorIdx);
		}

	public:
		std::shared_ptr< NetServiceAcceptorTcp > _accepterTcp;
		std::shared_ptr< NetServiceConnectorTcp > _connectorTcp;

	protected:
		inline static std::atomic< size_t > s_identitySeed = { 1 };




	public:
		static size_t addWorkerThreads(const size_t workerCount
			, std::optional< boost::asio::io_context* > optIoc = std::nullopt
		) {
			if (optIoc.has_value()) {
				if (!s_ioc) {
					s_ioc = optIoc.value();
				}
			}

			if (nullptr == s_ioc) {
				throw std::runtime_error("set io-context first");
			}

			for (size_t i = 0; i < workerCount; ++i) {
				std::shared_ptr<boost::thread> thread(new boost::thread(
					boost::bind(&boost::asio::io_context::run, s_ioc)));

				s_threads.push_back(thread);
			}

			return s_threads.size();
		}

		static AcceptorPtrType createTcpAcceptor(const size_t acceptorIdx
			, ServiceParams& svcParams
			, AcceptorUserParams& userParams
		) {
			if (!s_ioc) {
				s_ioc = &svcParams.ioc_;
			}

			if (s_acceptors.size() <= acceptorIdx) {
				s_acceptors.resize(acceptorIdx + 1);
			}

			if (!s_acceptors[acceptorIdx]) {
				s_acceptors[acceptorIdx] = std::make_shared<NetService>(svcParams, userParams, acceptorIdx);

				if (s_acceptors[acceptorIdx]->_accepterTcp->acceptWait(
					userParams.addr
					, userParams.port
					, userParams.workerThreadsCount
				)) {

					addWorkerThreads(userParams.workerThreadsCount, s_ioc);
				}
				else {
					LOGE("failed createAcceptor. addr:{}, port:{}, acceptorIdx:{}"
						, userParams.addr
						, userParams.port
						, acceptorIdx
					);
				}

			}

			return s_acceptors[acceptorIdx];
		}

		static AcceptorPtrType createAcceptor(
			ServiceParams& svcParams
			, AcceptorUserParams& userParams) {
			return createTcpAcceptor(s_acceptors.size(), svcParams, userParams);
		}

		static std::optional< AcceptorPtrType > getAcceptor(const size_t acceptorIdx) {
			assert(s_acceptors.size() > acceptorIdx);
			if (s_acceptors.size() <= acceptorIdx) {
				return std::nullopt;
			}
			return s_acceptors[acceptorIdx];
		}


		template <typename EVENT_RECEIVER_TYPE>
		static AcceptorPtrType registAcceptor(EVENT_RECEIVER_TYPE& eventReceiver
			, boost::asio::io_context& ioc
			, PacketProcedure::CustomPacketParser packetParser
			, PacketManipulator* packetManip
			, const uint16_t bindingPort
			, const uint32_t ioWorkerCnt = 0
			, const size_t serviceUpdateTimeMs = 1000
			, const size_t keepAliveTimeMs = 60000
			, const SessionType sessionType = SessionType::TCP
		)
		{
			EventReceiverAcceptorRegister<EVENT_RECEIVER_TYPE> acceptorHandler(&eventReceiver);

			ServiceParams serviceInitParams{
				ioc
				, acceptorHandler
				, packetParser
				, packetManip
				, serviceUpdateTimeMs
				, keepAliveTimeMs
			};

			AcceptorUserParams acceptorParams{
				""	// addr. empty string is 0.0.0.0.
				, bindingPort
				, ioWorkerCnt != 0 ? ioWorkerCnt : boost::thread::hardware_concurrency() * 2
				, false
				, sessionType
			};

			return createAcceptor(
				serviceInitParams
				, acceptorParams
			);
		};

		template <typename EVENT_RECEIVER_TYPE>
		static AcceptorPtrType accept(EVENT_RECEIVER_TYPE& eventReceiver
			, boost::asio::io_context& ioc
			, const uint16_t bindingPort
			, const uint32_t ioWorkerCnt = 0
			, const size_t serviceUpdateTimeMs = 1000
			, const size_t keepAliveTimeMs = 60000
			, const SessionType sessionType = SessionType::TCP
		)
		{
			EventReceiverAcceptorRegister<EVENT_RECEIVER_TYPE> acceptorHandler(&eventReceiver);

			ServiceParams serviceInitParams{
				ioc
				, acceptorHandler
				, PacketJsonParser::parse
				, PacketJsonParser::get()
				, serviceUpdateTimeMs
				, keepAliveTimeMs
			};

			AcceptorUserParams acceptorParams{
				""	// addr. empty string is 0.0.0.0.
				, bindingPort
				, ioWorkerCnt != 0 ? ioWorkerCnt : boost::thread::hardware_concurrency() * 2
				, false
				, sessionType
			};

			return createAcceptor(
				serviceInitParams
				, acceptorParams
			);
		};

		static ConnectorPtrType createTcpConnector(const size_t connectorIdx
			, ServiceParams& svcParams
			, ConnectorUserParams& userParams
		) {
			if (!s_ioc) {
				s_ioc = &svcParams.ioc_;
			}

			if (s_connectors.size() <= connectorIdx) {
				s_connectors.resize(connectorIdx + 1);
			}

			if (!s_connectors[connectorIdx]) {
				s_connectors[connectorIdx] = std::make_shared<NetService>(svcParams, userParams);
			}

			return s_connectors[connectorIdx];
		}


		static ConnectorPtrType createConnector(
			ServiceParams& svcParams
			, ConnectorUserParams& userParams
		) {
			return createTcpConnector(s_connectors.size(), svcParams, userParams);
		}



		template <typename EVENT_RECEIVER_TYPE>
		static ConnectorPtrType registConnector(EVENT_RECEIVER_TYPE& eventReceiver
			, boost::asio::io_context& ioc
			, PacketProcedure::CustomPacketParser packetParser
			, PacketManipulator* packetManip
			, const size_t serviceUpdateTimeMs
			, const size_t keepAliveTimeMs
			, const std::string ipAddr
			, const uint16_t port
			, const SessionType sessionType = SessionType::TCP
		)
		{
			EventReceiverConnectorRegister<EVENT_RECEIVER_TYPE> connectorHandler(&eventReceiver);

			ServiceParams serviceInitParams{
				ioc
				, connectorHandler
				, packetParser
				, packetManip
				, serviceUpdateTimeMs
				, keepAliveTimeMs
			};

			ConnectorUserParams connectorParam{
				ipAddr
				, port
				, sessionType
			};

			return createConnector(
				serviceInitParams
				, connectorParam
			);
		};

		template <typename EVENT_RECEIVER_TYPE>
		static ConnectorPtrType connect(EVENT_RECEIVER_TYPE& eventReceiver
			, boost::asio::io_context& ioc
			, const std::string ipAddr
			, const uint16_t port
			, const size_t serviceUpdateTimeMs = 1000
			, const size_t keepAliveTimeMs = 0
			, const SessionType sessionType = SessionType::TCP
		)
		{
			EventReceiverConnectorRegister<EVENT_RECEIVER_TYPE> connectorHandler(&eventReceiver);

			ServiceParams serviceInitParams{
				ioc
				, connectorHandler
				, PacketJsonParser::parse
				, PacketJsonParser::get()
				, serviceUpdateTimeMs
				, keepAliveTimeMs
			};

			ConnectorUserParams connectorParam{
				ipAddr
				, port
				, sessionType
			};

			auto svc = createConnector(
				serviceInitParams
				, connectorParam
			);

			svc->_connectorTcp->connectWait(
				1	// session count
				, 0	// 
			);
		};


		static void run(std::optional<boost::asio::io_context*> optIoc) {
			if (optIoc.has_value()) {
				if (!s_ioc) {
					s_ioc = optIoc.value();
				}
			}
			LOGI("mlnnet run..");

			s_ioc->run();

			for (auto it : s_threads) {
				it->join();
			}
			s_threads.clear();

			LOGI("mlnnet stop..");
		}

		static void setWork() {
			auto atLeastJob = std::make_shared<boost::asio::deadline_timer>(*s_ioc);
			atLeastJob->expires_from_now(
				boost::posix_time::hours(1));

			atLeastJob->async_wait([](const boost::system::error_code&) {});
		}

		static void waitServiceStart(boost::asio::io_context* ios, PostStartCallbackType postServiceStart) {
			while (true) {
				if (false == ios->stopped()) {
					break;
				}
				boost::this_thread::sleep(boost::posix_time::seconds(1));
			}
			postServiceStart();
		}

		static void runService(
			std::optional<PostStartCallbackType> optPostServiceStart
			, std::optional<boost::asio::io_context*> optIoc
		) {
			if (optIoc.has_value()) {
				if (!s_ioc) {
					s_ioc = optIoc.value();
				}
			}
			assert(s_ioc && "set ioc first");

			setWork();

			if (true == s_threads.empty()) {
				std::shared_ptr<boost::thread> thread(new boost::thread(
					boost::bind(&boost::asio::io_context::run, s_ioc)));

				s_threads.push_back(thread);
			}

			if (optPostServiceStart.has_value()) {
				boost::thread watThread = boost::thread(
					&waitServiceStart
					, s_ioc
					, optPostServiceStart.value());

				run(s_ioc);
			}
			else {
				run(s_ioc);
			}
		}
	};//class NetService
}//namespace mln::net {