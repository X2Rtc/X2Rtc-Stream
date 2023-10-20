#include "X2ProtoMpegTsHandler.h"
#include "RefCountedObject.h"
#include "Common/Parser.h"
#include "Extension/H264.h"
#include "Extension/H265.h"
#include "libflv/include/mpeg4-aac.h"

#define ADTS_HEADER_LEN 7

namespace x2rtc {
static int getAacFrameLength(const uint8_t* data, size_t bytes) {
	uint16_t len;
	if (bytes < 7) return -1;
	if (0xFF != data[0] || 0xF0 != (data[1] & 0xF0)) {
		return -1;
	}
	len = ((uint16_t)(data[3] & 0x03) << 11) | ((uint16_t)data[4] << 3) | ((uint16_t)(data[5] >> 5) & 0x07);
	return len;
}

class TsTrackDelegateHelper : public std::enable_shared_from_this<TsTrackDelegateHelper>, public mediakit::FrameWriterInterface {
public:
	using onFrame = std::function<void(mediakit::Frame::Ptr frame)>;

	~TsTrackDelegateHelper() override {}

	TsTrackDelegateHelper(onFrame on_rtp) {
		on_frame_ = std::move(on_rtp);
	}

	bool inputFrame(const mediakit::Frame::Ptr& frame) override {
		on_frame_(std::move(frame));
		return true;
	}

private:
	onFrame on_frame_;
};

X2ProtoHandler* createX2ProtoMpegTsHandler()
{
	X2ProtoMpegTsHandler* x2ProtoHandler = new X2ProtoMpegTsHandler();
	return x2ProtoHandler;
}
X2ProtoMpegTsHandler::X2ProtoMpegTsHandler(void)
{

}
X2ProtoMpegTsHandler::~X2ProtoMpegTsHandler(void)
{

}

//* For X2ProtoHandler
bool X2ProtoMpegTsHandler::SetStreamId(const char* strStreamId)
{
	if (!toolkit::start_with(strStreamId, "#!::")) {
		return false;
	}
	std::string streamid = strStreamId;
	{
		media_info_.schema = SRT_SCHEMA;
		
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
		ts_decoder_ = mediakit::DecoderImp::createDecoder(mediakit::DecoderImp::decoder_ts, this);
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPublish(media_info_.app.c_str(), media_info_.stream.c_str(), "", media_info_.param_strs.c_str());
		}
	}
	else {
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPlay(media_info_.app.c_str(), media_info_.stream.c_str(), "", media_info_.param_strs.c_str());

			cb_listener_->OnX2ProtoHandlerSetPlayCodecType(X2Codec_AAC, X2Codec_None);
		}
	}

	return true;
}
void X2ProtoMpegTsHandler::MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn)
{

 }
void X2ProtoMpegTsHandler::RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen)
{
	try {
		if (ts_decoder_ != NULL) {
			ts_decoder_->input(pData, nLen);
		}
	}
	catch (...) {
		ts_decoder_ = NULL;

		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerClose();
		}
	}
}
void X2ProtoMpegTsHandler::Close()
{
	if (ts_decoder_ != NULL) {
		ts_decoder_ = NULL;
	}
}

//* For mediakit::MediaSinkInterface
bool X2ProtoMpegTsHandler::addTrack(const mediakit::Track::Ptr& track) 
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
	track->addDelegate(std::make_shared<TsTrackDelegateHelper>([this](mediakit::Frame::Ptr in) {
		gotFrame(in);
		}));
	return true; 
}
bool X2ProtoMpegTsHandler::inputFrame(const mediakit::Frame::Ptr& frame)
{
	if (frame->getTrackType() == mediakit::TrackType::TrackAudio) {
		if (audio_track_ != NULL) {
			audio_track_->inputFrame(frame);
		}
	}
	else 	if (frame->getTrackType() == mediakit::TrackType::TrackVideo) {
		if (video_track_ != NULL) {
			video_track_->inputFrame(frame);
		}
		//gotFrame(frame);
	}
	return true;
#if 0
	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
		x2CodecData->lDts = frame->dts();
		x2CodecData->lPts = frame->pts();
		if (frame->getTrackType() == mediakit::TrackType::TrackAudio) { 
			if (frame->getCodecId() == mediakit::CodecId::CodecAAC) {
				x2CodecData->eCodecType = X2Codec_AAC;
				if (frame->prefixSize()) {
					struct mpeg4_aac_t aacAdts;
					if (mpeg4_aac_adts_load((uint8_t*)frame->data(), frame->size(), &aacAdts) < 0) {
						return false;
					}
					int audSampleHz = aacAdts.sampling_frequency;
					if (audSampleHz <= 0) {
						return false;
					}
					bool ret = false;
					//ÓÐadtsÍ·£¬³¢ÊÔ·ÖÖ¡
					int64_t dts = frame->dts();
					int64_t pts = frame->pts();

					auto ptr = frame->data();
					auto end = frame->data() + frame->size();
					while (ptr < end) {
						auto frame_len = getAacFrameLength((uint8_t*)ptr, end - ptr);
						if (frame_len < ADTS_HEADER_LEN) {
							break;
						}
						if (frame_len == (int)frame->size()) {
							gotSplitAacFrame(frame);
							return true;
						}
						auto sub_frame = std::make_shared<mediakit::FrameTSInternal<mediakit::FrameFromPtr> >(frame, (char*)ptr, frame_len, ADTS_HEADER_LEN, dts, pts);
						ptr += frame_len;
						if (ptr > end) {
							WarnL << "invalid aac length in adts header: " << frame_len
								<< ", remain data size: " << end - (ptr - frame_len);
							break;
						}
						sub_frame->setCodecId(mediakit::CodecAAC);
						if (gotSplitAacFrame(sub_frame)) {
							ret = true;
						}
						dts += 1024 * 1000 / audSampleHz;
						pts += 1024 * 1000 / audSampleHz;
					}
					return true;
				}
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
			if (frame->getCodecId() == mediakit::CodecId::CodecH264) {
				x2CodecData->eCodecType = X2Codec_H264;
			}
			else if (frame->getCodecId() == mediakit::CodecId::CodecH265) {
				x2CodecData->eCodecType = X2Codec_H265;
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
#endif
}

bool X2ProtoMpegTsHandler::gotFrame(const mediakit::Frame::Ptr& frame)
{
	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->lDts = frame->dts();
		x2CodecData->lPts = frame->pts();
		if (frame->getTrackType() == mediakit::TrackType::TrackAudio) {
			x2CodecData->SetData((uint8_t*)frame->data(), frame->size());
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
					int nLen = 4+ strVps.length() + 4 + strSps.length() + 4 + strPps.length();
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
bool X2ProtoMpegTsHandler::gotSplitAacFrame(const mediakit::Frame::Ptr& frame)
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

}	// namespace x2rtc