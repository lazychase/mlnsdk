#pragma once


#include "protocol.h"
#include "packetParser.hpp"
#include "../packetProcedure.hpp"

#if defined (MLN_NET_USE_JSONPARSER_BOOSTJSON)
#include "jsonParserBoostjson.hpp"
#endif


namespace mln::net {

	template< typename JSON_VALUE_TYPE >
	class PacketJsonHandler
	{
	public:
		using TyJsonValue = JSON_VALUE_TYPE;

		using JsonParsingCallback = std::function<
			std::tuple<bool, JSON_VALUE_TYPE, std::string>(unsigned char* body, uint32_t bodySize, bool getUrl)
		>;

		using JsonPacketHandlerType = std::function<
			void(std::shared_ptr<UserBase>, const std::string&, JSON_VALUE_TYPE&) >;


		void setJsonBodyParser(JsonParsingCallback cb) { _jsonParsingCallback = cb; }

		void init(PacketProcedure* packetProcedure) {
			packetProcedure->registPacket(
				packetJson::PT_JSON::packet_value
				, std::bind(&PacketJsonHandler::readJsonPacket
					, this
					, std::placeholders::_1
					, std::placeholders::_2
					, std::placeholders::_3
				)
				);

			packetProcedure->registPacket(
				packetJson::PT_JSON_FOR_WEBSOCKET::packet_value
				, std::bind(&PacketJsonHandler::readJsonWebsocketPacket
					, this
					, std::placeholders::_1
					, std::placeholders::_2
					, std::placeholders::_3
				)
			);
			
		}

		bool readJsonPacket(Session::sptr session, uint32_t size, ByteStream& stream) {

			auto errorLog = [&](const char* msg) {
				auto [addr, port] = session->getEndPointSocket();
				LOGE("{}. remote:({}/{}).", msg, addr, port);
				session->closeReserve(0);
			};

			if (packetJson::PT_JSON::HEADER_SIZE > size) {
				errorLog("invalid packet");
				return false;
			}

			packetJson::PT_JSON req;
			stream.read((unsigned char*)&req, packetJson::PT_JSON::HEADER_SIZE);

			if (packetJson::PT_JSON::MAX_BODY_SIZE < req.bodySize
				|| 0 >= req.bodySize) {
				errorLog("body size error");
				return false;
			}

			try {
				stream.read((unsigned char*)req.body, req.bodySize);
			}
			catch (std::exception& e) {
				errorLog("body pop error");
				return false;
			}

			auto [result, jv, _] = _jsonParsingCallback((unsigned char*)&(req.body), req.bodySize, false);
			if (false == result) {
				errorLog("invalid json string");
				return false;
			}

			char urlString[sizeof(req.url)] = { 0, };
			memcpy(urlString, req.url, sizeof(req.url));

			dispatch(session, urlString, jv);

			return true;
		}

		bool readJsonWebsocketPacket(Session::sptr session, uint32_t size, ByteStream& stream) {
			
			auto [result, jv, urlString] = _jsonParsingCallback(stream.data(), stream.size(), true);
			if (false == result) {
				auto [addr, port] = session->getEndPointSocket();
				LOGE("{}. remote:({}/{}).", "invalid json string", addr, port);
				session->closeReserve(0);
				return false;
			}

			dispatch(session, urlString, jv);

			return true;
		}

		void dispatch(Session::sptr session, const std::string& url, JSON_VALUE_TYPE& jv) {
			auto exceptionHandler = [&session, &url, &jv](const char* msg) {
				auto [addr, port] = session->getEndPointSocket();
				LOGE("{}, remote:({}/{}).", msg, addr, port);
				session->closeReserve(0);
			};

			if (auto it = _URLs.find(url);  _URLs.end() != it) {
				try {
					auto user = session->getUser();
					if (!user) {
						[[unlikely]]
						LOGC("lost user-ptr.");
						session->closeReserve(0);
						return;
					}

					it->second(user, url, jv);
					return;
				}
				catch (std::exception& e) {
					LOGE(e.what());
					exceptionHandler("invalid json parsing");
				}
				catch (...) {
					exceptionHandler("invalid json parsing");
				}
			}
			else {
				exceptionHandler( ("invalid url:"+url).c_str());
			}
		}

		void registJsonPacketHandler(const char* url, JsonPacketHandlerType handler) {
			_URLs[url] = handler;
		}

	protected:
		std::map<std::string, JsonPacketHandlerType > _URLs;
		JsonParsingCallback _jsonParsingCallback;
	};



}//namespace mln::net {
