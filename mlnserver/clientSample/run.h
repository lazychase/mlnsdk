#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "sampleConnector.h"

namespace mlnserver {
	class SampleClientTest
	{
	public:
		static void TestRun(std::shared_ptr<boost::asio::io_context> spIoc, const int32_t port)
		{
			SampleConnector::tryConnect1(*spIoc.get(), port);
		}
	}; 
}//namespace mlnserver {