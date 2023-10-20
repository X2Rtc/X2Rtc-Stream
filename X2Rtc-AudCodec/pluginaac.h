//pluginaac.h
#ifndef __PLUGIN_AAC_H__
#define __PLUGIN_AAC_H__

#include "pluginaac_export.h"

// the AAC Encoder handler.
typedef void* aac_enc_t;
PLUGIN_AAC_API aac_enc_t aac_encoder_open(unsigned char ucAudioChannel, unsigned int u32AudioSamplerate, unsigned int u32PCMBitSize, int nBitrate, bool mp4);
PLUGIN_AAC_API void aac_encoder_close(void*pHandle);
PLUGIN_AAC_API int aac_encoder_encode_frame(void*pHandle, unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen);

// the AAC Decoder handler.
typedef void* aac_dec_t;
PLUGIN_AAC_API aac_dec_t aac_decoder_open(unsigned char* adts, unsigned int len, unsigned char* outChannels, unsigned int* outSampleHz);
PLUGIN_AAC_API void aac_decoder_close(void*pHandle);
PLUGIN_AAC_API int aac_decoder_decode_frame(void*pHandle, unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen);

#endif	// __PLUGIN_AAC_H__