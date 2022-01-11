#pragma once

#include <type_traits>
#include <cassert>
#include <cstdint>

#pragma warning( push )
#pragma warning( disable : 4351 )

#ifndef MLN_NET_PACKET_JSON_CODE_VALUE_MACRO
#define MLN_NET_PACKET_JSON_CODE_VALUE_MACRO header.code = std::decay<decltype(*this)>::type::packet_value
#endif

#pragma pack(1)   

namespace mln::net {

	namespace packetJson
	{
		using TyPacketCode = uint32_t;

		struct HEADER {
			int32_t			size;
			TyPacketCode	code;
		};

		struct PT_JSON {
			enum { packet_value = 1 };

			enum {
				MAX_BODY_SIZE = 8192,
				MAX_URL_STRING = 32,
				HEADER_SIZE = 47,
			};

#pragma region JsonPacketHeader
			HEADER		header;
			uint32_t	sequenceNo;
			int8_t		isCompressed = 0;
			int8_t		url[MAX_URL_STRING];
			uint16_t	bodySize = 0;
#pragma endregion
			int8_t		jsonBody[MAX_BODY_SIZE];

			PT_JSON() {
				static_assert(HEADER_SIZE == sizeof(PT_JSON) - sizeof(jsonBody)
					, "check header-size");

				MLN_NET_PACKET_JSON_CODE_VALUE_MACRO;
				memset(url, 0, sizeof(url));
			}
		};

		struct PT_HEARTBEAT {
			enum { packet_value = 3 };

			HEADER		header;

			PT_HEARTBEAT() {
				MLN_NET_PACKET_JSON_CODE_VALUE_MACRO;
			}
		};

	};// namespace packetJson
}//namespace mln::net{

#pragma pack()
#pragma warning ( pop )



#define RSP_SEQ	"packetSequenceNum"
#define RSP_RC	"resultCode"
#define RSP_RM	"resultMsg"
#define RSP_OK	"OK"
#define RSP_RC_SYSTEM_ERROR		99