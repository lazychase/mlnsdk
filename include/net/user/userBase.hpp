#pragma once

#include <optional>
#include <memory>

#include "../netTypes.h"
#include "../session.hpp"
#include "../enc/packetEncType.h"



namespace mln::net {

	class Room;

	class UserBase
	{
		friend class UserManagerBase;

	public:
		using sptr = std::shared_ptr<UserBase>;
		using wptr = std::weak_ptr<UserBase>;

		UserBase(Session::sptr session) {
			_session = session;
			_sessionType = session->_sessionType;
		}
		virtual ~UserBase() = default;

		bool initEncKey(const EncType::Type encType, void*, unsigned char* buffer, const uint32_t bufferSize, const bool writeHeader) {
			switch (encType){
			case EncType::GREETING:
				// send
				if (auto session = _session.lock(); session) {
					session->sendByteStream((void*)("hello"), 5, writeHeader);
				}
				break;
			}//switch (encType)
			return true;
		}

		int32_t send(unsigned char* data, const uint32_t size, const bool writeHeader) {
			switch (_encType){
			case EncType::GREETING:
			{
				if (auto session = _session.lock(); session) {
					session->sendByteStream(data, size, writeHeader);
					return size;
				}
				return 0;
			}break;
			}//switch (header.encType)
			return 0;
		}

		template <typename T>
		int32_t send(T& data, const bool writeHeader = true) {
			return send((unsigned char*)&data, sizeof(data), writeHeader);
		}

		int32_t decrypt(
			const EncType::Type encType
			, char* entryptedData, const uint32_t entryptedDataSize
			, char* decryptedBuffer, const uint32_t decryptedBufferSize) {
			
			switch (_encType){
			case EncType::GREETING:
				break;

			}//switch (_encType)
			return 0;
		}


		std::optional<Session::sptr> getSession() {
			if (auto session = _session.lock(); session) {
				return session;
			}
			return std::nullopt;
		}

		void closeReserve(const size_t timeAfterMs) {
			if (auto session = _session.lock(); session) {
				session->closeReserve(timeAfterMs);
			}
		}

		int sendJsonPacket(const std::string& url, std::string& body) {
			static uint32_t packetSeqNum = 0;
			++packetSeqNum;

			packetJson::PT_JSON packet;
			memcpy(packet.url, url.c_str(), url.length());
			packet.bodySize = (uint16_t)body.length();

			packet.sequenceNo = -1;
			packet.isCompressed = false;

			memcpy(packet.body, body.c_str(), body.length());

			return send((unsigned char*)&packet
				, packetJson::PT_JSON::HEADER_SIZE + packet.bodySize
				, false // PT_JSON packet is designed to include header information.
			);
		}

		int sendJsonWebsocketPacket(const std::string& payload) {
			if (auto session = _session.lock(); session) {
				session->sendRaw((unsigned char*)payload.data(), payload.size());
				return payload.size();
			}
			return false;
		}

		virtual UserIdType getUserId() const {return _userId;}

	public:
		inline static EncType::Type _encType = EncType::GREETING;

	public:
		UserIdType _userId = 0;

		SessionType _sessionType;
		Session::wptr _session;
		bool m_connected = false;

		std::weak_ptr<boost::asio::io_context::strand> _wpStrandRoom;
	};

}//namespace mln::net {