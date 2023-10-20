#include "X2ProtoRtpHandler.h"
#include "RefCountedObject.h"
#include "Common/Parser.h"
#include "Extension/H264.h"
#include "Extension/H265.h"
#include "Extension/Factory.h"

namespace x2rtc {
class X2RtpRingDelegateHelper : public toolkit::RingDelegate<RtpPacket::Ptr> {
public:
	using onRtp = std::function<void(RtpPacket::Ptr in, bool is_key)>;

	~X2RtpRingDelegateHelper() override = default;

	X2RtpRingDelegateHelper(onRtp on_rtp) {
		on_rtp_ = std::move(on_rtp);
	}

	void onWrite(RtpPacket::Ptr in, bool is_key) override {
		on_rtp_(std::move(in), is_key);
	}

private:
	onRtp on_rtp_;
};

class RtpTrackDelegateHelper : public std::enable_shared_from_this<RtpTrackDelegateHelper>, public mediakit::FrameWriterInterface {
public:
	using onFrame = std::function<void(mediakit::Frame::Ptr frame)>;

	~RtpTrackDelegateHelper() override {}

	RtpTrackDelegateHelper(onFrame on_rtp) {
		on_frame_ = std::move(on_rtp);
	}

	bool inputFrame(const mediakit::Frame::Ptr& frame) override {
		on_frame_(std::move(frame));
		return true;
	}

private:
	onFrame on_frame_;
};

X2ProtoHandler* createX2ProtoRtpHandler()
{
	X2ProtoRtpHandler* x2ProtoHandler = new X2ProtoRtpHandler();
	return x2ProtoHandler;
}


X2ProtoRtpHandler::X2ProtoRtpHandler(void)
	: b_use_udp_(true)
{

}
X2ProtoRtpHandler::~X2ProtoRtpHandler(void)
{

}

//* For X2ProtoHandler
bool X2ProtoRtpHandler::SetStreamId(const char* strStreamId)
{
	if (!toolkit::start_with(strStreamId, "x2::")) {
		return false;
	}
	std::string streamid = strStreamId;
	{
		media_info_.schema = RTSP_SCHEMA;

		std::string real_streamid = streamid.substr(4);
		std::string vhost, app, stream_name;

		auto params = mediakit::Parser::parseArgs(real_streamid, ",", "=");

		for (auto it : params) {
			if (it.first == "h") {
				vhost = it.second;
			}
			else if (it.first == "r") {
				auto tmps = toolkit::split(it.second, "/");
				if (tmps.size() < 2) {
					continue;
				}
				app = tmps[0];
				stream_name = tmps[1];
			}
			else {
				if (media_info_.param_strs.empty()) {
					media_info_.param_strs = it.first + "=" + it.second;
				}
				else {
					media_info_.param_strs += "&" + it.first + "=" + it.second;
				}
			}
		}
		if (app == "" || stream_name == "") {
			return false;
		}

		if (vhost != "") {
			media_info_.vhost = vhost;
		}
		else {
			media_info_.vhost = DEFAULT_VHOST;
		}

		media_info_.app = app;
		media_info_.stream = stream_name;

		TraceL << " mediainfo=" << media_info_.shortUrl() << " params=" << media_info_.param_strs;
	}

	auto params = mediakit::Parser::parseArgs(media_info_.param_strs);
	if (params["m"] == "publish") {
		x2rtp_gb_process_ = std::make_shared<X2RtpGBProcess>(media_info_, this);
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPublish(media_info_.app.c_str(), media_info_.stream.c_str(), "", media_info_.param_strs.c_str());
		}
	}
	else {
		int nSsrc = atoi(params["ssrc"].c_str());
		if (params["t"].compare("ps") == 0) {
			rtp_ps_encoder_ = std::make_shared<CommonRtpEncoder>(CodecInvalid, nSsrc, 1200, 90000, 96, 0);
			rtp_ps_encoder_->setRtpRing(std::make_shared<RtpRing::RingType>());
			rtp_ps_encoder_->getRtpRing()->setDelegate(std::make_shared<X2RtpRingDelegateHelper>([this](RtpPacket::Ptr rtp, bool is_key) {
				onRTP(std::move(rtp), is_key);
				}));
		}
		else {
			rtp_raw_encoder_ = Factory::getRtpEncoderByCodecId(CodecG711A, 8000, 8, nSsrc);
			rtp_raw_encoder_->setRtpRing(std::make_shared<RtpRing::RingType>());
			rtp_raw_encoder_->getRtpRing()->setDelegate(std::make_shared<X2RtpRingDelegateHelper>(
				[this](RtpPacket::Ptr rtp, bool is_key) { onRTP(std::move(rtp), true); }));
		}
		if (params["net"].compare("tcp") == 0) {
			b_use_udp_ = false;
		}
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPlay(media_info_.app.c_str(), media_info_.stream.c_str(), "", media_info_.param_strs.c_str());

			cb_listener_->OnX2ProtoHandlerSetPlayCodecType(X2Codec_G711A, X2Codec_None);
		}
	}
	return true;
}
void X2ProtoRtpHandler::MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn)
{
	if (codec == X2Codec_G711A) {
		auto frame = std::make_shared<FrameFromPtr>(CodecG711A, (char*)data, bytes, dts, pts, 0);
		if (rtp_raw_encoder_ != NULL) {
			rtp_raw_encoder_->inputFrame(frame);
		}
		else if (rtp_ps_encoder_ != NULL) {
			rtp_ps_encoder_->inputFrame(frame);
		}
	}
}
void X2ProtoRtpHandler::RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen)
{
	if (nLen > 1500) {
		printf("X2ProtoRtpHandler::RecvDataFromNetwork larger data len: %d \r\n", nLen);
	}
	
	if (isRtp((char*)pData, nLen))
	{
		RtpHeader* header = (RtpHeader*)pData;
		RtcpContextForRecv::onRtp(ntohs(header->seq), ntohl(header->stamp), 0/*0: Not send SR*/, 90000/*ps/ts is 90K sampleHz*/, nLen);

		sendRtcp(ntohl(header->ssrc));
	}
	else {
		RtcpContextForRecv::onRtcp((RtcpHeader*)pData);
		return;
	}

	try {
		if (x2rtp_gb_process_ != NULL) {
			x2rtp_gb_process_->inputRtp(true, (char*)pData, nLen);
		}
	}
	catch (...) {
		x2rtp_gb_process_ = NULL;

		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerClose();
		}
	}
}
void X2ProtoRtpHandler::Close()
{
	if (x2rtp_gb_process_ != NULL) {
		x2rtp_gb_process_ = NULL;
	}

	rtp_ps_encoder_ = NULL;
	rtp_raw_encoder_ = NULL;

	audio_track_ = NULL;
	video_track_ = NULL;
}

//* For mediakit::MediaSinkInterface
bool X2ProtoRtpHandler::addTrack(const mediakit::Track::Ptr& track)
{
	if (track->getTrackType() == mediakit::TrackAudio) {
		audio_track_ = track;
		_type_to_stamp.emplace(track->getTrackType(), Stamp());
	}
	else if (track->getTrackType() == mediakit::TrackVideo) {
		video_track_ = track;
		_type_to_stamp.emplace(track->getTrackType(), Stamp());
	}
	else {
		return false;
	}
	track->addDelegate(std::make_shared<RtpTrackDelegateHelper>([this](mediakit::Frame::Ptr in) {
		auto frame_tmp = std::make_shared<FrameStamp>(in, _type_to_stamp[in->getTrackType()], true);
		gotFrame(frame_tmp);
		}));
	return true;
}
bool X2ProtoRtpHandler::inputFrame(const mediakit::Frame::Ptr& frame)
{
	if (frame->getTrackType() == mediakit::TrackType::TrackAudio) {
		if (audio_track_ != NULL) {
			audio_track_->inputFrame(frame);
		}
	}
	else if (frame->getTrackType() == mediakit::TrackType::TrackVideo) {
		if (video_track_ != NULL) {
			video_track_->inputFrame(frame);
		}
	}
	return true;
}

bool X2ProtoRtpHandler::gotFrame(const mediakit::Frame::Ptr& frame)
{
	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->lDts = frame->dts();
		x2CodecData->lPts = frame->pts();
		if (frame->getTrackType() == mediakit::TrackType::TrackAudio) {
			x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
			//printf("gotFrame codecId: %d len: %d\r\n", frame->getCodecId(), frame->size());
			if (frame->getCodecId() == mediakit::CodecId::CodecAAC) {
				x2CodecData->eCodecType = X2Codec_AAC;
			}
			else if (frame->getCodecId() == mediakit::CodecId::CodecG711U) {
				x2CodecData->eCodecType = X2Codec_G711U;
			}
			else if (frame->getCodecId() == mediakit::CodecId::CodecG711A) {
				x2CodecData->eCodecType = X2Codec_G711A;
			}
			else if (frame->getCodecId() == mediakit::CodecId::CodecOpus) {
				x2CodecData->eCodecType = X2Codec_OPUS;
			}
			else {
				return false;
			}
			x2CodecData->bIsAudio = true;
		}
		else if (frame->getTrackType() == mediakit::TrackType::TrackVideo) {
			if (frame->dropAble() || frame->configFrame()) {
				return false;
			}

			if (frame->getCodecId() == mediakit::CodecId::CodecH264) {
				x2CodecData->eCodecType = X2Codec_H264;
				if (frame->keyFrame()) {
					char pHdr[4] = { 0x0, 0x0, 0x0, 0x1 };
					std::string strSps = static_cast<mediakit::H264Track*>(video_track_.get())->getSps();
					std::string strPps = static_cast<mediakit::H264Track*>(video_track_.get())->getPps();
					int nLen = 4 + strSps.length() + 4 + strPps.length();
					nLen += +frame->size();
					char* pData = new char[nLen];
					int nUsed = 0;
					memcpy(pData, pHdr, 4);
					nUsed += 4;
					memcpy(pData + nUsed, strSps.c_str(), strSps.length());
					nUsed += strSps.length();
					memcpy(pData + nUsed, pHdr, 4);
					nUsed += 4;
					memcpy(pData + nUsed, strPps.c_str(), strPps.length());
					nUsed += strPps.length();
					memcpy(pData + nUsed, frame->data(), frame->size());
					x2CodecData->pData = (uint8_t*)pData;
					x2CodecData->nLen = nLen;
				}
				else {
					x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
				}
			}
			else if (frame->getCodecId() == mediakit::CodecId::CodecH265) {
				x2CodecData->eCodecType = X2Codec_H265;
				if (frame->keyFrame()) {
					char pHdr[4] = { 0x0, 0x0, 0x0, 0x1 };
					std::string strVps = static_cast<mediakit::H265Track*>(video_track_.get())->getVps();
					std::string strSps = static_cast<mediakit::H265Track*>(video_track_.get())->getSps();
					std::string strPps = static_cast<mediakit::H265Track*>(video_track_.get())->getPps();
					int nLen = 4 + strVps.length() + 4 + strSps.length() + 4 + strPps.length();
					nLen += +frame->size();
					char* pData = new char[nLen];
					int nUsed = 0;
					memcpy(pData, pHdr, 4);
					nUsed += 4;
					memcpy(pData + nUsed, strVps.c_str(), strVps.length());
					nUsed += strVps.length();
					memcpy(pData + nUsed, pHdr, 4);
					nUsed += 4;
					memcpy(pData + nUsed, strSps.c_str(), strSps.length());
					nUsed += strSps.length();
					memcpy(pData + nUsed, pHdr, 4);
					nUsed += 4;
					memcpy(pData + nUsed, strPps.c_str(), strPps.length());
					nUsed += strPps.length();
					memcpy(pData + nUsed, frame->data(), frame->size());
					x2CodecData->pData = (uint8_t*)pData;
					x2CodecData->nLen = nLen;
				}
				else {
					x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
				}
			}
			else {
				return false;
			}
			x2CodecData->bIsAudio = false;
			x2CodecData->bKeyframe = frame->keyFrame();
		}

		cb_listener_->OnX2ProtoHandlerRecvCodecData(x2CodecData);
	}
	return true;
}
bool X2ProtoRtpHandler::gotSplitAacFrame(const mediakit::Frame::Ptr& frame)
{
	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
		x2CodecData->lDts = frame->dts();
		x2CodecData->lPts = frame->pts();
		x2CodecData->bIsAudio = true;
		x2CodecData->eCodecType = X2Codec_AAC;

		cb_listener_->OnX2ProtoHandlerRecvCodecData(x2CodecData);
	}

	return true;
}

// rtp打包后回调
void X2ProtoRtpHandler::onRTP(toolkit::Buffer::Ptr rtp, bool is_key)
{
	/*GB28181 Tcp protocol is RFC4571（RTP OVER TCP）
	0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    ---------------------------------------------------------------
   |             LENGTH            |  RTP or RTCP packet ...       |
    ---------------------------------------------------------------
	*/

	/* Rtsp Tcp protocol is RFC2326
	| magic number | channel number | data length | data  
	magic number：RTP tag, "$"
	*/
	if (cb_listener_ != NULL) {
		if (b_use_udp_) {
			cb_listener_->OnX2ProtoHandlerSendData(NULL, 0, (uint8_t*)rtp->data() + RtpPacket::kRtpTcpHeaderSize, rtp->size() - RtpPacket::kRtpTcpHeaderSize);
		}
		else {
			cb_listener_->OnX2ProtoHandlerSendData((uint8_t*)rtp->data() + 2, RtpPacket::kRtpTcpHeaderSize - 2, (uint8_t*)rtp->data() + RtpPacket::kRtpTcpHeaderSize, rtp->size() - RtpPacket::kRtpTcpHeaderSize);
			//cb_listener_->OnX2ProtoHandlerSendData((uint8_t*)rtp->data(), RtpPacket::kRtpTcpHeaderSize, (uint8_t*)rtp->data() + RtpPacket::kRtpTcpHeaderSize, rtp->size() - RtpPacket::kRtpTcpHeaderSize);
		}
	}
}

void X2ProtoRtpHandler::sendRtcp(uint32_t rtp_ssrc)
{
	// 每5秒发送一次rtcp
	if (_ticker.elapsedTime() < 5000 || !x2rtp_gb_process_) {
		return;
	}
	_ticker.resetTime();

	toolkit::Buffer::Ptr rtp = RtcpContextForRecv::createRtcpRR(rtp_ssrc + 1, rtp_ssrc);
	if (rtp != nullptr) {
		if (b_use_udp_) {
			cb_listener_->OnX2ProtoHandlerSendData(NULL, 0, (uint8_t*)rtp->data(), rtp->size());
		}
		else {
			//Set 4 bytes header of rtp over tcp
			uint8_t data[4];
			data[0] = '$';
			data[1] = 0;
			data[2] = (rtp->size() >> 8) & 0xFF;
			data[3] = rtp->size() & 0xFF;
			cb_listener_->OnX2ProtoHandlerSendData(data + 2, 2, (uint8_t*)rtp->data(), rtp->size());
		}
		
	}
}

}	// namespace x2rtc