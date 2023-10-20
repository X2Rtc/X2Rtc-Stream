#include "X2RtpAudioStreamRecv.h"
#include "media/base/media_constants.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/rtp_rtcp/source/rtp_format.h"
#include "modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.h"
#include "modules/rtp_rtcp/source/rtp_header_extensions.h"
#include "rtc_base/logging.h"
#include "rtc_base/numerics/safe_minmax.h"

constexpr double kAudioSampleDurationSeconds = 0.01;

// Video Sync.
constexpr int kVoiceEngineMinMinPlayoutDelayMs = 0;
constexpr int kVoiceEngineMaxMinPlayoutDelayMs = 10000;

X2RtpAudioStreamRecv::X2RtpAudioStreamRecv(audio_coding::OnCompleteFrameCallback*callback)
	: clock_(webrtc::Clock::GetRealTimeClock())
	, callback_(callback)
	, ntp_estimator_(clock_)
{

}
X2RtpAudioStreamRecv::~X2RtpAudioStreamRecv(void)
{

}

void X2RtpAudioStreamRecv::SetReceiveCodecs(const std::map<int, int>& codecs)
{
	for (const auto& kv : codecs) {
		RTC_DCHECK_GE(kv.second, 1000);
		payload_type_frequencies_[kv.first] = kv.second;
	}
}

void X2RtpAudioStreamRecv::SetRtpExtensions(webrtc::RtpHeaderExtensionMap&extensions)
{
	rtp_header_extensions_ = extensions;
}
bool X2RtpAudioStreamRecv::UpdateRtcpTimestamp(int64_t rtt, uint32_t ntp_secs, uint32_t ntp_frac, uint32_t rtp_timestamp)
{
	return ntp_estimator_.UpdateRtcpTimestamp(rtt, ntp_secs, ntp_frac, rtp_timestamp);
}

void X2RtpAudioStreamRecv::RecvRtpData(const uint8_t*rtp_packet, int rtp_packet_length)
{
	webrtc::RtpPacketReceived packet;
	if (!packet.Parse((uint8_t*)rtp_packet, (size_t)rtp_packet_length))
		return;
	packet.set_arrival_time_ms(clock_->TimeInMilliseconds());

	packet.IdentifyExtensions(rtp_header_extensions_);
	//packet.set_payload_type_frequency(webrtc::kVideoPayloadTypeFrequency);

	OnRtpPacket(packet);
}

void X2RtpAudioStreamRecv::DoTick()
{

}



void X2RtpAudioStreamRecv::OnRtpPacket(const webrtc::RtpPacketReceived& packet)
{
	int64_t now_ms = rtc::TimeMillis();

	{
		last_received_rtp_timestamp_ = packet.Timestamp();
		last_received_rtp_system_time_ms_ = now_ms;
	}

	const auto& it = payload_type_frequencies_.find(packet.PayloadType());
	if (it == payload_type_frequencies_.end())
		return;
	// TODO(nisse): Set payload_type_frequency earlier, when packet is parsed.
	webrtc::RtpPacketReceived packet_copy(packet);
	packet_copy.set_payload_type_frequency(it->second);

	//rtp_receive_statistics_->OnRtpPacket(packet_copy);

	webrtc::RTPHeader header;
	packet_copy.GetHeader(&header);

	ReceivePacket(packet_copy.data(), packet_copy.size(), header);
}

void X2RtpAudioStreamRecv::ReceivePacket(const uint8_t* packet,
	size_t packet_length,
	const webrtc::RTPHeader& header)
{
	const uint8_t* payload = packet + header.headerLength;
	assert(packet_length >= header.headerLength);
	size_t payload_length = packet_length - header.headerLength;

	size_t payload_data_length = payload_length - header.paddingLength;

	// E2EE Custom Audio Frame Decryption (This is optional).
	// Keep this buffer around for the lifetime of the OnReceivedPayloadData call.
	rtc::Buffer decrypted_audio_payload;
#if 0
	if (frame_decryptor_ != nullptr) {
		const size_t max_plaintext_size = frame_decryptor_->GetMaxPlaintextByteSize(
			cricket::MEDIA_TYPE_AUDIO, payload_length);
		decrypted_audio_payload.SetSize(max_plaintext_size);

		const std::vector<uint32_t> csrcs(header.arrOfCSRCs,
			header.arrOfCSRCs + header.numCSRCs);
		const FrameDecryptorInterface::Result decrypt_result =
			frame_decryptor_->Decrypt(
				cricket::MEDIA_TYPE_AUDIO, csrcs,
				/*additional_data=*/nullptr,
				rtc::ArrayView<const uint8_t>(payload, payload_data_length),
				decrypted_audio_payload);

		if (decrypt_result.IsOk()) {
			decrypted_audio_payload.SetSize(decrypt_result.bytes_written);
		}
		else {
			// Interpret failures as a silent frame.
			decrypted_audio_payload.SetSize(0);
		}

		payload = decrypted_audio_payload.data();
		payload_data_length = decrypted_audio_payload.size();
	}
	else if (crypto_options_.sframe.require_frame_encryption) {
		RTC_DLOG(LS_ERROR)
			<< "FrameDecryptor required but not set, dropping packet";
		payload_data_length = 0;
	}
#endif
	OnReceivedPayloadData(
		rtc::ArrayView<const uint8_t>(payload, payload_data_length), header);
}

void X2RtpAudioStreamRecv::OnReceivedPayloadData(rtc::ArrayView<const uint8_t> payload,
	const webrtc::RTPHeader& rtpHeader)
{
	int64_t ntp_time_ms =
		ntp_estimator_.Estimate(rtpHeader.timestamp);
	//printf("Recv aud rtpTime: %u ntpTime: %lld\r\n", rtpHeader.timestamp, ntp_time_ms);
	callback_->OnCompleteFrame(payload.data(), payload.size(), rtpHeader.sequenceNumber, rtpHeader.timestamp, ntp_time_ms);
}