#pragma once


#include "../user.h"

namespace mlnserver {

	class SampleConnector
	{
	public:
		void onConnect(mln::net::Session::sptr session) {
			LOGD("onConnect - {}/{}"
				, session->socket().remote_endpoint().address().to_string()
				, session->socket().remote_endpoint().port());

			// create user.
			auto user = std::make_shared<User>(session);
			session->setUser(user);

			std::istringstream json_data(R"json(
  {
    "how": "are",
    "you": "Im",
    "fine": "thx",
    "andu": "hello everyone"
}
 )json");
			auto ss = json_data.str();

			user->sendJsonPacket("/lobby/login", ss);
		}

		void onConnectFailed(mln::net::Session::sptr session) {
			LOGE("onConnectFailed");
		}

		void onClose(mln::net::Session::sptr session) {
			LOGD("onClose - {}/{}"
				, session->socket().remote_endpoint().address().to_string()
				, session->socket().remote_endpoint().port());
		}
		void onCloseFailed(mln::net::Session::sptr session) {}

		void onUpdate(uint64_t elapse) {}
		void onExpiredSession(mln::net::Session::sptr session) {}
		void noHandler(mln::net::Session::sptr session, mln::net::ByteStream& packet) {}

	public:
		void initHandler(mln::net::PacketProcedure* packetProcedure) {
			using namespace mln::net;

#if defined (MLN_NET_USE_JSONPARSER_BOOSTJSON)
			auto static handler = PacketJsonHandler<boost::json::value>();

			handler.init(packetProcedure);
			handler.setJsonBodyParser(mln::net::boostjson::parse);
			handler.registJsonPacketHandler("/lobby/login", [](
				UserBase::sptr user
				, const std::string& url
				, auto& jv
				) {
					assert(url == "/lobby/login");

					LOGD("received packet from server. (S->C) url:{}", url);
					std::cout << boost::json::serialize(jv) << std::endl;
				});
#endif
		}

		static void tryConnect1(boost::asio::io_context& ioc, const uint16_t port)
		{
			static SampleConnector connectorInstance;

			mln::net::EventReceiverConnectorRegister<SampleConnector>
				connectorHandler(&connectorInstance);

			mln::net::ServiceParams serviceInitParamsForClnt{
				ioc
				, connectorHandler
				, mln::net::PacketJsonParser::parse
				, mln::net::PacketJsonParser::get()
				, 1000
				, 0
			};

			mln::net::ConnectorUserParams connectorUserParam{
				"127.0.0.1"
				, port
			};

			auto svc = mln::net::NetService::createConnector(
				serviceInitParamsForClnt
				, connectorUserParam
			);

			svc->_connector->connectWait(
				1	// session count
				, 0	// 
			);
		}

		static void tryConnect2(boost::asio::io_context& ioc, const uint16_t port)
		{
			static SampleConnector connectorInstance;

			auto svc = mln::net::NetService::registConnector(
				connectorInstance
				, ioc
				, mln::net::PacketJsonParser::parse
				, mln::net::PacketJsonParser::get()
				, 1000
				, 0
				, "127.0.0.1"
				, port
			);

			svc->_connector->connectWait(
				1	// session count
				, 0	// 
			);
		}

		static void tryConnect3(boost::asio::io_context& ioc, const uint16_t port)
		{
			static SampleConnector connectorInstance;

			mln::net::NetService::connect(
				connectorInstance
				, ioc
				, "127.0.0.1"
				, port
			);
		}
	};

}//namespace mlnserver {
