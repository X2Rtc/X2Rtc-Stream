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
#ifndef __X2_RTP_AUDIO_STREAM_RECV_H__
#define __X2_RTP_AUDIO_STREAM_RECV_H__
#include "api/rtp_headers.h"
#include "api/array_view.h"
#include "modules/rtp_rtcp/include/remote_ntp_time_estimator.h"
#include "modules/rtp_rtcp/include/rtp_header_extension_map.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/clock.h"

namespace audio_coding {
	class OnCompleteFrameCallback {
	public:
		virtual ~OnCompleteFrameCallback() {}
		virtual void OnCompleteFrame(const uint8_t*pData, int nLen, uint16_t nSeqn, uint32_t nTimestamp, int64_t ntpTimestamp) = 0;
	};
}


class X2RtpAudioStreamRecv
{
public:
	X2RtpAudioStreamRecv(audio_coding::OnCompleteFrameCallback*callback);
	virtual ~X2RtpAudioStreamRecv(void);

	void SetReceiveCodecs(const std::map<int, int>& codecs);
	void SetRtpExtensions(webrtc::RtpHeaderExtensionMap&extensions);
	bool UpdateRtcpTimestamp(int64_t rtt, uint32_t ntp_secs, uint32_t ntp_frac, uint32_t rtp_timestamp);

	void RecvRtpData(const uint8_t*rtp_packet, int rtp_packet_length);
	void DoTick();

private:
	void OnRtpPacket(const webrtc::RtpPacketReceived& packet);
	void ReceivePacket(const uint8_t* packet,
		size_t packet_length,
		const webrtc::RTPHeader& header);

	void OnReceivedPayloadData(rtc::ArrayView<const uint8_t> payload,
		const webrtc::RTPHeader& rtpHeader);

private:
	webrtc::Clock* const clock_;
	audio_coding::OnCompleteFrameCallback*callback_;
	webrtc::RtpHeaderExtensionMap rtp_header_extensions_;

	// Indexed by payload type.
	std::map<uint8_t, int> payload_type_frequencies_;

	// Info for GetSyncInfo is updated on network or worker thread, and queried on
	// the worker thread.
	absl::optional<uint32_t> last_received_rtp_timestamp_;
		
	absl::optional<int64_t> last_received_rtp_system_time_ms_;

	webrtc::RemoteNtpTimeEstimator ntp_estimator_;
};

#endif	// __X2_RTP_AUDIO_STREAM_RECV_H__
