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
#ifndef __X2_PROXY_H__
#define __X2_PROXY_H__
#include <stdio.h>
#include <stdint.h>
#include "X2NetProcess.h"

namespace x2rtc {

class X2Proxy
{
public:
	X2Proxy(void) {}
	virtual ~X2Proxy(void) {}

	virtual int StartTask(const char*strSvrIp, int nSvrPort) { return -1; };
	virtual int StopTask() { return -1; };
	virtual int RunOnce() { return -1; };

	virtual int ImportX2NetProcess(x2rtc::X2ProcessType eType, int nPort) { return -1; };
};

X2Proxy* createX2Proxy();

}	// namespace x2rtc
#endif __X2_PROXY_H__
