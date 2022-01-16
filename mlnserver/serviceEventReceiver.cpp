#include "serviceEventReceiver.h"

#include "user.h"
#include "userManager.h"
#include "net/packetJson/handler.hpp"

using namespace mlnserver;
using namespace mln::net;

#ifndef CONV_UTF8
#ifdef WIN32
#define CONV_UTF8(msg) utility::conversions::to_utf8string(msg)
#define CONV_STRT(msg) utility::conversions::to_string_t(msg)
#else
#define CONV_UTF8(msg)  msg
#define CONV_STRT(msg)  msg
#endif
#endif//#ifndef CONV_UTF8

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
	auto static handler = PacketJsonHandler<web::json::value>();
	handler.init(packetProcedure);
	handler.setJsonBodyParser(mln::net::cpprest::parse);

	// 서브패킷들(json packets)을 등록
	handler.registJsonPacketHandler("/lobby/login", [](
		UserBase::sptr userBase
		, const std::string& url
		, auto & jv
		) {

		assert(url == "/lobby/login");
		auto sessionOpt = userBase->getSession();
		std::string sessionTypeString;
		if (sessionOpt.has_value()) {
			sessionTypeString = sessionOpt.value()->getSessionTypeString();
		}

		LOGD("received packet from client. (C->S) url:{}, sessionType:{}"
			, url
			, sessionTypeString
		);

		auto receivedJsonString = jv.serialize();
		std::cout << CONV_UTF8(receivedJsonString) << std::endl;

		auto user = std::static_pointer_cast<User>(userBase);
		std::string replyString(receivedJsonString.begin(), receivedJsonString.end());

		if (SessionType::TCP == user->_sessionType) {
			user->sendJsonPacket(url, replyString);
		}
		else {
			auto obj = web::json::value::object();
			obj[U("url")] = web::json::value::string(utility::conversions::to_string_t(url));
			obj[U("body")] = web::json::value::string(receivedJsonString);
			std::ostringstream oss;
			obj.serialize(oss);
			user->sendJsonWebsocketPacket(oss.str().data());
		}
		
		
	});

}