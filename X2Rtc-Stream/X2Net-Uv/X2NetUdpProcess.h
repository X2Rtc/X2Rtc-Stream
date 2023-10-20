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
#ifndef __X2_NET_UDP_PROCESS_H__
#define __X2_NET_UDP_PROCESS_H__
#include "X2NetProcess.h"
#include "X2NetUdpConnection.h"

namespace x2rtc {



class X2NetUdpProcess : public X2NetProcess
{
public:
	X2NetUdpProcess(void);
	virtual ~X2NetUdpProcess(void);

	virtual int Init(int nPort);
	virtual int DeInit();

	//* For X2NetProcess
	virtual int RunOnce();

private:
	X2NetUdpConnection* x2net_udp_connection_;


private:

};

}	// namespace x2rtc
#endif	// __X2_NET_UDP_PROCESS_H__


