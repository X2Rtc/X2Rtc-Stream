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
#ifndef __X2_RTC_DEF_H__
#define __X2_RTC_DEF_H__
#include <stdio.h>
#include <stdint.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define X2R_CALL __cdecl
#if defined(X2RTC_EXPORTS)
#define X2R_API extern "C" __declspec(dllexport)
#define X2R_CPP_API  __declspec(dllexport)
#else
#define X2R_API extern "C" __declspec(dllimport)
#define X2R_CPP_API __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#define X2R_API __attribute__((visibility("default"))) extern "C"
#define X2R_CPP_API __attribute__((visibility("default")))
#define X2R_CALL
#elif defined(__ANDROID__) || defined(__linux__)
#define X2R_API extern "C" __attribute__((visibility("default")))
#define X2R_CPP_API __attribute__((visibility("default")))
#define X2R_CALL
#else
#define X2R_API extern "C"
#define X2R_CPP_API
#define X2R_CALL
#endif

#endif	// __X2_RTC_DEF_H__