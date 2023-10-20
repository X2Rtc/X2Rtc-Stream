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
#ifndef __X2_IMPL_PROXY_H__
#define __X2_IMPL_PROXY_H__
#include "X2Proxy.h"
#include "jthread.h"
#include "Libuv.h"
#include "X2NetPeer.h"
#include "X2Net-Uv/X2NetTcpClient.h"

namespace x2rtc {
class X2ProxyImpl : public X2Proxy, public JThread, public X2NetProcess::Listener, public X2NetPeer::Listener
	, public X2NetTcpClient::Listener, public X2MediaDispatch::Listener
{
public:
	X2ProxyImpl(void);
	virtual ~X2ProxyImpl(void);

	//* For X2Proxy
	virtual int StartTask(const char* strSvrIp, int nSvrPort);
	virtual int StopTask();
	virtual int RunOnce();
	virtual int ImportX2NetProcess(x2rtc::X2ProcessType eType, int nPort);

	void ReportStreamStats(int64_t nUtcTime);

	//* For JThread
	virtual void* Thread();
	void OnTimer();

	//* For X2NetProcess::Listener
	virtual void OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection);
	virtual void OnX2NetProcessSendMessage(const char* strMsg);

	//* For X2NetPeer::Listener
	virtual void OnX2NetPeerEvent(
		X2NetPeer* x2NetPeer, x2rtc::scoped_refptr<X2RtcEventRef> x2RtcEvent);
	virtual void OnX2NetPeerClosed(X2NetPeer* x2NetPeer);
	virtual void OnX2NetPeerStreamStats(bool isPublish, const std::string& strAppId, const std::string& strStreamId,
		StreamInfo* streamInfo, StreamStats* streamStats);

	//* For X2NetTcpClient::Listener
	virtual void OnX2NetTcpClientDataReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
	virtual void OnX2NetTcpClientConnected();
	virtual void OnX2NetTcpClientClosed();
	virtual void OnX2NetTcpClientFailed();

	//* For X2MediaDispatch::Listener
	virtual void OnX2MediaDispatchPublishOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType);
	virtual void OnX2MediaDispatchPublishClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType);
	virtual void OnX2MediaDispatchSubscribeOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType);
	virtual void OnX2MediaDispatchSubscribeClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType);

private:
	//:> Thread for UV loop
	bool b_running_;
	uS::Loop* uv_loop_;
	uS::Timer* uv_timer_;

private:
	//:> 
	typedef std::map<x2rtc::X2ProcessType, X2NetProcess*>MapX2NetProcess;
	JMutex cs_map_x2net_process_;
	MapX2NetProcess map_x2net_process_;

	JMutex cs_list_x2net_peer_delete_;
	std::list<X2NetPeer*>list_x2net_peer_delete_;

private:
	//:> Tcp client
	// Connect to register server to get config & report info etc...
	X2NetTcpClient* x2net_tcp_client_;
	int64_t			n_tcp_reconnect_time_;

	JMutex cs_list_recv_data_;
	std::list< x2rtc::scoped_refptr<X2NetDataRef>> list_recv_data_;

private:
	//:> Stream stats
	struct StreamStatsInfo {
		StreamInfo streamInfo;
		std::list<StreamStats>listStreamStats;
	};
	typedef std::map<std::string/*StreamId|UserId*/, StreamStatsInfo> MapStreamInfo;
	typedef std::map<std::string/*AppId*/, MapStreamInfo> MapAppPubStream;
	JMutex cs_map_app_pub_stream_;
	MapAppPubStream	map_app_pub_stream_;

	typedef std::map<std::string/*StreamId*/, MapStreamInfo>MapSubStreamInfo;
	typedef std::map<std::string/*AppId*/, MapSubStreamInfo>MapAppSubStream;
	JMutex cs_map_app_sub_stream_;
	MapAppSubStream map_app_sub_stream_;

	int64_t n_next_report_pub_stats_time_;		// 3s interval
	int64_t n_next_report_sub_stats_time_;		// 10s interval

private:
	bool b_registed_;
	bool b_abort_;
	int64_t	n_subscribe_report_time_;

	std::string str_node_id_;
};

}	// namespace x2rtc
#endif	// __X2_IMPL_PROXY_H__