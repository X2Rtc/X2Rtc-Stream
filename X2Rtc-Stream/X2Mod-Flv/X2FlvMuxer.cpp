/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "X2FlvMuxer.h"
#include "Util/File.h"
#include "Rtmp/utils.h"
#include "X2HttpSession.h"

#define FILE_BUF_SIZE (64 * 1024)

using namespace std;
using namespace toolkit;

namespace mediakit {
    class X2FlvRingDelegateHelper : public toolkit::RingDelegate<RtmpPacket::Ptr> {
    public:
        using onRtmp = std::function<void(RtmpPacket::Ptr in, bool is_key)>;

        ~X2FlvRingDelegateHelper() override {}

        X2FlvRingDelegateHelper(onRtmp on_rtp) {
            _on_rtmp = std::move(on_rtp);
        }

        void onWrite(RtmpPacket::Ptr in, bool is_key) override {
            _on_rtmp(std::move(in), is_key);
        }

    private:
        onRtmp _on_rtmp;
    };

FlvMuxer::FlvMuxer(){
    _packet_pool.setSize(64);

    _rtmp_ring = std::make_shared<RtmpRing::RingType>();
    _rtmp_ring->setDelegate(std::make_shared<mediakit::X2FlvRingDelegateHelper>([this](mediakit::RtmpPacket::Ptr in, bool is_key) {
        onWriteRtmp(in, true);
        }));
}

void FlvMuxer::SendAacAudio(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
{
    if (aac_rtmp_enc_ == NULL) {
        aac_rtmp_enc_ = std::make_shared< AACRtmpEncoder>(nullptr);
        aac_rtmp_enc_->setRtmpRing(_rtmp_ring);
    }

    if (aac_rtmp_enc_ != NULL) {
        auto frame = std::make_shared<FrameFromPtr>(CodecAAC, (char*)data, bytes, dts, pts, ADTS_HEADER_LEN);
        aac_rtmp_enc_->inputFrame(frame);
    }
}
void FlvMuxer::SendH264Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
{
    if (h264_rtmp_enc_ == NULL) {
        h264_rtmp_enc_ = std::make_shared< H264RtmpEncoder>(nullptr);
        h264_rtmp_enc_->setRtmpRing(_rtmp_ring);
    }

    if (h264_rtmp_enc_ != NULL) {
        splitH264((char*)data, bytes, prefixSize((char*)data, bytes), [&](const char* ptr, size_t len, size_t prefix) {
            auto frame = FrameImp::create<H264Frame>();
            frame->_dts = dts;
            frame->_pts = pts;
            frame->_buffer.assign((char*)ptr, len);
            frame->_prefix_size = prefix;
            h264_rtmp_enc_->inputFrame(frame);
            });
    }
}
void FlvMuxer::SendH265Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
{
    if (h265_rtmp_enc_ == NULL) {
        h265_rtmp_enc_ = std::make_shared< H265RtmpEncoder>(nullptr);
        h265_rtmp_enc_->setRtmpRing(_rtmp_ring);
    }

    if (h265_rtmp_enc_ != NULL) {
        splitH264((char*)data, bytes, prefixSize((char*)data, bytes), [&](const char* ptr, size_t len, size_t prefix) {
            auto frame = FrameImp::create<H265Frame>();
            frame->_dts = dts;
            frame->_pts = pts;
            frame->_buffer.assign((char*)ptr, len);
            frame->_prefix_size = prefix;
            h265_rtmp_enc_->inputFrame(frame);
            });
    }
}

void FlvMuxer::start() {
    onWriteFlvHeader();
}

BufferRaw::Ptr FlvMuxer::obtainBuffer() {
    return _packet_pool.obtain2();
}

BufferRaw::Ptr FlvMuxer::obtainBuffer(const void *data, size_t len) {
    auto buffer = obtainBuffer();
    buffer->assign((const char *) data, len);
    return buffer;
}

void FlvMuxer::onWriteFlvHeader() {
    //发送flv文件头
    auto buffer = obtainBuffer();
    buffer->setCapacity(sizeof(FLVHeader));
    buffer->setSize(sizeof(FLVHeader));

    FLVHeader *header = (FLVHeader *) buffer->data();
    memset(header, 0, sizeof(FLVHeader));
    header->flv[0] = 'F';
    header->flv[1] = 'L';
    header->flv[2] = 'V';
    header->version = FLVHeader::kFlvVersion;
    header->length = htonl(FLVHeader::kFlvHeaderLength);
    header->have_video = true;// src->haveVideo();
    header->have_audio = true;// src->haveAudio();
    //memset时已经赋值为0
    //header->previous_tag_size0 = 0;

    //flv header
    onWrite(buffer, false);


#if 0
    auto &metadata = src->getMetaData();
    if (metadata) {
        //在有metadata的情况下才发送metadata
        //其实metadata没什么用，有些推流器不产生metadata
        AMFEncoder invoke;
        invoke << "onMetaData" << metadata;
        onWriteFlvTag(MSG_DATA, std::make_shared<BufferString>(invoke.data()), 0, false);
    }

    //config frame
    src->getConfigFrame([&](const RtmpPacket::Ptr &pkt) {
        onWriteRtmp(pkt, true);
    });
#endif
}

void FlvMuxer::onWriteFlvTag(const RtmpPacket::Ptr &pkt, uint32_t time_stamp, bool flush) {
    onWriteFlvTag(pkt->type_id, pkt, time_stamp, flush);
}

void FlvMuxer::onWriteFlvTag(uint8_t type, const Buffer::Ptr &buffer, uint32_t time_stamp, bool flush) {
    RtmpTagHeader header;
    header.type = type;
    set_be24(header.data_size, (uint32_t) buffer->size());
    header.timestamp_ex = (time_stamp >> 24) & 0xff;
    set_be24(header.timestamp, time_stamp & 0xFFFFFF);

    //tag header
    onWrite(obtainBuffer((char *) &header, sizeof(header)), false);

    //tag data
    onWrite(buffer, false);

    //PreviousTagSize
    uint32_t size = htonl((uint32_t) (buffer->size() + sizeof(header)));
    onWrite(obtainBuffer((char *) &size, 4), flush);
}

void FlvMuxer::onWriteRtmp(const RtmpPacket::Ptr &pkt, bool flush) {
    onWriteFlvTag(pkt, pkt->time_stamp, flush);
}

void FlvMuxer::stop() {
    if (_ring_reader) {
        _ring_reader.reset();
        onDetach();
    }
}

}//namespace mediakit
