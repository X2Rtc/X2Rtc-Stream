#include "X2ProtoRtmpHandler.h"
#include "RefCountedObject.h"
#include "absl/memory/memory.h"

namespace x2rtc {

class RtmpTrackDelegateHelper : public std::enable_shared_from_this<RtmpTrackDelegateHelper>, public mediakit::FrameWriterInterface {
public:
	using onFrame = std::function<void(mediakit::Frame::Ptr frame)>;

	~RtmpTrackDelegateHelper() override {}

	RtmpTrackDelegateHelper(onFrame on_rtp) {
		on_frame_ = std::move(on_rtp);
	}

	bool inputFrame(const mediakit::Frame::Ptr& frame) override {
		on_frame_(std::move(frame));
		return true;
	}

private:
	onFrame on_frame_;
};

X2ProtoHandler* createX2ProtoRtmpHandler()
{
	return new X2ProtoRtmpHandler();
}
X2ProtoRtmpHandler::X2ProtoRtmpHandler(void)
{
	x2rtmp_session_ = absl::make_unique<X2RtmpSession>();
	x2rtmp_session_->SetListener(this);
}
X2ProtoRtmpHandler::~X2ProtoRtmpHandler(void)
{
	x2rtmp_session_ = NULL;
}

//* For X2ProtoHandler
void X2ProtoRtmpHandler::MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn)
{
	if (x2rtmp_session_ != NULL) {
		if (codec == X2Codec_AAC) {
			x2rtmp_session_->SendAacAudio(data, bytes, dts, pts);
		}
		else if (codec == X2Codec_H264) {
			x2rtmp_session_->SendH264Video(data, bytes, dts, pts);
		}
		else if (codec == X2Codec_H265) {
			x2rtmp_session_->SendH265Video(data, bytes, dts, pts);
		}
	}
}
void X2ProtoRtmpHandler::RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen)
{
	try {
		// 可能会抛出std::runtime_error异常的代码
		if (x2rtmp_session_ != NULL) {
			x2rtmp_session_->RecvData((char*)pData, nLen);
		}
	}
	catch (...) {
		// 处理std::runtime_error异常的代码
		//if (rtmpXPeer->HasError()) 
		if (x2rtmp_session_ != NULL) {
			x2rtmp_session_->Close();
			x2rtmp_session_ = NULL;
		}
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerClose();
		}
	}
	
}
void X2ProtoRtmpHandler::Close()
{
	if (x2rtmp_session_ != NULL) {
		x2rtmp_session_->Close();
		x2rtmp_session_ = NULL;
	}

	if (rtmp_demuxer_ != NULL) {
		rtmp_demuxer_ = NULL;
	}
	audio_track_ = NULL;
	video_track_ = NULL;
}

//* For X2RtmpSession::Listener
void X2ProtoRtmpHandler::OnPublish(const char* app, const char* stream, const char* type)
{
	if (rtmp_demuxer_ == NULL) {
		rtmp_demuxer_ = std::make_shared<RtmpDemuxer>();
		rtmp_demuxer_->setTrackListener(this);
	}

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerPublish(app, stream, "", type);
	}
}
void X2ProtoRtmpHandler::OnPlay(const char* app, const char* stream, double start, double duration, uint8_t reset)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerPlay(app, stream, "", "");

		cb_listener_->OnX2ProtoHandlerSetPlayCodecType(X2Codec_AAC, X2Codec_None);
	}
}
void X2ProtoRtmpHandler::OnClose()
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerClose();
	}

	if (rtmp_demuxer_ != NULL) {
		rtmp_demuxer_ = NULL;
	}

	audio_track_ = NULL;
	video_track_ = NULL;
}
void X2ProtoRtmpHandler::OnSendData(const char* pData, int nLen)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerSendData(NULL, 0, (uint8_t*)pData, nLen);
	}
}
void X2ProtoRtmpHandler::OnSendData(const char* pHdr, int nHdrLen, const char* pData, int nLen)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerSendData((uint8_t*)pHdr, nHdrLen, (uint8_t*)pData, nLen);
	}

}
void X2ProtoRtmpHandler::OnRecvRtmpData(RtmpPacket::Ptr packet)
{
	if (rtmp_demuxer_ != NULL) {
		rtmp_demuxer_->inputRtmp(packet);
	}
}
void X2ProtoRtmpHandler::OnRecvScript(const void* data, size_t bytes, uint32_t timestamp)
{

}
void X2ProtoRtmpHandler::OnRecvAudio(const void* data, size_t bytes, uint32_t timestamp)
{

}
void X2ProtoRtmpHandler::OnRecvVideo(bool isKeyframe, const void* data, size_t bytes, uint32_t timestamp)
{

}

//* For TrackListener
bool X2ProtoRtmpHandler::addTrack(const Track::Ptr& track)
{
	if (track->getTrackType() == mediakit::TrackAudio) {
		audio_track_ = track;
	}
	else if (track->getTrackType() == mediakit::TrackVideo) {
		video_track_ = track;
	}
	else {
		return false;
	}

	track->addDelegate(std::make_shared<RtmpTrackDelegateHelper>([this](mediakit::Frame::Ptr in) {
		gotFrame(in);
		}));
	return true;
}

bool X2ProtoRtmpHandler::gotFrame(const mediakit::Frame::Ptr& frame)
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
			x2CodecData->nAudSeqn = n_aud_seqn_++;
		}
		else if (frame->getTrackType() == mediakit::TrackType::TrackVideo) {
			//printf("Recv video dts: %lld pts: %lld len: %d\r\n", x2CodecData->lDts, x2CodecData->lPts, frame->size());
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

}	// namespace x2rtc