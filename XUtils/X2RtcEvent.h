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
#ifndef __X2_RTC_EVENT_H__
#define __X2_RTC_EVENT_H__
#include <list>
#include <map>
#include <string>
#include <string.h>
#include "X2RtcRef.h"

namespace x2rtc {
struct X2RtcEvent
{
	X2RtcEvent(void) : nType(0), pDataPtr(NULL) {

	}
	virtual ~X2RtcEvent(void) {
		if (pDataPtr != NULL) {
			delete pDataPtr;
			pDataPtr = NULL;
		}
	}
	struct DataPtr {
		DataPtr(const char* data, int len) {
			nLen = len;
			pData = new char[len + 1];
			memcpy(pData, data, len);
		}
		virtual ~DataPtr(void) {
			delete[] pData;
		}

		char* pData;
		int nLen;
	};

	int nType;
	std::map<std::string, int> mapInt;
	std::map<std::string, std::string> mapStr;

	DataPtr* pDataPtr;
};

TemplateX2RtcRef(X2RtcEvent);

x2rtc::scoped_refptr< X2RtcEventRef> LoadFromJson(const char* strJson);

std::string ConvertToJson(x2rtc::scoped_refptr< X2RtcEventRef> x2RtcEvent);


}	// namespace x2rtc
#endif	//__X2_RTC_EVENT_H__
