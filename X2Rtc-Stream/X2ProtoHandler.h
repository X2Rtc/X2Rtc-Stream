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
#ifndef __X2_PROTO_HANDLER_H__
#define __X2_PROTO_HANDLER_H__
#include <stdint.h>
#include "X2CodecType.h"

namespace x2rtc {

enum X2ProtoType
{
	X2Proto_Rtmp = 0,
	X2Proto_Flv = 1,
	X2Proto_MpegTS = 2,
	X2Proto_MpegPS = 3,
	X2Proto_Rtc = 4,
	X2Proto_Rtp = 5,
	X2Proto_Talk = 6,
};

class X2ProtoHandler
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2ProtoHandlerPublish(const char* app, const char* stream, const char* token, const char*strParam) = 0;
		virtual void OnX2ProtoHandlerPlay(const char* app, const char* stream, const char* token, const char* strParam) = 0;
		virtual void OnX2ProtoHandlerPlayStateChanged(bool bPlayed) = 0;
		virtual void OnX2ProtoHandlerClose() = 0;
		virtual void OnX2ProtoHandlerSetPlayCodecType(X2CodecType eAudCodecType, X2CodecType eVidCodecType) = 0;
		virtual void OnX2ProtoHandlerSendData(const uint8_t* pHdr, size_t nHdrLen, const uint8_t* pData, size_t nLen) = 0;
		virtual void OnX2ProtoHandlerRecvScript(const uint8_t* data, size_t bytes, int64_t timestamp) = 0;
		virtual void OnX2ProtoHandlerRecvCodecData(scoped_refptr<X2CodecDataRef> x2CodecData) = 0;
	};
public:
	X2ProtoHandler(void):cb_listener_(NULL), b_has_error_(false), e_type_(X2Proto_Rtmp){};
	virtual ~X2ProtoHandler(void) {};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

	void SetType(X2ProtoType eType) {
		e_type_ = eType;
	}
	X2ProtoType GetType() {
		return e_type_;
	}

	bool HasError() { return b_has_error_; };
	virtual bool SetStreamId(const char* strStreamId) { return false; };
	virtual void MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn) = 0;
	virtual void RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen) = 0;
	virtual void Close() = 0;

protected:
	Listener* cb_listener_;
	bool b_has_error_;

	X2ProtoType e_type_;
};

X2ProtoHandler* createX2ProtoHandler(X2ProtoType eType);

}	// namespace x2rtc

#endif	// __X2_PROTO_HANDLER_H__
