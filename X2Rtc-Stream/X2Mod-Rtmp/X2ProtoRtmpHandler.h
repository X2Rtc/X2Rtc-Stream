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
#ifndef __X2_PROTO_RTMP_HANDLER_H__
#define __X2_PROTO_RTMP_HANDLER_H__
#include "X2ProtoHandler.h"
#include "X2RtmpSession.h"
#include "Rtmp/RtmpDemuxer.h"
using namespace mediakit;

namespace x2rtc {

class X2ProtoRtmpHandler : public X2ProtoHandler, public X2RtmpSession::Listener, public TrackListener
{
public:
	X2ProtoRtmpHandler(void);
	virtual ~X2ProtoRtmpHandler(void);

    //* For X2ProtoHandler
    virtual void MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn);
    virtual void RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen);
    virtual void Close();

	//* For X2RtmpSession::Listener
    virtual void OnPublish(const char* app, const char* stream, const char* type);
    virtual void OnPlay(const char* app, const char* stream, double start, double duration, uint8_t reset);
    virtual void OnClose();
    virtual void OnSendData(const char* pData, int nLen);
    virtual void OnSendData(const char* pHdr, int nLen1, const char* pData, int nLen);
    virtual void OnRecvRtmpData(RtmpPacket::Ptr packet);
    virtual void OnRecvScript(const void* data, size_t bytes, uint32_t timestamp);
    virtual void OnRecvAudio(const void* data, size_t bytes, uint32_t timestamp);
    virtual void OnRecvVideo(bool isKeyframe, const void* data, size_t bytes, uint32_t timestamp);

    //* For TrackListener
    virtual bool addTrack(const Track::Ptr& track);
    bool gotFrame(const mediakit::Frame::Ptr& frame);

private:
	X2RtmpSession::Ptr x2rtmp_session_;

    uint16_t n_aud_seqn_{ 0 };
private:
    mediakit::Track::Ptr audio_track_;
    mediakit::Track::Ptr video_track_;
    RtmpDemuxer::Ptr rtmp_demuxer_;
};

}	// namespace x2rtc
#endif	// __X2_PROTO_RTMP_HANDLER_H__