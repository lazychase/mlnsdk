#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <stdint.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>

namespace mln::net {

	

	template < size_t MAX_SCHEDULES, size_t TIME_INCREMENT_SIZE_MS >
	class SimpleScheduler
		: public std::enable_shared_from_this<
		SimpleScheduler<MAX_SCHEDULES, TIME_INCREMENT_SIZE_MS> >
	{
	public:
		using CallbackType = std::function<void(const uint64_t elapsed)>;
		using ScheduleIDType = size_t;
		struct SchedulerInfo {
			ScheduleIDType scheduerID_ = 0;
			const uint64_t interval_;
			int64_t intervalRemain_ = 0;
			CallbackType callback_ = nullptr;

			SchedulerInfo(
				ScheduleIDType scheduerID
				, uint64_t interval
				, CallbackType callback)
				: scheduerID_(scheduerID)
				, interval_(interval)
				, intervalRemain_(interval)
				, callback_(callback)
			{
			}
		};
		
		using STRAND = boost::asio::io_context::strand;

		SimpleScheduler(boost::asio::io_context& ioc)
			: m_ioc(ioc)
			, m_updateTimer(ioc)
			, m_strand(m_ioc)
		{
		}

		void TimerStart()
		{
			m_processing = true;
			ReserveUpdate(TIME_INCREMENT_SIZE_MS);
		}

		std::tuple< bool, ScheduleIDType > AddJobInterval(const uint64_t waitCount, CallbackType cb)
		{
			if (0 >= waitCount
				|| nullptr == cb) {
				return std::make_tuple(false, 0);
			}

			const auto entryIndex = m_currentEntries++;

			m_schedulerEntry[entryIndex] = std::make_unique<SchedulerInfo>(
				entryIndex
				, waitCount * TIME_INCREMENT_SIZE_MS
				, cb
				);

			return std::make_tuple(true, entryIndex);
		}

		void TimerStop()
		{
			m_processing = false;

			boost::system::error_code ec;
			m_updateTimer.cancel(ec);
		}

	protected:
		void UpdateHandler(const boost::system::error_code& ec)
		{
			if (!ec && m_processing) {
				for (auto i = 0; i < m_currentEntries; ++i) {
					auto& ent = *m_schedulerEntry[i].get();

					ent.intervalRemain_ -= TIME_INCREMENT_SIZE_MS;
					if (0 >= ent.intervalRemain_) {
						ent.intervalRemain_ = ent.interval_;
						ent.callback_(0);
					}
				}
					
				ReserveUpdate(TIME_INCREMENT_SIZE_MS);
			}
		}

		void ReserveUpdate(const int interval)
		{
			m_updateTimer.expires_from_now(
				boost::posix_time::milliseconds(interval));

			m_updateTimer.async_wait(boost::asio::bind_executor(m_strand
				, boost::bind(&SimpleScheduler::UpdateHandler
					, this
					, boost::asio::placeholders::error)));
		}


	protected:
		boost::asio::deadline_timer m_updateTimer;
		boost::asio::io_context& m_ioc;
		STRAND m_strand;
		bool m_processing = false;

		std::array< std::unique_ptr<SchedulerInfo>, MAX_SCHEDULES > m_schedulerEntry;
		size_t m_currentEntries = 0;
	};




	template < size_t MAX_SCHEDULES, size_t TIME_INCREMENT_SIZE_MS, bool USING_EXTERNAL_STRAND >
	class SimpleSchedulerES
		: public std::enable_shared_from_this<
		SimpleSchedulerES<MAX_SCHEDULES, TIME_INCREMENT_SIZE_MS, USING_EXTERNAL_STRAND > >
	{
	public:
		using CallbackType = std::function<void(const uint64_t elapsed)>;
		using ScheduleIDType = size_t;

		struct Info
		{
			ScheduleIDType scheduerID_ = 0;
			const uint64_t interval_;
			int64_t intervalRemain_ = 0;
			CallbackType callback_ = nullptr;

			Info(
				ScheduleIDType scheduerID
				, uint64_t interval
				, CallbackType callback)
				: scheduerID_(scheduerID)
				, interval_(interval)
				, intervalRemain_(interval)
				, callback_(callback)
			{
			}
		};

		using STRAND = boost::asio::io_context::strand;

		SimpleSchedulerES(boost::asio::io_context& ioc)
			: m_ioc(ioc)
			, m_updateTimer(ioc)
		{
		}

		void Init()
		{
			if (true == USING_EXTERNAL_STRAND) {
				throw std::runtime_error("external starnd");
			}

		}

		void Init(std::shared_ptr<STRAND> spStrand)
		{
			if (false == USING_EXTERNAL_STRAND) {
				throw std::runtime_error("external starnd");
			}

			m_wpStrandExternal = spStrand;
		}

		void TimerStart()
		{
			auto spStrand = GetStrand();
			if (!spStrand) {
				throw std::runtime_error("noen of starnd");
			}
			m_processing = true;
			ReserveUpdate(spStrand, TIME_INCREMENT_SIZE_MS);
		}

		std::tuple< bool, ScheduleIDType > AddJobInterval(const uint64_t waitCount, CallbackType cb)
		{
			if (0 >= waitCount
				|| nullptr == cb) {
				return std::make_tuple(false, 0);
			}

			if (USING_EXTERNAL_STRAND == false) {
				if (!m_spStrand) {
					m_spStrand = std::make_shared<STRAND>(m_ioc);
				}
			}

			std::shared_ptr<STRAND> spStrand = GetStrand();
			if (!spStrand) {
				throw std::runtime_error("noen of starnd");
			}

			const auto entryIndex = m_currentEntries++;

			m_schedulerEntry[entryIndex] = std::make_unique<Info>(
				entryIndex
				, waitCount * TIME_INCREMENT_SIZE_MS
				, cb
				);

			return std::make_tuple(true, entryIndex);
		}

		void TimerStop()
		{
			m_processing = false;

			boost::system::error_code ec;
			m_updateTimer.cancel(ec);
		}

	protected:
		void UpdateHandler(const boost::system::error_code& ec)
		{
			if (USING_EXTERNAL_STRAND) {
				if (!m_wpStrandExternal.lock()) {
					return;
				}
			}

			if (!ec && m_processing) {
				for (auto i = 0; i < m_currentEntries; ++i) {
					auto& ent = *m_schedulerEntry[i].get();

					ent.intervalRemain_ -= TIME_INCREMENT_SIZE_MS;
					if (0 >= ent.intervalRemain_) {
						ent.intervalRemain_ = ent.interval_;
						ent.callback_(0);
					}
				}

				auto spStrand = GetStrand();
				if (spStrand) {
					ReserveUpdate(spStrand, TIME_INCREMENT_SIZE_MS);
				}

			}//if (!ec) {
		}

		std::shared_ptr<STRAND> GetStrand()
		{
			if (USING_EXTERNAL_STRAND) {
				auto spStrand = m_wpStrandExternal.lock();
				if (spStrand) {
					return spStrand;
				}
			}
			else {
				return m_spStrand;
			}
			return nullptr;
		}

		void ReserveUpdate(std::shared_ptr<STRAND> strand, const int interval)
		{
			m_updateTimer.expires_from_now(
				boost::posix_time::milliseconds(interval));

			m_updateTimer.async_wait(boost::asio::bind_executor(strand
				, boost::bind(&SimpleSchedulerES::UpdateHandler
					//, shared_from_this()
					, std::enable_shared_from_this<SimpleSchedulerES<MAX_SCHEDULES, TIME_INCREMENT_SIZE_MS, USING_EXTERNAL_STRAND > >::shared_from_this()
					, boost::asio::placeholders::error)));
		}


	protected:
		boost::asio::deadline_timer m_updateTimer;
		boost::asio::io_context& m_ioc;
		std::weak_ptr<STRAND> m_wpStrandExternal;
		std::shared_ptr<STRAND> m_spStrand;
		bool m_processing = false;

		std::array< std::unique_ptr<Info>, MAX_SCHEDULES > m_schedulerEntry;
		size_t m_currentEntries = 0;
	};


}//namespace mln::net {