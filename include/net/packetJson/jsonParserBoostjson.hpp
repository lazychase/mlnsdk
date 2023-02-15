
namespace mln::net::boostjson {
	static std::tuple<bool, boost::json::value, std::string> parse(unsigned char* body, uint32_t bodySize, bool getUrlString) {

		try {
			std::string jsonBodyString{ (char*)body, bodySize };

			if (!getUrlString) {
				return { true, boost::json::parse(jsonBodyString), "" };
			}
			else {
				boost::json::value jsonObj = boost::json::parse(jsonBodyString);
				std::string urlString = boost::json::value_to<std::string>(jsonObj.at("url"));

				return { true, jsonObj.as_object()["body"], urlString };
			}
		}
		catch (std::exception e) {
			std::cout << "failed parse(). msg:" << e.what() << std::endl;
			return { false, boost::json::value(nullptr), "" };
		}
	}
}//namespace mln::net::boostjson {
