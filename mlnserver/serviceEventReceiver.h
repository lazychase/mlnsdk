#pragma once

#include <net/session.hpp>
#include <net/packetProcedure.hpp>
#include <net/byteStream.hpp>

namespace mlnserver {
	class ServiceEventReceiver
	{
	public:
		void onAccept(mln::net::Session::sptr session);
		void onAcceptFailed(mln::net::Session::sptr session);

		void onClose(mln::net::Session::sptr session);
		void onCloseFailed(mln::net::Session::sptr session);

		void onUpdate(uint64_t deltaTimsMs);

		void onExpiredSession(mln::net::Session::sptr session);

		void noHandler(mln::net::Session::sptr session, mln::net::ByteStream& byteStream);
		//////////////////////////////////////////////////////////////////
	public:
		void initHandler(mln::net::PacketProcedure* packetProc);
	};
}//namespace mlnserver {