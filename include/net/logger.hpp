#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

//#ifndef SPDLOG_FMT_EXTERNAL
//#define SPDLOG_FMT_EXTERNAL
//#endif

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

//namespace spdlog {
//	class logger;
//}

namespace mln::net
{
	static const char* LOG_PATTERN_DEFAULT = "[%D %T] [%n] [%t] [%l] : %v";

	class Logger
	{
	public:
		static Logger& instance() {static Logger _instance;return _instance;}

	public:
		class Initializer;
		class GlobalConfig;
		class Config;
		class ConfigFile;

		class Switcher {
		public:
			Switcher(Initializer& init)
				: init_(init)
			{}

			GlobalConfig& global() { return init_.global(); }
			Config& console() { return init_.console(); }
			ConfigFile& file() { return init_.file(); }
			void done() { return init_.done(); }

			Initializer& init_;
		};

		class GlobalConfig
			: public Switcher
		{
		public:
			friend class Initializer;

			GlobalConfig(Initializer& init)
				: init_(init)
				, Switcher(init) 
			{
			}

		public:
			GlobalConfig& loggerName(const std::string& loggerName) { loggerName_ = loggerName; return *this; }
			GlobalConfig& flushEverySec(const size_t flushEverySec) { flushEverySec_ = flushEverySec; return *this; }

		private:
			Initializer& init_;

			std::string loggerName_;
			size_t flushEverySec_;
		};

		class Config
			: public Switcher
		{
		public:
			friend class Initializer;

			Config(Initializer& init)
				: init_(init)
				, Switcher(init)
			{}

			Config& lv(const spdlog::level::level_enum lv) {
				using_ = true;

				lv_ = lv;
				return *this;
			}

			Config& pattern(const char* pattern) {
				if (nullptr == pattern) {
					pattern_ = LOG_PATTERN_DEFAULT;
				}
				else {
					pattern_ = pattern;
				}
				return *this;
			}

		private:
			spdlog::level::level_enum lv_ = spdlog::level::trace;
			std::string pattern_;
			Initializer& init_;

			bool using_ = false;
		};

		class ConfigFile
			: public Switcher
		{
		public:
			friend class Initializer;

			ConfigFile(Initializer& init)
				: init_(init)
				, Switcher(init)
			{}

			ConfigFile& fileNameBase(const std::string& fileNameBase) { fileNameBase_ = fileNameBase; return *this; }
			ConfigFile& maxFileSize(const size_t maxFileSize) { maxFileSize_ = maxFileSize; return *this; }
			ConfigFile& maxFiles(const size_t maxFiles) { maxFiles_ = maxFiles; return *this; }
			ConfigFile& lv(const spdlog::level::level_enum lv) {
				using_ = true;

				lv_ = lv;
				return *this;
			}
			ConfigFile& pattern(const char* pattern) {
				if (nullptr == pattern) {
					pattern_ = LOG_PATTERN_DEFAULT;
				}
				else {
					pattern_ = pattern;
				}
				return *this;
			}

		private:
			std::string fileNameBase_;
			size_t maxFileSize_;
			size_t maxFiles_;

			spdlog::level::level_enum lv_ = spdlog::level::trace;
			std::string pattern_;
			Initializer& init_;

			bool using_ = false;
		};

		class Initializer {
		public:
			Initializer(Logger* logger)
				: _logger(logger)
				, global_(*this)
				, console_(*this)
				, file_(*this)
			{}

		public:
			GlobalConfig& global() { return global_; }
			Config& console() { return console_; }
			ConfigFile& file() { return file_; }
			//Config& packetDump() { return packetDump_; }
			void done() {
				std::vector<spdlog::sink_ptr> sinks;

				if (console_.using_) {
					auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
					console_sink->set_level(console_.lv_);
					console_sink->set_pattern(console_.pattern_);

					sinks.emplace_back(console_sink);
				}

				if (file_.using_) {
#ifdef _WIN32
					auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
						file_.fileNameBase_
						, file_.maxFileSize_
						, file_.maxFiles_
						);
#else
					auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
						file_.fileNameBase_
						, file_.maxFileSize_
						, file_.maxFiles_
						, false
						);
#endif
					file_sink->set_level(file_.lv_);
					file_sink->set_pattern(file_.pattern_);

					sinks.emplace_back(file_sink);
				}

				_logger->_logger = std::make_shared<spdlog::logger>(global_.loggerName_, sinks.begin(), sinks.end());
				_logger->_logger->set_level(spdlog::level::trace);

				if (0 < global_.flushEverySec_) {
					_logger->_logger->flush_on(spdlog::level::trace);
					spdlog::flush_every(std::chrono::seconds(global_.flushEverySec_));
				}
			}

			GlobalConfig global_;
			Config console_;
			ConfigFile file_;
			//Config packetDump_;

			Logger* _logger = nullptr;
		};

		void InitRotate(const std::string& loggerName, const std::string& fileNameBase, const size_t maxFileSize, const size_t maxFiles, const int flushEverySec = 0)
		{
			auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
			console_sink->set_level(spdlog::level::trace);
			console_sink->set_pattern(LOG_PATTERN_DEFAULT);

			//auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filename_base, daily_h, daily_m, true, max_days);
#ifdef _WIN32
			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(fileNameBase, maxFileSize, maxFiles);
#else
			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(fileNameBase, maxFileSize, maxFiles, false);
#endif
			file_sink->set_level(spdlog::level::trace);
			file_sink->set_pattern(LOG_PATTERN_DEFAULT);

			spdlog::sinks_init_list sink_list = { file_sink, console_sink };

			_logger = std::make_shared<spdlog::logger>(loggerName, sink_list.begin(), sink_list.end());

			_logger->set_level(spdlog::level::trace);

			if (0 < flushEverySec) {
				_logger->flush_on(spdlog::level::trace);
				spdlog::flush_every(std::chrono::seconds(flushEverySec));
			}
		}
		void Init(const std::string& loggerName, const std::string& fileNameBase, const int flushEverySec = 0)
		{
			auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
			console_sink->set_level(spdlog::level::trace);
			console_sink->set_pattern(LOG_PATTERN_DEFAULT);

			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fileNameBase, true);
			file_sink->set_level(spdlog::level::trace);
			file_sink->set_pattern(LOG_PATTERN_DEFAULT);

			spdlog::sinks_init_list sink_list = { file_sink, console_sink };

			_logger = std::make_shared<spdlog::logger>(loggerName, sink_list.begin(), sink_list.end());

			_logger->set_level(spdlog::level::trace);

			if (0 < flushEverySec) {
				_logger->flush_on(spdlog::level::trace);
				spdlog::flush_every(std::chrono::seconds(flushEverySec));
			}

		}
		void Flush() {
			_logger->flush();
		}

		Initializer Create() {
			return Logger::Initializer(this);
		}

	public:
		std::shared_ptr<spdlog::logger> _logger;
	};
}//namespace mln::net


//#define _LTRACE(...)	((void)(0))

#define LOGT		mln::net::Logger::instance()._logger->trace
#define LOGD		mln::net::Logger::instance()._logger->debug
#define LOGI		mln::net::Logger::instance()._logger->info
#define LOGW		mln::net::Logger::instance()._logger->warn
#define LOGE		mln::net::Logger::instance()._logger->error
#define LOGC		mln::net::Logger::instance()._logger->critical

