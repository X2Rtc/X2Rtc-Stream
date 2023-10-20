#include "X2RtpVideoStreamRecv.h"
#include "absl/algorithm/container.h"
#include "absl/memory/memory.h"
#include "media/base/media_constants.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.h"
#include "modules/rtp_rtcp/source/rtp_header_extensions.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "modules/video_coding/frame_object.h"
#include "modules/video_coding/h264_sprop_parameter_sets.h"
#include "modules/video_coding/h264_sps_pps_tracker.h"
#include "modules/video_coding/packet_buffer.h"
#include "rtc_base/logging.h"

const int kDefaultH264PlType = 0;
// TODO(philipel): Change kPacketBufferStartSize back to 32 in M63 see:
//                 crbug.com/752886
constexpr int kPacketBufferStartSize = 512;
constexpr int kPacketBufferMaxSize = 2048;
constexpr int kMinBaseMinimumDelayMs = 0;
constexpr int kMaxBaseMinimumDelayMs = 10000;

constexpr int kMaxWaitForFrameMs = 3000;
// The default number of milliseconds to pass before re-requesting a key frame
// to be sent.
static constexpr int kMaxWaitForKeyFrameMs = 200;
// TODO(https://bugs.webrtc.org/9974): Consider removing this workaround.
// Maximum time between frames before resetting the FrameBuffer to avoid RTP
// timestamps wraparound to affect FrameBuffer.
constexpr int kInactiveStreamThresholdMs = 600000;  //  10 minutes.

X2RtpVideoStreamRecv::X2RtpVideoStreamRecv(int redType, int ulpType, webrtc::video_coding::OnCompleteFrameCallback*callback)
	: clock_(webrtc::Clock::GetRealTimeClock())
	, red_payload_type(redType)
	, ulpfec_payload_type(ulpType)
	, ntp_secs_(0)
	, ntp_frac_(0)
	, rtp_timestamp_(0)
	, complete_frame_callback_(callback)
	, depacketizer_(NULL)

	, ntp_estimator_(clock_)
	, packet_buffer_(clock_, kPacketBufferStartSize, kPacketBufferMaxSize)
	, has_received_frame_(false)
	, absolute_capture_time_receiver_(clock_)
{
	reference_finder_ =
		absl::make_unique<webrtc::video_coding::RtpFrameReferenceFinder>(this);

}
X2RtpVideoStreamRecv::~X2RtpVideoStreamRecv(void)
{
	if (depacketizer_ != NULL) {
		delete depacketizer_;
		depacketizer_ = NULL;
	}
}

void X2RtpVideoStreamRecv::AddReceiveCodec(const webrtc::VideoCodec& video_codec,
	const std::map<std::string, std::string>& codec_params,
	bool raw_payload)
{
	absl::optional<webrtc::VideoCodecType> video_type;
	if (!raw_payload) {
		video_type = video_codec.codecType;
	}
	payload_type_map_.emplace(video_codec.plType, video_type);
	pt_codec_params_.emplace(video_codec.plType, codec_params);
}

void X2RtpVideoStreamRecv::FrameContinuous(int64_t picture_id)
{
#if 0
	if (!nack_module_)
		return;
	int seq_num = -1;
	{
		rtc::CritScope lock(&last_seq_num_cs_);
		auto seq_num_it = last_seq_num_for_pic_id_.find(picture_id);
		if (seq_num_it != last_seq_num_for_pic_id_.end())
			seq_num = seq_num_it->second;
	}
	if (seq_num != -1)
		nack_module_->ClearUpTo(seq_num);
#endif
}
void X2RtpVideoStreamRecv::FrameDecoded(int64_t picture_id)
{
	int seq_num = -1;
	{
		rtc::CritScope lock(&last_seq_num_cs_);
		auto seq_num_it = last_seq_num_for_pic_id_.find(picture_id);
		if (seq_num_it != last_seq_num_for_pic_id_.end()) {
			seq_num = seq_num_it->second;
			last_seq_num_for_pic_id_.erase(last_seq_num_for_pic_id_.begin(),
				++seq_num_it);
		}
	}
	if (seq_num != -1) {
		packet_buffer_.ClearTo(seq_num);
		reference_finder_->ClearTo(seq_num);
	}
}
void X2RtpVideoStreamRecv::SetRtpExtensions(webrtc::RtpHeaderExtensionMap&extensions)
{
	rtp_header_extensions_ = extensions;
}
bool X2RtpVideoStreamRecv::UpdateRtcpTimestamp(int64_t rtt, uint32_t ntp_secs, uint32_t ntp_frac, uint32_t rtp_timestamp)
{
	ntp_secs_ = ntp_secs;
	ntp_frac_ = ntp_frac;
	rtp_timestamp_ = rtp_timestamp;
	return ntp_estimator_.UpdateRtcpTimestamp(rtt, ntp_secs, ntp_frac, rtp_timestamp);
}

void X2RtpVideoStreamRecv::RecvRtpData(const uint8_t*rtp_packet, int rtp_packet_length)
{
	webrtc::RtpPacketReceived packet;
	if (!packet.Parse((uint8_t*)rtp_packet, (size_t)rtp_packet_length))
		return;
	if (packet.PayloadType() == red_payload_type) {
		RTC_LOG(LS_WARNING) << "Discarding recovered packet with RED encapsulation";
		return;
	}
	packet.set_arrival_time_ms(clock_->TimeInMilliseconds());

	packet.IdentifyExtensions(rtp_header_extensions_);
	packet.set_payload_type_frequency(webrtc::kVideoPayloadTypeFrequency);
	// TODO(nisse): UlpfecReceiverImpl::ProcessReceivedFec passes both
	// original (decapsulated) media packets and recovered packets to
	// this callback. We need a way to distinguish, for setting
	// packet.recovered() correctly. Ideally, move RED decapsulation out
	// of the Ulpfec implementation.

	ReceivePacket(packet);
}

// TODO(philipel): Stop using VCMPacket in the new jitter buffer and then
  //                 remove this function. Public only for tests.
void X2RtpVideoStreamRecv::OnReceivedPayloadData(rtc::ArrayView<const uint8_t> codec_payload,
	const webrtc::RtpPacketReceived& rtp_packet,
	const webrtc::RTPVideoHeader& video)
{
	// |ntp_time_ms_| won't be valid until at least 2 RTCP SRs are received.
	int64_t ntpTime = ntp_estimator_.Estimate(rtp_packet.Timestamp());
	//printf("Vid ntpTime: %lld \r\n", ntpTime);
	webrtc::video_coding::PacketBuffer::Packet packet(
		rtp_packet, video, ntpTime, clock_->TimeInMilliseconds());

	// Try to extrapolate absolute capture time if it is missing.
	// TODO(bugs.webrtc.org/10739): Add support for estimated capture clock
	// offset.
	packet.packet_info.set_absolute_capture_time(
		absolute_capture_time_receiver_.OnReceivePacket(
			webrtc::AbsoluteCaptureTimeReceiver::GetSource(packet.packet_info.ssrc(),
				packet.packet_info.csrcs()),
			packet.packet_info.rtp_timestamp(),
			// Assume frequency is the same one for all video frames.
			webrtc::kVideoPayloadTypeFrequency,
			packet.packet_info.absolute_capture_time()));

	webrtc::RTPVideoHeader& video_header = packet.video_header;
	video_header.rotation = webrtc::kVideoRotation_0;
	video_header.content_type = webrtc::VideoContentType::UNSPECIFIED;
	video_header.video_timing.flags = webrtc::VideoSendTiming::kInvalid;
	video_header.is_last_packet_in_frame |= rtp_packet.Marker();
	video_header.frame_marking.temporal_id = webrtc::kNoTemporalIdx;

	if (const auto* vp9_header =
		absl::get_if<webrtc::RTPVideoHeaderVP9>(&video_header.video_type_header)) {
		video_header.is_last_packet_in_frame |= vp9_header->end_of_frame;
		video_header.is_first_packet_in_frame |= vp9_header->beginning_of_frame;
	}

	rtp_packet.GetExtension<webrtc::VideoOrientation>(&video_header.rotation);
	rtp_packet.GetExtension<webrtc::VideoContentTypeExtension>(
		&video_header.content_type);
	rtp_packet.GetExtension<webrtc::VideoTimingExtension>(&video_header.video_timing);
	rtp_packet.GetExtension<webrtc::PlayoutDelayLimits>(&video_header.playout_delay);
	rtp_packet.GetExtension<webrtc::FrameMarkingExtension>(&video_header.frame_marking);

	webrtc::RtpGenericFrameDescriptor& generic_descriptor =
		packet.generic_descriptor.emplace();
	if (rtp_packet.GetExtension<webrtc::RtpGenericFrameDescriptorExtension01>(
		&generic_descriptor)) {
		if (rtp_packet.HasExtension<webrtc::RtpGenericFrameDescriptorExtension00>()) {
			RTC_LOG(LS_WARNING) << "RTP packet had two different GFD versions.";
			return;
		}
		generic_descriptor.SetByteRepresentation(
			rtp_packet.GetRawExtension<webrtc::RtpGenericFrameDescriptorExtension01>());
	}
	else if ((rtp_packet.GetExtension<webrtc::RtpGenericFrameDescriptorExtension00>(
		&generic_descriptor))) {
		generic_descriptor.SetByteRepresentation(
			rtp_packet.GetRawExtension<webrtc::RtpGenericFrameDescriptorExtension00>());
	}
	else {
		packet.generic_descriptor = absl::nullopt;
	}
	if (packet.generic_descriptor != absl::nullopt) {
		video_header.is_first_packet_in_frame =
			packet.generic_descriptor->FirstPacketInSubFrame();
		video_header.is_last_packet_in_frame =
			rtp_packet.Marker() ||
			packet.generic_descriptor->LastPacketInSubFrame();

		if (packet.generic_descriptor->FirstPacketInSubFrame()) {
			video_header.frame_type =
				packet.generic_descriptor->FrameDependenciesDiffs().empty()
				? webrtc::VideoFrameType::kVideoFrameKey
				: webrtc::VideoFrameType::kVideoFrameDelta;
		}

		video_header.width = packet.generic_descriptor->Width();
		video_header.height = packet.generic_descriptor->Height();
	}

	// Color space should only be transmitted in the last packet of a frame,
	// therefore, neglect it otherwise so that last_color_space_ is not reset by
	// mistake.
	if (video_header.is_last_packet_in_frame) {
		video_header.color_space = rtp_packet.GetExtension<webrtc::ColorSpaceExtension>();
		if (video_header.color_space ||
			video_header.frame_type == webrtc::VideoFrameType::kVideoFrameKey) {
			// Store color space since it's only transmitted when changed or for key
			// frames. Color space will be cleared if a key frame is transmitted
			// without color space information.
			last_color_space_ = video_header.color_space;
		}
		else if (last_color_space_) {
			video_header.color_space = last_color_space_;
		}
	}

#if 0
	if (loss_notification_controller_) {
		if (rtp_packet.recovered()) {
			// TODO(bugs.webrtc.org/10336): Implement support for reordering.
			RTC_LOG(LS_INFO)
				<< "LossNotificationController does not support reordering.";
		}
		else if (!packet.generic_descriptor) {
			RTC_LOG(LS_WARNING) << "LossNotificationController requires generic "
				"frame descriptor, but it is missing.";
		}
		else {
			loss_notification_controller_->OnReceivedPacket(
				rtp_packet.SequenceNumber(), *packet.generic_descriptor);
		}
	}
#endif
	packet.times_nacked = -1;


	if (codec_payload.empty()) {
		NotifyReceiverOfEmptyPacket(packet.seq_num);
		//rtcp_feedback_buffer_.SendBufferedRtcpFeedback();
		return;
	}

	if (packet.codec() == webrtc::kVideoCodecH264) {
		// Only when we start to receive packets will we know what payload type
		// that will be used. When we know the payload type insert the correct
		// sps/pps into the tracker.
		if (packet.payload_type != last_payload_type_) {
			last_payload_type_ = packet.payload_type;
			InsertSpsPpsIntoTracker(packet.payload_type);
		}

		webrtc::video_coding::H264SpsPpsTracker::FixedBitstream fixed =
			tracker_.CopyAndFixBitstream(codec_payload, &packet.video_header);

		switch (fixed.action) {
		case webrtc::video_coding::H264SpsPpsTracker::kRequestKeyframe:
			//rtcp_feedback_buffer_.RequestKeyFrame();
			//rtcp_feedback_buffer_.SendBufferedRtcpFeedback();
			//RTC_FALLTHROUGH();
		case webrtc::video_coding::H264SpsPpsTracker::kDrop:
			return;
		case webrtc::video_coding::H264SpsPpsTracker::kInsert:
			packet.video_payload = std::move(fixed.bitstream);
			break;
		}

	}
	else {
		packet.video_payload.SetData(codec_payload.data(), codec_payload.size());
	}

	//rtcp_feedback_buffer_.SendBufferedRtcpFeedback();
	frame_counter_.Add(packet.timestamp);
	OnInsertedPacket(packet_buffer_.InsertPacket(&packet));
}

// Implements OnCompleteFrameCallback.
void X2RtpVideoStreamRecv::OnCompleteFrame(
	std::unique_ptr<webrtc::video_coding::EncodedFrame> frame)
{
	int64_t picture_id = frame->id.picture_id;
	{
		rtc::CritScope lock(&last_seq_num_cs_);
		webrtc::video_coding::RtpFrameObject* rtp_frame =
			static_cast<webrtc::video_coding::RtpFrameObject*>(frame.get());
		last_seq_num_for_pic_id_[rtp_frame->id.picture_id] =
			rtp_frame->last_seq_num();
	}

	last_completed_picture_id_ =
	std::max(last_completed_picture_id_, frame->id.picture_id);
	complete_frame_callback_->OnCompleteFrame(str_rid_, std::move(frame));

	FrameDecoded(picture_id);
}

void X2RtpVideoStreamRecv::RequestKeyFrame()
{
	//keyframe_required_ = true;
}
// Entry point doing non-stats work for a received packet. Called
// for the same packet both before and after RED decapsulation.
void X2RtpVideoStreamRecv::ReceivePacket(const webrtc::RtpPacketReceived& packet)
{
	if (packet.payload_size() == 0) {
		// Padding or keep-alive packet.
		// TODO(nisse): Could drop empty packets earlier, but need to figure out how
		// they should be counted in stats.
		NotifyReceiverOfEmptyPacket(packet.SequenceNumber());
		return;
	}
	if (packet.PayloadType() == red_payload_type) {
		ParseAndHandleEncapsulatingHeader(packet);
		return;
	}

	const auto type_it = payload_type_map_.find(packet.PayloadType());
	if (type_it == payload_type_map_.end()) {
		return;
	}
	if (depacketizer_ == NULL) {
		depacketizer_ = webrtc::RtpDepacketizer::Create(type_it->second);
	}
	//auto depacketizer = absl::WrapUnique(webrtc::RtpDepacketizer::Create(type_it->second));

	if (!depacketizer_) {
		RTC_LOG(LS_ERROR) << "Failed to create depacketizer.";
		return;
	}
	webrtc::RtpDepacketizer::ParsedPayload parsed_payload;
	if (!depacketizer_->Parse(&parsed_payload, packet.payload().data(),
		packet.payload().size())) {
		RTC_LOG(LS_WARNING) << "Failed parsing payload.";
		return;
	}

	OnReceivedPayloadData(
		rtc::MakeArrayView(parsed_payload.payload, parsed_payload.payload_length),
		packet, parsed_payload.video);
}

void X2RtpVideoStreamRecv::ParseAndHandleEncapsulatingHeader(const webrtc::RtpPacketReceived& packet)
{
	if (packet.PayloadType() == red_payload_type &&
		packet.payload_size() > 0) {
		if (packet.payload()[0] == ulpfec_payload_type) {
			// Notify video_receiver about received FEC packets to avoid NACKing these
			// packets.
			NotifyReceiverOfEmptyPacket(packet.SequenceNumber());
		}
#if 0
		if (!ulpfec_receiver_->AddReceivedRedPacket(
			packet, config_.rtp.ulpfec_payload_type)) {
			return;
		}
		ulpfec_receiver_->ProcessReceivedFec();
#endif
	}
}
void X2RtpVideoStreamRecv::NotifyReceiverOfEmptyPacket(uint16_t seq_num)
{
	{
		//rtc::CritScope lock(&reference_finder_lock_);
		reference_finder_->PaddingReceived(seq_num);
	}
	OnInsertedPacket(packet_buffer_.InsertPadding(seq_num));
}
void X2RtpVideoStreamRecv::UpdateHistograms()
{

}
bool X2RtpVideoStreamRecv::IsRedEnabled() const
{
	return false;
}
void X2RtpVideoStreamRecv::InsertSpsPpsIntoTracker(uint8_t payload_type)
{
	auto codec_params_it = pt_codec_params_.find(payload_type);
	if (codec_params_it == pt_codec_params_.end())
		return;

	RTC_LOG(LS_INFO) << "Found out of band supplied codec parameters for"
		<< " payload type: " << static_cast<int>(payload_type);

	webrtc::H264SpropParameterSets sprop_decoder;
	auto sprop_base64_it =
		codec_params_it->second.find(cricket::kH264FmtpSpropParameterSets);

	if (sprop_base64_it == codec_params_it->second.end())
		return;

	if (!sprop_decoder.DecodeSprop(sprop_base64_it->second.c_str()))
		return;

	tracker_.InsertSpsPpsNalus(sprop_decoder.sps_nalu(),
		sprop_decoder.pps_nalu());
}
void X2RtpVideoStreamRecv::OnInsertedPacket(webrtc::video_coding::PacketBuffer::InsertResult result)
{
	for (std::unique_ptr<webrtc::video_coding::RtpFrameObject>& frame : result.frames) {
		OnAssembledFrame(std::move(frame));
	}
	if (result.buffer_cleared) {
		RequestKeyFrame();
	}
}
void X2RtpVideoStreamRecv::OnAssembledFrame(std::unique_ptr<webrtc::video_coding::RtpFrameObject> frame)
{
	absl::optional<webrtc::RtpGenericFrameDescriptor> descriptor =
		frame->GetGenericFrameDescriptor();

#if 0
	if (loss_notification_controller_ && descriptor) {
		loss_notification_controller_->OnAssembledFrame(
			frame->first_seq_num(), descriptor->FrameId(),
			descriptor->Discardable().value_or(false),
			descriptor->FrameDependenciesDiffs());
	}
#endif

	// If frames arrive before a key frame, they would not be decodable.
	// In that case, request a key frame ASAP.
	if (!has_received_frame_) {
		if (frame->FrameType() != webrtc::VideoFrameType::kVideoFrameKey) {
			// |loss_notification_controller_|, if present, would have already
			// requested a key frame when the first packet for the non-key frame
			// had arrived, so no need to replicate the request.rtp_header_extensions_
			//if (!loss_notification_controller_) 
			{
				RequestKeyFrame();
			}
		}
		has_received_frame_ = true;
	}

	// Reset |reference_finder_| if |frame| is new and the codec have changed.
	if (current_codec_) {
		bool frame_is_newer =
			webrtc::AheadOf(frame->Timestamp(), last_assembled_frame_rtp_timestamp_);

		if (frame->codec_type() != current_codec_) {
			if (frame_is_newer) {
				// When we reset the |reference_finder_| we don't want new picture ids
				// to overlap with old picture ids. To ensure that doesn't happen we
				// start from the |last_completed_picture_id_| and add an offset in case
				// of reordering.
				reference_finder_ =
					absl::make_unique<webrtc::video_coding::RtpFrameReferenceFinder>(
						this, last_completed_picture_id_ +
						std::numeric_limits<uint16_t>::max());
				current_codec_ = frame->codec_type();
			}
			else {
				// Old frame from before the codec switch, discard it.
				return;
			}
		}

		if (frame_is_newer) {
			last_assembled_frame_rtp_timestamp_ = frame->Timestamp();
		}
	}
	else {
		current_codec_ = frame->codec_type();
		last_assembled_frame_rtp_timestamp_ = frame->Timestamp();
	}

	reference_finder_->ManageFrame(std::move(frame));
}

