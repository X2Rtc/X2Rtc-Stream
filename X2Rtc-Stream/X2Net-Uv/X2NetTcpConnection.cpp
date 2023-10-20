#define MS_CLASS "RTC::X2NetTcpConnection"
// #define MS_LOG_DEV_LEVEL 3

#include "X2NetTcpConnection.h"
#include "X2Logger.hpp"
#include "X2Utils.hpp"
#include "X2RtcCheck.h"
#include <cstring> // std::memmove(), std::memcpy()


namespace x2rtc
{
/* Instance methods. */
X2TcpDataHanlder::X2TcpDataHanlder(size_t bufferSize) : X2TcpConnectionHandler(bufferSize)
	, cb_listener_(NULL)
	, user_data_(NULL)
{
	X2TcpConnectionHandler::SetSelfUvLoop(uv_default_loop());
};
X2TcpDataHanlder::~X2TcpDataHanlder(void)
{

}
void X2TcpDataHanlder::SetListener(Listener* listener)
{
	cb_listener_ = listener;
}
void X2TcpDataHanlder::SetClosed()
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2TcpDataHanlderClosed();
	}
}
void X2TcpDataHanlder::UserOnTcpConnectionRead()
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
			if (packetLen != 0)
			{
				if (cb_listener_ != NULL) {
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData(packet, packetLen);
					cb_listener_->OnX2TcpDataHanlderPacketReceived(x2NetData);
				}
			}

			this->frameStart = 0;
			this->bufferDataLen = 0;

			break;
		}

		// Exit the parsing loop.
		break;
	}
}

static void cb_loop_timer (uS::Timer* timer)
{
	X2NetTcpConnection* x2NetTcpConn = (X2NetTcpConnection*)timer->userData;
	x2NetTcpConn->RunOnce();
}
X2NetTcpConnection::X2NetTcpConnection()
	: b_sock_closed_(false)
	, uv_timer_(NULL)
	, x2tcp_data_handler_(NULL)
{
	MS_TRACE();
	uv_timer_ = new uS::Timer(uv_default_loop());
	uv_timer_->userData = this;
	uv_timer_->start(cb_loop_timer, 10, 1);
}

X2NetTcpConnection::~X2NetTcpConnection()
{
	MS_TRACE();
	X2RTC_CHECK(x2tcp_data_handler_ == NULL);

	if (uv_timer_ != NULL) {
		uv_timer_->stop();
		uv_timer_->close();
		uv_timer_ = NULL;
	}
}

void X2NetTcpConnection::SetX2TcpDataHanlder(X2TcpDataHanlder* handler)
{
	if (x2tcp_data_handler_ != NULL) {
		x2tcp_data_handler_->SetListener(NULL);
	}

	x2tcp_data_handler_ = handler;
	if (x2tcp_data_handler_ != NULL) {
		x2tcp_data_handler_->SetListener(this);

		char ipPort[128];
		sprintf(ipPort, "%s:%d", x2tcp_data_handler_->GetPeerIp().c_str(), x2tcp_data_handler_->GetPeerPort());
		str_remote_ip_port_ = ipPort;
	}
}
//* For X2NetConnection
bool X2NetTcpConnection::CanDelete()
{
	return X2NetConnection::IsForceClosed() && b_sock_closed_;
}

//* For X2TcpDataHanlder::Listener
void X2NetTcpConnection::OnX2TcpDataHanlderPacketReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

void X2NetTcpConnection::OnX2TcpDataHanlderClosed()
{
	if (uv_timer_ != NULL) {
		uv_timer_->stop();
		uv_timer_->close();
		uv_timer_ = NULL;
	}

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionClosed(this);
	}

	b_sock_closed_ = true;
}

int X2NetTcpConnection::RunOnce()
{
	if (IsForceClosed()) {
		if (x2tcp_data_handler_ != NULL) {
			try {
				x2tcp_data_handler_->Close();
			}
			catch (std::exception&) {
			}
		}
		return 0;
	}
	while (1) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		x2NetData = GetX2NetData();
		if (x2NetData != NULL) {
			if (!b_sock_closed_) {
				if (x2NetData->pHdr != NULL && x2NetData->nHdrLen > 0) {
					if (x2tcp_data_handler_ != NULL) {
						x2tcp_data_handler_->Write(x2NetData->pHdr, x2NetData->nHdrLen, x2NetData->pData, x2NetData->nLen, NULL);
					}
				}
				else {
					if (x2tcp_data_handler_ != NULL) {
						x2tcp_data_handler_->Write(x2NetData->pData, x2NetData->nLen, NULL, 0, NULL);
					}
				}
			}
			if (x2NetData->bSendOnce) {
				break;	//@Eric - RunOnce 
			}
		}
		else {
			break;
		}
	}

	return 0;
}



} // namespace RTC
