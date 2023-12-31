#include "X2RtpGBProcess.h"
#include "Extension/CommonRtp.h"
#include "Extension/Factory.h"
#include "Extension/G711.h"
#include "Extension/H264.h"
#include "Extension/H265.h"
#include "Extension/Opus.h"
#include "Extension/JPEG.h"
#include "Rtp/TSDecoder.h"
#include "Util/File.h"
#include "Common/config.h"
#include "Rtsp/RtpReceiver.h"
#include "Rtsp/Rtsp.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

namespace x2rtc {

    // 判断是否为ts负载
    static inline bool checkTS(const uint8_t* packet, size_t bytes) {
        return bytes % TS_PACKET_SIZE == 0 && packet[0] == TS_SYNC_BYTE;
    }

    class RtpReceiverImp : public RtpTrackImp {
    public:
        using Ptr = std::shared_ptr<RtpReceiverImp>;

        RtpReceiverImp(int sample_rate, RtpTrackImp::OnSorted cb, RtpTrackImp::BeforeSorted cb_before = nullptr) {
            _sample_rate = sample_rate;
            setOnSorted(std::move(cb));
            setBeforeSorted(std::move(cb_before));
            // GB28181推流不支持ntp时间戳
            setNtpStamp(0, 0);
        }

        ~RtpReceiverImp() override = default;

        bool inputRtp(TrackType type, uint8_t* ptr, size_t len) {
            return RtpTrack::inputRtp(type, _sample_rate, ptr, len).operator bool();
        }

    private:
        int _sample_rate;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////

    X2RtpGBProcess::X2RtpGBProcess(const MediaInfo& media_info, MediaSinkInterface* sink) {
        assert(sink);
        _media_info = media_info;
        _interface = sink;
    }

    void X2RtpGBProcess::onRtpSorted(RtpPacket::Ptr rtp) {
        _rtp_decoder[rtp->getHeader()->pt]->inputRtp(rtp, false);
    }

    void X2RtpGBProcess::flush() {
        if (_decoder) {
            _decoder->flush();
        }
    }

    bool X2RtpGBProcess::inputRtp(bool, const char* data, size_t data_len) {
        GET_CONFIG(uint32_t, h264_pt, RtpProxy::kH264PT);
        GET_CONFIG(uint32_t, h265_pt, RtpProxy::kH265PT);
        GET_CONFIG(uint32_t, ps_pt, RtpProxy::kPSPT);
        GET_CONFIG(uint32_t, opus_pt, RtpProxy::kOpusPT);

        RtpHeader* header = (RtpHeader*)data;
        auto pt = header->pt;
        auto& ref = _rtp_receiver[pt];
        if (!ref) {
            if (_rtp_receiver.size() > 2) {
                // 防止pt类型太多导致内存溢出
                throw std::invalid_argument("rtp pt类型不得超过2种!");
            }
            switch (pt) {
            case Rtsp::PT_PCMA:
            case Rtsp::PT_PCMU: {
                // CodecG711U or CodecG711A
                ref = std::make_shared<RtpReceiverImp>(8000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                auto track = std::make_shared<G711Track>(pt == Rtsp::PT_PCMU ? CodecG711U : CodecG711A, 8000, 1, 16);
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
            case Rtsp::PT_JPEG: {
                // mjpeg
                ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                auto track = std::make_shared<JPEGTrack>();
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
            default: {
                if (pt == opus_pt) {
                    // opus负载
                    ref = std::make_shared<RtpReceiverImp>(48000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<OpusTrack>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else if (pt == h265_pt) {
                    // H265负载
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<H265Track>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else if (pt == h264_pt) {
                    // H264负载
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<H264Track>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else {
                    if (pt != Rtsp::PT_MP2T && pt != ps_pt) {
                        WarnL << "rtp payload type未识别(" << (int)pt << "),已按ts或ps负载处理";
                    }
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    // ts或ps负载
                    _rtp_decoder[pt] = std::make_shared<CommonRtpDecoder>(CodecInvalid, 32 * 1024);
                    // 设置dump目录
                    GET_CONFIG(string, dump_dir, RtpProxy::kDumpDir);
                    if (!dump_dir.empty()) {
                        auto save_path = File::absolutePath(_media_info.stream + ".mpeg", dump_dir);
                        _save_file_ps.reset(File::create_file(save_path.data(), "wb"), [](FILE* fp) {
                            if (fp) {
                                fclose(fp);
                            }
                            });
                    }
                }
                break;
            }
            }
            // 设置frame回调
            _rtp_decoder[pt]->addDelegate([this](const Frame::Ptr& frame) {
                onRtpDecode(frame);
                return true;
                });
        }

        return ref->inputRtp(TrackVideo, (unsigned char*)data, data_len);
    }

    void X2RtpGBProcess::onRtpDecode(const Frame::Ptr& frame) {
        if (frame->getCodecId() != CodecInvalid) {
            // 这里不是ps或ts
            _interface->inputFrame(frame);
            return;
        }

        // 这是TS或PS
        if (_save_file_ps) {
            fwrite(frame->data(), frame->size(), 1, _save_file_ps.get());
        }

        if (!_decoder) {
            // 创建解码器
            if (checkTS((uint8_t*)frame->data(), frame->size())) {
                // 猜测是ts负载
                InfoL << _media_info.stream << " judged to be TS";
                _decoder = DecoderImp::createDecoder(DecoderImp::decoder_ts, _interface);
            }
            else {
                // 猜测是ps负载
                InfoL << _media_info.stream << " judged to be PS";
                _decoder = DecoderImp::createDecoder(DecoderImp::decoder_ps, _interface);
            }
        }

        if (_decoder) {
            _decoder->input(reinterpret_cast<const uint8_t*>(frame->data()), frame->size());
        }
    }


}	// namespace x2rtc
