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
#ifndef __X2_NET_WEBRTC_PROCESS_H__
#define __X2_NET_WEBRTC_PROCESS_H__
#include "X2NetProcess.h"
#include "X2NetWebRtcConnection.h"
#include "X2WhipTcpConnection.h"
#include "UvThread.h"
#include "RTC/Shared.hpp"
#include "RTC/TcpServer.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"

namespace x2rtc {

class X2NetWebRtcProcess : public X2NetProcess , public UvThread, 
	public Channel::ChannelSocket::Listener, public Channel::ChannelPSocket::Listener,
	public PayloadChannel::PayloadChannelSocket::Listener, public X2NetWebRtcConnection::Callback,
	public RTC::TcpServer::Listener, public RTC::TcpConnection::Listener
{
public:
	X2NetWebRtcProcess(void);
	virtual ~X2NetWebRtcProcess(void);

	virtual int Init(int nPort);
	virtual int DeInit();
	virtual int SetParameter(const char* strJsParams);

	void SendRequset(const std::string& strMethod, const std::string& strHandlerId, const std::string& strData);

	//* For UvThread
	virtual void OnUvThreadStart();
	virtual void OnUvThreadStop();
	virtual void OnUvThreadTimer();

	//* For Channel::ChannelSocket::Listener
	virtual void HandleRequest(Channel::ChannelRequest* request);
	virtual void OnChannelClosed(Channel::ChannelSocket* channel);

	//* For Channel::ChannelPSocket::Listener
	virtual void OnChannelPResponse(
		Channel::ChannelPSocket* channel, json* respone);
	virtual void OnChannelPClosed(Channel::ChannelPSocket* channel);

	virtual void HandleRequest(PayloadChannel::PayloadChannelRequest* request);
	virtual void HandleNotification(PayloadChannel::PayloadChannelNotification* notification);
	virtual void OnPayloadChannelClosed(PayloadChannel::PayloadChannelSocket* payloadChannel);

	//* For X2NetWebRtcConnection::Callback
	virtual void OnX2NetWebRtcConnectionSendRequest(X2NetWebRtcConnection* x2NetWebRtcConn, const std::string& strMethod, const std::string& strId, const std::string& strData);
	virtual void OnX2NetWebRtcConnectionLostConnection(X2NetWebRtcConnection* x2NetWebRtcConn, const std::string& strId);
	virtual void OnX2NetWebRtcConnectionAddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword, ClientIpListener* ipListener);
	virtual void OnX2NetWebRtcConnectionRemoveRemoteId(const std::string& strRemoteId);

public:
	//* For RTC::TcpServer::Listener
	virtual void OnRtcTcpConnectionAccept(
		RTC::TcpServer* tcpServer, RTC::TcpConnection** connection);
	virtual void OnRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection);

	//* For RTC::TcpConnection::Listener
	virtual int OnTcpConnectionPacketProcess(RTC::TcpConnection* connection, const uint8_t* data, size_t len);

private:
	// Channel socket. If Worker instance runs properly, this socket is closed by
	// it in its destructor. Otherwise it's closed here by also letting libuv
	// deallocate its UV handles.
	std::unique_ptr<Channel::ChannelSocket> channel_rtc_{ nullptr };
	std::unique_ptr<Channel::ChannelPSocket> channel_local_{ nullptr };

	std::unique_ptr< PayloadChannel::PayloadChannelSocket> payload_channel_{ nullptr };

	uint32_t n_req_id_{ 0 };

private:
	std::unique_ptr <RTC::Shared> rtc_shared_;

	bool b_init_;
	uint32_t n_id_idx_;

	struct RtcRequest {
		std::string strMethod;
		X2NetWebRtcConnection* x2NetWebRtcConn;
	};
	std::map<uint32_t, RtcRequest> map_rtc_request_waitfor_response_;

private:
	RTC::TcpServer* tcp_server_;
	int n_listen_port_;

};

}	// namespace x2rtc
#endif	// __X2_NET_WEBRTC_PROCESS_H__