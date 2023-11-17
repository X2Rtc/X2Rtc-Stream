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
#ifndef __X2NET_PEER_H__
#define __X2NET_PEER_H__
#include "X2NetConnection.h"
#include "X2RtcEvent.h"
#include "X2ProtoHandler.h"
#include "X2RtcThreadPool.h"
#include "X2MediaDispatch.h"

namespace x2rtc {
	struct StreamStats {
		StreamStats(void) : nAudBytes(0), nVidBytes(0), nLossRate(0), nRtt(0), nAudAllBytes(0), nVidAllBytes(0), nNetAllBytes(0),  nVidKeyFrameInterval(0)
			, nUsedAllTime(0){

		}
		uint32_t nAudBytes;					// Audio bytes peer 1s
		uint32_t nVidBytes;					// Video bytes peer 1s
		uint32_t nLossRate;					// Loss percent.
		uint32_t nRtt;						// Rtt: Round-Trip Time
		uint32_t nAudAllBytes;				// All audio bytes tranpoted.
		uint32_t nVidAllBytes;				// All video bytes tranpoted.
		uint32_t nNetAllBytes;				// All bytes tranpoted.
		uint32_t nVidKeyFrameInterval;		// Video Key frame interval
		uint32_t nUsedAllTime;				// Used all time, it's duration.
	};
	struct StreamInfo
	{
		StreamInfo(void) :eProtoType(X2Proto_Rtmp), eAudCodecType(X2Codec_None), eVidCodecType(X2Codec_None), nConnectedTime(0), nFirstAudInterval(0), nFirstVidInterval(0) {
			
		}
		std::string strUserId;
		std::string strSessionId;
		std::string strRemoteAddr;
		std::string strEventType;
		X2ProtoType eProtoType;
		X2CodecType eAudCodecType;
		X2CodecType eVidCodecType;

		uint32_t nConnectedTime;		// Connected time: between network connected and protocol negotiated.
		uint32_t nFirstAudInterval;		// First Audio frame recved from/sendto client interval
		uint32_t nFirstVidInterval;		// First Video frame recved from/sendto client interval
	};

class X2NetPeer : X2NetConnection::Listener, public X2ProtoHandler::Listener, public X2MediaPuber::Listener, public X2MediaSuber::Listener, public X2RtcTick
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2NetPeerEvent(
			X2NetPeer* x2NetPeer, x2rtc::scoped_refptr< X2RtcEventRef> x2RtcEvent) = 0;
		virtual void OnX2NetPeerClosed(X2NetPeer* x2NetPeer) = 0;

		virtual void OnX2NetPeerStreamStats(bool isPublish, const std::string& strAppId, const std::string& strStreamId, 
			StreamInfo*streamInfo, StreamStats* streamStats) = 0;
	};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

protected:
	Listener* cb_listener_;

public:
	X2NetPeer(void);
	virtual ~X2NetPeer(void);

	/* SetX2NetConnection
	 * Desc: Set net connection.
	 */
	void SetX2NetConnection(X2NetType x2NetType, X2NetConnection* x2NetConn);

	/* Init
	 * Desc: Must init once.
	 */
	void Init();
	/* DeInit
	 * Desc: If u inited, must DeInit when not used. 
	 */
	void DeInit();

	void SetRecvVideoScaleRD(int nScaleRd);
	void SetRecvStatus(bool bRecvAudio, bool bRecvVideo);

	//* For X2NetConnection::Listener
	virtual void OnX2NetConnectionPacketReceived(
		X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
	virtual void OnX2NetConnectionClosed(X2NetConnection* connection);
	virtual void OnX2NetRemoteIpPortChanged(X2NetConnection* connection, struct sockaddr* remoteAddr);
	virtual void OnX2NetGotNetStats(X2NetConnection* connection, bool isAudio, int nRtt, int nLossRate, int nBufSize);

	//* For X2ProtoHandler::Listener
	virtual void OnX2ProtoHandlerPublish(const char* app, const char* stream, const char* token, const char* strParam);
	virtual void OnX2ProtoHandlerPlay(const char* app, const char* stream, const char* token, const char* strParam);
	virtual void OnX2ProtoHandlerPlayStateChanged(bool bPlayed);
	virtual void OnX2ProtoHandlerClose();
	virtual void OnX2ProtoHandlerSetPlayCodecType(X2CodecType eAudCodecType, X2CodecType eVidCodecType);
	virtual void OnX2ProtoHandlerSendData(const uint8_t* pHdr, size_t nHdrLen, const uint8_t* pData, size_t nLen);
	virtual void OnX2ProtoHandlerRecvScript(const uint8_t* data, size_t bytes, int64_t timestamp);
	virtual void OnX2ProtoHandlerRecvCodecData(scoped_refptr<X2CodecDataRef> x2CodecData);

	//* For X2MediaPuber::Listener
	virtual void OnX2MediaPuberClosed(X2MediaPuber* x2Puber);
	//* For X2MediaSuber::Listener
	virtual void OnX2MediaSuberGotData(x2rtc::scoped_refptr<X2CodecDataRef> x2CodecData);

	//* For X2RtcTick
	virtual void OnX2RtcTick();

private:
	struct VideoRecvStatus;
	bool VideoCanSend(int nScaleRD, bool bKeyFrame, VideoRecvStatus* vidRecvStatus);

private:
	X2NetConnection* x2net_connection_;
	X2ProtoHandler* x2proto_handler_;

	bool b_notify_close_by_other_;			// Connection closed by other, may be other's published a same stream.
	bool b_notify_close_from_network_;		// Connection closed by network, may be some network issues occured.
	bool b_notify_close_from_proto_;		// Connection closed by protocol, may be client closed itself.
	bool b_self_closed_;					// Closed myself
	bool b_set_params_;						// Has been set the parameters.

	std::string str_params_;				// Parameters.

private:
	JMutex cs_list_recv_x2net_data_;
	std::list<x2rtc::scoped_refptr<X2NetDataRef>> list_recv_x2net_data_;

	JMutex cs_list_recv_sub_data_;
	std::list<x2rtc::scoped_refptr<X2CodecDataRef>> list_recv_sub_data_;

private:
	std::string str_app_id_;
	std::string str_stream_id_;
	X2MediaPuber *x2media_puber_;
	X2MediaSuber* x2media_suber_;

	int64_t n_next_try_publish_time_;
	int64_t n_last_recv_keyframe_time_;
	int64_t n_send_data_min_gap_time_;
	int64_t n_peer_created_time_;
	bool b_is_publish_;			// Publish or Play stream
	bool b_play_self_ctrl_;		// 

	int64_t n_next_stats_time_;
	StreamStats stream_stats_;

	StreamInfo stream_info_;


private:
	struct VideoRecvStatus {
		VideoRecvStatus(void) {
			b_remote_recv_audio_ = true;
			b_remote_recv_video_ = true;

			b_need_switch_stream_ = false;
			b_cur_sub_stream_ = false;
			b_tmp_need_main_keyframe_ = true;
			b_tmp_need_sub_keyframe_ = true;

			n_last_scale_rd_ok_ = 1;
			n_req_scale_rd_ = 1;
		}
		bool					b_remote_recv_audio_;
		bool					b_remote_recv_video_;

		bool					b_need_switch_stream_;		// 切换视频流(切换过程中，需要等到切换的KeyFrame到来才成功)
		bool					b_cur_sub_stream_;			// 当前正在接收哪路视频流(防止大小流快速切换，都没有来关键帧，非关键帧混合发送花屏)
		bool					b_tmp_need_main_keyframe_;	// Default: true
		bool					b_tmp_need_sub_keyframe_;	// Default: true
		int						n_last_scale_rd_ok_;		// 切换到对应的流
		int						n_req_scale_rd_;
	};

	VideoRecvStatus vid_recv_status_;
};

}	// namespace x2rtc
#endif	// __X2NET_PEER_H__