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


#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/bind/bind.hpp>
#include <boost/chrono.hpp>
//#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/pool/pool.hpp>
//#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/future.hpp>
#pragma warning ( default : 4819 )

//#include <iostream>
//#include <memory>
//#include <string>
