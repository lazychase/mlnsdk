#pragma once

#include <functional>
#include <mutex>

#include "options.h"

#include <mysql_driver.h>
#include <mysql_connection.h>
//#include <cppconn/driver.h>
//#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>

#define USE_MYSQL_CONNECT_BY_OPTIONMAP

namespace mln::net::mysql-legacy {

	class Connection
	{
	public:
		enum class Status{
			CLOSED,
			CONNECTING,
			CONNECTED,
			USING
		};

		inline static std::once_flag s_mysqlDriverCreateFlag;
		inline static sql::Driver* s_mysqlDriver = nullptr;

		Connection() = default;
		Connection(const PoolOptions &poolOpts) {
			_poolOpts = poolOpts;
		}

		Status getStatus() const { return _status; }
		void setStatus(const Status s) { _status = s; }
		sql::ResultSetMetaData* getMetaData() const {return _res->getMetaData();}


		bool connect(const std::string& addr, const std::string& port, const std::string& id, const std::string& pw
			, const std::string& db, const bool useAutoReconnect)
		{
			try {
				std::call_once(s_mysqlDriverCreateFlag, [&] {
					s_mysqlDriver = get_driver_instance();
				});
				/*std::call_once(s_mysqlDriverCreateFlag, [&] {
					s_mysqlDriver = sql::mysql::get_driver_instance();
				});*/
				

#ifdef USE_MYSQL_CONNECT_BY_OPTIONMAP
				// referenced : https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html
				sql::ConnectOptionsMap connection_properties;
				connection_properties["hostName"] = addr + ":" + port;
				connection_properties["userName"] = id;
				connection_properties["password"] = pw;
				connection_properties["schema"] = db;
				connection_properties["OPT_RECONNECT"] = true;
				connection_properties["OPT_READ_TIMEOUT"] = 3;	
				connection_properties["OPT_CONNECT_TIMEOUT"] = 3;
				connection_properties["OPT_WRITE_TIMEOUT"] = 3;
				connection_properties["CLIENT_MULTI_STATEMENTS"] = true;

				_conn = s_mysqlDriver->connect(connection_properties);
#else//#ifdef USE_MYSQL_CONNECT_BY_OPTIONMAP
				std::string connString = "tcp://";
				connString += addr;
				connString += ":";
				connString += port;

				_conn = s_mysqlDriver->connect(connString.c_str(), id, pw);
				_conn->setSchema(db);
#endif//#ifdef USE_MYSQL_CONNECT_BY_OPTIONMAP
			}
			catch (sql::SQLException& e) {
				printException(e);
				return false;
			}

			setStatus(Status::CONNECTED);

			return true;
		}

		bool switchDB(const char* dbname){
			return workWithCheckSessionValid([&]() {
				_conn->setSchema(dbname);
			});
		}

		template <typename... ARGS>
		inline bool exeSql(const char* formatString, ARGS&&... rest) {

			if (_res) {
				delete _res;
				_res = nullptr;
			}

			if (false == prepareSql(formatString, rest...)) {
				return false;
			}

			return workWithCheckSessionValid([&]() {
				_res = _pstmt->executeQuery(_queryString);
			});
		}

		void dbTest()
		{
			using namespace mln::net;

			auto dbConn = std::make_unique<mysql::Connection>();
			if (false == dbConn->connect("127.0.0.1", "3306", "myId", "myPw", "test_db", false)) {
				return;
			}

			dbConn->exeSql("SELECT * from build where idx = ? and project_key = ?"
				, 39
				, 6
			);
		}

	protected:
		bool workWithCheckSessionValid(std::function<void()> work) {
			// is valid ? 

			// is closed ?

			try {
				if (true == _conn->isClosed()) {
					[[unlikely]]
					if (false == _conn->reconnect()) {
						_lastExceptionMsg = "closed session. trying to reconnecting";
						return false;
					}
				}
				work();
			}
			catch (sql::SQLException& e) {
				switch (e.getErrorCode()) {
				case 2006:	// CR_SERVER_GONE_ERROR
				case 2013:	// CR_SERVER_LOST
				case 2048:	// CR_INVALID_CONN_HANDLE
				case 2055:	// CR_SERVER_LOST_EXTENDED
					_lastExceptionMsg = "invalid session. code:" + std::to_string(e.getErrorCode());
					break;
				default:
					_lastExceptionMsg = "invalid session. not defined exception. code:" + std::to_string(e.getErrorCode());
				}
				printException(e);
				return false;
			}

			return true;
		}

		void printException(sql::SQLException& e) {
			if (e.getErrorCode() != 0) {
				_lastExceptionMsg = std::string("# ERR: SQLException. ");
				_lastExceptionMsg += "# ERR: ";
				_lastExceptionMsg += e.what();
				_lastExceptionMsg += " (MySQL error code: ";
				_lastExceptionMsg += std::to_string(e.getErrorCode());
				_lastExceptionMsg += ", SQLState: ";
				_lastExceptionMsg += e.getSQLState();

				std::cout << _lastExceptionMsg << std::endl;
			}
		}

		template <typename... ARGS>
		inline bool prepareSql(const char* formatString, ARGS&&... rest) {
			_queryString = formatString;
			if (false == prepare(_queryString.c_str())) {
				return false;
			}
			setInputParam(1, rest...);
			return true;
		}

		bool prepare(const char* queryString){
			if (!_pstmt) {
				delete _pstmt;
				_pstmt = nullptr;
			}

			return workWithCheckSessionValid([&]() {
				_pstmt = _conn->prepareStatement(queryString);
			});
		}

		template <typename A0, typename... ARGS>
		inline void setInputParam(const uint32_t idx, A0 a1, ARGS&&... rest){
			setParam(idx, a1);
			setInputParam(idx + 1, rest...);
		}

		template<typename LAST>
		inline void setInputParam(const uint32_t idx, LAST value){
			setParam(idx, value);
		}
		inline void setInputParam(const uint32_t idx){}

		template <typename T>
		inline void setParam(const uint32_t idx, T value);

		template <>
		inline void setParam<>(const uint32_t idx, const short value) {
			_pstmt->setInt(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const unsigned short value) {
			_pstmt->setUInt(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const int value) {
			_pstmt->setInt(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const unsigned int value){
			_pstmt->setUInt(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const int64_t value){
			_pstmt->setInt64(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const uint64_t value){
			_pstmt->setUInt64(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const std::string& value){
			_pstmt->setString(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const char* value){
			_pstmt->setString(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, char* value){
			_pstmt->setString(idx, value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, const unsigned char* value){
			_pstmt->setString(idx, (char*)value);
		}

		template <>
		inline void setParam<>(const uint32_t idx, unsigned char* value){
			_pstmt->setString(idx, (char*)value);
		}

	protected:
		PoolOptions _poolOpts;
		Status _status = Status::CLOSED;
		sql::Connection* _conn = nullptr;
		sql::ResultSet* _res = nullptr;
		sql::PreparedStatement* _pstmt = nullptr;

		std::string _queryString;
		std::string _lastExceptionMsg;
	};

}//namespace mln::net::mysql - legacy{
