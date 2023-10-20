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
#ifndef __X2_FLV_MUXER_H__
#define __X2_FLV_MUXER_H__

#include "Rtmp/Rtmp.h"
#include "Rtmp/RtmpMediaSource.h"
#include "Extension/AACRtmp.h"
#include "Extension/H264Rtmp.h"
#include "Extension/H265Rtmp.h"

namespace mediakit {

class FlvMuxer {
public:
    using Ptr = std::shared_ptr<FlvMuxer>;
    FlvMuxer();
    virtual ~FlvMuxer() = default;

    void start();
    void stop();

    void SendAacAudio(const void* data, size_t bytes, uint32_t dts, uint32_t pts);
    void SendH264Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts);
    void SendH265Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts);

protected:
    virtual void onWrite(const toolkit::Buffer::Ptr &data, bool flush) = 0;
    virtual void onDetach() = 0;
    virtual std::shared_ptr<FlvMuxer> getSharedPtr() = 0;

private:
    void onWriteFlvHeader();
    void onWriteRtmp(const RtmpPacket::Ptr &pkt, bool flush);
    void onWriteFlvTag(const RtmpPacket::Ptr &pkt, uint32_t time_stamp, bool flush);
    void onWriteFlvTag(uint8_t type, const toolkit::Buffer::Ptr &buffer, uint32_t time_stamp, bool flush);
    toolkit::BufferRaw::Ptr obtainBuffer(const void *data, size_t len);
    toolkit::BufferRaw::Ptr obtainBuffer();

private:
    toolkit::ResourcePool<toolkit::BufferRaw> _packet_pool;
    RtmpMediaSource::RingType::RingReader::Ptr _ring_reader;

private:
    RtmpRing::RingType::Ptr _rtmp_ring;
    AACRtmpEncoder::Ptr aac_rtmp_enc_;
    H264RtmpEncoder::Ptr h264_rtmp_enc_;
    H265RtmpEncoder::Ptr h265_rtmp_enc_;
};


}//namespace mediakit
#endif //__X2_FLV_MUXER_H__
