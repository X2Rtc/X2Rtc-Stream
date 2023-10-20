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
#ifndef __RTMP_X1_SESSION_H__
#define __RTMP_X1_SESSION_H__
#include <unordered_map>
#include "amf.h"
#include "Rtmp.h"
#include "utils.h"
#include "RtmpProtocol.h"
#include "RtmpMediaSource.h"
#include "Util/TimeTicker.h"
#include "Network/Session.h"
#include "Extension/AACRtmp.h"
#include "Extension/H264Rtmp.h"
#include "Extension/H265Rtmp.h"

namespace mediakit {
class X2RtmpSession : public std::enable_shared_from_this<X2RtmpSession>, public RtmpProtocol, public FrameWriterInterface::Ptr
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;

    public:
        virtual void OnPublish(const char* app, const char* stream, const char* type) = 0;
        virtual void OnPlay(const char* app, const char* stream, double start, double duration, uint8_t reset) = 0;
        virtual void OnClose() = 0;
        virtual void OnSendData(const char* pData, int nLen) = 0;
        virtual void OnSendData(const char* pHdr, int nLen1, const char* pData, int nLen) = 0;
        virtual void OnRecvRtmpData(RtmpPacket::Ptr packet) = 0;
        virtual void OnRecvScript(const void* data, size_t bytes, uint32_t timestamp) = 0;
        virtual void OnRecvAudio(const void* data, size_t bytes, uint32_t timestamp) = 0;
        virtual void OnRecvVideo(bool isKeyframe, const void* data, size_t bytes, uint32_t timestamp) = 0;

    };
public:
    using Ptr = std::shared_ptr<X2RtmpSession>;

    X2RtmpSession();
    ~X2RtmpSession() override;

    bool HasError() { return false; };
    void SendData(int codec, const void* data, size_t bytes, uint32_t timestamp);
    void SendAacAudio(const void* data, size_t bytes, uint32_t dts, uint32_t pts);
    void SendH264Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts);
    void SendH265Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts);
    void RecvData(const char* pData, int nLen);
    void DoTick();
    void Close();

    void SetListener(Listener* listener) {
        cb_listener_ = listener;
    };

private:
    void onProcessCmd(AMFDecoder& dec);
    void onCmd_connect(AMFDecoder& dec);
    void onCmd_createStream(AMFDecoder& dec);

    void onCmd_publish(AMFDecoder& dec);
    void onCmd_deleteStream(AMFDecoder& dec);

    void onCmd_play(AMFDecoder& dec);
    void onCmd_play2(AMFDecoder& dec);
    void doPlay(AMFDecoder& dec);
    void doPlayResponse(const std::string& err, const std::function<void(bool)>& cb);
    void sendPlayResponse(const std::string& err, const RtmpMediaSource::Ptr& src);

    void onCmd_seek(AMFDecoder& dec);
    void onCmd_pause(AMFDecoder& dec);
    void onCmd_playCtrl(AMFDecoder& dec);
    void setMetaData(AMFDecoder& dec);

    void onSendMedia(const RtmpPacket::Ptr& pkt);
    void onSendRawData(toolkit::Buffer::Ptr buffer) override {
        _total_bytes += buffer->size();
        //send(std::move(buffer));
        if (cb_listener_ != NULL) {
            cb_listener_->OnSendData(buffer->data(), buffer->size());
        }
    }
    void onRtmpChunk(RtmpPacket::Ptr chunk_data) override;

    template<typename first, typename second>
    inline void sendReply(const char* str, const first& reply, const second& status) {
        AMFEncoder invoke;
        invoke << str << _recv_req_id << reply << status;
        sendResponse(MSG_CMD, invoke.data());
    }

    void setSocketFlags();
    std::string getStreamId(const std::string& str);
    void dumpMetadata(const AMFValue& metadata);
    void sendStatus(const std::initializer_list<std::string>& key_value);

private:
    bool _set_meta_data = false;
    double _recv_req_id = 0;
    //断连续推延时
    uint32_t _continue_push_ms = 0;
    //消耗的总流量
    uint64_t _total_bytes = 0;
    std::string _tc_url;
    //数据接收超时计时器
    toolkit::Ticker _ticker;
    MediaInfo _media_info;
    //std::weak_ptr<RtmpMediaSource> _play_src;
    AMFValue _push_metadata;
    //RtmpMediaSourceImp::Ptr _push_src;
    bool _push_src;

private:
    RtmpRing::RingType::Ptr _rtmp_ring;
    AACRtmpEncoder::Ptr aac_rtmp_enc_;
    H264RtmpEncoder::Ptr h264_rtmp_enc_;
    H265RtmpEncoder::Ptr h265_rtmp_enc_;

protected:
    Listener* cb_listener_;
};
};
#endif	// __RTMP_X1_SESSION_H__