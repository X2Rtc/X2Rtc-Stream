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
#ifndef __XUTIL_H__
#define __XUTIL_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

uint32_t XGetTimestamp(void);
uint32_t XGetTimeSecond();
int64_t XGetUtcTimestamp(void);
uint16_t XGetLastSequence(uint16_t usSequence1,uint16_t usSequence2);
uint32_t XGenerateSSRC(void);
uint32_t XGetRandomNum();
void XSleep(uint32_t ulMS);

int XSplitChar(const std::string& source, char delimiter,std::vector<std::string>* fields);
void XGetRandomStr(std::string&sRandStr, int len);
void CreateRandomString(std::string& sRandStr, uint32_t len);

int XParseHttpParam(const std::string& strParam, std::map<std::string, std::string>* mapParam);

int XParseCustomParam(const std::string& strParam, std::map<std::string, std::string>* mapParam, char*arrSp, int nSp);

bool XIsFile(std::string path);
bool XIsDirectory(std::string path);
bool XCreateRecursionDir(std::string path);
std::string XGetProcessID();
std::string XGetProcessName();
std::string XGetOsUUId();
std::string XGetCurrentDirectory();

bool XIsIPv4(const std::string& rStr);
bool XIsIPv6(const std::string& rStr);

int16_t X2ReadShort(char** pptr);
void X2WriteShort(char** pptr, uint16_t anInt);


int64_t X2AvRescale(int64_t a, int64_t b, int64_t c);

bool X2IsH264BFrame(const char* pData, int nLen);

#endif 