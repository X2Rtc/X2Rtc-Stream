#include "X2ProtoRtcHandler.h"
#include "RefCountedObject.h"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "common_video/h264/h264_common.h"
#include "common_video/h265/h265_common.h"
#include "media/base/media_constants.h"
#include "modules/rtp_rtcp/source/rtp_packet_to_send.h"
#include "XUtil.h"
#include "X2Clock.h"

namespace x2rtc {
extern bool IsJson(const uint8_t* pData, int nLen);
#define MaxValue_Int32 4294967295
static bool IsTime32HigherThan_X(uint32_t lhs, uint32_t rhs)		// lhs > rhs
{
	return ((lhs > rhs) && (lhs - rhs <= MaxValue_Int32 / 2)) ||
		((rhs > lhs) && (rhs - lhs > MaxValue_Int32 / 2));
}
static bool IsTime32LowerOrEqualThan_X(uint32_t lhs, uint32_t rhs)
{
	return !IsTime32HigherThan_X(lhs, rhs);
}

X2ProtoHandler* createX2ProtoRtcHandler()
{
	X2ProtoRtcHandler* x2ProtoHandler = new X2ProtoRtcHandler();
	return x2ProtoHandler;
}


X2ProtoRtcHandler::X2ProtoRtcHandler(void)
{

}
X2ProtoRtcHandler::~X2ProtoRtcHandler(void)
{

}

//* For X2ProtoHandler
bool X2ProtoRtcHandler::SetStreamId(const char* strStreamId)
{
	std::map<std::string, std::string> mapParam;
	XParseHttpParam(strStreamId, &mapParam);
	char strParam[2048];
	int nPos = 0;
	std::map<std::string, std::string>::iterator itmr = mapParam.begin();
	while (itmr != mapParam.end()) {
		if (itmr->first.compare("app") != 0 && itmr->first.compare("stream") != 0 && itmr->first.compare("token") != 0) {
			if (nPos == 0) {
				nPos += sprintf(strParam + nPos, "%s=%s", itmr->first.c_str(), itmr->second.c_str());
			}
			else {
				nPos += sprintf(strParam + nPos, "&%s=%s", itmr->first.c_str(), itmr->second.c_str());
			}
		}
		itmr++;
	}
	if (mapParam["type"] == "publish") {
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPublish(mapParam["app"].c_str(), mapParam["stream"].c_str(), mapParam["token"].c_str(), strParam);
		}
	}
	else {
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerPlay(mapParam["app"].c_str(), mapParam["stream"].c_str(), mapParam["token"].c_str(), strParam);
			e_aud_codec_type_ = X2Codec_OPUS;
			if (mapParam.find("useAAC") != mapParam.end()) {
				e_aud_codec_type_ = X2Codec_AAC;
			}
			cb_listener_->OnX2ProtoHandlerSetPlayCodecType(e_aud_codec_type_, X2Codec_None);
		}
	}
	return true;
}
void X2ProtoRtcHandler::MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn)
{
	if (codec == X2Codec_OPUS || codec == X2Codec_AAC) {
		/*
			音频时间戳的单位就是采样率的倒数，例如采样率48000，那么1秒就有48000个采样，每个采样1/48ms，每个采样对应一个时间戳。RTP音频包一般打包20ms的数据，对应的采样数为 48000 * 20 / 1000 = 960，也就是说每个音频包里携带960个音频采样，因为1个采样对应1个时间戳，那么相邻两个音频RTP包的时间戳之差就是960
		*/
		uint32_t rtpTimeStamp = X2AvRescale(dts, (sample_rate_ / 1000), 1);
		//printf("SendAudioData seqn: %d time: %lld rtpTimeStamp: %u curTime: %u\r\n", audSeqn, timestamp, rtpTimeStamp, XGetTimestamp());
		std::unique_ptr<webrtc::RtpPacketToSend> packet = absl::make_unique<webrtc::RtpPacketToSend>(
			nullptr, 1500);
		uint8_t* buffer = packet->AllocatePayload((int)bytes);
		memcpy(buffer, data, bytes);
		packet->SetSequenceNumber(audSeqn);
		packet->SetSsrc(0);
		packet->SetTimestamp(rtpTimeStamp);
		packet->SetPayloadType(audio_payload_type_);
		
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerSendData(NULL, NULL, packet->data(), packet->size());
		}
	}
	else if (codec == X2Codec_H264) {
		
		webrtc::RtpPacketizer::PayloadSizeLimits limits;
		// Generate a header describing a single fragment.
		webrtc::RTPFragmentationHeader header;
		//printf("WRtcXSubConnection::SendVideoData isKey: %d len: %d\r\n", bKeyFrame, nLen);
#if 0	//@Eric - If client not support B-Frame, how to process?
		if (X2IsH264BFrame((char*)data, bytes)) {
			//printf("Find B-Frame len: %d\r\n", bytes);
			//return;
		}
#endif
		bool bKeyFrame = (data[4] & 0x1f) == 7;
		size_t payload_size = bytes;
		uint8_t* payload = (uint8_t*)data;
		{
			// For H.264 search for start codes.
			const std::vector<webrtc::H264::NaluIndex> nalu_idxs =
				webrtc::H264::FindNaluIndices(payload, payload_size);
			if (nalu_idxs.empty()) {
				//RTC_LOG(LS_ERROR) << "Not foud nalus!";
				return;
			}
			header.VerifyAndAllocateFragmentationHeader(nalu_idxs.size());
			for (size_t i = 0; i < nalu_idxs.size(); i++) {
				header.fragmentationOffset[i] = nalu_idxs[i].payload_start_offset;
				header.fragmentationLength[i] = nalu_idxs[i].payload_size;
			}
		}
		webrtc::RTPVideoHeader rtp_video_header;
		rtp_video_header.rotation = (webrtc::VideoRotation)0;
		auto& h264_header = rtp_video_header.video_type_header.emplace<webrtc::RTPVideoHeaderH264>();
		h264_header.packetization_mode = webrtc::H264PacketizationMode::NonInterleaved;
		std::unique_ptr<webrtc::RtpPacketizer> packetizer = webrtc::RtpPacketizer::Create(
			webrtc::kVideoCodecH264, rtc::ArrayView<const uint8_t>((uint8_t*)data, bytes), limits, rtp_video_header, &header);

		uint32_t rtpTimeStamp = X2AvRescale(dts, (cricket::kVideoCodecClockrate / 1000), 1);
		//uint64_t millis = x2rtc::ClockUtils::timePointToMs(x2rtc::clock::now());
		//uint32_t rtpTimeStamp = X2AvRescale(millis, cricket::kVideoCodecClockrate, 1000);
		//printf("SendVideoData key: %d time: %lld rtpTimeStamp: %u curTime: %u\r\n", bKeyFrame, timestamp, rtpTimeStamp, XGetTimestamp());
		const size_t num_packets = packetizer->NumPackets();
		for (size_t i = 0; i < num_packets; ++i) {
			std::unique_ptr<webrtc::RtpPacketToSend> packet = absl::make_unique<webrtc::RtpPacketToSend>(
				nullptr, 1500);
			packet->IdentifyExtensions(video_extensions_);
			if (pts > 0) {//@Eric - Must set Extensions before set payload
				webrtc::AbsoluteCaptureTime absTime;
				absTime.absolute_capture_timestamp = X2AvRescale(pts, (cricket::kVideoCodecClockrate / 1000), 1);
				absTime.estimated_capture_clock_offset = absl::nullopt;
				packet->set_abs_capture_time_ms(absTime);
			}

			if (!packetizer->NextPacket(packet.get()))
				return;
			packet->SetSequenceNumber(video_seqnum_++);
			packet->SetSsrc(0);
			packet->SetTimestamp(rtpTimeStamp);
			packet->SetPayloadType(video_payload_type_);
			// Put packetization finish timestamp into extension.
			if (packet->HasExtension<webrtc::VideoTimingExtension>()) {
				packet->set_packetization_finish_time_ms(rtc::TimeMillis());	// clock_->TimeInMilliseconds()
			}
			packet->set_packet_type(webrtc::RtpPacketToSend::Type::kVideo);
			if ((i + 1 == num_packets)/*LastPacket*/) {
				packet->SetMarker(true);
			}

			if (cb_listener_ != NULL) {
				cb_listener_->OnX2ProtoHandlerSendData(NULL, NULL, packet->data(), packet->size());
			}
		}
	}
	else if (codec == X2Codec_H265) {
		webrtc::RtpPacketizer::PayloadSizeLimits limits;
		// Generate a header describing a single fragment.
		webrtc::RTPFragmentationHeader header;
		//printf("WRtcXSubConnection::SendVideoData isKey: %d len: %d\r\n", bKeyFrame, nLen);
		size_t payload_size = bytes;
		uint8_t* payload = (uint8_t*)data;
		{
			// For H.265 search for start codes.
			const std::vector<webrtc::H265::NaluIndex> nalu_idxs =
				webrtc::H265::FindNaluIndices(payload, payload_size);
			if (nalu_idxs.empty()) {
				//RTC_LOG(LS_ERROR) << "Not foud nalus!";
				return;
			}
			header.VerifyAndAllocateFragmentationHeader(nalu_idxs.size());
			for (size_t i = 0; i < nalu_idxs.size(); i++) {
				header.fragmentationOffset[i] = nalu_idxs[i].payload_start_offset;
				header.fragmentationLength[i] = nalu_idxs[i].payload_size;
			}
		}
		webrtc::RTPVideoHeader rtp_video_header;
		rtp_video_header.rotation = (webrtc::VideoRotation)0;
		auto& h265_header = rtp_video_header.video_type_header.emplace<webrtc::RTPVideoHeaderH265>();
		h265_header.packetization_mode = webrtc::H265PacketizationMode::NonInterleaved;
		std::unique_ptr<webrtc::RtpPacketizer> packetizer = webrtc::RtpPacketizer::Create(
			webrtc::kVideoCodecH265, rtc::ArrayView<const uint8_t>((uint8_t*)data, bytes), limits, rtp_video_header, &header);

		uint32_t rtpTimeStamp = X2AvRescale(dts, (cricket::kVideoCodecClockrate / 1000), 1);
		//uint64_t millis = x2rtc::ClockUtils::timePointToMs(x2rtc::clock::now());
		//uint32_t rtpTimeStamp = X2AvRescale(millis, cricket::kVideoCodecClockrate, 1000);
		//printf("SendVideoData key: %d time: %lld rtpTimeStamp: %u curTime: %u\r\n", bKeyFrame, timestamp, rtpTimeStamp, XGetTimestamp());
		const size_t num_packets = packetizer->NumPackets();
		//float fTimeGap = (float)(bKeyFrame ? 33 : 16) / num_packets;
		//int64_t nCurUtcTime = rtc::TimeUTCMillis();
		for (size_t i = 0; i < num_packets; ++i) {
			std::unique_ptr<webrtc::RtpPacketToSend> packet = absl::make_unique<webrtc::RtpPacketToSend>(
				nullptr, 1500);
			packet->IdentifyExtensions(video_extensions_);
			if (pts > 0) {//@Eric - Must set Extensions before set payload
				webrtc::AbsoluteCaptureTime absTime;
				absTime.absolute_capture_timestamp = X2AvRescale(pts, (cricket::kVideoCodecClockrate / 1000), 1);
				absTime.estimated_capture_clock_offset = absl::nullopt;
				packet->set_abs_capture_time_ms(absTime);
			}

			if (!packetizer->NextPacket(packet.get()))
				return;
			packet->SetSequenceNumber(video_seqnum_++);
			packet->SetSsrc(0);
			packet->SetTimestamp(rtpTimeStamp);
			packet->SetPayloadType(video_payload_type_);
			// Put packetization finish timestamp into extension.
			if (packet->HasExtension<webrtc::VideoTimingExtension>()) {
				packet->set_packetization_finish_time_ms(rtc::TimeMillis());	// clock_->TimeInMilliseconds()
			}
			packet->set_packet_type(webrtc::RtpPacketToSend::Type::kVideo);
			if ((i + 1 == num_packets)/*LastPacket*/) {
				packet->SetMarker(true);
			}

			if (cb_listener_ != NULL) {
				cb_listener_->OnX2ProtoHandlerSendData(NULL, NULL, packet->data(), packet->size());
			}
		}
	}
}
void X2ProtoRtcHandler::RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen)
{
	if (IsJson(pData, nLen)) {
		ProcessEvent(pData, nLen);
	}
	else {
		ProcessData(nDataType, pData, nLen);
	}
}
void X2ProtoRtcHandler::Close()
{

}

//* For OnCompleteFrameCallback
void X2ProtoRtcHandler::OnCompleteFrame(std::unique_ptr<webrtc::video_coding::EncodedFrame> frame)
{
	uint32_t timeStamp = frame->Timestamp() / (cricket::kVideoCodecClockrate / 1000);
	//printf("Recv vid rtpTime: %u ntpTime: %lld\r\n", timeStamp, frame->EncodedImage().ntp_time_ms_);
	if (frame->is_keyframe()) {
		printf("OnCompleteFrame rid: %s time: %u ntp: %lld\r\n", "h", timeStamp, frame->EncodedImage().ntp_time_ms_);
	}
	MediaProducerRecvVideoFrame("h", (char*)frame->data(), frame->size(), frame->is_keyframe(), frame->rotation(), timeStamp, frame->EncodedImage().ntp_time_ms_);
}
void X2ProtoRtcHandler::OnCompleteFrame(const std::string& strRId, std::unique_ptr<webrtc::video_coding::EncodedFrame> frame)
{
	uint32_t timeStamp = frame->Timestamp() / (cricket::kVideoCodecClockrate / 1000);
	if (frame->is_keyframe()) {
		printf("OnCompleteFrame rid: %s time: %u ntp: %lld\r\n", strRId.c_str(), timeStamp, frame->EncodedImage().ntp_time_ms_);
	}
	MediaProducerRecvVideoFrame(strRId, (char*)frame->data(), frame->size(), frame->is_keyframe(), frame->rotation(), timeStamp, frame->EncodedImage().ntp_time_ms_);
}
void X2ProtoRtcHandler::OnCompleteFrame(const uint8_t* pData, int nLen, uint16_t nSeqn, uint32_t nTimestamp, int64_t ntpTimestamp)
{
	uint32_t timeStamp = nTimestamp / (sample_rate_ / 1000);
	//printf("Recv aud rtpTime: %u ntpTime: %lld\r\n", timeStamp, ntpTimestamp);
	MediaProducerRecvAudioFrame((char*)pData, nLen, nSeqn, timeStamp, ntpTimestamp);
}

void X2ProtoRtcHandler::MediaProducerRecvVideoFrame(const std::string& strRId, const char* pData, int nLen, bool bKeyframe, int rotate, uint32_t nRtpTimestamp, int64_t nNtpTimeMs)
{
	int64_t nSyncTime = 0;
	if (nNtpTimeMs > 0) {
		if (sync_av_time_.nFirstVidNtpTime < 0 || sync_av_time_.nFirstAudNtpTime < 0) {
			sync_av_time_.nFirstVidNtpTime = nNtpTimeMs;
			if (sync_av_time_.nFirstAudNtpTime >= 0) {
				sync_av_time_.nTimestampBase = sync_av_time_.nVidElapseTime > sync_av_time_.nAudElapseTime ? sync_av_time_.nVidElapseTime : sync_av_time_.nAudElapseTime;
			}
		}
	}
	if (sync_av_time_.nFirstAudNtpTime >= 0 && sync_av_time_.nFirstVidNtpTime >= 0) {
		nSyncTime = sync_av_time_.nTimestampBase + (nNtpTimeMs - sync_av_time_.nFirstVidNtpTime);
		//printf("Vid - ntp syncTime: %u\r\n", nSyncTime);
	}
	else {
		if (sync_av_time_.nFirstRecvAudioRtpTime < 0)
			sync_av_time_.nFirstRecvAudioRtpTime = nRtpTimestamp;
		int64_t rtp_time_elapsed_since_first_frame =
			(sync_av_time_.audioTimeHandler.Unwrap(nRtpTimestamp) -
				sync_av_time_.nFirstRecvAudioRtpTime);
		//int64_t elapsed_time_ms = rtp_time_elapsed_since_first_frame / (cricket::kVideoCodecClockrate / 1000);
		sync_av_time_.nVidElapseTime = rtp_time_elapsed_since_first_frame;
		nSyncTime = rtp_time_elapsed_since_first_frame;

		//printf("Vid - rtp syncTime: %u\r\n", nSyncTime);
	}

	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->SetData((uint8_t*)pData, nLen);
		x2CodecData->eCodecType = X2Codec_H264;
		x2CodecData->lDts = nSyncTime;
		x2CodecData->lPts = nSyncTime;
		x2CodecData->bIsAudio = false;
		x2CodecData->bKeyframe = bKeyframe;
		x2CodecData->nVidRotate = rotate;
		if (strRId.compare("h") == 0) {
			x2CodecData->nVidScaleRD = 1;
		}
		else if (strRId.compare("m") == 0) {
			x2CodecData->nVidScaleRD = 2;
		}
		else if (strRId.compare("s") == 0) {
			x2CodecData->nVidScaleRD = 3;
		}
		else if (strRId.compare("l") == 0) {
			x2CodecData->nVidScaleRD = 4;
		}

		cb_listener_->OnX2ProtoHandlerRecvCodecData(x2CodecData);
	}
}
void X2ProtoRtcHandler::MediaProducerRecvAudioFrame(const char* pData, int nLen, uint16_t nSeqn, uint32_t nRtpTimestamp, int64_t nNtpTimeMs)
{
	int64_t nSyncTime = 0;
	if (nNtpTimeMs > 0) {
		if (sync_av_time_.nFirstAudNtpTime < 0 || sync_av_time_.nFirstVidNtpTime < 0) {
			sync_av_time_.nFirstAudNtpTime = nNtpTimeMs;
			if (sync_av_time_.nFirstVidNtpTime >= 0) {
				sync_av_time_.nTimestampBase = sync_av_time_.nVidElapseTime > sync_av_time_.nAudElapseTime ? sync_av_time_.nVidElapseTime : sync_av_time_.nAudElapseTime;
			}
		}
	}
	if (sync_av_time_.nFirstAudNtpTime >= 0 && sync_av_time_.nFirstVidNtpTime >= 0) {
		nSyncTime = sync_av_time_.nTimestampBase + (nNtpTimeMs - sync_av_time_.nFirstAudNtpTime);
		//printf("Aud - ntp syncTime: %u\r\n", nSyncTime);
	}
	else {
		if (sync_av_time_.nFirstRecvVideoRtpTime < 0)
			sync_av_time_.nFirstRecvVideoRtpTime = nRtpTimestamp;
		int64_t rtp_time_elapsed_since_first_frame =
			(sync_av_time_.videoTimeHandler.Unwrap(nRtpTimestamp) -
				sync_av_time_.nFirstRecvVideoRtpTime);
		//int64_t elapsed_time_ms = rtp_time_elapsed_since_first_frame / (cricket::kVideoCodecClockrate / 1000);
		sync_av_time_.nAudElapseTime = rtp_time_elapsed_since_first_frame;
		nSyncTime = rtp_time_elapsed_since_first_frame;
		//printf("Aud - rtp syncTime: %u\r\n", nSyncTime);
	}

	//printf("RtwbConnectionImpl::MediaProducerRecvAudioFrame len: %d curTime: %u syncTime: %u\r\n", nLen, XGetTime32(), nSyncTime);
	if (cb_listener_ != NULL) {
		scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
		x2CodecData->SetData((uint8_t*)pData, nLen);
		x2CodecData->eCodecType = X2Codec_OPUS;
		x2CodecData->lDts = nSyncTime;
		x2CodecData->lPts = nSyncTime;
		x2CodecData->bIsAudio = true;
		x2CodecData->nAudSeqn = nSeqn;
		//printf("MediaProducerRecvAudioFrame len: %d seqn: %d pts: %u rtpTime: %u curTime: %u\r\n", nLen, nSeqn, nSyncTime, nRtpTimestamp, XGetTimestamp());

		cb_listener_->OnX2ProtoHandlerRecvCodecData(x2CodecData);
	}
}

void X2ProtoRtcHandler::ProcessEvent(const uint8_t* pData, size_t nLen)
{
	//printf("X2ProtoRtcHandler::ProcessEvent : %s \r\n", pData);
	json jsEvent = json::parse(pData);
	if (jsEvent["Event"] == "ProducerRtcpSenderReport") {
		if (jsEvent["Type"] == "audio") {
			if (rtp_aud_stream_recv_ != NULL) {
				//printf("*Aud ntp time rtt:%d sec: %u frac:%u rtp: %u\r\n", rtt, ntp.seconds, ntp.fractions, rtpStream->GetSenderReportTs());
				rtp_aud_stream_recv_->UpdateRtcpTimestamp(jsEvent["Rtt"], jsEvent["NtpSeconds"], jsEvent["NtpFractions"], jsEvent["SenderReportTs"]);
			}
		}
		else if (jsEvent["Type"] == "video") {
			if (rtp_vid_stream_recv_ != NULL) {
				//printf("*Vid ntp time rtt:%d sec: %u frac:%u rtp: %u\r\n", rtt, ntp.seconds, ntp.fractions, rtpStream->GetSenderReportTs());
				rtp_vid_stream_recv_->UpdateRtcpTimestamp(jsEvent["Rtt"], jsEvent["NtpSeconds"], jsEvent["NtpFractions"], jsEvent["SenderReportTs"]);
			}
		}
	}
	else if (jsEvent["Event"] == "StatueChanged") {
		bool bConnected = jsEvent["Connected"];
		cb_listener_->OnX2ProtoHandlerPlayStateChanged(bConnected);
		if (bConnected) {
			cb_listener_->OnX2ProtoHandlerSetPlayCodecType(e_aud_codec_type_, X2Codec_None);
		}
	}
	else if (jsEvent["Event"] == "NewProducer") {
		json jsRtpParam = jsEvent["RtpParam"];
		RTC::RtpParameters rtpParam(jsRtpParam);
		const auto& codecs = rtpParam.codecs;
		auto& encoding = rtpParam.encodings[0];
		auto* mediaCodec = rtpParam.GetCodecForEncoding(encoding);
		webrtc::RtpHeaderExtensionMap extensions;
		for (auto& exten : rtpParam.headerExtensions)
		{
			if (exten.id != 0u) {
				extensions.RegisterByUri(exten.id, exten.uri);
			}
		}

		if (jsEvent["Type"] == "audio") {
			audio_payload_type_ = jsEvent["PayloadType"];
			sample_rate_ = jsEvent["SampleRate"];

			rtp_aud_stream_recv_.reset(new X2RtpAudioStreamRecv(this));
			std::map<int, int> codecs;
			codecs[mediaCodec->payloadType] = mediaCodec->clockRate;
			rtp_aud_stream_recv_->SetReceiveCodecs(codecs);
			rtp_aud_stream_recv_->SetRtpExtensions(extensions);
		}
		else if (jsEvent["Type"] == "video") {
			video_payload_type_ = jsEvent["PayloadType"];
			if (rtpParam.encodings.size() == 1) {
				rtp_vid_stream_recv_.reset(new X2RtpVideoStreamRecv(0, 0, this));
				webrtc::VideoCodec codec;
				codec.plType = mediaCodec->payloadType;
				if (mediaCodec->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::H264) {
					codec.codecType = webrtc::kVideoCodecH264;
				}
				else if (mediaCodec->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::VP8) {
					codec.codecType = webrtc::kVideoCodecVP8;
				}
				else if (mediaCodec->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::VP9) {
					codec.codecType = webrtc::kVideoCodecVP9;
				}
				std::map<std::string, std::string> map;
				rtp_vid_stream_recv_->AddReceiveCodec(codec, map, false);
				rtp_vid_stream_recv_->SetRtpExtensions(extensions);
				rtp_vid_stream_recv_->SetRId("h");
			}
			else {
				for (auto& encodingV : rtpParam.encodings) {
					auto* mediaCodecV = rtpParam.GetCodecForEncoding(encodingV);
					if (encodingV.rid.length() > 0) {
						std::shared_ptr<X2RtpVideoStreamRecv> rtpRidStreamRecv;
						rtpRidStreamRecv.reset(new X2RtpVideoStreamRecv(0, 0, this));
						webrtc::VideoCodec codec;
						codec.plType = mediaCodecV->payloadType;
						if (mediaCodecV->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::H264) {
							codec.codecType = webrtc::kVideoCodecH264;
						}
						else if (mediaCodecV->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::VP8) {
							codec.codecType = webrtc::kVideoCodecVP8;
						}
						else if (mediaCodecV->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::VP9) {
							codec.codecType = webrtc::kVideoCodecVP9;
						}
						std::map<std::string, std::string> map;
						rtpRidStreamRecv->AddReceiveCodec(codec, map, false);
						rtpRidStreamRecv->SetRtpExtensions(extensions);
						rtpRidStreamRecv->SetRId(encodingV.rid);

						map_rtp_vid_stream_recv_[encodingV.rid] = rtpRidStreamRecv;
					}
					
				}
			}
			
			video_extensions_ = extensions;
		}
		else {
			return;
		}
	}
	else if (jsEvent["Event"] == "CloseProducer") {
		if (jsEvent["Type"] == "audio") {
			rtp_aud_stream_recv_ = NULL;
		}
		else if (jsEvent["Type"] == "video") {
			rtp_vid_stream_recv_ = NULL;
		}
	}
	else if (jsEvent["Event"] == "NewConsumer") {
		if (jsEvent["Type"] == "audio") {
			audio_payload_type_ = jsEvent["PayloadType"];
			sample_rate_ = jsEvent["SampleRate"];

		}
		else if (jsEvent["Type"] == "video") {
			video_payload_type_ = jsEvent["PayloadType"];

			json jsRtpParam = jsEvent["RtpParam"];
			RTC::RtpParameters rtpParam(jsRtpParam);
			webrtc::RtpHeaderExtensionMap extensions;
			for (auto& exten : rtpParam.headerExtensions)
			{
				if (exten.id != 0u) {
					extensions.RegisterByUri(exten.id, exten.uri);
				}
			}
			video_extensions_ = extensions;
		}
	}
}
void X2ProtoRtcHandler::ProcessData(int nDataType, const uint8_t* pData, size_t nLen)
{
	if ((RTC::Media::Kind)nDataType == RTC::Media::Kind::AUDIO) {
		if (rtp_aud_stream_recv_ != NULL) {
			rtp_aud_stream_recv_->RecvRtpData(pData, nLen);
		}
	}
	else if ((RTC::Media::Kind)nDataType == RTC::Media::Kind::VIDEO) {
		if (rtp_vid_stream_recv_ != NULL) {
			rtp_vid_stream_recv_->RecvRtpData(pData, nLen);
		}
		else {
			webrtc::RtpPacketReceived packet;
			if (packet.Parse((uint8_t*)pData, (size_t)nLen))
			{
				packet.IdentifyExtensions(video_extensions_);
				std::string rid;
				packet.GetExtension<webrtc::RtpStreamId>(&rid);
				if (rid.length() > 0) {
					if (map_ssrc_to_rid_.find(packet.Ssrc()) == map_ssrc_to_rid_.end()) {
						map_ssrc_to_rid_[packet.Ssrc()] = rid;
					}
				}
			}

			if (map_ssrc_to_rid_.find(packet.Ssrc()) != map_ssrc_to_rid_.end()) {
				std::string& rid = map_ssrc_to_rid_[packet.Ssrc()];
				if (map_rtp_vid_stream_recv_.find(rid) != map_rtp_vid_stream_recv_.end()) {
					map_rtp_vid_stream_recv_[rid]->RecvRtpData(pData, nLen);
					//printf("Recv rid : %s ssrc: %u video len: %d\r\n", rid.c_str(), packet.Ssrc(), nLen);
				}
			}
		}
	}
}

}	// namespace x2rtc