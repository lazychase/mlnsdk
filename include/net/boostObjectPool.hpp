#pragma once

#include <mutex>
#include <type_traits>

#include <boost/pool/pool.hpp>

namespace mln::net {
	template<typename OBJ_TYPE>
	class BoostObjectPool
	{
	public:
		static void* operator new(size_t size) {
			return static_cast<OBJ_TYPE*>(_pool.malloc());
		}

		static void operator delete(void* ptr, size_t size) {
			_pool.free(ptr);
		}
		static void destruct(OBJ_TYPE* ptr) {
			delete ptr;
		}

	private:
		/*inline static boost::pool<> _pool{ sizeof(OBJ_TYPE) };*/
		static boost::pool<> _pool;
	};
	template<typename OBJ_TYPE>
	boost::pool<> BoostObjectPool<OBJ_TYPE>::_pool(sizeof(OBJ_TYPE));


	template<typename OBJ_TYPE>
	class BoostObjectPoolTs
	{
	public:
		static void* operator new(size_t size) {
			std::lock_guard< std::mutex > lock(_mutex);
			return static_cast<OBJ_TYPE*>(_pool.malloc());
		}

		static void operator delete(void* ptr, size_t size) {
			std::lock_guard< std::mutex > lock(_mutex);
			_pool.free(ptr);
		}
		static void destruct(OBJ_TYPE* ptr) {
			delete ptr;
		}

	private:
		/*inline static boost::pool<> _pool{ sizeof(OBJ_TYPE) };*/
		static boost::pool<> _pool;
		inline static std::mutex _mutex;
	};

	template<typename OBJ_TYPE>
	boost::pool<> BoostObjectPoolTs<OBJ_TYPE>::_pool(sizeof(OBJ_TYPE));
};//namespace mln::net {
