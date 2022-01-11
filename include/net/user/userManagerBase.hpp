#pragma once

#include <functional>
#include <map>
#include <shared_mutex>
//#include <fmt/core.h>
#include "../netTypes.h"
#include "../logger.hpp"

#include "../byteStream.hpp"
#include "../enc/packetEncType.h"
#include "../session.hpp"

#include "userSessionPair.hpp"


namespace mln::net {

	class UserBase;

	class UserManagerBase
	{
	public:
		using FuncTargetType = std::function< void(Session::sptr)>;

		virtual ~UserManagerBase() = default;

		bool createUser(const EncType::Type encType, void* headerPtr, Session::sptr session
			, uint32_t size, ByteStream& stream) {

			//ROOM_PROTOCOL::PT_SERVER_TIME serverTime;
			//typedef std::decay<decltype(ROOM_PROTOCOL::PT_SERVER_TIME::timeOnServer)>::type timeType;
			//serverTime.timeOnServer = (timeType)time(NULL);
			//user->send(serverTime);
			return true;
		}

		bool decryptPacket(const EncType::Type encType, Session::sptr conn
			, uint32_t packetSize, ByteStream& packet, ByteStream& decryptedPacket) {
			//const int decryptedSize = user->decrypt( ... //
			return true;
		}
		
		template < typename T >
		bool addUserBase(Session::sptr session) {
			if (session->getUser()) {
				[[unlikely]]
				LOGE("created userobj already. ident:{}", session->getIdentity());
			}

			std::shared_ptr<UserBase> user = allocUserBase<T>(session);
			session->setUser(user);

			std::unique_lock<std::shared_mutex> wlock(_mtx_users);
			if (false == _users.insert({ session->getIdentity(), session }).second) {
				[[unlikely]]
				LOGE("failed insert user. identity : {}", session->getIdentity());
				return false;
			}
			return true;
		}

		template < typename T >
		std::shared_ptr<UserBase> allocUserBase(Session::sptr session) {
			return std::shared_ptr<T>(
				new T(session)
				, T::destruct);
		}

		template < typename T >
		void deleteUserBase(Session::sptr session){
			auto spUser = session->getUser();
			if (!spUser) {
				[[unlikely]]
				return;
			}

			std::unique_lock<std::shared_mutex> wlock(_mtx_users);

			if (auto it = _users.find(session->getIdentity()); _users.end() != it) {
				_users.erase(it);
				session->setUser(nullptr);
			}
			else {
				[[unlikely]]
				LOGE("none user. userIdentity : {}", session->getIdentity());
				session->setUser(nullptr);
				return;
			}
		}

		std::optional<Session::wptr> getUser(const SessionIdType identity) {

			std::shared_lock<std::shared_mutex> rlock(_mtx_users);

			if (auto pair = _users.find(identity); _users.end() != pair) {
				[[likely]]
				return pair->second;
			}
			return std::nullopt;
		}

		int functionTarget(FuncTargetType callback, const SessionIdType sessionKey) {
			
			std::shared_lock<std::shared_mutex> rlock(_mtx_users);

			if (auto it = _users.find(sessionKey); _users.end() != it) {
				[[likely]]
				if (it->second) {
					callback(it->second);
				}
				return 0;
			}
			else {
				return -1;
			}
		}

		std::vector<size_t> functionTarget(FuncTargetType callback, const std::vector<size_t>& sessionKeys) {

			std::vector<size_t> failedSessionKeys;
			std::vector< Session::sptr > sessions;

			{
				std::shared_lock<std::shared_mutex> rlock(_mtx_users);

				for (auto sessionKey : sessionKeys) {
					if (auto it = _users.find(sessionKey); _users.end() != it) {
						if (it->second) {
							sessions.push_back(it->second);
							continue;
						}
					}
					failedSessionKeys.push_back(sessionKey);
				}
			}

			for (auto& sptr : sessions) {
				callback(sptr);
			}
			return failedSessionKeys;
		}

		void functionForeach(FuncTargetType callback) {
			
			std::shared_lock<std::shared_mutex> rlock(_mtx_users);

			for (auto& [k, spUser] : _users) {
				if (spUser) {
					callback(spUser);
				}
			}
		}

		static void onClose(Session::sptr conn) {
		}

		size_t getUserCount() const {
			std::shared_lock<std::shared_mutex> rlock(_mtx_users);
			return _users.size();
		}


		template< typename USER >
		bool addUser(Session::sptr session) {
			return addUserBase<USER>(session);
		}

		template< typename USER >
		void closedUser(Session::sptr session)
		{
			auto spUser = std::static_pointer_cast<USER>(session->getUser());

			// 여기는 ...  룸 작성한 뒤 주석 풀기
			// 
			//auto spRoom = spUser->m_wpRoom.lock();
			//if (spRoom) {
			//	boost::asio::dispatch(boost::asio::bind_executor(
			//		*spRoom->GetStrand().get()
			//		, boost::bind(&Room::Leave, spRoom.get()
			//			, spUser->m_userAccount.nUserSN
			//			, true // noti to others
			//		)));

			//	spUser->m_wpRoom.reset();
			//}
			//else {
			//	UserManagerBasis::deleteUserBasis<User>(conn);
			//}

			_userSessionPair.deleteUserSessionKeyPair(spUser->getUserId());
		}

		template < typename USER >
		void deleteUser(Session::sptr session)
		{
			deleteUserBase<USER>(session);
		}


		std::tuple<bool, size_t> insertUserSessionKeyPair(const UserIdType userKey
			, const SessionIdType sessionKey){
			return _userSessionPair.insert(userKey, sessionKey);
		}

		void deleteUserSessionKeyPair(const UserIdType userKey){
			_userSessionPair.deleteUserSessionKeyPair(userKey);
		}

	public:
		UserSessionPair _userSessionPair;

	protected:
		mutable std::shared_mutex _mtx_users;

		std::map< SessionIdType, Session::sptr > _users;
	};
}//namespace mln::net {