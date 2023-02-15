#include "serviceEventReceiver.h"

#include "user.h"
#include "userManager.h"

using namespace mlnserver;
using namespace mln::net;

#if defined (MLN_NET_USE_JSONPARSER_BOOSTJSON)
#include <boost/json.hpp>
#endif//#if defined (MLN_NET_USE_JSONPARSER_BOOSTJSON)

void ServiceEventReceiver::onAccept(Session::sptr session)
{
	auto [addr, port] = session->getEndPointSocket();
	LOGD("accept. remote endpoint:{}/{}", addr, port);

	g_userManager.addUser<User>(session);
}

void ServiceEventReceiver::onClose(Session::sptr session)
{
	auto [addr, port] = session->getEndPointSocket();
	LOGD("close. remote endpoint:{}/{}", addr, port);

	g_userManager.closedUser<User>(session);
}

void ServiceEventReceiver::onUpdate(uint64_t deltaMs)
{

}

void ServiceEventReceiver::noHandler(Session::sptr session, ByteStream& packet)
{
	LOGW("no Handler.");
	session->closeReserve(0);
}

void ServiceEventReceiver::onAcceptFailed(Session::sptr session)
{
	LOGW("failed accept");
}

void ServiceEventReceiver::onCloseFailed(Session::sptr session)
{
	LOGW("failed close");
}

void ServiceEventReceiver::onExpiredSession(Session::sptr session)
{
	auto [addr, port] = session->getEndPointSocket();
	LOGW("Expired Session. addr:{}, port:{}", addr, port);
	session->closeReserve(0);
}

void ServiceEventReceiver::initHandler(PacketProcedure* packetProcedure)
{
	using namespace mln::net;

	// packetJson::PT_JSON 패킷을 등록.
	auto static handler = PacketJsonHandler<boost::json::value>();
	handler.init(packetProcedure);
	handler.setJsonBodyParser(mln::net::boostjson::parse);

	// 서브패킷들(json packets)을 등록
	handler.registJsonPacketHandler("/lobby/login", [](
		UserBase::sptr userBase
		, const std::string& url
		, auto& jv
		) {

			assert(url == "/lobby/login");
			auto spSession = userBase->getSession();
			std::string sessionTypeString;
			if (spSession) {
				sessionTypeString = spSession->getSessionTypeString();
			}

			LOGD("received packet from client. (C->S) url:{}, sessionType:{}"
				, url
				, sessionTypeString
			);

			auto receivedJsonString = boost::json::serialize(jv);
			std::cout << receivedJsonString << std::endl;

			auto user = std::static_pointer_cast<User>(userBase);
			std::string replyString(receivedJsonString.begin(), receivedJsonString.end());

			if (SessionType::TCP == user->_sessionType) {
				user->sendJsonPacket(url, replyString);
			}
			else {
				auto obj = boost::json::object();
				obj["url"] = url;
				obj["body"] = receivedJsonString;
				user->sendJsonWebsocketPacket(boost::json::serialize(obj));
			}
		});
}
