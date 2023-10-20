#include "X2MediaDispatch.h"
#include "jmutexautolock.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"

bool gConfSendMuteDataWhenNoPub = false;	// Audio & Video
bool gConfSendMuteAudWhenPubNoAudio = false;

namespace x2rtc {

X2MediaPuber::X2MediaPuber(void) 
	: cb_listener_(NULL)
	, stream_media_suber_(NULL)
	, n_aud_sub_codec_changed_(0)
	, n_max_cache_time_(15000)
	, n_last_recv_keyframe_time_(0)
	, x2aud_decoder_(NULL)
{
	X2RtcThreadPool::Inst().RegisteX2RtcTick(this, this);
}
X2MediaPuber::~X2MediaPuber(void)
{
	X2RtcThreadPool::Inst().UnRegisteX2RtcTick(this);
	RtnUnPublish();

	if (x2aud_decoder_ != NULL) {
		x2aud_decoder_->DeInit();
		delete x2aud_decoder_;
		x2aud_decoder_ = NULL;
	}
	std::map<X2CodecType, AudEncoder>::iterator itaer = mapAudEncoder.begin();
	while(itaer != mapAudEncoder.end()) {
		itaer->second.audEncoder->DeInit();
		delete itaer->second.audEncoder;
		itaer = mapAudEncoder.erase(itaer);
	}
}
void X2MediaPuber::ForceClose()
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2MediaPuberClosed(this);
	}
}
void X2MediaPuber::OnX2GtRtnPublishOK()
{

}
void X2MediaPuber::OnX2GtRtnPublishForceClosed(int nCode)
{
	ForceClose();
}
//* For X2RtcTick
void X2MediaPuber::OnX2RtcTick()
{
	if (n_last_recv_keyframe_time_ != 0 && n_last_recv_keyframe_time_ + n_max_cache_time_ <= XGetUtcTimestamp()) {
		n_last_recv_keyframe_time_ = 0;
		video_keyframe_cache_ = NULL;
		list_cache_data_.clear();
	}

	if (n_last_recv_keyframe_time_ == 0) {
		while (1) {
			if (list_cache_data_.size() > 0) {
				if (list_cache_data_.back()->nDataRecvTime - list_cache_data_.front()->nDataRecvTime > n_max_cache_time_) {
					list_cache_data_.pop_front();
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}
	}


	while (1) {
		x2rtc::scoped_refptr<X2CodecDataRef> x2CodecData = GetCodecData();
		if (x2CodecData != NULL) {
			//printf("Recv data type: %d pts: %lld dts:%lld\r\n", x2CodecData->bIsAudio, x2CodecData->lPts, x2CodecData->lDts);
			if (!x2CodecData->bIsAudio && x2CodecData->bKeyframe) {
				if (x2CodecData->nVidScaleRD == 1) {
					list_cache_data_.clear();
					n_last_recv_keyframe_time_ = XGetUtcTimestamp();
					printf("Recv key frame time: %lld pts: %lld len: %d\r\n", n_last_recv_keyframe_time_, x2CodecData->lPts, x2CodecData->nLen);
				}
			}
			
			{
				JMutexAutoLock l(cs_stream_media_suber_);
				if (stream_media_suber_ != NULL) {
					if(x2CodecData->bIsAudio)
					{
						JMutexAutoLock j(stream_media_suber_->csMapAudSubCodec);
						if (n_aud_sub_codec_changed_ != stream_media_suber_->nAudSubCodecChanged) {
							n_aud_sub_codec_changed_ = stream_media_suber_->nAudSubCodecChanged;
							std::map<X2CodecType, int >::iterator itavr = stream_media_suber_->mapAudSubCodec.begin();
							while (itavr != stream_media_suber_->mapAudSubCodec.end()) {
								if (itavr->first != x2CodecData->eCodecType) {
									if (mapAudEncoder.find(itavr->first) == mapAudEncoder.end()) {
										switch (itavr->first) {
										case X2Codec_AAC: {
											mapAudEncoder[X2Codec_AAC].audEncoder = createX2AudEncoder();
											mapAudEncoder[X2Codec_AAC].audEncoder->Init(X2_AUD_A_CODEC_AAC, 44100, 2, 64000, true);
										}break;
										case X2Codec_OPUS: {
											mapAudEncoder[X2Codec_OPUS].audEncoder = createX2AudEncoder();
											mapAudEncoder[X2Codec_OPUS].audEncoder->Init(X2_AUD_A_CODEC_OPUS, 48000, 2, 48000, true);
										}break;
										case X2Codec_G711A: {
											mapAudEncoder[X2Codec_G711A].audEncoder = createX2AudEncoder();
											mapAudEncoder[X2Codec_G711A].audEncoder->Init(X2_AUD_A_CODEC_G711A, 8000, 1, 0, true);
										}break;
										case X2Codec_G711U: {
											mapAudEncoder[X2Codec_G711U].audEncoder = createX2AudEncoder();
											mapAudEncoder[X2Codec_G711U].audEncoder->Init(X2_AUD_A_CODEC_G711U, 8000, 1, 0, true);
										}break;
										}
									}
								}
								itavr++;
							}

							std::map<X2CodecType, AudEncoder>::iterator itaer = mapAudEncoder.begin();
							while (itaer != mapAudEncoder.end()) {
								if (stream_media_suber_->mapAudSubCodec.find(itaer->first) == stream_media_suber_->mapAudSubCodec.end()) {
									itaer->second.audEncoder->DeInit();
									delete itaer->second.audEncoder;
									itaer = mapAudEncoder.erase(itaer);
								}
								else {
									itaer++;
								}
							}
						}
					}
					JMutexAutoLock k(stream_media_suber_->csMapMediaSuber);
					MapMediaSuber::iterator itsr = stream_media_suber_->mapMediaSuber.begin();
					while (itsr != stream_media_suber_->mapMediaSuber.end()) {
						MediaSuber& mediaSuber = itsr->second;
						if (!x2CodecData->bIsAudio && !mediaSuber.bVideoStreamSubOK) {
							if (x2CodecData->nVidScaleRD == 1) {
								if (x2CodecData->bKeyframe) {
									mediaSuber.bVideoStreamSubOK = true;
									mediaSuber.bCacheFrameSended = true;
								}
							}
						}
						if (!mediaSuber.bVideoStreamSubOK) {
							if (mediaSuber.bRealTimeStream) {
								if (video_keyframe_cache_ != NULL) {
									if (!mediaSuber.bCacheFrameSended) {
										mediaSuber.bCacheFrameSended = true;
										itsr->second.x2MediaSuber->RecvCodecData(video_keyframe_cache_);
									}
								}
								if (x2CodecData->bIsAudio) {
									if (itsr->second.eAudCodecType == x2CodecData->eCodecType) {
										itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
									}
								}
							}
							else {
								if (!mediaSuber.bCacheFrameSended) {
									mediaSuber.bCacheFrameSended = true;
									std::list<x2rtc::scoped_refptr<X2CodecDataRef>>::iterator itdvr = list_cache_data_.begin();
									while (1) {
										if (itdvr != list_cache_data_.end()) {
											x2rtc::scoped_refptr<X2CodecDataRef> x2CacheData = *itdvr;
											if (x2CacheData->bIsAudio) {
												if (itsr->second.eAudCodecType == x2CacheData->eCodecType) {
													itsr->second.x2MediaSuber->RecvCodecData(x2CacheData);
												}
											}
											else {
												itsr->second.x2MediaSuber->RecvCodecData(x2CacheData);
											}
											itdvr++;
										}

										if (itdvr == list_cache_data_.end()) {
											break;
										}
									}
								}
								mediaSuber.bVideoStreamSubOK = true;
								if (x2CodecData->bIsAudio) {
									if (itsr->second.eAudCodecType == x2CodecData->eCodecType) {
										itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
									}
								}
								else {
									itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
								}
							}
							itsr++;
							continue;
						}

						if (x2CodecData->bIsAudio) {
							if (itsr->second.eAudCodecType == x2CodecData->eCodecType) {
								itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
							}
						}
						else {
							itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
						}
						itsr++;
					}
				} else {
					std::map<X2CodecType, AudEncoder>::iterator itaer = mapAudEncoder.begin();
					while (itaer != mapAudEncoder.end()) {
						itaer->second.audEncoder->DeInit();
						delete itaer->second.audEncoder;
						itaer = mapAudEncoder.erase(itaer);
					}
				}
			}

			if (n_last_recv_keyframe_time_ == 0) {
				if (!x2CodecData->bIsAudio) {
					return;
				}
			}

			if (x2CodecData->bIsAudio) {
				//printf("recv aud len: %d pts: %lld time: %u \r\n", x2CodecData->nLen, x2CodecData->lPts, XGetTimestamp());
				if (x2aud_decoder_ == NULL) {
					if (x2CodecData->eCodecType == X2Codec_OPUS) {
						x2aud_decoder_ = createX2AudDecoder();
						x2aud_decoder_->Init(X2_AUD_A_CODEC_OPUS, 48000, 2, 0);
					}
					else if (x2CodecData->eCodecType == X2Codec_AAC) {
						x2aud_decoder_ = createX2AudDecoder();
						x2aud_decoder_->Init(X2_AUD_A_CODEC_AAC, 44100, 2, 0);
					}
					else if (x2CodecData->eCodecType == X2Codec_G711A) {
						x2aud_decoder_ = createX2AudDecoder();
						x2aud_decoder_->Init(X2_AUD_A_CODEC_G711A, 8000, 1, 0);
					}
					else if (x2CodecData->eCodecType == X2Codec_G711U) {
						x2aud_decoder_ = createX2AudDecoder();
						x2aud_decoder_->Init(X2_AUD_A_CODEC_G711U, 8000, 1, 0);
					}
				}
				if (x2aud_decoder_ != NULL) {
					x2aud_decoder_->DoDecode2((char*)x2CodecData->pData, x2CodecData->nLen, x2CodecData->nAudSeqn, x2CodecData->lDts, this);
				}
			}

			if (x2CodecData->nVidScaleRD == 1) {//* Just cache raw video
				list_cache_data_.push_back(x2CodecData);
			}
		}
		else {
			break;
		}
	}
}
void X2MediaPuber::RecvCodecData(x2rtc::scoped_refptr<X2CodecDataRef>x2CodecData)
{
	x2CodecData->nDataRecvTime = XGetUtcTimestamp();
	{
		JMutexAutoLock l(cs_list_recv_data_);
		list_recv_data_.push_back(x2CodecData);
		if (!x2CodecData->bIsAudio && x2CodecData->bKeyframe) {
			if (x2CodecData->nVidScaleRD == 1) {
				video_keyframe_cache_ = x2CodecData;
			}
		}
	}
}
x2rtc::scoped_refptr<X2CodecDataRef>X2MediaPuber::GetCodecData()
{
	x2rtc::scoped_refptr<X2CodecDataRef>x2CodecData = NULL;
	{
		JMutexAutoLock l(cs_list_recv_data_);
		if (list_recv_data_.size() > 0) {
			x2CodecData = list_recv_data_.front();
			list_recv_data_.pop_front();
		}
	}
	return x2CodecData;
}

void X2MediaPuber::SetStreamMediaSuber(StreamMediaSuber* streamMediaSuber)
{
	JMutexAutoLock l(cs_stream_media_suber_);
	stream_media_suber_ = streamMediaSuber;
	printf("SetStreamMediaSuber this(%p): %p \r\n", this, streamMediaSuber == NULL ? 0x0 : streamMediaSuber);
	if (stream_media_suber_ == NULL) {
		n_aud_sub_codec_changed_ = 0;
	}
}

//* For IX2AudDecoder::Listener
void X2MediaPuber::OnX2AudDecoderGotFrame(void* audioSamples, int nLen, uint32_t samplesPerSec, size_t nChannels, int64_t nDts)
{
	//printf("OnX2AudDecoderGotFrame len: %d sampleHz: %d, channels: %d dts: %lld\r\n", nLen, samplesPerSec, nChannels, nDts);
	if (mapAudEncoder.size() > 0) {
		int16_t pPcmData[1920];
		uint8_t pEncData[1000];
		std::map<X2CodecType, AudEncoder>::iterator itaer = mapAudEncoder.begin();
		while (itaer != mapAudEncoder.end()) {
			AudEncoder& x2AudEncoder = itaer->second;
			int nResampleLen = x2AudEncoder.audEncoder->ReSample10Ms(audioSamples, nChannels, samplesPerSec, pPcmData);
			if (nResampleLen > 0) {
				int ret = x2AudEncoder.audEncoder->DoEncode((char*)pPcmData, nResampleLen, 1000, pEncData);
				if (ret > 0) {
					//printf("AAc encode len: %d time: %u pts: %lld\r\n", ret, XGetTimestamp(), nPts);
					scoped_refptr<X2CodecDataRef> x2CodecData = new x2rtc::RefCountedObject<X2CodecDataRef>();
					x2CodecData->SetData(pEncData, ret);
					x2CodecData->lDts = nDts;
					x2CodecData->lPts = nDts;
					x2CodecData->eCodecType = itaer->first;
					x2CodecData->bIsAudio = true;
					x2CodecData->nAudSeqn = x2AudEncoder.nSeqn++;

					{
						JMutexAutoLock l(cs_stream_media_suber_);
						if (stream_media_suber_ != NULL) {
							JMutexAutoLock k(stream_media_suber_->csMapMediaSuber);
							MapMediaSuber::iterator itsr = stream_media_suber_->mapMediaSuber.begin();
							while (itsr != stream_media_suber_->mapMediaSuber.end()) {
								MediaSuber& mediaSuber = itsr->second;
								if (itsr->second.eAudCodecType == x2CodecData->eCodecType) {
									itsr->second.x2MediaSuber->RecvCodecData(x2CodecData);
								}
		
								itsr++;
							}
						}
					}

					list_cache_data_.push_back(x2CodecData);
				}
			}
			itaer++;
		}
	}
}

void X2MediaSuber::RecvCodecData(x2rtc::scoped_refptr<X2CodecDataRef>x2CodecData)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2MediaSuberGotData(x2CodecData);
	}
}


X2MediaDispatch::X2MediaDispatch(void) 
{

}
X2MediaDispatch::~X2MediaDispatch(void)
{

}

void X2MediaDispatch::RunOnce()
{
	while (1) {
		StreamMediaSuber* streamMSuber = NULL;
		{
			JMutexAutoLock l(cs_list_stream_media_suber_delete_);
			if (list_stream_media_suber_delete_.size() > 0) {
				streamMSuber = list_stream_media_suber_delete_.front();
				list_stream_media_suber_delete_.pop_front();
			}
		}

		if (streamMSuber != NULL) {
			delete streamMSuber;
			streamMSuber = NULL;
		}
		else {
			break;
		}
	}
	
}

bool X2MediaDispatch::TryPublish(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType, X2MediaPuber* x2MediaPuber)
{
	bool bPubOk = false;
	{
		JMutexAutoLock l(cs_map_app_x2media_puber_);
		if (map_app_x2media_puber_.find(strAppId) != map_app_x2media_puber_.end()) {
			MapX2MediaPuber& mapX2MediaPuber = map_app_x2media_puber_[strAppId];
			if (mapX2MediaPuber.find(strStreamId) != mapX2MediaPuber.end()) {
				mapX2MediaPuber[strStreamId]->ForceClose();
				return false;
			}
		}

		{
			if (map_app_x2media_puber_.find(strAppId) == map_app_x2media_puber_.end()) {
				map_app_x2media_puber_[strAppId];
			}
			MapX2MediaPuber& mapX2MediaPuber = map_app_x2media_puber_[strAppId];
			if (mapX2MediaPuber.find(strStreamId) == mapX2MediaPuber.end()) {
				mapX2MediaPuber[strStreamId] = x2MediaPuber;
				bPubOk = true;
				x2MediaPuber->SetEventType(strEventType);
				x2MediaPuber->RtnPublish(strAppId.c_str(), strStreamId.c_str());
			}
		}
	}

	if (bPubOk) {
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2MediaDispatchPublishOpen(strAppId, strStreamId, strEventType);
		}

		JMutexAutoLock l(cs_map_app_stream_media_suber_);
		if (map_app_stream_media_suber_.find(strAppId) != map_app_stream_media_suber_.end()) {
			MapStreamMediaSuber& mapStreamMediaSuber = map_app_stream_media_suber_[strAppId];
			if (mapStreamMediaSuber.find(strStreamId) != mapStreamMediaSuber.end()) {
				StreamMediaSuber* streamMediaSuber = mapStreamMediaSuber[strStreamId];
				x2MediaPuber->SetStreamMediaSuber(streamMediaSuber);
			}
		}
	}
	else {
		X2RTC_CHECK(false);
	}

	return true;
}

void X2MediaDispatch::UnPublish(const std::string& strAppId, const std::string& strStreamId)
{
	bool bUnPub = false;
	std::string strEventType;
	{
		JMutexAutoLock l(cs_map_app_x2media_puber_);
		if (map_app_x2media_puber_.find(strAppId) != map_app_x2media_puber_.end()) {
			MapX2MediaPuber& mapX2MediaPuber = map_app_x2media_puber_[strAppId];
			if (mapX2MediaPuber.find(strStreamId) != mapX2MediaPuber.end()) {
				bUnPub = true;
				strEventType = mapX2MediaPuber[strStreamId]->GetEventType();
				mapX2MediaPuber[strStreamId]->SetStreamMediaSuber(NULL);
				mapX2MediaPuber[strStreamId]->RtnUnPublish();	//同时取消远程发布
				mapX2MediaPuber.erase(strStreamId);

				if (mapX2MediaPuber.size() == 0) {
					map_app_x2media_puber_.erase(strAppId);
				}
			}
		}
	}
	if (bUnPub) {
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2MediaDispatchPublishClose(strAppId, strStreamId, strEventType);
		}
	}
}

void X2MediaDispatch::Subscribe(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType, X2MediaSuber* x2MediaSuber)
{
	StreamMediaSuber* streamMediaSuberNew = NULL;
	{
		JMutexAutoLock l(cs_map_app_stream_media_suber_);
		if (map_app_stream_media_suber_.find(strAppId) == map_app_stream_media_suber_.end()) {
			map_app_stream_media_suber_[strAppId];
		}
		MapStreamMediaSuber& mapStreamMediaSuber = map_app_stream_media_suber_[strAppId];
		if (mapStreamMediaSuber.find(strStreamId) == mapStreamMediaSuber.end()) {
			mapStreamMediaSuber[strStreamId];
			StreamMediaSuber* streamMediaSuber = new StreamMediaSuber();
			streamMediaSuber->strAppId = strAppId;
			streamMediaSuber->strStreamId = strStreamId;
			streamMediaSuber->strEventType = strEventType;
			mapStreamMediaSuber[strStreamId] = streamMediaSuber;
			streamMediaSuberNew = streamMediaSuber;

			if (cb_listener_ != NULL) {
				cb_listener_->OnX2MediaDispatchSubscribeOpen(strAppId, strStreamId, strEventType);
			}
		}
		StreamMediaSuber* streamMediaSuber = mapStreamMediaSuber[strStreamId];
		JMutexAutoLock k(streamMediaSuber->csMapMediaSuber);
		if (streamMediaSuber->mapMediaSuber.find(x2MediaSuber) == streamMediaSuber->mapMediaSuber.end()) {
			MediaSuber& mediaSuber = streamMediaSuber->mapMediaSuber[x2MediaSuber];
			mediaSuber.bRealTimeStream = !x2MediaSuber->GetCacheGop();
			mediaSuber.x2MediaSuber = x2MediaSuber;
		}
	}

	if (streamMediaSuberNew != NULL) {
		JMutexAutoLock l(cs_map_app_x2media_puber_);
		if (map_app_x2media_puber_.find(strAppId) != map_app_x2media_puber_.end()) {
			MapX2MediaPuber& mapX2MediaPuber = map_app_x2media_puber_[strAppId];
			if (mapX2MediaPuber.find(strStreamId) != mapX2MediaPuber.end()) {
				mapX2MediaPuber[strStreamId]->SetStreamMediaSuber(streamMediaSuberNew);
			}
		}
	}
}
void X2MediaDispatch::UnSubscribe(const std::string& strAppId, const std::string& strStreamId, X2MediaSuber* x2MediaSuber)
{
	StreamMediaSuber* streamMediaSuberDelete = NULL;
	{
		JMutexAutoLock l(cs_map_app_stream_media_suber_);
		if (map_app_stream_media_suber_.find(strAppId) != map_app_stream_media_suber_.end()) {
			MapStreamMediaSuber& mapStreamMediaSuber = map_app_stream_media_suber_[strAppId];
			if (mapStreamMediaSuber.find(strStreamId) != mapStreamMediaSuber.end()) {
				StreamMediaSuber* streamMediaSuber = mapStreamMediaSuber[strStreamId];
				JMutexAutoLock k(streamMediaSuber->csMapMediaSuber);
				if (streamMediaSuber->mapMediaSuber.find(x2MediaSuber) != streamMediaSuber->mapMediaSuber.end()) {
					MediaSuber& mediaSuber = streamMediaSuber->mapMediaSuber[x2MediaSuber];
					if (mediaSuber.eAudCodecType != X2Codec_None) {
						JMutexAutoLock j(streamMediaSuber->csMapAudSubCodec);
						if (streamMediaSuber->mapAudSubCodec.find(mediaSuber.eAudCodecType) != streamMediaSuber->mapAudSubCodec.end()) {
							streamMediaSuber->mapAudSubCodec[mediaSuber.eAudCodecType]--;
							if (streamMediaSuber->mapAudSubCodec[mediaSuber.eAudCodecType] == 0) {
								streamMediaSuber->mapAudSubCodec.erase(mediaSuber.eAudCodecType);
								streamMediaSuber->nAudSubCodecChanged++;
							}
						}
					}

					streamMediaSuber->mapMediaSuber.erase(x2MediaSuber);
					if (streamMediaSuber->mapMediaSuber.size() == 0) {
						streamMediaSuberDelete = streamMediaSuber;
						mapStreamMediaSuber.erase(strStreamId);
						if (mapStreamMediaSuber.size() == 0) {
							map_app_stream_media_suber_.erase(strAppId);
						}

						if (cb_listener_ != NULL) {
							cb_listener_->OnX2MediaDispatchSubscribeClose(strAppId, strStreamId, streamMediaSuber->strEventType);
						}
					}
				}
			}
		}
	}

	if (streamMediaSuberDelete != NULL) {
		{
			JMutexAutoLock l(cs_map_app_x2media_puber_);
			if (map_app_x2media_puber_.find(strAppId) != map_app_x2media_puber_.end()) {
				MapX2MediaPuber& mapX2MediaPuber = map_app_x2media_puber_[strAppId];
				if (mapX2MediaPuber.find(strStreamId) != mapX2MediaPuber.end()) {
					mapX2MediaPuber[strStreamId]->SetStreamMediaSuber(NULL);
				}
			}
		}

		{
			JMutexAutoLock j(cs_list_stream_media_suber_delete_);
			list_stream_media_suber_delete_.push_back(streamMediaSuberDelete);
		}
	}
}
void X2MediaDispatch::ResetSubCodecType(const std::string& strAppId, const std::string& strStreamId, X2MediaSuber* x2MediaSuber, X2CodecType eAudCodecType, X2CodecType eVidCodecType)
{
	JMutexAutoLock l(cs_map_app_stream_media_suber_);
	if (map_app_stream_media_suber_.find(strAppId) != map_app_stream_media_suber_.end()) {
		MapStreamMediaSuber& mapStreamMediaSuber = map_app_stream_media_suber_[strAppId];
		if (mapStreamMediaSuber.find(strStreamId) != mapStreamMediaSuber.end()) {
			StreamMediaSuber* streamMediaSuber = mapStreamMediaSuber[strStreamId];
			JMutexAutoLock k(streamMediaSuber->csMapMediaSuber);
			if (streamMediaSuber->mapMediaSuber.find(x2MediaSuber) != streamMediaSuber->mapMediaSuber.end()) {
				MediaSuber& mediaSuber = streamMediaSuber->mapMediaSuber[x2MediaSuber];
				mediaSuber.eAudCodecType = eAudCodecType;
				mediaSuber.eVidCodecType = eVidCodecType;
				if (mediaSuber.eAudCodecType != X2Codec_None) {
					JMutexAutoLock j(streamMediaSuber->csMapAudSubCodec);
					if (streamMediaSuber->mapAudSubCodec.find(eAudCodecType) == streamMediaSuber->mapAudSubCodec.end()) {
						streamMediaSuber->mapAudSubCodec[eAudCodecType] = 0;
						streamMediaSuber->nAudSubCodecChanged++;
					}
					streamMediaSuber->mapAudSubCodec[eAudCodecType]++;
				}
			}
		}
	}
}

void X2MediaDispatch::StatsSubscribe(std::string& strSubStats)
{
	rapidjson::Document		jsonAppSubDoc;
	rapidjson::StringBuffer jsonAppSubStr;
	rapidjson::Writer<rapidjson::StringBuffer> jsonChanUserWriter(jsonAppSubStr);
	jsonAppSubDoc.SetObject();
	rapidjson::Value jarrAppSubs(rapidjson::kArrayType);

	{
		JMutexAutoLock l(cs_map_app_stream_media_suber_);
		MapAppStreamMediaSuber::iterator itar = map_app_stream_media_suber_.begin();
		while (itar != map_app_stream_media_suber_.end()) {
			rapidjson::Value jarrSubs(rapidjson::kArrayType);
			MapStreamMediaSuber& mapStreamMediaSuber = itar->second;
			MapStreamMediaSuber::iterator itsr = mapStreamMediaSuber.begin();
			while (itsr != mapStreamMediaSuber.end()) {
				jarrSubs.PushBack(itsr->first.c_str(), jsonAppSubDoc.GetAllocator());
				itsr++;
			}
			if (jarrSubs.Size() > 0) {
				rapidjson::Value jAppSub(rapidjson::kObjectType);
				jAppSub.AddMember("AppId", itar->first.c_str(), jsonAppSubDoc.GetAllocator());
				jAppSub.AddMember("Subscribe", jarrSubs, jsonAppSubDoc.GetAllocator());

				jarrAppSubs.PushBack(jAppSub, jsonAppSubDoc.GetAllocator());
			}
			itar++;
		}
	}

	if (jarrAppSubs.Size() > 0) {
		jsonAppSubDoc.AddMember("List", jarrAppSubs, jsonAppSubDoc.GetAllocator());
		jsonAppSubDoc.Accept(jsonChanUserWriter);
		strSubStats = jsonAppSubStr.GetString();
	}
}

}	// namespace x2rtc