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

    // �ж��Ƿ�Ϊts����
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
            // GB28181������֧��ntpʱ���
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
                // ��ֹpt����̫�ർ���ڴ����
                throw std::invalid_argument("rtp pt���Ͳ��ó���2��!");
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
                    // opus����
                    ref = std::make_shared<RtpReceiverImp>(48000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<OpusTrack>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else if (pt == h265_pt) {
                    // H265����
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<H265Track>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else if (pt == h264_pt) {
                    // H264����
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    auto track = std::make_shared<H264Track>();
                    _interface->addTrack(track);
                    _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                }
                else {
                    if (pt != Rtsp::PT_MP2T && pt != ps_pt) {
                        WarnL << "rtp payload typeδʶ��(" << (int)pt << "),�Ѱ�ts��ps���ش���";
                    }
                    ref = std::make_shared<RtpReceiverImp>(90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });
                    // ts��ps����
                    _rtp_decoder[pt] = std::make_shared<CommonRtpDecoder>(CodecInvalid, 32 * 1024);
                    // ����dumpĿ¼
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
            // ����frame�ص�
            _rtp_decoder[pt]->addDelegate([this](const Frame::Ptr& frame) {
                onRtpDecode(frame);
                return true;
                });
        }

        return ref->inputRtp(TrackVideo, (unsigned char*)data, data_len);
    }

    void X2RtpGBProcess::onRtpDecode(const Frame::Ptr& frame) {
        if (frame->getCodecId() != CodecInvalid) {
            // ���ﲻ��ps��ts
            _interface->inputFrame(frame);
            return;
        }

        // ����TS��PS
        if (_save_file_ps) {
            fwrite(frame->data(), frame->size(), 1, _save_file_ps.get());
        }

        if (!_decoder) {
            // ����������
            if (checkTS((uint8_t*)frame->data(), frame->size())) {
                // �²���ts����
                InfoL << _media_info.stream << " judged to be TS";
                _decoder = DecoderImp::createDecoder(DecoderImp::decoder_ts, _interface);
            }
            else {
                // �²���ps����
                InfoL << _media_info.stream << " judged to be PS";
                _decoder = DecoderImp::createDecoder(DecoderImp::decoder_ps, _interface);
            }
        }

        if (_decoder) {
            _decoder->input(reinterpret_cast<const uint8_t*>(frame->data()), frame->size());
        }
    }


}	// namespace x2rtc
