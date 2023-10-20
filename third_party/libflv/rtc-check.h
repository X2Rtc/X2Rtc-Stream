#ifndef __RTC_CHECK_H__
#define __RTC_CHECK_H__
#include <assert.h>
#include <stdio.h>

//@Eric - add check
#ifndef WIN32
#define RELEASE
#endif
#ifdef RELEASE
static void AssertF() {}
#else 
static void AssertF() {
	assert(0);
}
#endif

typedef void(*rtc_flv_check)(const char* fmt, ...);
extern rtc_flv_check funcRtcFlvCheck;

#define RTC_F_CHECK(condition)  \
	if(!(condition)) { \
		AssertF(); \
	}

//if(funcRtcFlvCheck != NULL) { funcRtcFlvCheck("[flv] Assert file: %s line: %d msg: %s\r\n", __FILE__, __LINE__, #condition); } \

#endif	// __RTC_CHECK_H__
