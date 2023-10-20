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
#ifndef __X2_RTC_CHECK_H__
#define __X2_RTC_CHECK_H__
#include <assert.h>
#include <stdio.h>
//@Eric - add check
#ifndef WIN32
#define RELEASE
#endif
#ifdef RELEASE
static void AssertR() {}
#else 
static void AssertR() {
	assert(0);
}
#endif

typedef void(*x2rtc_check)(const char* fmt, ...);
extern x2rtc_check funcX2RtcCheck;

#define X2RTC_CHECK(condition)  \
	if(!(condition)) { \
		if(funcX2RtcCheck != NULL) { funcX2RtcCheck("[x2rtc] Assert file: %s line: %d msg: %s\r\n", __FILE__, __LINE__, #condition); } \
		AssertR(); \
	}

#endif	//__X2RTC_CHECK_H__
