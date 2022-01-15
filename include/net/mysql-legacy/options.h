#pragma once

#include <string>

namespace mln::net::mysql-legacy {
	struct TargetOptions
	{
		std::string addr_;
		std::string port_;
		std::string id_;
		std::string pw_;
		std::string db_;
	};

	struct PoolOptions
	{
		uint32_t idle_timeout_sec = 0;
		uint32_t pool_min = 1;
		uint32_t pool_limit = 100;
	};
}//namespace mln::net::mysql-legacy {