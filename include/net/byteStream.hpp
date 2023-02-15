#pragma once

#include "boostObjectPool.hpp"
#include "netTypes.h"

namespace mln::net {

	class ByteStream
		: public BoostObjectPoolTs<ByteStream>
	{
	public:
		using sptr = std::shared_ptr< ByteStream >;

		const static size_t DEFAULT_BUFFER_SIZE = 1024 * 8;

		ByteStream(const size_t bufferSize = DEFAULT_BUFFER_SIZE)
			: _buffer(bufferSize)
		{
			clear();
		}

		ByteStream(PacketManipulator* packetManip, const bool preWriteAsHeaderSize, const size_t bufferSize = DEFAULT_BUFFER_SIZE)
			: _packetManipulator(packetManip)
			, _buffer(bufferSize)
		{
			clear();

			if (true == preWriteAsHeaderSize) {
				advancePosWrite(std::get<PacketManipulatorEnum::GetHeaderSize>(*_packetManipulator)());
			}
		}


		void clear() {
			_posWrite = _posRead = 0;
		}

		size_t size() const {
			return _posWrite - _posRead;
		}

		void setHeaderSize() {
			if (_packetManipulator) {
				std::get<PacketManipulatorEnum::WriteHeaderSize>(*_packetManipulator)(
					size()
					, &_buffer[0] + _posRead
					);
			}
		}

		unsigned char* data() const {
			return (unsigned char *)&_buffer[0] + _posRead;
		}

		bool tryRead(unsigned char * dst, const size_t bytes) {
			if (bytes > size()) [[unlikely]]
				return false;
			
			memcpy(dst, &_buffer[0] + _posRead, bytes);
			return true;
		}

		unsigned char* posBufferWrite() const {return (unsigned char*)&_buffer[0] + _posWrite;}

		/*size_t capacity() const { return _buffer.capacity(); }*/
		size_t capacity() const { return _buffer.size() - _posWrite; }

		size_t available() const {
			/*return &_buffer[0] + _buffer.capacity() - _posWrite;*/
			return capacity();
		}

		ByteStream& write(const unsigned char* src, const size_t bytes) {
			checkAdvanceWriteBuffer(bytes);

			memcpy(&_buffer[0] + _posWrite, src, bytes);
			_posWrite += bytes;
			return *this;
		}

		ByteStream& advancePosWrite(const size_t bytes) {
			checkAdvanceWriteBuffer(bytes);

			_posWrite += bytes;
			return *this;
		}

		ByteStream& read(unsigned char* dst, const size_t bytes) {
			checkAdvanceReadBuffer(bytes);

			memcpy(dst, &_buffer[0] + _posRead, bytes);

			_posRead += bytes;
			return *this;
		}

		ByteStream& advancePosRead(const size_t bytes) {
			checkAdvanceReadBuffer(bytes);

			_posRead += bytes;
			
			if (_posRead == _posWrite) {
				clear();
			}
			
			return *this;
		}

		template< typename T >
		inline ByteStream& operator >> (T& rhs) {
			return read((unsigned char*)&rhs, sizeof(rhs));
		}

		template< typename T >
		inline ByteStream& operator << (T& rhs) {
			return write((unsigned char*)&rhs, sizeof(rhs));
		}

		template< typename T >
		inline ByteStream& operator << (T&& rhs) {
			return write((unsigned char*)&rhs, sizeof(rhs));
		}


		void setManipulator(PacketManipulator* manip) { _packetManipulator = manip; }

	private:
		void checkAdvanceReadBuffer(const size_t bytes) {
			if (size() < bytes) {
				[[unlikely]]
				throw std::runtime_error("reading larger than the data size.");
			}

			if (available() < capacity() * 0.7f) {
				rearrange();
			}
		}

		void checkAdvanceWriteBuffer(const size_t bytes) {
			if (_buffer.capacity() < (size() + bytes)) {
				
				// network data was received but could not be processed and the queue was full.
				// Increase the size of the ByteStream's size or reduce the number of requests"

				const size_t nextCapacity = size() + bytes;
				_buffer.reserve(nextCapacity);
				_buffer.resize(nextCapacity);
			}
		}

		void rearrange() {
			if (_posRead == _posWrite) {
				clear();
			}
			else {
				const size_t bytesCount = size();
				memmove(&_buffer[0], &_buffer[0] + _posRead, bytesCount);
				_posRead = 0;
				_posWrite = bytesCount;
			}
		}

	private:
		/*unsigned char * _posRead = nullptr;
		unsigned char * _posWrite = nullptr;*/
		
		int32_t _posRead;
		int32_t _posWrite;

		std::vector<unsigned char> _buffer;
		PacketManipulator* _packetManipulator = nullptr;
	};
}//namespace mln::net {
