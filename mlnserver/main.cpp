#include <net/session.hpp>
#include <net/logger.hpp>

#ifdef _WIN32
#include <net/exceptionHandler.hpp>
#endif

#include <net/netService.hpp>
#include <net/packetJson/packetParser.hpp>
#include "asioContext.h"
#include "serviceEventReceiver.h"


#include "clientSample/run.h"


bool ioServiceThread();

int main(int argc, char* argv[])
{
	using namespace mln::net;

	// init logger
	Logger::instance().Create()
		.global()
			.loggerName("mln-server-log")
			.flushEverySec(0)
		.console()
			.lv(spdlog::level::trace)
			.pattern(nullptr)
		.file()
			.lv(spdlog::level::trace)
			.pattern(nullptr)
			.fileNameBase("mln-server")
			.maxFileSize(1048576 * 100)
			.maxFiles(30)
		.done();

	LOGD("logger initialized");


#ifdef _WIN32
	mln::net::ExceptionHandler::init();

	if (FALSE == SetConsoleTitleA("mln-server")) {
		LOGE("SetConsoleTitle failed {}", GetLastError());
	}
#endif

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
