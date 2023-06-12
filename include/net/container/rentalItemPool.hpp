#pragma once

#include <queue>

namespace mln::net {

template<typename T>
class RentalItem
{
public:
	using sptr = std::shared_ptr< RentalItem >;

	RentalItem(T* item)
		: _value(item)
	{
	}

	~RentalItem()
	{
		if (_reusable) {
			if (_releaseFunction) {
				_releaseFunction(_value);
			}
		}
		else {
			//_conn->close();
		}
	}

	T& valueRef() { return *_value; }
	T* value() { return _value; }

private:
	T* _value;

public:
	bool _reusable = true;
	std::function<bool(T*) > _releaseFunction = nullptr;
};


template <typename T>
class RentalItemPool
{
public:
	RentalItem<T>::sptr getItem() {
		std::lock_guard<std::mutex> lock(_mutex);

		if (false == _pool.empty()) {

			T* itemPtr = _pool.front();
			/*RentalItem<T>::sptr item = std::make_shared< RentalItem<T> >(itemPtr);*/
			auto item = std::make_shared< RentalItem<T> >(itemPtr);
			_pool.pop();

			return item;
		}
		return createItem();
	}

	RentalItem<T>::sptr createItem() {
		/*RentalItem<T>::sptr item = std::make_shared<RentalItem<T>>(T());
		return item;*/
		auto item = std::make_shared< RentalItem<T> >(new T());
		item->_releaseFunction = std::bind(&RentalItemPool<T>::releaseItem, this, std::placeholders::_1);
		return item;
	}

	bool releaseItem(T* itemPtr) {
		if (!itemPtr) {
			[[unlikely]];
			return false;
		}

		std::lock_guard<std::mutex> lock(_mutex);
		_pool.push(itemPtr);
		return true;
	}

private:
	mutable std::mutex _mutex;

	using ItemPool = std::queue< T* >;
	ItemPool _pool;
};

}//namespace mln::net
