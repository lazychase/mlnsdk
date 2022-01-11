#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "eventReceiver.hpp"
#include "serviceParamTypes.h"

namespace mln::net {

	struct NetCommonObjects
	{
	public:
		NetCommonObjects(ServiceParams& svcParams)
			: _ioc(svcParams.ioc_)
			, _strand(svcParams.ioc_)
			, _keepAliveTimeMs(svcParams.keepAliveTimeMs_)
			, _updateTimeMs(svcParams.serviceUpdateTimeMs_)
			, _updater(svcParams.ioc_, boost::posix_time::milliseconds(svcParams.serviceUpdateTimeMs_))
		{
			_packetProc = std::make_unique<PacketProcedure>(svcParams.packetParser_, svcParams.manip_);

			svcParams.receiver_.clone(&_eventReceiver);
			_eventReceiver.initHandler(_packetProc.get());
		}

		boost::asio::io_context& _ioc;
		boost::asio::io_context::strand _strand;
		std::unique_ptr<PacketProcedure> _packetProc;
		EventReceiver _eventReceiver;
		size_t _keepAliveTimeMs = 0;
		size_t _updateTimeMs = 0;
		
		size_t _index;
		
	private:
		boost::chrono::system_clock::time_point		_prevTime;
		boost::asio::deadline_timer					_updater;

	public:
		size_t getIndex() const { return _index; }
		void setIndex(const size_t idx) { _index = idx; }

		void expireTimerReady() {
			if (0 != _updateTimeMs) {
				_prevTime = boost::chrono::system_clock::now();
				_updater.expires_from_now(boost::posix_time::milliseconds(_updateTimeMs));

				_updater.async_wait(boost::asio::bind_executor(_strand, boost::bind(
					&NetCommonObjects::handleUpdate, this, boost::asio::placeholders::error)));
			}
		}

		void handleUpdate(const boost::system::error_code& ec) {
			if (ec) {
				[[unlikely]]
				LOGW("failed handleUpdate. code:{}, msg:{}", ec.value(), ec.message());
			}
			else {
				boost::chrono::system_clock::time_point now = boost::chrono::system_clock::now();
				unsigned long elapse
					= (unsigned long)boost::chrono::duration_cast<boost::chrono::milliseconds>(now - _prevTime).count();
				_prevTime = now;

				_eventReceiver.onUpdate(elapse);

				_updater.expires_from_now(boost::posix_time::milliseconds(_updateTimeMs));
				_updater.async_wait(boost::asio::bind_executor(_strand, boost::bind(
					&NetCommonObjects::handleUpdate, this, boost::asio::placeholders::error)));
			}
		}
	};

}//namespace mln::net {



