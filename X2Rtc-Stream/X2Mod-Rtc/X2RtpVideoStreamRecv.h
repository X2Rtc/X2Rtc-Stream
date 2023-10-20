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
#ifndef __X2_RTP_VIDEO_STREAM_RECV_H__
#define __X2_RTP_VIDEO_STREAM_RECV_H__
#include "call/syncable.h"
#include "modules/rtp_rtcp/source/rtp_format.h"
#include "modules/rtp_rtcp/source/absolute_capture_time_receiver.h"
#include "modules/rtp_rtcp/include/remote_ntp_time_estimator.h"
#include "modules/rtp_rtcp/include/rtp_header_extension_map.h"
#include "modules/video_coding/h264_sps_pps_tracker.h"
#include "modules/video_coding/packet_buffer.h"
#include "modules/video_coding/rtp_frame_reference_finder.h"
#include "modules/video_coding/unique_timestamp_counter.h"
#include "rtc_base/critical_section.h"

class X2RtpVideoStreamRecv : public webrtc::video_coding::OnCompleteFrameCallback
{
public:
	X2RtpVideoStreamRecv(int redType, int ulpType, webrtc::video_coding::OnCompleteFrameCallback*callback);
	virtual ~X2RtpVideoStreamRecv(void);

	void SetRId(const std::string& strRId) { str_rid_ = strRId; }

	void AddReceiveCodec(const webrtc::VideoCodec& video_codec,
		const std::map<std::string, std::string>& codec_params,
		bool raw_payload);
	void FrameContinuous(int64_t picture_id);
	void FrameDecoded(int64_t picture_id);

	void SetRtpExtensions(webrtc::RtpHeaderExtensionMap&extensions);
	bool UpdateRtcpTimestamp(int64_t rtt, uint32_t ntp_secs, uint32_t ntp_frac, uint32_t rtp_timestamp);

	void RecvRtpData(const uint8_t*rtp_packet, int rtp_packet_length);

	// TODO(philipel): Stop using VCMPacket in the new jitter buffer and then
  //                 remove this function. Public only for tests.
	void OnReceivedPayloadData(rtc::ArrayView<const uint8_t> codec_payload,
		const webrtc::RtpPacketReceived& rtp_packet,
		const webrtc::RTPVideoHeader& video);

	// Implements OnCompleteFrameCallback.
	void OnCompleteFrame(
		std::unique_ptr<webrtc::video_coding::EncodedFrame> frame) override;

private:
	void RequestKeyFrame();
	// Entry point doing non-stats work for a received packet. Called
	// for the same packet both before and after RED decapsulation.
	void ReceivePacket(const webrtc::RtpPacketReceived& packet);
	// Parses and handles RED headers.
	// This function assumes that it's being called from only one thread.
	void ParseAndHandleEncapsulatingHeader(const webrtc::RtpPacketReceived& packet);
	void NotifyReceiverOfEmptyPacket(uint16_t seq_num);
	void UpdateHistograms();
	bool IsRedEnabled() const;
	void InsertSpsPpsIntoTracker(uint8_t payload_type);
	void OnInsertedPacket(webrtc::video_coding::PacketBuffer::InsertResult result);
	void OnAssembledFrame(std::unique_ptr<webrtc::video_coding::RtpFrameObject> frame);

private:
	int red_payload_type;
	int ulpfec_payload_type;
	uint32_t ntp_secs_;
	uint32_t ntp_frac_;
	uint32_t rtp_timestamp_;
	std::string str_rid_;
	webrtc::Clock* const clock_;
	webrtc::video_coding::OnCompleteFrameCallback* complete_frame_callback_;
	webrtc::RtpDepacketizer* depacketizer_;

	webrtc::RemoteNtpTimeEstimator ntp_estimator_;
	webrtc::RtpHeaderExtensionMap rtp_header_extensions_;

	webrtc::video_coding::PacketBuffer packet_buffer_;
	webrtc::UniqueTimestampCounter frame_counter_;
	std::unique_ptr<webrtc::video_coding::RtpFrameReferenceFinder> reference_finder_;

private:
	absl::optional<webrtc::VideoCodecType> current_codec_;
	uint32_t last_assembled_frame_rtp_timestamp_;

	rtc::CriticalSection last_seq_num_cs_;
	std::map<int64_t, uint16_t> last_seq_num_for_pic_id_;
	webrtc::video_coding::H264SpsPpsTracker tracker_;

	// Maps payload type to codec type, for packetization.
	std::map<uint8_t, absl::optional<webrtc::VideoCodecType>> payload_type_map_;

	// TODO(johan): Remove pt_codec_params_ once
	// https://bugs.chromium.org/p/webrtc/issues/detail?id=6883 is resolved.
	// Maps a payload type to a map of out-of-band supplied codec parameters.
	std::map<uint8_t, std::map<std::string, std::string>> pt_codec_params_;
	int16_t last_payload_type_ = -1;

	bool has_received_frame_;

private:
	absl::optional<webrtc::ColorSpace> last_color_space_;

	webrtc::AbsoluteCaptureTimeReceiver absolute_capture_time_receiver_;

	int64_t last_completed_picture_id_ = 0;
};

#endif	// __X2_RTP_VIDEO_STREAM_RECV_H__