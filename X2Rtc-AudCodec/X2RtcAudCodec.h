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
#ifndef __X2_RTC_AUD_CODEC_H__
#define __X2_RTC_AUD_CODEC_H__
#include "X2RtcDef.h"
#include <stdint.h>

namespace x2rtc {
	/** Audio codec types */
	enum X2_AUD_A_CODEC_TYPE {
		/** Raw PCM data */
		X2_AUD_A_CODEC_NONE = 0,	
		/** Standard OPUS */
		X2_AUD_A_CODEC_OPUS = 1,
		/** Standard AAC */
		X2_AUD_A_CODEC_AAC = 2,
		/** Standard G711A/PCMA */
		X2_AUD_A_CODEC_G711A = 3,
		/** Standard G711u/PCMU */
		X2_AUD_A_CODEC_G711U = 4,

		X2_AUD_A_CODEC_MAX,
	};

	class IX2AudEncoder
	{
	protected:
		IX2AudEncoder() :e_type_(X2_AUD_A_CODEC_NONE) {};
	public:
		virtual ~IX2AudEncoder() {};

		X2_AUD_A_CODEC_TYPE GetCodecType() {
			return e_type_;
		}

		virtual bool Init(X2_AUD_A_CODEC_TYPE aType, int nSampleHz, int nChannel, int nBitrate, bool bMusic) = 0;
		virtual void DeInit() = 0;
		virtual int DoEncode(const char* pData, int nLen, size_t length_encoded_buffer, uint8_t* encoded) = 0;
		virtual int ReSample10Ms(const void* audioSamples, const size_t nChannels, const uint32_t samplesPerSec, int16_t* temp_output) = 0;

	protected:
		X2_AUD_A_CODEC_TYPE e_type_;
	};

	X2R_API IX2AudEncoder* X2R_CALL createX2AudEncoder();


	class IX2AudDecoder
	{
	public:
		class Listener
		{
		public:
			Listener() {}
			virtual ~Listener() {};
			virtual void OnX2AudDecoderGotFrame(void* audioSamples, int nLen, uint32_t samplesPerSec, size_t nChannels, int64_t nPts) = 0;
		};
	protected:
		IX2AudDecoder():e_type_(X2_AUD_A_CODEC_NONE){};
	public:
		virtual ~IX2AudDecoder(void) {};

		X2_AUD_A_CODEC_TYPE GetCodecType() {
			return e_type_;
		}

		virtual bool Init(X2_AUD_A_CODEC_TYPE aType, int nSampleHz, int nChannel, int nBitrate) = 0;
		virtual void DeInit() = 0;

		virtual void DoDecode(const char* pData, int nLen, uint16_t nSeqn, int64_t nPts) = 0;
		virtual void DoDecode2(const char* pData, int nLen, uint16_t nSeqn, int64_t nPts, Listener* listener) = 0;
		virtual void DoClear() = 0;
		virtual int MixAudPcmData(bool mix, int nVolume, void* audioSamples, uint32_t samplesPerSec, size_t nChannels) = 0;
		virtual int MixAudPcmData(bool mix, void* audioSamples, uint32_t samplesPerSec, size_t nChannels) = 0;

	protected:
		X2_AUD_A_CODEC_TYPE e_type_;

	};

	X2R_API IX2AudDecoder* X2R_CALL createX2AudDecoder();


	X2R_API size_t X2R_CALL IX2RtcG711_EncodeA(const int16_t* speechIn,
                          size_t len,
                          uint8_t* encoded);
	X2R_API size_t X2R_CALL IX2RtcG711_EncodeU(const int16_t* speechIn,
							  size_t len,
							  uint8_t* encoded);
	X2R_API size_t X2R_CALL IX2RtcG711_DecodeA(const uint8_t* encoded,
							  size_t len,
							  int16_t* decoded,
							  int16_t* speechType);
	X2R_API size_t X2R_CALL IX2RtcG711_DecodeU(const uint8_t* encoded,
							  size_t len,
							  int16_t* decoded,
							  int16_t* speechType);


}	// namespace x2rtc
#endif	// __X2_RTC_AUD_CODEC_H__