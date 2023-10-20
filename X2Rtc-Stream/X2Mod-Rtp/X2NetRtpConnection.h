/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __X2_NET_RTP_CONNECTION_H__ 
#define __X2_NET_RTP_CONNECTION_H__
#include "X2NetConnection.h"
#include "X2NetProcess.h"

namespace x2rtc {
class X2NetRtpConnection : public X2NetConnection, public X2NetProcess::Listener, public X2NetConnection::Listener
{
public:
	X2NetRtpConnection(void);
	virtual ~X2NetRtpConnection(void);

	int InitTcp(const std::string& strStreamId, int nPort);
	int InitUdp(const std::string& strStreamId, int nRtpPort, int nRtcpPort);
	int Init(const std::string& strStreamId, X2NetConnection* x2NetRtpConn, X2NetConnection* x2NetRtcpConn);
	int DeInit();
	int UpdateRemoteAddr(struct sockaddr* remoteAddr);

	int RunOnce();

	void RecvData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);

	void RecvRtcpData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);

	//* For X2NetConnection
	virtual void Send(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);

	//* For X2NetConnection
	virtual bool CanDelete();

	//* For X2NetProcess::Listener
	virtual void OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection);

	//* For X2NetConnection::Listener
	virtual void OnX2NetConnectionPacketReceived(
		X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
	virtual void OnX2NetConnectionClosed(X2NetConnection* connection);

private:
	int64_t				n_connection_lost_time_;	// If there is no data for a long time, disconnect directly
	bool				b_lost_connection_;
	X2NetConnection*	x2net_extern_rtp_connection_;
	X2NetConnection*	x2net_extern_rtcp_connection_;
	
	struct sockaddr remote_addr_;
	bool b_got_remote_rtcp_addr_;
	struct sockaddr remote_rtcp_addr_;

private:
	//* TCP 收流
	X2NetProcess* x2net_tcp_process_;
	X2NetConnection* x2net_tcp_connection_;

private:
	//* UDP 独立端口收流
	X2NetProcess* x2net_rtp_process_;
	X2NetProcess* x2net_rtcp_process_;
	X2NetConnection* x2net_rtp_connection_;
	X2NetConnection* x2net_rtcp_connection_;
};

}	// namespace x2rtc
#endif	// __X2_NET_RTP_CONNECTION_H__