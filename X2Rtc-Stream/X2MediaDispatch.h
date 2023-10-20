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
#ifndef __X2_MEDIA_DISPATCH_H__
#define __X2_MEDIA_DISPATCH_H__
#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include "X2CodecType.h"
#include "jmutex.h"
#include "X2RtcThreadPool.h"
#include "X2RtcAudCodec.h"
#include "X2ProtoHandler.h"
#include "X2MediaRtn.h"

namespace x2rtc {

class X2MediaSuber
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2MediaSuberGotData(x2rtc::scoped_refptr<X2CodecDataRef> x2CodecData) = 0;
	};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};
protected:
	Listener* cb_listener_;
public:
	X2MediaSuber(void):e_proto_type_(X2Proto_Rtmp), b_cache_gop_(false){};
	virtual ~X2MediaSuber(void) {};

	void RecvCodecData(x2rtc::scoped_refptr<X2CodecDataRef>x2CodecData);

	void SetProto(X2ProtoType eType) {
		e_proto_type_ = eType;
	}
	X2ProtoType GetProto() {
		return e_proto_type_;
	}

	void SetCacheGop(bool bCacheGop) {
		b_cache_gop_ = bCacheGop;
	}

	bool GetCacheGop() {
		return b_cache_gop_;
	}

private:
	X2ProtoType e_proto_type_;
	bool b_cache_gop_;			
};

struct MediaSuber {
	MediaSuber(void) : eAudCodecType(X2Codec_None), eVidCodecType(X2Codec_None), bCacheFrameSended(false), bVideoStreamSubOK(false), bRealTimeStream(false), x2MediaSuber(NULL) {
	}
	X2CodecType eAudCodecType;		// If it is none, don't need transcode.
	X2CodecType eVidCodecType;
	bool bCacheFrameSended;
	bool bVideoStreamSubOK;
	bool bRealTimeStream;		// RealTime stream like WebRtc, just need one IFrame 
	X2MediaSuber* x2MediaSuber;
};
typedef std::map<void*, MediaSuber>MapMediaSuber;
struct StreamMediaSuber : public X2MediaRtnSub {
	StreamMediaSuber(void):nAudSubCodecChanged(0){};
	virtual ~StreamMediaSuber(void) {};

	std::string strAppId;
	std::string strStreamId;
	std::string strEventType;

	JMutex csMapAudSubCodec;
	uint32_t nAudSubCodecChanged;
	std::map<X2CodecType, int >mapAudSubCodec;	//Audio support transcode, it's useful for MediaPuber to know how many subscribed which audio codec type.
	JMutex csMapMediaSuber;
	MapMediaSuber mapMediaSuber;
};

class X2MediaPuber : public X2MediaRtnPub, public X2RtcTick, public IX2AudDecoder::Listener
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2MediaPuberClosed(X2MediaPuber*x2Puber) = 0;
	};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};
protected:
	Listener* cb_listener_;
public:
	X2MediaPuber(void);
	virtual ~X2MediaPuber(void);

	void SetEventType(const std::string& strEventType) {
		str_event_type_ = strEventType;
	}
	const std::string& GetEventType() {
		return str_event_type_;
	}
	void ForceClose();

	//* For X2MediaRtn
	virtual void OnX2GtRtnPublishOK();
	virtual void OnX2GtRtnPublishForceClosed(int nCode);

	//* For X2RtcTick
	virtual void OnX2RtcTick();

	void RecvCodecData(x2rtc::scoped_refptr<X2CodecDataRef>x2CodecData);
	x2rtc::scoped_refptr<X2CodecDataRef>GetCodecData();

	void SetStreamMediaSuber(StreamMediaSuber* streamMediaSuber);

	//* For IX2AudDecoder::Listener
	virtual void OnX2AudDecoderGotFrame(void* audioSamples, int nLen, uint32_t samplesPerSec, size_t nChannels, int64_t nDts);

private:
	JMutex cs_list_recv_data_;
	std::list<x2rtc::scoped_refptr<X2CodecDataRef>> list_recv_data_;
	x2rtc::scoped_refptr<X2CodecDataRef> video_keyframe_cache_;

private:
	std::list<x2rtc::scoped_refptr<X2CodecDataRef>> list_cache_data_;

	uint32_t n_max_cache_time_;
	int64_t n_last_recv_keyframe_time_;	// If long time to no receive keyframe, we need to force close the connection.

	std::string str_event_type_;

private:
	JMutex cs_stream_media_suber_;
	StreamMediaSuber* stream_media_suber_;
	uint32_t n_aud_sub_codec_changed_;

private:
	IX2AudDecoder* x2aud_decoder_;
	struct AudEncoder
	{
		AudEncoder() : audEncoder(NULL), nSeqn(0){}
		IX2AudEncoder* audEncoder;
		uint16_t nSeqn;
	};
	std::map<X2CodecType, AudEncoder>mapAudEncoder;
};

class X2MediaDispatch
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2MediaDispatchPublishOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType) = 0;
		virtual void OnX2MediaDispatchPublishClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType) = 0;
		virtual void OnX2MediaDispatchSubscribeOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType) = 0;
		virtual void OnX2MediaDispatchSubscribeClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType) = 0;
	};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

protected:
	Listener* cb_listener_{ nullptr };
public:
	X2MediaDispatch(void);
	virtual ~X2MediaDispatch(void);
	
	static X2MediaDispatch& Inst() {
		static X2MediaDispatch gMgr;
		return gMgr;
	}

	void RunOnce();

	bool TryPublish(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType, X2MediaPuber* x2MediaPuber);
	void UnPublish(const std::string& strAppId, const std::string& strStreamId);

	void Subscribe(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType, X2MediaSuber* x2MediaSuber);
	void UnSubscribe(const std::string& strAppId, const std::string& strStreamId, X2MediaSuber* x2MediaSuber);
	void ResetSubCodecType(const std::string& strAppId, const std::string& strStreamId, X2MediaSuber* x2MediaSuber, X2CodecType eAudCodecType, X2CodecType eVidCodecType);

	void StatsSubscribe(std::string&strSubStats);
public:
	//Publish
	typedef std::map<std::string, X2MediaPuber*>MapX2MediaPuber;
	typedef std::map<std::string, MapX2MediaPuber>MapAppX2MediaPuber;

	JMutex cs_map_app_x2media_puber_;
	MapAppX2MediaPuber map_app_x2media_puber_;

private:
	//Subscribe
	typedef std::map<std::string/*streamId*/, StreamMediaSuber*>MapStreamMediaSuber;
	typedef std::map<std::string/*appId*/, MapStreamMediaSuber>MapAppStreamMediaSuber;
	
	JMutex cs_map_app_stream_media_suber_;
	MapAppStreamMediaSuber map_app_stream_media_suber_;

	JMutex cs_list_stream_media_suber_delete_;
	std::list< StreamMediaSuber*> list_stream_media_suber_delete_;
};

}	// namespace x2rtc

#endif	// __X2_MEDIA_DISPATCH_H__