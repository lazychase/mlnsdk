#include <exception>
#include <string>
#include <tuple>

#include <cpprest/json.h>


namespace mln::net::cpprest {
	static std::tuple<bool, web::json::value> parse(unsigned char* body, uint32_t bodySize) {

		/*auto obj = web::json::value::object();*/

		try {
			std::string myJsonString((char*)body, bodySize);
			return { true, web::json::value::parse(myJsonString) };
		}
		catch (std::exception e) {
			return { false, web::json::value::null() };
		}
	}
}//namespace mln::net::mlncpprest {