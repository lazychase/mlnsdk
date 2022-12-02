#include <net/session.hpp>
#include <net/logger.hpp>
#include <net/http/httpServer.hpp>

#ifdef _WIN32
#include <net/exceptionHandler.hpp>
#endif

#include <net/netService.hpp>
#include <net/packetJson/packetParser.hpp>
#include "asioContext.h"
#include "serviceEventReceiver.h"


#include "clientSample/run.h"


bool ioServiceThread();
void testHttpServer();

int main(int argc, char* argv[])
{
	using namespace mln::net;

	// init logger
	Logger::createDefault();
	LOGD("logger initialized");


#ifdef _WIN32
	mln::net::ExceptionHandler::init();

	if (FALSE == SetConsoleTitleA("mln-server")) {
		LOGE("SetConsoleTitle failed {}", GetLastError());
	}
#endif

	testHttpServer();


	return ioServiceThread();
}




bool acceptSpecificParser() 
{
	using namespace mlnserver;
	using namespace mln::net;

	static ServiceEventReceiver eventReceiver;

	auto acceptor = NetService::registAcceptor(
		eventReceiver
		, *g_ioc.get()
		, PacketJsonParser::parse
		, PacketJsonParser::get()
		, 9090
	);

	if (!acceptor) {
		LOGE("failed registAcceptor().");
		return false;
	}
	return true;
}

bool acceptDefaultJsonParser()
{
	using namespace mlnserver;
	using namespace mln::net;

	static ServiceEventReceiver eventReceiver;

	auto acceptor = NetService::accept(
		eventReceiver
		, *g_ioc.get()
		, 9090
	);

	if (!acceptor) {
		LOGE("failed registAcceptor().");
		return false;
	}
	return true;
}


bool ioServiceThread()
{
	using namespace mlnserver;
	using namespace mln::net;

	acceptDefaultJsonParser();
	//acceptSpecificParser();

	NetService::runService([](){
		LOGI("server started.");

		// sample client
		mlnserver::SampleClientTest::TestRun(g_ioc, 9090);

	}, g_ioc.get());
	return true;
}


#define HTTP_SERVER_REGISTER_URL(url) mln::net::HttpServer::instance().GetRegisterFunc(url)

void testHttpServer()
{
	HTTP_SERVER_REGISTER_URL("/test")
		([](mln::net::HttpServer::HeaderMap&& header, std::string&& body, mln::net::HttpServer::SendFunc& send) {
		return send("{\"user\": \"testUser\"}");
			});


	mln::net::HttpServer::instance().startAsync(28888, g_ioc);

}
