#pragma once

#include <mysqlx/xdevapi.h>

#include <cpprest/base_uri.h>
#include <net/logger.hpp>
#include <optional>
#include <struct_mapping/struct_mapping.h>

#ifndef CONV_UTF8
#ifdef WIN32
#define CONV_UTF8(msg) utility::conversions::to_utf8string(msg)
#define CONV_STRT(msg) utility::conversions::to_string_t(msg)
#else
#define CONV_UTF8(msg)  msg
#define CONV_STRT(msg)  msg
#endif
#endif//#ifndef CONV_UTF8

namespace mln::net::mysql8 {

	class Connector
	{
	protected:
		Connector() = default;
	public:
		static Connector& instance() { static Connector _instance; return _instance; }

        struct ClientOptions {
            struct Pooling {
                bool enabled = true; // Connection pooling enabled. When the option is set to false, a regular, non-pooled connection is returned, and the other connection pool options listed below are ignored.
                int maxSize = 25;
                int queueTimeout = 5000; // The maximum number of milliseconds a request is allowed to wait for a connection to become available. A zero value means infinite
                int maxIdleTime = 5000; // The maximum number of milliseconds a connection is allowed to idle in the queue before being closed. A zero value means infinite.

                static void regist() {
                    struct_mapping::reg(&ClientOptions::Pooling::enabled, "enabled");
                    struct_mapping::reg(&ClientOptions::Pooling::maxSize, "maxSize");
                    struct_mapping::reg(&ClientOptions::Pooling::queueTimeout, "queueTimeout");
                    struct_mapping::reg(&ClientOptions::Pooling::maxIdleTime, "maxIdleTime");
                }
            };

            Pooling pooling;

            static void regist() {
                struct_mapping::reg(&ClientOptions::pooling, "pooling");
                Pooling::regist();
            }
        };

        void init(const std::string& id, const std::string& pw
            , const std::string& host, const std::string& dbname
            , std::optional< ClientOptions > opts
            , const bool useEncodeUrl = true
        ) {
           
            std::string connString = format("mysqlx://{}:{}@{}/{}"
                , !useEncodeUrl ? id : CONV_UTF8(web::uri::encode_uri(CONV_STRT(id)))
                , !useEncodeUrl ? pw : CONV_UTF8(web::uri::encode_uri(CONV_STRT(pw)))
                , host
                , dbname
            );

            try {
                ClientOptions::regist();

                std::ostringstream json_data;
                if (opts.has_value()) {
                    struct_mapping::map_struct_to_json(opts.value(), json_data, "  ");
                }
                else {
                    ClientOptions opt;
                    struct_mapping::map_struct_to_json(opt, json_data, "  ");
                }

                _client = std::make_unique<mysqlx::Client>(
                    connString, json_data.str().c_str());

                _dbname = dbname;
            }
            catch (std::exception ex) {
                LOGE("exception in {}. msg:{}", __func__, ex.what());
                throw ex;
            }
        }

        static std::string stringFromTs(const int64_t unixTimeStamp) {
            std::tm* t = std::gmtime(&unixTimeStamp);
            std::stringstream ss;
            ss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
            return ss.str();
        }

    private:
        std::unique_ptr<mysqlx::Client> _client;
        std::string _dbname;

	};//class Connector

}//namespace mln::net::mysql8{