#pragma once

#include <functional>
#include <tuple>
#include <unordered_map>

#include "netTypes.h"
#include "session.hpp"
#include "byteStream.hpp"

namespace mln::net {

	class PacketProcedure
	{
		friend class Session;
		friend class NetService;
		friend class NetServiceAcceptorTcp;
		friend class NetServiceConnectorTcp;


	public:
		using PacketHandlerTy = std::function<bool(Session::sptr, uint32_t, ByteStream&)>;
		using PacketMapTy = std::unordered_map<uint32_t, PacketHandlerTy >;

		using CustomPacketParser = std::function<bool(Session::sptr, ByteStream::sptr
			, PacketProcedure&, PacketMapTy&, PacketMapTy&)>;

		using PacketHandlerTy_cipherInit =
			std::function<bool(const uint16_t, void*, Session::sptr, uint32_t, ByteStream&)>;

		using PacketHandlerTy_cipherDecrypt =
			std::function<bool(const uint16_t, Session::sptr, uint32_t, ByteStream&, ByteStream&)>;

		using PacketHandlerTy_headerBody =
			std::function<bool(Session::sptr, void*, ByteStream&)>;

		using EncryptParameters = std::tuple<
			uint16_t								// encryption type
			, PacketHandlerTy_cipherInit		// Init function type
			, PacketHandlerTy_cipherDecrypt		// decryptor function type
		>;

		PacketProcedure(
			PacketProcedure::CustomPacketParser packetParser, PacketManipulator* manip)
			: _packetParsingFunction(packetParser)
			, _packetHeaderManip(manip)
		{
		}

		~PacketProcedure() {
			_instanceElements.clear();
			_staticElements.clear();
		}

		bool registPacket(const uint32_t packetId, PacketHandlerTy fn) {
			return _staticElements.insert({packetId, fn}).second;
		}

		template< typename ListenerType >
		bool registPacket(
			const uint32_t packetId
			, bool(ListenerType::*func)(Session::sptr, uint32_t, ByteStream&)
			, ListenerType* listener) {
			using namespace std::placeholders;
			return _instanceElements.insert({ packetId, std::bind(func, listener, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) }).second;
		}

		template< typename ListenerType >
		void setCipher(
			const uint16_t encType
			, bool(ListenerType::* fEncryption)(uint16_t, void*, Session::sptr, uint32_t, ByteStream&)
			, bool(ListenerType::* fDecryption)(uint16_t, Session::sptr, uint32_t, ByteStream&, ByteStream&)
			, ListenerType* instance) {

			using namespace std::placeholders;
			_cipherHandlers[encType] = std::make_tuple(
				encType
				, std::bind(fEncryption, instance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
				, std::bind(fDecryption, instance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			);
			setUserCipherType(encType);
		}

		void setCipher(uint16_t encType, PacketHandlerTy_cipherInit initFunc, PacketHandlerTy_cipherDecrypt decFunc) {
			_cipherHandlers[encType] = std::make_tuple(encType
				, initFunc
				, decFunc);
			setUserCipherType(encType);
		}

		template< typename ListenerType >
		void setMsgHandler_HeaderBody(
			bool(ListenerType::* func)(Session::sptr, void*, ByteStream&)
			, ListenerType* instance) {
			using namespace std::placeholders;
			_msgHandlerHeaderBody = std::bind(func, instance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}

		void setMsgHandler_HeaderBody(PacketHandlerTy_headerBody func) {
			_msgHandlerHeaderBody = func;
		}

		EncryptParameters& getCipherHandler(const uint16_t encType) {
			return _cipherHandlers[encType];
		}

		PacketHandlerTy_headerBody& getMsgHandler_HeaderBody() { return _msgHandlerHeaderBody; }
		void setUserCipherType(const uint16_t cipherType) { _userCipherType = cipherType; }
		uint16_t getUserCipherType() const { return _userCipherType; }

		bool dispatch(Session::sptr session, ByteStream::sptr packet) {
			if (Session::SocketStatus::OPEN != session->getSocketStatus()) {
				return false;
			}

			bool result = false;

			if (NULL != _packetParsingFunction) {
				result = _packetParsingFunction(session, packet, *this, _instanceElements, _staticElements);
			}

			session->decReadHandlerPendingCount();

			return result;
		}

		PacketManipulator* getManip() const {return _packetHeaderManip;}

	private:
		PacketMapTy _instanceElements;
		PacketMapTy _staticElements;

		uint16_t		_userCipherType = 0;
		EncryptParameters	_cipherHandlers[UINT8_MAX];
		PacketHandlerTy_headerBody _msgHandlerHeaderBody = NULL;

	protected:
		CustomPacketParser _packetParsingFunction;
		PacketManipulator* _packetHeaderManip;
	};
};//namespace mln::net {