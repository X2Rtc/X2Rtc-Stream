﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_TRACK_H
#define ZLMEDIAKIT_TRACK_H

#include <memory>
#include <string>
#include "Frame.h"
#include "Rtsp/Rtsp.h"

namespace mediakit{

/**
 * 媒体通道描述类，也支持帧输入输出
 */
class Track : public FrameDispatcher , public CodecInfo{
public:
    using Ptr = std::shared_ptr<Track>;
    Track() = default;
    virtual ~Track() = default;

    /**
     * 是否准备好，准备好才能获取譬如sps pps等信息
     */
    virtual bool ready() = 0;

    /**
     * 克隆接口，用于复制本对象用
     * 在调用该接口时只会复制派生类的信息
     * 环形缓存和代理关系不能拷贝，否则会关系紊乱
     */
    virtual Track::Ptr clone() = 0;

    /**
     * 生成sdp
     * @return  sdp对象
     */
    virtual Sdp::Ptr getSdp() = 0;

    /**
     * 返回比特率
     * @return 比特率
     */
    virtual int getBitRate() const { return _bit_rate; }

    /**
     * 设置比特率
     * @param bit_rate 比特率
     */
    virtual void setBitRate(int bit_rate) { _bit_rate = bit_rate; }

    /**
     * 复制拷贝，只能拷贝派生类的信息，
     * 环形缓存和代理关系不能拷贝，否则会关系紊乱
     */
    Track(const Track &that){
        _bit_rate = that._bit_rate;
    }

private:
    int _bit_rate = 0;
};

/**
 * 视频通道描述Track类，支持获取宽高fps信息
 */
class VideoTrack : public Track {
public:
    using Ptr = std::shared_ptr<VideoTrack>;

    /**
     * 返回视频高度
     */
    virtual int getVideoHeight() const { return 0; }

    /**
     * 返回视频宽度
     */
    virtual int getVideoWidth() const { return 0; }

    /**
     * 返回视频fps
     */
    virtual float getVideoFps() const { return 0; }
};

/**
 * 音频Track派生类，支持采样率通道数，采用位数信息
 */
class AudioTrack : public Track {
public:
    using Ptr = std::shared_ptr<AudioTrack>;

    /**
     * 返回音频采样率
     */
    virtual int getAudioSampleRate() const  {return 0;};

    /**
     * 返回音频采样位数，一般为16或8
     */
    virtual int getAudioSampleBit() const {return 0;};

    /**
     * 返回音频通道数
     */
    virtual int getAudioChannel() const {return 0;};
};

class AudioTrackImp : public AudioTrack{
public:
    using Ptr = std::shared_ptr<AudioTrackImp>;

    /**
     * 构造函数
     * @param codecId 编码类型
     * @param sample_rate 采样率(HZ)
     * @param channels 通道数
     * @param sample_bit 采样位数，一般为16
     */
    AudioTrackImp(CodecId codecId,int sample_rate, int channels, int sample_bit){
        _codecid = codecId;
        _sample_rate = sample_rate;
        _channels = channels;
        _sample_bit = sample_bit;
    }

    /**
     * 返回编码类型
     */
    CodecId getCodecId() const override{
        return _codecid;
    }

    /**
     * 是否已经初始化
     */
    bool ready() override {
        return true;
    }

    /**
     * 返回音频采样率
     */
    int getAudioSampleRate() const override{
        return _sample_rate;
    }

    /**
     * 返回音频采样位数，一般为16或8
     */
    int getAudioSampleBit() const override{
        return _sample_bit;
    }

    /**
     * 返回音频通道数
     */
    int getAudioChannel() const override{
        return _channels;
    }
private:
    CodecId _codecid;
    int _sample_rate;
    int _channels;
    int _sample_bit;
};

class TrackSource{
public:
    TrackSource() = default;
    virtual ~TrackSource() = default;

    /**
     * 获取全部的Track
     * @param trackReady 是否获取全部已经准备好的Track
     */
    virtual std::vector<Track::Ptr> getTracks(bool trackReady = true) const = 0;

    /**
     * 获取特定Track
     * @param type track类型
     * @param trackReady 是否获取全部已经准备好的Track
     */
    Track::Ptr getTrack(TrackType type , bool trackReady = true) const {
        auto tracks = getTracks(trackReady);
        for(auto &track : tracks){
            if(track->getTrackType() == type){
                return track;
            }
        }
        return nullptr;
    }
};

}//namespace mediakit
#endif //ZLMEDIAKIT_TRACK_H