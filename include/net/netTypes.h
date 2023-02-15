#pragma once

namespace mln::net {

	using SessionIdType = size_t;
	using UserIdType = size_t;

	enum class SessionType
	{
		TCP,
		WEBSOCKET
	};

	struct PacketManipulatorEnum {
		const static size_t GetHeaderSize = 0;
		const static size_t WriteHeaderSize = 1;
	};

	using PacketManipulator = std::tuple<
		std::function<size_t()>
		, std::function<void(size_t, unsigned char*)>
	>;

	struct UserSessionKey {
		uint64_t userKey_;
		uint64_t sessionKey_;

		bool operator < (const UserSessionKey rhs) const {
			return userKey_ < rhs.userKey_;
		}
	};

}//namespace mln::net {
