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
#ifndef __X2_PROTO_MPEG_TS_HANDLER_H__
#define __X2_PROTO_MPEG_TS_HANDLER_H__
#include "X2ProtoHandler.h"
#include "Decoder.h"
#include "Common/MediaSource.h"

namespace x2rtc {

class X2ProtoMpegTsHandler : public X2ProtoHandler, public mediakit::MediaSinkInterface
{
public:
	X2ProtoMpegTsHandler(void);
	virtual ~X2ProtoMpegTsHandler(void);

	//* For X2ProtoHandler
	virtual bool SetStreamId(const char* strStreamId);
	virtual void MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn);
	virtual void RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen);
	virtual void Close();


	//* For mediakit::MediaSinkInterface
	virtual void resetTracks() {};
	virtual void addTrackCompleted() {};
	virtual bool addTrack(const mediakit::Track::Ptr& track);
	virtual bool inputFrame(const mediakit::Frame::Ptr& frame);
	bool gotFrame(const mediakit::Frame::Ptr& frame);
	bool gotSplitAacFrame(const mediakit::Frame::Ptr& frame);

private:
	mediakit::DecoderImp::Ptr ts_decoder_;
	mediakit::MediaInfo media_info_;

	mediakit::Track::Ptr audio_track_;
	mediakit::Track::Ptr video_track_;
};


}	// namespace x2rtc

#endif	// __X2_PROTO_MPEG_TS_HANDLER_H__
