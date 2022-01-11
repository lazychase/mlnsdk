#pragma once

#include <net/packetJson/protocol.h>
#include <net/user/userBase.hpp>
#include <net/boostObjectPool.hpp>

namespace mlnserver {

	class User
		: public mln::net::UserBase
		, public mln::net::BoostObjectPoolTs<User>
	{
	public:
		User(mln::net::Session::sptr session)
			: UserBase(session)
		{}
	};
}//namespace mlnserver {