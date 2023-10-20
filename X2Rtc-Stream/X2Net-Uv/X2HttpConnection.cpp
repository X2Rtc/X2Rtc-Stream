#define MS_CLASS "x2rtc::X2HttpConnection"
// #define MS_LOG_DEV_LEVEL 3

#include "X2HttpConnection.h"
#include "X2Logger.hpp"
#include "X2Utils.hpp"
#include <cstring> // std::memmove(), std::memcpy()

namespace x2rtc
{
	/* Static. */

	static constexpr size_t ReadBufferSize{ 65536 };
	thread_local static uint8_t ReadBuffer[ReadBufferSize];

	/* Instance methods. */

	X2HttpConnection::X2HttpConnection(Listener* listener, size_t bufferSize)
	  : ::X2TcpConnectionHandler::X2TcpConnectionHandler(bufferSize), listener(listener)
	{
		MS_TRACE();

		X2TcpConnectionHandler::SetSelfUvLoop(uv_default_loop());
	}

	X2HttpConnection::~X2HttpConnection()
	{
		MS_TRACE();
	}

	void X2HttpConnection::UserOnTcpConnectionRead()
	{
		MS_TRACE();

		MS_DEBUG_DEV(
			"data received [local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
			GetLocalIp().c_str(),
			GetLocalPort(),
			GetPeerIp().c_str(),
			GetPeerPort());

		// Be ready to parse more than a single frame in a single TCP chunk.
		while (true)
		{
			// We may receive multiple packets in the same TCP chunk. If one of them is
			// a DTLS Close Alert this would be closed (Close() called) so we cannot call
			// our listeners anymore.
			if (IsClosed())
				return;

			size_t dataLen = this->bufferDataLen - this->frameStart;
			size_t packetLen = dataLen;


			// We have packetLen bytes.
			if (dataLen > 0)
			{
				const uint8_t* packet = this->buffer + this->frameStart;

				// Update received bytes and notify the listener.
				int nProcessed = 0;
				if (packetLen != 0)
				{
					if (listener != NULL) {
						nProcessed = listener->OnHttpConnectionPacketProcess(this, packet, packetLen);
					}
				}

				this->frameStart = 0;
				this->bufferDataLen -= nProcessed;
				break;
			}

			// Exit the parsing loop.
			break;
		}
	}

	void X2HttpConnection::SendResponse(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2)
	{
		MS_TRACE();

		X2TcpConnectionHandler::Write(data1, len1, data2, len2, NULL);
	}
} // namespace RTC
