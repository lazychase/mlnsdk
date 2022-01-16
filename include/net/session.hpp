#pragma once

#ifdef MLN_USE_BEAST_WEBSOCKET
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#endif

#include <functional>
#include <memory>
#include <optional>
#include <span>

//#include "boostInclude.h"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "boostObjectPool.hpp"
#include "eventReceiver.hpp"
#include "byteStream.hpp"
#include "logger.hpp"
#include "netTypes.h"

namespace mln::net
{

	class UserBase;

	using MLN_SOCKET_TCP = boost::asio::ip::tcp::socket;
	using MLN_SOCKET_WEB = boost::beast::websocket::stream<boost::beast::tcp_stream>;

	class Session
		: public std::enable_shared_from_this< Session >
		, public BoostObjectPool<Session>
	{
		friend class PacketProcedure;

	public:
		const SessionType _sessionType;

		using sptr = std::shared_ptr<Session>;
		using wptr = std::weak_ptr<Session>;

		using DispatcherType = std::function< bool(sptr, ByteStream::sptr) >;

		enum class SocketStatus{
			OPEN,
			PENDING_CLOSE,
			CLOSED,
		};

		static const int RECV_BUFFER_SIZE = 8192;
		static const size_t HANDLE_READ_RETRY_MAX = 5;

	private:
		inline static std::atomic< size_t > s_identitySeed = { 1 };

	public:

		Session(const SessionType sessionType, boost::asio::io_context& ioc)
			: _sessionType(sessionType)
			, _ioc(ioc)
			, _socket(ioc)
#ifdef MLN_USE_BEAST_WEBSOCKET
			, _socketWeb(ioc)
#endif
			, _strand(ioc)
			, _keepAliveTimer(ioc)
			, _closeReserveTimer(ioc)
		{
			_identity = s_identitySeed.fetch_add(1, std::memory_order_relaxed);

			_socketWeb.binary(true);
		}

		Session(const SessionType sessionType, boost::asio::io_context& ioc
			, EventReceiver* evntReceiver
			, PacketManipulator* packetManip
			, DispatcherType dispatcherCallback
		)
			: _sessionType(sessionType)
			, _ioc(ioc)
			, _socket(ioc)
#ifdef MLN_USE_BEAST_WEBSOCKET
			, _socketWeb(ioc)
#endif
			, _strand(ioc)
			, _socketStatus(SocketStatus::CLOSED)
			, _packetManipulator(packetManip)
			, _dispatchCallback(dispatcherCallback)
			/*, _postRetryCount*/
			, _keepAliveTimer(ioc)
			, _closeReserveTimer(ioc)
			, _eventReceiver(evntReceiver)
			/*, _connectionID(connectionID)*/
		{
			_identity = s_identitySeed.fetch_add(1, std::memory_order_relaxed);

			_socketWeb.binary(true);
		}

		static sptr create(const SessionType sessionType
			, boost::asio::io_context& ioc
			, DispatcherType dispatcherCallback
			, PacketManipulator* packetManip
			, EventReceiver* evntReceiver
			, const size_t keepAliveTimeMs = 0
			, const size_t connectionID = 0
		)
		{
			return std::shared_ptr< Session >(new Session(sessionType
				, ioc
				, evntReceiver
				, packetManip
				, dispatcherCallback
				//, keepAliveTimeMs
				//, connectionID
			), Session::destruct);
		}


		void setSocketStatus(const SocketStatus s) { _socketStatus = s; }
		SocketStatus getSocketStatus() const { return _socketStatus; }

		boost::asio::ip::tcp::socket& socket() {
#ifdef MLN_USE_BEAST_WEBSOCKET
			if (SessionType::TCP == _sessionType) {
				return _socket;
			}
			else {
				return _socketWeb.next_layer().socket();
			}
#else
			return _socket;
#endif
		}

		const boost::asio::ip::tcp::socket& socket() const {
#ifdef MLN_USE_BEAST_WEBSOCKET
			if (SessionType::TCP == _sessionType) {
				return _socket;
			}
			else {
				return _socketWeb.next_layer().socket();
			}
#else
			return _socket;
#endif
		}

#ifdef MLN_USE_BEAST_WEBSOCKET
		boost::beast::websocket::stream<boost::beast::tcp_stream>& websocket() {
			return _socketWeb;
		}

#endif

		boost::asio::io_context::strand& strand() { return _strand; }

		void startAccept() {
			_recvStream = ByteStream::sptr(
				new ByteStream()
			);

			socket().set_option(boost::asio::ip::tcp::no_delay(true));
			socket().set_option(boost::asio::socket_base::linger(true, 0));

			if (SessionType::TCP == _sessionType) {
				socket().async_read_some(
					boost::asio::buffer(_recvBuffer, sizeof(_recvBuffer))
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleRead
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred)));
			}
			else {
				websocket().async_read(
					_recvBufferWeb
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleRead
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred)));
			}

			_socketStatus = SocketStatus::OPEN;

			renewExpireTime();

		}
		void startConnect(const SessionType sessionType = SessionType::TCP) {
			_recvStream = ByteStream::sptr(
				new ByteStream()
			);

			socket().set_option(boost::asio::ip::tcp::no_delay(true));
			socket().set_option(boost::asio::socket_base::linger(true, 0));

			if (SessionType::TCP == sessionType) {
				socket().async_read_some(
					boost::asio::buffer(_recvBuffer, sizeof(_recvBuffer))
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleRead
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred)));
			}
			else {
				websocket().async_read(
					_recvBufferWeb
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleRead
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred)));
			}

			_socketStatus = SocketStatus::OPEN;

			renewExpireTime();
		}

		void renewExpireTime() {
			if (!_keepAliveTime) {return;}

			_keepAliveTimer.expires_from_now(
				boost::posix_time::milliseconds(_keepAliveTime));

			_keepAliveTimer.async_wait(
				boost::asio::bind_executor(_strand
					, boost::bind(&Session::onExpired
						, this
						, boost::asio::placeholders::error)));
		}

		void send(ByteStream::sptr sendStream)
		{
			sendStream->setHeaderSize();

			if (SessionType::TCP == _sessionType) {
				boost::asio::async_write(
					socket()
					, boost::asio::buffer(sendStream->data(), sendStream->size())
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleWrite
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred
							, sendStream
						))
				);
			}
			else { // SesstionType::WEBSOCKET
				websocket().async_write(
					boost::asio::buffer(sendStream->data(), sendStream->size())
					, _strand.wrap(
						boost::bind(&Session::handleWrite
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred
							, sendStream
						))
				);
			}
			
		}

		void sendFixedBuffer(std::span<unsigned char> sendBuffer)
		{
			if (SessionType::TCP == _sessionType) {
				boost::asio::async_write(
					socket()
					, boost::asio::buffer(sendBuffer.data(), sendBuffer.size())
					, boost::asio::bind_executor(_strand
						, boost::bind(&Session::handleWrite
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred
							, std::nullopt
						))
				);
			}
			else {
				websocket().async_write(
					boost::asio::buffer(sendBuffer.data(), sendBuffer.size())
					, _strand.wrap(
						boost::bind(&Session::handleWrite
							, shared_from_this()
							, boost::asio::placeholders::error
							, boost::asio::placeholders::bytes_transferred
							, std::nullopt
						))
				);
			}
			
		}

		void sendByteStream(void* sendBuffer, const size_t sendSize, const bool writeHeader) {
			ByteStream::sptr sendStream = ByteStream::sptr(
				new ByteStream(_packetManipulator, writeHeader)
			);

			sendStream->write((unsigned char*)sendBuffer, sendSize);
			send(sendStream);
		}


		void onExpired(const boost::system::error_code& ec) {
			if (!ec) {
				if (SocketStatus::OPEN == _socketStatus) {
					[[likely]]
					if (_eventReceiver) {
						_eventReceiver->onExpiredSession(shared_from_this());
					}
				}
				else {
					[[unlikely]]
					LOGW("socket status not OPEN");
				}
			}
			else {
				[[unlikely]]
				if (ec != boost::asio::error::operation_aborted) {
					LOGW("error in onExpired(). msg:{}", ec.message());
				}
			}
		}

		void handleRead(const boost::system::error_code ec, const size_t bytes_transferred) {
			
			if (!ec) {
				//fmt::print("recv data. size:{}\n", bytes_transferred);

				if (SessionType::TCP == _sessionType) {
					_recvStream->write(_recvBuffer, bytes_transferred);
				}
				else {
					_recvStream->write((unsigned char*)_recvBufferWeb.data().data(), bytes_transferred);
					unsigned char nullChar = NULL;
					_recvStream->write(&nullChar, sizeof(nullChar));
					_recvBufferWeb.consume(_recvBufferWeb.size());
				}

				incReadHandlerPendingCount();

				boost::asio::post(boost::asio::bind_executor(_strand
					, boost::bind(&Session::dispatch
						, shared_from_this()
						, _recvStream)));

				if (SessionType::TCP == _sessionType) {
					socket().async_read_some(
						boost::asio::buffer(_recvBuffer, sizeof(_recvBuffer))
						, boost::asio::bind_executor(_strand
							, boost::bind(&Session::handleRead
								, shared_from_this()
								, boost::asio::placeholders::error
								, boost::asio::placeholders::bytes_transferred)));
				}else{
					websocket().async_read(
						_recvBufferWeb
						, boost::asio::bind_executor(_strand
							, boost::bind(&Session::handleRead
								, shared_from_this()
								, boost::asio::placeholders::error
								, boost::asio::placeholders::bytes_transferred)));

				}

				renewExpireTime();
			}
			else { //if (!ec) {
				if (boost::asio::error::shut_down != ec.value()	// shutdown
					&& boost::asio::error::eof != ec.value()	// close by client
					&& 0 < _readHandlerPending)
				{
					// boost::asio::error::interrupted
					// boost::asio::error::try_again

					if (HANDLE_READ_RETRY_MAX > _postRetryCount++) {

						boost::asio::post(boost::asio::bind_executor(_strand
							, boost::bind(&Session::handleRead
								, shared_from_this()
								, ec
								, bytes_transferred)));

						return;
					}

					LOGW("increase postRetryCount Limit. msg value : {}, msg string:{}, , pending:{}"
						, std::to_string(ec.value()), ec.message(), std::to_string(_readHandlerPending));
				}

				//LOGE("error in handleRead(). code:{}, msg:{}", ec.value(), ec.message());
				tryCloseByFailed(boost::asio::ip::tcp::socket::shutdown_receive);
			}//if (!ec) {
		}

		void dispatch(ByteStream::sptr byteStream) {
			_dispatchCallback(shared_from_this(), byteStream);
		}

		void handleWrite(const boost::system::error_code& ec
			, size_t bytes_transferred, std::optional<ByteStream::sptr> sendBuffer)
		{
			if (!ec) {
				[[likely]]
				//fmt::print("write data. size:{}\n", bytes_transferred);
				renewExpireTime();
				return;
			}

			// boost::asio::error::interrupted
			// boost::asio::error::try_again

			LOGW("error in handleWrite(). code:{}, msg:{}", ec.value(), ec.message());

			tryCloseByFailed(boost::asio::ip::tcp::socket::shutdown_send);
		}

		void tryCloseByFailed(boost::asio::socket_base::shutdown_type what) {
			boost::system::error_code ec;
			if (SessionType::TCP == _sessionType) {
				socket().shutdown(what, ec);
			}

			if (ec) {
				LOGW("error in tryCloseByFailed(). msg:{}", ec.message());
			}

			if (_socketStatus == SocketStatus::OPEN) {
				_socketStatus = SocketStatus::PENDING_CLOSE;

				_keepAliveTimer.cancel(ec);
				_closeReserveTimer.cancel(ec);

				boost::asio::post(
					boost::asio::bind_executor(_strand
						, boost::bind(&Session::onClose
							, shared_from_this())));
			}
		}

		void onClose() {

			boost::system::error_code ec;

			if (SocketStatus::CLOSED != getSocketStatus()) {
				setSocketStatus(SocketStatus::CLOSED);

				_keepAliveTimer.cancel(ec);
				_closeReserveTimer.cancel(ec);

				_eventReceiver->onClose(shared_from_this());

				if (SessionType::TCP == _sessionType) {
					if (socket().is_open()) {
						socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					}
				}
				else {
					if (websocket().is_open()) {
						websocket().close(boost::beast::websocket::close_code::normal, ec);
					}
				}

				if (ec) {
					LOGW("error in onClose(). status check : {}", ec.message());
				}
			}
		
		}

		void closeReserve(const size_t timeAfterMs, std::function<void(void)> post = nullptr)
		{
			_closeReserveTimer.expires_from_now(
				boost::posix_time::milliseconds(timeAfterMs));

			_closeReserveTimer.async_wait(boost::asio::bind_executor(_strand
				, boost::bind(&Session::onCloseReserveTime
					, this
					, boost::asio::placeholders::error, post)));
		}

		void onCloseReserveTime(const boost::system::error_code& ec, std::function<void(void)> post)
		{
			if (ec) {
				LOGW("error in onCloseReserveTime(). errorCode:{}, errorMsg:{}", ec.value(), ec.message());
			}

			if (_socketStatus == SocketStatus::OPEN) {
				onClose();

				if (post) {
					post();
				}
			}
		}

		std::tuple<std::string, uint16_t> getEndPointSocket() const {
			return { _endPoint.address().to_string()
				, _endPoint.port() };
		}

		void setServiceID(const size_t id) {_netServiceID = id;}
		void setUser(std::shared_ptr<UserBase> spUser) {_spUser = spUser;}
		std::shared_ptr<UserBase> getUser() const { return _spUser;}

		SessionIdType getIdentity() const {return _identity;}
		EventReceiver* getEventReceiver() const {return _eventReceiver;}

		void saveEndPoint() {_endPoint = socket().remote_endpoint();}
		boost::asio::ip::tcp::endpoint getEndPoint() const {return _endPoint;}


	protected:
		size_t incReadHandlerPendingCount() {
			return _readHandlerPending++;
		}

		size_t decReadHandlerPendingCount() {
			return _readHandlerPending--;
		}


	protected:
		SocketStatus _socketStatus;

		boost::asio::io_context& _ioc;

		boost::asio::ip::tcp::socket _socket;
#ifdef MLN_USE_BEAST_WEBSOCKET
		boost::beast::websocket::stream<boost::beast::tcp_stream> _socketWeb;
#endif
		boost::asio::io_context::strand _strand;
		boost::asio::ip::tcp::endpoint _endPoint;

		size_t _keepAliveTime = 0;
		boost::asio::deadline_timer _keepAliveTimer;
		boost::asio::deadline_timer _closeReserveTimer;

		size_t _readHandlerPending = 0;
		size_t _postRetryCount = 0;
		unsigned char _recvBuffer[RECV_BUFFER_SIZE];
		ByteStream::sptr _recvStream;
#ifdef MLN_USE_BEAST_WEBSOCKET
		boost::beast::flat_buffer _recvBufferWeb;
#endif


		EventReceiver* _eventReceiver = nullptr;
		DispatcherType _dispatchCallback;

		PacketManipulator* _packetManipulator = nullptr;

		size_t _netServiceID;
		SessionIdType _identity;

		std::shared_ptr<UserBase> _spUser;
	};

}//namespace mln::net
