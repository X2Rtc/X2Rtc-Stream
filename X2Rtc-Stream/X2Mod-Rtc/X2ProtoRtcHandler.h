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
#ifndef __X2_PROTO_RTC_HANDLER_H__
#define __X2_PROTO_RTC_HANDLER_H__
#include "X2ProtoHandler.h"
#include "X2RtpVideoStreamRecv.h"
#include "X2RtpAudioStreamRecv.h"

namespace x2rtc {

struct SyncTime
{
	SyncTime() {
		Clear();
	}
	void Clear() {

		nFirstAudNtpTime = -1;
		nFirstVidNtpTime = -1;

		nFirstRecvAudioRtpTime = -1;
		nFirstRecvVideoRtpTime = -1;

		nAudElapseTime = 0;
		nVidElapseTime = 0;
		nTimestampBase = 0;
	}


	int64_t nFirstAudNtpTime;
	int64_t nFirstVidNtpTime;

	int64_t nFirstRecvAudioRtpTime;
	int64_t nFirstRecvVideoRtpTime;

	int64_t nAudElapseTime;
	int64_t nVidElapseTime;
	int64_t nTimestampBase;
	rtc::TimestampWrapAroundHandler audioTimeHandler;
	rtc::TimestampWrapAroundHandler videoTimeHandler;
};

class X2ProtoRtcHandler : public X2ProtoHandler, public webrtc::video_coding::OnCompleteFrameCallback, audio_coding::OnCompleteFrameCallback
{
public:
	X2ProtoRtcHandler(void);
	virtual ~X2ProtoRtcHandler(void);

	//* For X2ProtoHandler
	virtual bool SetStreamId(const char* strStreamId);
	virtual void MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn);
	virtual void RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen);
	virtual void Close();

	//* For OnCompleteFrameCallback
	virtual void OnCompleteFrame(std::unique_ptr<webrtc::video_coding::EncodedFrame> frame);
	virtual void OnCompleteFrame(const std::string& strRId, std::unique_ptr<webrtc::video_coding::EncodedFrame> frame);
	virtual void OnCompleteFrame(const uint8_t* pData, int nLen, uint16_t nSeqn, uint32_t nTimestamp, int64_t ntpTimestamp);

	void MediaProducerRecvVideoFrame(const std::string&strRId, const char* pData, int nLen, bool bKeyframe, int rotate, uint32_t nRtpTimestamp, int64_t nNtpTimeMs);
	void MediaProducerRecvAudioFrame(const char* pData, int nLen, uint16_t nSeqn, uint32_t nRtpTimestamp, int64_t nNtpTimeMs);

protected:
	void ProcessEvent(const uint8_t* pData, size_t nLen);
	void ProcessData(int nDataType, const uint8_t* pData, size_t nLen);

private:
	int video_payload_type_{ 0 };
	int audio_payload_type_{ 0 };
	int sample_rate_{ 48000 };
	uint32_t		video_seqnum_{ 0 };
	SyncTime		sync_av_time_;
	X2CodecType e_aud_codec_type_{ X2Codec_OPUS };
	std::shared_ptr<X2RtpVideoStreamRecv> rtp_vid_stream_recv_;
	std::shared_ptr<X2RtpAudioStreamRecv> rtp_aud_stream_recv_;

	std::map<std::string, std::shared_ptr<X2RtpVideoStreamRecv>> map_rtp_vid_stream_recv_;
	std::map<uint32_t, std::string> map_ssrc_to_rid_;

	webrtc::RtpHeaderExtensionMap video_extensions_;

};

}	// namespace x2rtc
#endif	// __X2_PROTO_RTC_HANDLER_H__
