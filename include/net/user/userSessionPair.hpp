#pragma once

#include <memory>
#include <tuple>

#include "../netTypes.h"
#include "../container/lockfreeLinkedlist.h"

template<typename T>
class LockFreeLinkedList;

namespace mln::net {

	class UserSessionPair
	{
	public:
		std::tuple<bool, size_t> insert(const UserIdType userKey, const SessionIdType sessionKey){

			if (_connectedUserList->Insert( UserSessionKey{userKey, sessionKey})) {
				[[likely]]
				return { true, _connectedUserList->size() };
			}
			else {
				return { false, _connectedUserList->size() };
			}
		}

		void deleteUserSessionKeyPair(const UserIdType userKey) {
			_connectedUserList->Delete({ userKey, 0 });
		}

		bool findSessionKey(const UserIdType userKey, SessionIdType& sessionKey) const
		{
			if (UserSessionKey pairData{ userKey, sessionKey }; true == _connectedUserList->Find(pairData)) {
				[[likely]]
				sessionKey = pairData.sessionKey_;
				return true;
			}
			return false;
		}

	private:
		std::unique_ptr< LockFreeLinkedList < UserSessionKey > >
			_connectedUserList{ std::make_unique< LockFreeLinkedList< UserSessionKey >>() };
	};

}//namespace mln::net {
