#define MS_CLASS "RTC::X2WhipTcpConnection"
// #define MS_LOG_DEV_LEVEL 3

#include "X2WhipTcpConnection.h"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove(), std::memcpy()


namespace x2rtc
{
/* Instance methods. */
X2WhipTcpConnection::X2WhipTcpConnection(RTC::TcpConnection::Listener* listener, size_t bufferSize)
	: RTC::TcpConnection(listener, bufferSize)
	, cb_listener_(listener)
{
};
X2WhipTcpConnection::~X2WhipTcpConnection(void)
{

}

void X2WhipTcpConnection::SendResponse(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2)
{
	RTC::TcpConnection::Write(data1, len1, data2, len2, NULL);
}
void X2WhipTcpConnection::UserOnTcpConnectionRead()
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
				if (cb_listener_ != NULL) {
					nProcessed = cb_listener_->OnTcpConnectionPacketProcess(this, packet, packetLen);
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

} // namespace RTC
