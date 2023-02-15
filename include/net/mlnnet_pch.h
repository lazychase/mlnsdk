#pragma once

#pragma warning ( disable: 4819 )
#pragma warning( disable:4996)

//#define BOOST_THREAD_DONT_USE_CHRONO
//#define BOOST_THREAD_DONT_USE_MOVE
//#define BOOST_THREAD_DONT_USE_DATETIME
//#define BOOST_THREAD_DONT_USE_ATOMIC
//#define BOOST_THREAD_DONT_PROVIDE_CONDITION
//#define BOOST_THREAD_DONT_PROVIDE_NESTED_LOCKS
//#define BOOST_THREAD_DONT_PROVIDE_BASIC_THREAD_ID
//#define BOOST_THREAD_DONT_PROVIDE_GENERIC_SHARED_MUTEX_ON_WIN
//#define BOOST_THREAD_DONT_PROVIDE_SHARED_MUTEX_UPWARDS_CONVERSION
//#define BOOST_THREAD_DONT_PROVIDE_PROMISE_LAZY

#define BOOST_ASIO_HAS_MOVE
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION


#include <atomic>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

#ifdef MLN_USE_BEAST_WEBSOCKET
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/bind.hpp>
//#include <boost/bind/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/config.hpp>
#include <boost/json.hpp>
#include <boost/pool/pool.hpp>
#include <boost/thread.hpp>




#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <map>
#include <memory>
#include <mutex>

#include <shared_mutex>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>



// for spdlog
// 
//#ifndef SPDLOG_FMT_EXTERNAL
//#define SPDLOG_FMT_EXTERNAL
//#endif

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE // Must: define SPDLOG_ACTIVE_LEVEL before `#include "spdlog/spdlog.h"`
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>




// for mlnnet
#include <net/session.hpp>
#include <net/logger.hpp>
#include <net/packetProcedure.hpp>
#include <net/netService.hpp>
#include <net/user/userBase.hpp>
#include <net/packetJson/handler.hpp>


#pragma warning ( default : 4819 )












