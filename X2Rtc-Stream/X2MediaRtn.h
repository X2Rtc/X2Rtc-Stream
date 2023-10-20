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
#ifndef __X2_MEDIA_RTN_H__
#define __X2_MEDIA_RTN_H__
#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include "X2CodecType.h"
#include "X2GtRtnApi.h"

namespace x2rtc {

class X2MediaRtn
{
public:
	X2MediaRtn(void) {};
	virtual ~X2MediaRtn(void) {}

	static bool InitRtn(const char* strConf);
	static void DeInitRtn();
};

class X2MediaRtnPub
{
public:
	X2MediaRtnPub(void);
	virtual ~X2MediaRtnPub(void);

	void RtnPublish(const char* strAppId, const char* strStreamId);
	void RtnUnPublish();

	void OnX2GtRtnPublishEvent(X2GtRtnPubEvent eEventType, X2GtRtnPubState eState, const char* strInfo);
	void OnX2GtRtnRecvStream(X2MediaData* pMediaData);

	virtual void OnX2GtRtnPublishOK() = 0;
	virtual void OnX2GtRtnPublishForceClosed(int nCode) = 0;

private:
	bool b_publish_ok_;
	std::string str_app_id_;
	std::string str_stream_id_;
};

class X2MediaRtnSub
{
public:
	X2MediaRtnSub(void);
	virtual ~X2MediaRtnSub(void);

	void RtnSubscribe(const char* strAppId, const char* strStreamId);
	void RtnUnSubscribe();
};

}	// namespace x2rtc


#endif	// __X2_MEDIA_RTN_H__

