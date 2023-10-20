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
#ifndef __X2_CODEC_TYPE_H__
#define __X2_CODEC_TYPE_H__
#include <stdint.h>
#include <string.h>
#include "X2RtcRef.h"

namespace x2rtc {

#define X2CodecType_MAP(X2T) \
	X2T(X2Codec_None , -1, "None", "") \
	X2T(X2Codec_YUV420 , 0, "YUV420", "") \
	X2T(X2Codec_NV12 , 1, "NV12", "") \
	X2T(X2Codec_RGB565 , 5, "RGB565", "") \
	X2T(X2Codec_RGB888 , 6, "RGB888", "") \
	X2T(X2Codec_H264 , 10, "H264", "") \
	X2T(X2Codec_H265 , 11, "H265", "") \
	X2T(X2Codec_AV1 , 20, "AV1", "") \
	X2T(X2Codec_MJpg , 30, "MJpg", "") \
	X2T(X2Codec_PCM, 100, "PCM", "") \
	X2T(X2Codec_AAC, 101,"AAC", "") \
	X2T(X2Codec_MP3, 102, "MP3", "") \
	X2T(X2Codec_OPUS, 103, "OPUS", "") \
	X2T(X2Codec_G711A, 104, "G711A", "8khz") \
	X2T(X2Codec_G711U, 105, "G711U", "8khz") \
	X2T(X2Codec_G719A, 110, "G719A", "same as X2Codec_G719A_32") \
	X2T(X2Codec_G719A_32, 111, "G719A_32", "") \
	X2T(X2Codec_G719A_48, 112, "G719A_48", "") \
	X2T(X2Codec_G719A_64, 113, "G719A_64", "") \
	X2T(X2Codec_G722_48, 120, "G722_48", "16khz") \
	X2T(X2Codec_G722_56, 121, "G722_56", "16khz") \
	X2T(X2Codec_G722_64, 122, "G722_64", "16khz") \
	X2T(X2Codec_G7221_7_24, 130, "G7221_7_24", "16khz") \
	X2T(X2Codec_G7221_7_32, 131, "G7221_7_32", "16khz") \
	X2T(X2Codec_G7221_14_24, 132, "G7221_14_24", "32khz") \
	X2T(X2Codec_G7221_14_32, 133, "G7221_14_32", "32khz") \
	X2T(X2Codec_G7221_14_48, 134, "G7221_14_48", "32khz")

typedef enum X2CodecType
{
#define X2T(name,  value, str, desc) name = value,
		X2CodecType_MAP(X2T)
#undef X2T
		X2Codec_Max
}X2CodecType;

const char* getX2CodeTypeName(X2CodecType eType);


struct X2CodecData {
	X2CodecData() : pData(NULL), nLen(0), nDataRecvTime(0), eCodecType(X2Codec_None),  nAudSampleHz(0), nAudChannel(0), nAudSeqn(0), bIsAudio(false), bKeyframe(false), nVidRotate(0), nVidScaleRD(1), lDts(0), lPts(0) {
	}
	virtual ~X2CodecData(void) {
		delete[] pData;
	}

	void SetData(const uint8_t* data, int len) {
		nLen = len;
		if (pData != NULL) {
			delete[] pData;
			pData = NULL;
		}
		pData = new uint8_t[len + 1];
		memcpy(pData, data, len);
	}
	
	uint8_t* pData;			// Data data
	int nLen;				// Data len
	uint64_t nDataRecvTime;	// Data Recved time

	X2CodecType eCodecType;	// Data Type

	int nAudSampleHz;		// Audio SampleHz, if audio data is pcm
	int nAudChannel;		// Audio Channels, if audio data is pcm
	uint16_t nAudSeqn;		// Audio Seqn
	bool bIsAudio;			// Frame is Audio?
	bool bKeyframe;			// Video is Key Frame?
	int nVidRotate;			// Video rotate
	int nVidScaleRD;		// Video Scale Resolution Down 1: raw 2:w/2 h/2 3:w/3 h/3 4:w/4 h/4
	int64_t lDts;			// Dts: Decoding timestamp
	int64_t lPts;			// Pts: Presentation timestamp
};
TemplateX2RtcRef(X2CodecData);

}	// namespace x2rtc

#endif	// __X2_CODEC_TYPE_H__