#pragma once

#include "protocol.h"

#include "../netTypes.h"
#include "../byteStream.hpp"
#include "../packetProcedure.hpp"
#include "../session.hpp"

namespace mln::net {

	class PacketJsonParserManip
	{
	public:
		static size_t getHeaderSize() {return sizeof(packetJson::HEADER);}

		static void setHeaderSize(size_t currentSize, unsigned char* buffer) {
			packetJson::HEADER* header = reinterpret_cast<packetJson::HEADER*>(buffer);
			header->size = currentSize;
			header->code = packetJson::PT_JSON::packet_value;
		}
	};

	class PacketJsonParser
	{
	public:
		inline static PacketManipulator s_packetMainp{
			PacketJsonParserManip::getHeaderSize
			, PacketJsonParserManip::setHeaderSize
		};

		static PacketManipulator* get() {return &s_packetMainp;}

		static bool parse(Session::sptr session, ByteStream::sptr stream
			, [[maybe_unused]] PacketProcedure& pp
			, PacketProcedure::PacketMapTy& memberFuncMap
			, PacketProcedure::PacketMapTy& staticFuncMap
		){
			packetJson::HEADER header;

			do {
				if (false == stream->tryRead((unsigned char*)&header, sizeof(header))) {
					break;
				}

				if (header.size < 0 || header.size > USHRT_MAX) {
					return false;
				}

				if (header.size > stream->size()) {
					break;
				}

				auto packet = std::make_shared<ByteStream>();
				packet->write((unsigned char*)(stream->data()), header.size);
				stream->advancePosRead(header.size);

				if (auto it = staticFuncMap.find(header.code); staticFuncMap.end() != it) {
					it->second(session, header.size, *packet.get());
				}
				else {
					session->getEventReceiver()->noHandler(session, *packet.get());
					packet->clear();
					return false;
				}

			} while (true);

			return true;
		}
	};
}//namespace mln::net