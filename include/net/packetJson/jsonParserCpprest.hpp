#include <exception>
#include <string>
#include <tuple>

#include <cpprest/json.h>


namespace mln::net::cpprest {
	static std::tuple<bool, web::json::value, std::string> parse(unsigned char* body, uint32_t bodySize, bool getUrlString) {

		try {
			if (!getUrlString) {
				return { true, web::json::value::parse({(char*)body, bodySize}), "" };
			}
			else {
				auto bodyString = web::json::value::parse({(char*)body, bodySize});
				return { true, bodyString[U("body")], utility::conversions::to_utf8string(bodyString[U("url")].as_string()) };
			}
		}
		catch (std::exception e) {
			std::cout << "failed parse(). msg:" << e.what() << std::endl;
			return { false, web::json::value::null(), ""};
		}
	}
}//namespace mln::net::mlncpprest {