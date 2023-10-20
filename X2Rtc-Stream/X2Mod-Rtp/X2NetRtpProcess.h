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
#ifndef __X2_NET_RTP_PROCESS_H__
#define __X2_NET_RTP_PROCESS_H__
#include "X2NetProcess.h"

namespace x2rtc {

class X2NetRtpProcess : public X2NetProcess, public X2NetProcess::Listener, public X2NetConnection::Listener
{
public:
	X2NetRtpProcess(void);
	virtual ~X2NetRtpProcess(void);

	virtual int Init(int nPort);
	virtual int DeInit();
	virtual int SetParameter(const char* strJsParams);

	//* For X2NetProcess
	virtual int RunOnce();

	//* For X2NetProcess::Listener
	virtual void OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection);

	//* For X2NetConnection::Listener
	virtual void OnX2NetConnectionPacketReceived(
		X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
	virtual void OnX2NetConnectionClosed(X2NetConnection* connection);

private:
	X2NetProcess* x2net_rtp_process_;
	X2NetProcess* x2net_rtcp_process_;
	X2NetConnection* x2net_rtp_connection_;
	X2NetConnection* x2net_rtcp_connection_;

	uint32_t n_id_idx_;

	JMutex cs_list_recv_data_;
	std::list< x2rtc::scoped_refptr<X2NetDataRef>> list_recv_data_;

private:
	struct SsrcFrom
	{
		int64_t nReportTime;
		std::string strIpPort;

	};
	std::map<int32_t, SsrcFrom> map_ssrc_not_found_;
};

}	// namespace x2rtc
#endif	// __X2_NET_RTP_PROCESS_H__