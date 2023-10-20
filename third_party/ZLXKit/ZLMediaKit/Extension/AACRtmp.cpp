﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "AACRtmp.h"
#include "Rtmp/Rtmp.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

static string getConfig(const RtmpPacket &thiz) {
    string ret;
    if ((RtmpAudioCodec)thiz.getRtmpCodecId() != RtmpAudioCodec::aac) {
        return ret;
    }
    if (thiz.buffer.size() < 4) {
        WarnL << "get aac config failed, rtmp packet is: " << hexdump(thiz.data(), thiz.size());
        return ret;
    }
    ret = thiz.buffer.substr(2);
    return ret;
}

void AACRtmpDecoder::inputRtmp(const RtmpPacket::Ptr &pkt) {
    if (pkt->isConfigFrame()) {
        _aac_cfg = getConfig(*pkt);
        if (!_aac_cfg.empty()) {
            onGetAAC(nullptr, 0, 0);
        }
        return;
    }

    if (!_aac_cfg.empty()) {
        onGetAAC(pkt->buffer.data() + 2, pkt->buffer.size() - 2, pkt->time_stamp);
    }
}

void AACRtmpDecoder::onGetAAC(const char* data, size_t len, uint32_t stamp) {
    auto frame = FrameImp::create();
    frame->_codec_id = CodecAAC;

    //生成adts头
    char adts_header[32] = {0};
    auto size = dumpAacConfig(_aac_cfg, len, (uint8_t *) adts_header, sizeof(adts_header));
    if (size > 0) {
        frame->_buffer.assign(adts_header, size);
        frame->_prefix_size = size;
    } else {
        frame->_buffer.clear();
        frame->_prefix_size = 0;
    }

    if(len > 0){
        //追加负载数据
        frame->_buffer.append(data, len);
        frame->_dts = stamp;
    }

    if(size > 0 || len > 0){
        //有adts头或者实际aac负载
        RtmpCodec::inputFrame(frame);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

AACRtmpEncoder::AACRtmpEncoder(const Track::Ptr &track) {
    _track = dynamic_pointer_cast<AACTrack>(track);
}

void AACRtmpEncoder::makeConfigPacket() {
    if (_track && _track->ready()) {
        //从track中和获取aac配置信息
        _aac_cfg = _track->getConfig();
    }

    if (!_aac_cfg.empty()) {
        makeAudioConfigPkt();
    }
}

bool AACRtmpEncoder::inputFrame(const Frame::Ptr &frame) {
    if (_aac_cfg.empty()) {
        if (frame->prefixSize()) {
            // 包含adts头,从adts头获取aac配置信息
            _aac_cfg = makeAacConfig((uint8_t *)(frame->data()), frame->prefixSize());
        }
        makeConfigPacket();
    }

    if (_aac_cfg.empty()) {
        return false;
    }

    auto pkt = RtmpPacket::create();
    // header
    pkt->buffer.push_back(_audio_flv_flags);
    pkt->buffer.push_back((uint8_t)RtmpAACPacketType::aac_raw);
    // aac data
    pkt->buffer.append(frame->data() + frame->prefixSize(), frame->size() - frame->prefixSize());
    pkt->body_size = pkt->buffer.size();
    pkt->chunk_id = CHUNK_AUDIO;
    pkt->stream_index = STREAM_MEDIA;
    pkt->time_stamp = frame->dts();
    pkt->type_id = MSG_AUDIO;
    RtmpCodec::inputRtmp(pkt);
    return true;
}

void AACRtmpEncoder::makeAudioConfigPkt() {
    _audio_flv_flags = getAudioRtmpFlags(std::make_shared<AACTrack>(_aac_cfg));
    auto pkt = RtmpPacket::create();
    // header
    pkt->buffer.push_back(_audio_flv_flags);
    pkt->buffer.push_back((uint8_t)RtmpAACPacketType::aac_config_header);
    // aac config
    pkt->buffer.append(_aac_cfg);
    pkt->body_size = pkt->buffer.size();
    pkt->chunk_id = CHUNK_AUDIO;
    pkt->stream_index = STREAM_MEDIA;
    pkt->time_stamp = 0;
    pkt->type_id = MSG_AUDIO;
    RtmpCodec::inputRtmp(pkt);
}

}//namespace mediakit