#pragma once

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