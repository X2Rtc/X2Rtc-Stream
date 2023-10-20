﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_H265RTPCODEC_H
#define ZLMEDIAKIT_H265RTPCODEC_H

#include "Rtsp/RtpCodec.h"
#include "Extension/H265.h"
// for DtsGenerator
#include "Common/Stamp.h"

namespace mediakit{

/**
 * h265 rtp解码类
 * 将 h265 over rtsp-rtp 解复用出 h265-Frame
 * 《草案（H265-over-RTP）draft-ietf-payload-rtp-h265-07.pdf》
 */
class H265RtpDecoder : public RtpCodec {
public:
    using Ptr = std::shared_ptr<H265RtpDecoder>;

    H265RtpDecoder();
    ~H265RtpDecoder() {}

    /**
     * 输入265 rtp包
     * @param rtp rtp包
     * @param key_pos 此参数忽略之
     */
    bool inputRtp(const RtpPacket::Ptr &rtp, bool key_pos = true) override;

    CodecId getCodecId() const override{
        return CodecH265;
    }

private:
    bool unpackAp(const RtpPacket::Ptr &rtp, const uint8_t *ptr, ssize_t size, uint64_t stamp);
    bool mergeFu(const RtpPacket::Ptr &rtp, const uint8_t *ptr, ssize_t size, uint64_t stamp, uint16_t seq);
    bool singleFrame(const RtpPacket::Ptr &rtp, const uint8_t *ptr, ssize_t size, uint64_t stamp);

    bool decodeRtp(const RtpPacket::Ptr &rtp);
    H265Frame::Ptr obtainFrame();
    void outputFrame(const RtpPacket::Ptr &rtp, const H265Frame::Ptr &frame);

private:
    bool _using_donl_field = false;
    bool _gop_dropped = false;
    bool _fu_dropped = true;
    uint16_t _last_seq = 0;
    H265Frame::Ptr _frame;
    DtsGenerator _dts_generator;
};

/**
 * 265 rtp打包类
 */
class H265RtpEncoder : public H265RtpDecoder ,public RtpInfo{
public:
    using Ptr = std::shared_ptr<H265RtpEncoder>;

    /**
     * @param ui32Ssrc ssrc
     * @param ui32MtuSize mtu大小
     * @param ui32SampleRate 采样率，强制为90000
     * @param ui8PayloadType pt类型
     * @param ui8Interleaved rtsp interleaved
     */
    H265RtpEncoder(uint32_t ui32Ssrc,
                   uint32_t ui32MtuSize = 1400,
                   uint32_t ui32SampleRate = 90000,
                   uint8_t ui8PayloadType = 96,
                   uint8_t ui8Interleaved = TrackVideo * 2);
    ~H265RtpEncoder() {}

    /**
     * 输入265帧
     * @param frame 帧数据，必须
     */
    bool inputFrame(const Frame::Ptr &frame) override;

    /**
     * 刷新输出所有frame缓存
     */
    void flush() override;

private:
    void packRtp(const char *ptr, size_t len, uint64_t pts, bool is_mark, bool gop_pos);
    void packRtpFu(const char *ptr, size_t len, uint64_t pts, bool is_mark, bool gop_pos);
    void insertConfigFrame(uint64_t pts);
    bool inputFrame_l(const Frame::Ptr &frame, bool is_mark);
private:
    Frame::Ptr _sps;
    Frame::Ptr _pps;
    Frame::Ptr _vps;
    Frame::Ptr _last_frame;
};

}//namespace mediakit{

#endif //ZLMEDIAKIT_H265RTPCODEC_H
