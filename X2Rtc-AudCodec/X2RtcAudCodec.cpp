#include "X2RtcAudCodec.h"
#include "X2AudioUtil.h"
#include <list>
#include <string.h>
#include "pluginaac.h"
#include "webrtc/modules/audio_coding/codecs/g711/g711_interface.h"
#include "webrtc/modules/audio_coding/codecs/opus/opus_interface.h"
#include "jmutexautolock.h"
#include "webrtc/modules/audio_coding/acm2/acm_resampler.h"

const int kMaxDataSizeSamples = 1920;
#define AUD_120MS_LEN 23040	// 480 * 2 * 2 * 12	// 120
#define FRAME_LENGTH 960

namespace x2rtc {
class X2AudEncoder : public IX2AudEncoder
{
public:
	X2AudEncoder(void)
		: enc_inst_(NULL)
		, req_sample_hz_(48000)
		, req_channel_(1)
		, req_aud_len_(0)
		, n_complexity_(3)
		, audio_samples_(NULL)
		, audio_samples_len_(0)
		, audio_samples_size_(0)
	{};
	virtual ~X2AudEncoder(void) {
		DeInit();
	};

	virtual bool Init(X2_AUD_A_CODEC_TYPE aType, int nSampleHz, int nChannel, int nBitrate, bool bMusic) {
		e_type_ = aType;
		req_sample_hz_ = nSampleHz;
		req_channel_ = nChannel;
		req_aud_len_ = (req_sample_hz_ * req_channel_ * sizeof(int16_t)) / 100;
		if (e_type_ != X2_AUD_A_CODEC_AAC) {// AAC has own buffer
			audio_samples_size_ = req_aud_len_ * 2;	// 20ms
			if (audio_samples_ == NULL) {
				audio_samples_ = new char[audio_samples_size_];
			}
		}
		else {
			audio_samples_size_ = req_aud_len_;	// 10ms
		}
		if (aType == X2_AUD_A_CODEC_OPUS) {
			if (enc_inst_ == NULL) {
				OpusEncInst* opus_enc_ = NULL;
				if (WebRtcOpus_EncoderCreate(&opus_enc_, nChannel, 1, nSampleHz) == 0)
				{
					if (n_complexity_ != 0) {
						WebRtcOpus_SetComplexity(opus_enc_, n_complexity_);
					}
					else {
#if (defined(__APPLE__) || TARGET_OS_IPHONE) || (defined(__ANDROID__) || defined(WEBRTC_ANDROID)) || defined(WEBRTC_LINUX)
						WebRtcOpus_SetComplexity(opus_enc_, 5);
#else
						WebRtcOpus_SetComplexity(opus_enc_, 9);
#endif
					}

					WebRtcOpus_SetBitRate(opus_enc_, nBitrate);
					//WebRtcOpus_SetSignalType(opus_enc_, OPUS_SIGNAL_MUSIC);
					WebRtcOpus_SetMaxPlaybackRate(opus_enc_, nSampleHz);

					enc_inst_ = (void*)opus_enc_;
				}
				else
				{
					return false;
				}
			}
		}
		else if (e_type_ == X2_AUD_A_CODEC_AAC) {
			if (enc_inst_ == NULL) {
				int bitpersample = 16;
				enc_inst_ = aac_encoder_open(nChannel, nSampleHz, bitpersample, nBitrate, false);
			}
		}
		return true;
	};
	virtual void DeInit() {
		if (e_type_ == X2_AUD_A_CODEC_OPUS) {
			if (enc_inst_ != NULL) {
				OpusEncInst* opus_enc_ = (OpusEncInst*)enc_inst_;
				WebRtcOpus_EncoderFree(opus_enc_);
				opus_enc_ = NULL;
			}
		}
		else if (e_type_ == X2_AUD_A_CODEC_AAC) {
			if (enc_inst_ != NULL) {
				aac_encoder_close(enc_inst_);
			}
		}
		enc_inst_ = NULL;

		if (audio_samples_ != NULL) {
			delete[] audio_samples_;
			audio_samples_ = NULL;
		}
	};
	virtual int DoEncode(const char* pData, int nLen, size_t length_encoded_buffer, uint8_t* encoded) {
		uint32_t enc_len = 0;
		if (nLen <= 0) {
			return -1;
		}
		if (nLen + audio_samples_len_ > audio_samples_size_) {
			return -2;
		}
		if (nLen % req_aud_len_ != 0) {
			return -3;
		}
		if (e_type_ == X2_AUD_A_CODEC_AAC) {
			if (enc_inst_ != NULL) {
				int outLen = 0;
				outLen = aac_encoder_encode_frame(enc_inst_, (uint8_t*)pData, nLen, encoded, &enc_len);

			}
		}
		else {
			memcpy(audio_samples_ + audio_samples_len_, pData, nLen);
			audio_samples_len_ += nLen;
			if (audio_samples_len_ >= audio_samples_size_) {

				if (e_type_ == X2_AUD_A_CODEC_OPUS) {
					if (enc_inst_ != NULL) {
						OpusEncInst* opus_enc_ = (OpusEncInst*)enc_inst_;
						enc_len = WebRtcOpus_Encode(opus_enc_, (int16_t*)audio_samples_, (audio_samples_len_) / ((sizeof(int16_t) * req_channel_)), length_encoded_buffer, (uint8_t*)encoded);
					}
				}
				else if (e_type_ == X2_AUD_A_CODEC_G711A) {
					enc_len = WebRtcG711_EncodeA((int16_t*)audio_samples_, (audio_samples_len_) / ((sizeof(int16_t) * req_channel_)), (uint8_t*)encoded);
				}
				else if (e_type_ == X2_AUD_A_CODEC_G711U) {
					enc_len = WebRtcG711_EncodeU((int16_t*)audio_samples_, (audio_samples_len_) / ((sizeof(int16_t) * req_channel_)), (uint8_t*)encoded);
				}
				audio_samples_len_ = 0;
			}
		}

		return enc_len;
	};
	virtual int ReSample10Ms(const void* audioSamples, const size_t nChannels, const uint32_t samplesPerSec, int16_t* temp_output) {
		if (req_sample_hz_ != samplesPerSec || req_channel_ != nChannels) {

			int samples_per_channel_int = resampler_.Resample10Msec((int16_t*)audioSamples, samplesPerSec * nChannels,
				req_sample_hz_ * req_channel_, 1, 1920, temp_output);
		}
		else {
			memcpy(temp_output, audioSamples, (req_sample_hz_ * req_channel_ * sizeof(int16_t)) / 100);
		}

		int samples_per_channel_int = req_sample_hz_ / 100;
		int nSamplesOut = samples_per_channel_int * req_channel_;
		nSamplesOut *= sizeof(int16_t);
		return nSamplesOut;
	};

private:
	webrtc::acm2::ACMResampler resampler_;
	void* enc_inst_;
	//OpusEncInst			*opus_enc_;

	int					n_complexity_;

	int					req_sample_hz_;
	int					req_channel_;
	int					req_aud_len_;

private:
	char* audio_samples_;
	int					audio_samples_len_;
	int					audio_samples_size_;
};

IX2AudEncoder* X2R_CALL createX2AudEncoder()
{
	return new X2AudEncoder();
}

class X2AudDecoder : public IX2AudDecoder
{
public:
	X2AudDecoder()
		: dec_inst_(NULL)
		, audio_dec_samples_(NULL)
		, audio_dec_used_(0)
		, n_aud_sample_hz_(48000)
		, n_aud_channels_(1)
		, n_aud_10ms_len_(0) {
	};

	virtual ~X2AudDecoder(void) {
		DeInit();
	};

	virtual bool Init(X2_AUD_A_CODEC_TYPE aType, int nSampleHz, int nChannels, int nBitrate) {
		e_type_ = aType;
		n_aud_sample_hz_ = nSampleHz;
		n_aud_channels_ = nChannels;
		if (audio_dec_samples_ != NULL) {
			delete[] audio_dec_samples_;
			audio_dec_samples_ = NULL;
		}
		if (audio_dec_samples_ == NULL) {
			n_aud_10ms_len_ = (nSampleHz * nChannels * sizeof(int16_t)) / 100;
			audio_dec_samples_ = new char[AUD_120MS_LEN];
		}
		audio_dec_used_ = 0;
		if (e_type_ == X2_AUD_A_CODEC_OPUS) {
			//neteq_decoder_ = createNeteqDecoder("", *this);
			OpusDecInst* opus_dec_ = NULL;
			if (WebRtcOpus_DecoderCreate(&opus_dec_, nChannels, nSampleHz) == 0)
			{
				WebRtcOpus_DecoderInit(opus_dec_);
				dec_inst_ = opus_dec_;
			}
			else {
				return false;
			}
		}
		else if (e_type_ == X2_AUD_A_CODEC_AAC) {

		}

		return true;
	};
	virtual void DeInit() {
		if (dec_inst_ != NULL) {
			if (e_type_ == X2_AUD_A_CODEC_OPUS) {
				OpusDecInst* opus_dec_ = (OpusDecInst*)dec_inst_;
				WebRtcOpus_DecoderFree(opus_dec_);
			}
			else if (e_type_ == X2_AUD_A_CODEC_AAC) {
				aac_decoder_close(dec_inst_);
			}
			dec_inst_ = NULL;
		}
		if (audio_dec_samples_ != NULL) {
			delete[] audio_dec_samples_;
			audio_dec_samples_ = NULL;
		}

		DoClear();
	};

	virtual void DoDecode(const char* pData, int nLen, uint16_t nSeqn, int64_t nPts) {
		DoDecode2(pData, nLen, nSeqn, nPts, NULL);
	};
	virtual void DoDecode2(const char* pData, int nLen, uint16_t nSeqn, int64_t nPts, Listener* listener)
	{
		int16_t audType = 0;
		int ret = 0;
		if (e_type_ == X2_AUD_A_CODEC_NONE) {
			memcpy(audio_dec_samples_, pData, nLen);
			ret = nLen;
			ret /= (n_aud_channels_ * sizeof(short));
		}
		else if (e_type_ == X2_AUD_A_CODEC_OPUS) {
			OpusDecInst* opus_dec_ = (OpusDecInst*)dec_inst_;
#if 0
			int fec = WebRtcOpus_PacketHasFec((uint8_t*)(pData), nLen);
			if (fec) {
				printf("fec \r\n");
			}
#endif
			ret = WebRtcOpus_Decode(opus_dec_, (uint8_t*)(pData), nLen, (int16_t*)(audio_dec_samples_), &audType);
		}
		else if (e_type_ == X2_AUD_A_CODEC_AAC) {
			if (dec_inst_ == NULL) {
				uint32_t		aac_sample_hz_;
				uint8_t			aac_channels_;
				dec_inst_ = aac_decoder_open((uint8_t*)pData, nLen, &aac_channels_, &aac_sample_hz_);
				if (aac_channels_ == 0)
					aac_channels_ = 1;
				Init(X2_AUD_A_CODEC_AAC, aac_sample_hz_, aac_channels_, 64000);
			}
			else {
				uint32_t outLen = 0;
				ret = aac_decoder_decode_frame(dec_inst_, (uint8_t*)pData, nLen, (uint8_t*)audio_dec_samples_ + audio_dec_used_, &outLen);
				if (ret > 0) {
					ret += audio_dec_used_;
					ret /= (n_aud_channels_ * sizeof(short));
				}
			}
		}
		else if (e_type_ == X2_AUD_A_CODEC_G711A) {
			ret = WebRtcG711_DecodeA((uint8_t*)(pData), nLen, (int16_t*)(audio_dec_samples_), &audType);
		}
		else if (e_type_ == X2_AUD_A_CODEC_G711U) {
			ret = WebRtcG711_DecodeU((uint8_t*)(pData), nLen, (int16_t*)(audio_dec_samples_), &audType);
		}

		if (ret > 0) {
			int nUsed = 0;
			int nAudLen = ret * sizeof(int16_t) * n_aud_channels_;
			//RTC_LOG(LS_NONE) << "DoDecode audio Data seqn: " << nSeqn << " num: " << nAudLen/ n_aud_10ms_len_ << " time: " << rtc::Time32();
			int64_t nAudPts = nPts;
			if (audio_dec_used_ > 0) {
				nAudPts -= audio_dec_used_ * 10 / n_aud_10ms_len_;
			}
			while (nUsed + n_aud_10ms_len_ <= nAudLen)
			{
				if (listener != NULL) {
					listener->OnX2AudDecoderGotFrame(audio_dec_samples_ + nUsed, n_aud_10ms_len_, n_aud_sample_hz_, n_aud_channels_, nAudPts);
				}
				else 
				{
					char* pData = new char[n_aud_10ms_len_];
					memcpy(pData, audio_dec_samples_ + nUsed, n_aud_10ms_len_);
					JMutexAutoLock l(cs_lst_audio_);
					AudData* audData = new AudData();
					audData->pData = pData;
					audData->nLen = n_aud_10ms_len_;
					audData->nPts = nAudPts;
					lst_audio_.push_back(audData);
					if (lst_audio_.size() > 5) {
						delete[] lst_audio_.front();
						lst_audio_.pop_front();
					}

				}
				nAudPts += 10;
				nUsed += n_aud_10ms_len_;
			}
			if (nUsed < nAudLen) {
				audio_dec_used_ = nAudLen - nUsed;
				memmove(audio_dec_samples_, audio_dec_samples_ + nUsed, audio_dec_used_);
			}
			else {
				audio_dec_used_ = 0;
			}
		}

	}
	virtual void DoClear() {
		JMutexAutoLock l(cs_lst_audio_);
		while (lst_audio_.size() > 0) {
			delete lst_audio_.front();
			lst_audio_.pop_front();
		}
	};
	virtual int MixAudPcmData(bool mix, int nVolume, void* audioSamples, uint32_t samplesPerSec, size_t nChannels) {
		AudData* audData = NULL;
		{
			JMutexAutoLock l(cs_lst_audio_);
			if (lst_audio_.size() > 0) {
				audData = lst_audio_.front();
				lst_audio_.pop_front();
			}
		}
		if (audData != NULL) {
			char aud_data_resamp_[kMaxDataSizeSamples];
			int a_frame_size = samplesPerSec * nChannels * sizeof(int16_t) / 100;
			if (samplesPerSec != n_aud_sample_hz_ || n_aud_channels_ != nChannels) {
				resampler_.Resample10Msec((int16_t*)audData->pData, n_aud_sample_hz_ * n_aud_channels_,
					samplesPerSec * nChannels, 1, kMaxDataSizeSamples / sizeof(int16_t), (int16_t*)aud_data_resamp_);
			}
			else {
				memcpy(aud_data_resamp_, (char*)audData->pData, a_frame_size);
			}

			if (!mix) {
				if (nVolume != 100) {
					float voice_gain = 1.0;
					voice_gain = (float)nVolume / 100.0;
					short* pVolUnit = (short*)aud_data_resamp_;
					int nSize = n_aud_sample_hz_ / 100;
					for (int iIndex = 0; iIndex < nSize; iIndex += n_aud_channels_) {
						X2VolAudio(n_aud_channels_, &pVolUnit[iIndex], voice_gain);
					}
				}
				memcpy(audioSamples, aud_data_resamp_, a_frame_size);
			}
			else {
				char aud_data_mix_[kMaxDataSizeSamples];
				int nFrameSize = (samplesPerSec * nChannels / 100);
				float musice_gain = 1.0;
				if (nVolume != 100) {
					musice_gain = (float)nVolume / 100.0;
				}
				float voice_gain = 1.0;
				short* pMusicUnit = (short*)aud_data_resamp_;
				short* pMicUnit = (short*)audioSamples;
				short* pOutputPcm = (short*)aud_data_mix_;
				for (int iIndex = 0; iIndex < nFrameSize; iIndex = iIndex + nChannels) {
					X2MixAudio(nChannels, &pMusicUnit[iIndex], &pMicUnit[iIndex], musice_gain, voice_gain, &pOutputPcm[iIndex]);
				}
				memcpy(audioSamples, aud_data_mix_, a_frame_size);
			}

			delete audData;

			return 1;
		}

		//RTC_LOG(LS_NONE) << "MixAudPcmData no Data time:: " << rtc::Time32();

		return 0;
	};
	virtual int MixAudPcmData(bool mix, void* audioSamples, uint32_t samplesPerSec, size_t nChannels) {
		return MixAudPcmData(mix, 100, audioSamples, samplesPerSec, nChannels);
	};

private:
	webrtc::acm2::ACMResampler resampler_;
	void* dec_inst_;
	//OpusDecInst			*opus_dec_;
	char* audio_dec_samples_;
	int					audio_dec_used_;

	int		n_aud_sample_hz_;
	int		n_aud_channels_;
	int		n_aud_10ms_len_;

	struct AudData
	{
		AudData(void):pData(NULL), nLen(0), nPts(0){};
		virtual ~AudData(void) {
			if (pData != NULL) {
				delete[]pData;
				pData = NULL;
			}
		};
		char* pData;
		int nLen;
		int64_t nPts;
	};

	JMutex cs_lst_audio_;
	std::list<AudData*>lst_audio_;

};

IX2AudDecoder* X2R_CALL createX2AudDecoder()
{
	return new X2AudDecoder();
}

size_t IX2RtcG711_EncodeA(const int16_t* speechIn,
	size_t len,
	uint8_t* encoded)
{
	return WebRtcG711_EncodeA(speechIn, len, encoded);
}
size_t IX2RtcG711_EncodeU(const int16_t* speechIn,
	size_t len,
	uint8_t* encoded)
{
	return WebRtcG711_EncodeU(speechIn, len, encoded);
}
size_t IX2RtcG711_DecodeA(const uint8_t* encoded,
	size_t len,
	int16_t* decoded,
	int16_t* speechType)
{
	return WebRtcG711_DecodeA(encoded,len,decoded,speechType);
}
size_t IX2RtcG711_DecodeU(const uint8_t* encoded,
	size_t len,
	int16_t* decoded,
	int16_t* speechType)
{
	return WebRtcG711_DecodeU(encoded, len, decoded, speechType);
}

}	//namespace x2rtc