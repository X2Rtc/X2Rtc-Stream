#include "X2NetRtpConnection.h"
#include "XUtil.h"
#include "X2NetConnection.h"
#include "Rtsp/Rtsp.h"

namespace x2rtc {
extern void X2NetGetAddressInfo(const struct sockaddr* addr, int* family, std::string& ip, uint16_t* port);
extern bool X2NetCompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2);
extern X2NetProcess* createX2NetTcpProcess();
extern X2NetProcess* createX2NetUdpProcess();

X2NetRtpConnection::X2NetRtpConnection(void)
	: n_connection_lost_time_(0)
	, b_lost_connection_(false)
	, x2net_extern_rtp_connection_(NULL)
	, x2net_extern_rtcp_connection_(NULL)
	, b_got_remote_rtcp_addr_(false)
	, x2net_tcp_process_(NULL)
	, x2net_tcp_connection_(NULL)
	, x2net_rtp_process_(NULL)
	, x2net_rtcp_process_(NULL)
	, x2net_rtp_connection_(NULL)
	, x2net_rtcp_connection_(NULL)
{


}
X2NetRtpConnection::~X2NetRtpConnection(void)
{

}

int X2NetRtpConnection::InitTcp(const std::string& strStreamId, int nPort)
{
	n_connection_lost_time_ = XGetUtcTimestamp() + 15000;
	str_stream_id_ = strStreamId;
	if (x2net_tcp_process_ == NULL) {
		x2net_tcp_process_ = createX2NetTcpProcess();
		x2net_tcp_process_->SetListener(this);
		return x2net_tcp_process_->Init(nPort);
	}

	return 0;
}

int X2NetRtpConnection::InitUdp(const std::string& strStreamId, int nRtpPort, int nRtcpPort)
{
	n_connection_lost_time_ = XGetUtcTimestamp() + 15000;
	str_stream_id_ = strStreamId;
	if (x2net_rtp_process_ == NULL) {
		x2net_rtp_process_ = createX2NetUdpProcess();
		x2net_rtp_process_->SetListener(this);
		if (x2net_rtp_process_->Init(nRtpPort) != 0) {
			x2net_rtp_process_->DeInit();
			delete x2net_rtp_process_;
			x2net_rtp_process_ = NULL;
			return -1;
		}
	}

	if (x2net_rtcp_process_ == NULL) {
		x2net_rtcp_process_ = createX2NetUdpProcess();
		x2net_rtcp_process_->SetListener(this);
		if (x2net_rtcp_process_->Init(nRtcpPort) != 0) {
			{
				x2net_rtp_process_->DeInit();
				delete x2net_rtp_process_;
				x2net_rtp_process_ = NULL;
			}
			x2net_rtcp_process_->DeInit();
			delete x2net_rtcp_process_;
			x2net_rtcp_process_ = NULL;
			return -2;
		}
	}
	x2net_extern_rtp_connection_ = x2net_rtp_connection_;
	x2net_extern_rtcp_connection_ = x2net_rtcp_connection_;
	return 0;
}

int X2NetRtpConnection::Init(const std::string& strStreamId, X2NetConnection* x2NetRtpConn, X2NetConnection* x2NetRtcpConn)
{
	n_connection_lost_time_ = XGetUtcTimestamp() + 15000;
	str_stream_id_ = strStreamId;
	x2net_extern_rtp_connection_ = x2NetRtpConn;
	x2net_extern_rtcp_connection_ = x2NetRtcpConn;
	return 0;
}
int X2NetRtpConnection::DeInit() 
{
	x2net_extern_rtp_connection_ = NULL;
	x2net_extern_rtcp_connection_ = NULL;
	if (x2net_tcp_connection_ != NULL) {
		x2net_tcp_connection_->SetListener(NULL);
		x2net_tcp_connection_->ForceClose();
		x2net_tcp_connection_ = NULL;
	}
	b_lost_connection_ = true;

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionClosed(this);
	}

	if (x2net_tcp_process_ != NULL) {
		x2net_tcp_process_->DeInit();
		delete x2net_tcp_process_;
		x2net_tcp_process_ = NULL;
	}

	{//* 关闭Udp收流端口
		if (x2net_rtp_connection_ != NULL) {
			x2net_rtp_connection_->ForceClose();
			x2net_rtp_connection_ = NULL;
		}

		if (x2net_rtcp_connection_ != NULL) {
			x2net_rtcp_connection_->ForceClose();
			x2net_rtcp_connection_ = NULL;
		}

		if (x2net_rtp_process_ != NULL) {
			x2net_rtp_process_->DeInit();
			delete x2net_rtp_process_;
			x2net_rtp_process_ = NULL;
		}

		if (x2net_rtcp_process_ != NULL) {
			x2net_rtcp_process_->DeInit();
			delete x2net_rtcp_process_;
			x2net_rtcp_process_ = NULL;
		}
	}
	return 0;
}

int X2NetRtpConnection::UpdateRemoteAddr(struct sockaddr* remoteAddr)
{
	remote_addr_ = *remoteAddr;
	return 0;
}

int X2NetRtpConnection::RunOnce()
{
	if (IsForceClosed()) {
		n_connection_lost_time_ = XGetUtcTimestamp();
	}

	if (n_connection_lost_time_ <= XGetUtcTimestamp()) {
		b_lost_connection_ = true;
	}
	if (b_lost_connection_) {
		return -1;
	}
	{//* TCP
		if (x2net_tcp_process_ != NULL) {
			x2net_tcp_process_->RunOnce();
		}
	}

	{//* UDP
		if (x2net_rtp_process_ != NULL) {
			x2net_rtp_process_->RunOnce();
		}
		if (x2net_rtcp_process_ != NULL) {
			x2net_rtcp_process_->RunOnce();
		}
	}
	return 0;
}

void X2NetRtpConnection::RecvData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	if (!X2NetCompareAddresses(&remote_addr_, &x2NetData->peerAddr)) {
		remote_addr_ = x2NetData->peerAddr;
		int family = 0;
		std::string remoteIp;
		uint16_t remotePort = 0;
		X2NetGetAddressInfo(reinterpret_cast<const struct sockaddr*>(&remote_addr_), &family, remoteIp, &remotePort);

		char ipPort[128];
		sprintf(ipPort, "%s:%d", remoteIp.c_str(), remotePort);
		str_remote_ip_port_ = ipPort;

		if (cb_listener_ != NULL) {
			cb_listener_->OnX2NetRemoteIpPortChanged(this, &remote_addr_);
		}
	}
	
	n_connection_lost_time_ = XGetUtcTimestamp() + 15000;

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

void X2NetRtpConnection::RecvRtcpData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	//Rtcp data don't check Addr changed
	if (!X2NetCompareAddresses(&remote_rtcp_addr_, &x2NetData->peerAddr)) {
		remote_rtcp_addr_ = x2NetData->peerAddr;
		b_got_remote_rtcp_addr_ = true;
	}

	n_connection_lost_time_ = XGetUtcTimestamp() + 15000;

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

//* For X2NetConnection
void X2NetRtpConnection::Send(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	if (mediakit::isRtcp((char*)x2NetData->pData, x2NetData->nLen)) {
		if (b_got_remote_rtcp_addr_) {
			x2NetData->peerAddr = remote_rtcp_addr_;
		}
		else {
			x2NetData->peerAddr = remote_addr_;
			// 默认的，rtcp端口为rtp端口+1
			switch (x2NetData->peerAddr.sa_family) {
			case AF_INET: ((sockaddr_in*)&x2NetData->peerAddr)->sin_port = htons(ntohs(((sockaddr_in*)&x2NetData->peerAddr)->sin_port) + 1); break;
			case AF_INET6: ((sockaddr_in6*)&x2NetData->peerAddr)->sin6_port = htons(ntohs(((sockaddr_in6*)&x2NetData->peerAddr)->sin6_port) + 1); break;
			}
		}

		if (x2net_extern_rtcp_connection_ != NULL) {
			n_connection_lost_time_ = XGetUtcTimestamp() + 15000;

			x2net_extern_rtcp_connection_->Send(x2NetData);
		}
		return;
	}

	if (x2net_extern_rtp_connection_ != NULL) {
		x2NetData->peerAddr = remote_addr_;

		n_connection_lost_time_ = XGetUtcTimestamp() + 15000;

		x2net_extern_rtp_connection_->Send(x2NetData);
	}

	if (x2net_tcp_connection_ != NULL) {
		n_connection_lost_time_ = XGetUtcTimestamp() + 15000;

		x2net_tcp_connection_->Send(x2NetData);
	}
}

//* For X2NetConnection
bool X2NetRtpConnection::CanDelete()
{
	return IsForceClosed() && b_lost_connection_;
}


//* For X2NetProcess::Listener
void X2NetRtpConnection::OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection)
{
	if (x2NetType == X2Net_Tcp) {
		if (x2net_tcp_connection_ != NULL) {
			x2net_tcp_connection_->SetListener(NULL);
			x2net_tcp_connection_->ForceClose();
			x2net_tcp_connection_ = NULL;
		}

		x2net_tcp_connection_ = connection;
		if (x2net_tcp_connection_ != NULL) {
			x2net_tcp_connection_->SetListener(this);
		}
	}
	else {
		if (x2net_rtp_connection_ == NULL) {
			x2net_rtp_connection_ = connection;
			if (x2net_rtp_connection_ != NULL) {
				x2net_rtp_connection_->SetListener(this);
			}
		}
		else if (x2net_rtcp_connection_ == NULL) {
			x2net_rtcp_connection_ = connection;
			if (x2net_rtcp_connection_ != NULL) {
				x2net_rtcp_connection_->SetListener(this);
			}
		}
	}
}

//* For X2NetConnection::Listener
void X2NetRtpConnection::OnX2NetConnectionPacketReceived(
	X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}
void X2NetRtpConnection::OnX2NetConnectionClosed(X2NetConnection* connection)
{
	if (x2net_tcp_connection_ == connection) {
		x2net_tcp_connection_->SetListener(NULL);
		x2net_tcp_connection_->ForceClose();
		x2net_tcp_connection_ = NULL;
	}
}


}	// namespace x2rtc