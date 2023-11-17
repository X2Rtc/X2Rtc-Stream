#include "X2NetPeer.h"
#include "X2RtcLog.h"
#include "X2RtcCheck.h"

namespace x2rtc {

X2NetPeer::X2NetPeer(void)
	: cb_listener_(NULL)
	, x2net_connection_(NULL)
	, x2proto_handler_(NULL)
	, b_notify_close_by_other_(false)
	, b_notify_close_from_network_(false)
	, b_notify_close_from_proto_(false)
	, b_self_closed_(false)
	, b_set_params_(false)
	, x2media_puber_(NULL)
	, x2media_suber_(NULL)
	, n_next_try_publish_time_(0)
	, n_last_recv_keyframe_time_(0)
	, n_send_data_min_gap_time_(0)
	, b_is_publish_(false)
	, b_play_self_ctrl_(false)
	, n_next_stats_time_(0)
{
	n_peer_created_time_ = XGetUtcTimestamp();
}
X2NetPeer::~X2NetPeer(void)
{
	X2RTC_CHECK(x2net_connection_ == NULL);
	X2RTC_CHECK(x2proto_handler_ == NULL);
	X2RTC_CHECK(x2media_puber_ == NULL);
	X2RTC_CHECK(x2media_suber_ == NULL);
}

void X2NetPeer::SetX2NetConnection(X2NetType x2NetType, X2NetConnection* x2NetConn)
{
	x2net_connection_ = x2NetConn;
	stream_info_.strRemoteAddr = x2NetConn->GetRemoteIpPort();
	if (x2net_connection_ != NULL) {
		x2net_connection_->SetListener(this);

		if (x2proto_handler_ == NULL) {
			if (x2NetType == X2Net_Rtmp) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_Rtmp);
				stream_info_.eProtoType = X2Proto_Rtmp;
			}
			else if (x2NetType == X2Net_Srt) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_MpegTS);
				stream_info_.eProtoType = X2Proto_MpegTS;
			}
			else if (x2NetType == X2Net_Rtc) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_Rtc);
				stream_info_.eProtoType = X2Proto_Rtc;
			}
			else if (x2NetType == X2Net_Rtp) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_Rtp);
				stream_info_.eProtoType = X2Proto_Rtp;
			}
			else if (x2NetType == X2Net_Http) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_Flv);
				stream_info_.eProtoType = X2Proto_Flv;
			}
			else if (x2NetType == X2Net_Talk) {
				x2proto_handler_ = createX2ProtoHandler(X2Proto_Talk);
				stream_info_.eProtoType = X2Proto_Talk;
			}
		}

		if (x2proto_handler_ != NULL) {
			x2proto_handler_->SetListener(this);
			str_params_ = x2NetConn->GetStreamId();
			if (str_params_.length() > 0) {
				b_set_params_ = true;
			}
		}
		else {
			X2RtcPrintf(ERR, "Not found protoHandler for X2NetType : %s", getX2NetTypeName(x2NetType));
		}
	}
}

void X2NetPeer::Init()
{
	X2RtcThreadPool::Inst().RegisteMainX2RtcTick(this, this);
}
void X2NetPeer::DeInit()
{
	X2RtcThreadPool::Inst().UnRegisteMainX2RtcTick(this);

	X2RTC_CHECK(x2net_connection_ == NULL);
	X2RTC_CHECK(x2proto_handler_ == NULL);
	X2RTC_CHECK(x2media_puber_ == NULL);
	X2RTC_CHECK(x2media_suber_ == NULL);

	JMutexAutoLock l(cs_list_recv_x2net_data_);
	list_recv_x2net_data_.clear();
}

void X2NetPeer::SetRecvVideoScaleRD(int nScaleRd)
{
	if (vid_recv_status_.n_req_scale_rd_ != nScaleRd) {
		vid_recv_status_.n_req_scale_rd_ = nScaleRd;
		vid_recv_status_.b_need_switch_stream_ = true;
	}
}

void X2NetPeer::SetRecvStatus(bool bRecvAudio, bool bRecvVideo)
{
	if (vid_recv_status_.b_remote_recv_audio_ != bRecvAudio) {
		vid_recv_status_.b_remote_recv_audio_ = bRecvAudio;
	}

	if (vid_recv_status_.b_remote_recv_video_ != bRecvVideo) {
		if (bRecvVideo) {
			vid_recv_status_.b_tmp_need_main_keyframe_ = true;
			vid_recv_status_.b_tmp_need_sub_keyframe_ = true;
		}
	}
}

//* For X2NetConnection::Listener
void X2NetPeer::OnX2NetConnectionPacketReceived(
	X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	JMutexAutoLock l(cs_list_recv_x2net_data_);
	list_recv_x2net_data_.push_back(x2NetData);
}

void X2NetPeer::OnX2NetConnectionClosed(X2NetConnection* connection)
{
	b_notify_close_from_network_ = true;

	if (x2media_puber_ != NULL) {
		X2RtcPrintf(WARN, "Publish closed by network remote(%s) app: %s stream: %s", stream_info_.strRemoteAddr.c_str(), str_app_id_.c_str(), str_stream_id_.c_str());
	}
	else if (x2media_suber_ != NULL) {
		X2RtcPrintf(WARN, "Play closed by network remote(%s) app: %s stream: %s", stream_info_.strRemoteAddr.c_str(), str_app_id_.c_str(), str_stream_id_.c_str());
	}
	else {
		X2RtcPrintf(WARN, "Unkonw closed by network remote(%s) app: %s stream: %s", stream_info_.strRemoteAddr.c_str(), str_app_id_.c_str(), str_stream_id_.c_str());
	}
}
void X2NetPeer::OnX2NetRemoteIpPortChanged(X2NetConnection* connection, struct sockaddr* remoteAddr)
{
	X2RtcPrintf(INF, "OnX2NetRemoteIpPortChanged before: %s", stream_info_.strRemoteAddr.c_str());
	stream_info_.strRemoteAddr = connection->GetRemoteIpPort();
	X2RtcPrintf(INF, "OnX2NetRemoteIpPortChanged after: %s", stream_info_.strRemoteAddr.c_str());
}
void X2NetPeer::OnX2NetGotNetStats(X2NetConnection* connection, bool isAudio, int nRtt, int nLossRate, int nBufSize)
{
	stream_stats_.nRtt = nRtt;
	stream_stats_.nLossRate = nLossRate;
}
//* For X2ProtoHandler::Listener
void X2NetPeer::OnX2ProtoHandlerPublish(const char* app, const char* stream, const char* token, const char* strParam)
{
	X2RtcPrintf(INF, "OnX2ProtoHandlerPublish app: %s stream: %s", app, stream);
	str_app_id_ = app;
	str_stream_id_ = stream;
	b_is_publish_ = true;
	stream_info_.strEventType = "push";
	stream_info_.nConnectedTime = XGetUtcTimestamp() - n_peer_created_time_;
	if(strParam != NULL)
	{
		n_next_stats_time_ = XGetUtcTimestamp() + 1000;
		std::map<std::string, std::string> mapParam;
		XParseHttpParam(strParam, &mapParam);
		if (mapParam.find("uid") != mapParam.end()) {
			stream_info_.strUserId = mapParam["uid"];
		}
		else {
			XGetRandomStr(stream_info_.strUserId, 8);
		}
		if (mapParam.find("sid") != mapParam.end()) {
			stream_info_.strSessionId = mapParam["sid"];
		}
		else {
			XGetRandomStr(stream_info_.strSessionId, 16);
		}
		if (mapParam.find("event") != mapParam.end()) {
			stream_info_.strEventType = mapParam["event"];
		}
	}
	if (x2media_puber_ == NULL) {
		x2media_puber_ = new X2MediaPuber();
		x2media_puber_->SetListener(this);
		 
		if (!X2MediaDispatch::Inst().TryPublish(str_app_id_, str_stream_id_, stream_info_.strEventType, x2media_puber_)) {
			n_next_try_publish_time_ = XGetUtcTimestamp() + 100;
		}
		else {
			x2media_puber_->RtnPublish(str_app_id_.c_str(), str_stream_id_.c_str());
		}
	}
}
void X2NetPeer::OnX2ProtoHandlerPlay(const char* app, const char* stream, const char* token, const char* strParam)
{
	X2RtcPrintf(INF, "OnX2ProtoHandlerPlay app: %s stream: %s", app, stream);
	str_app_id_ = app;
	str_stream_id_ = stream;
	b_is_publish_ = false;
	bool bCacheGop = true;
	stream_info_.strEventType = "play";
	stream_info_.nConnectedTime = XGetUtcTimestamp() - n_peer_created_time_;
	if (strParam != NULL)
	{
		n_next_stats_time_ = XGetUtcTimestamp() + 1000;
		std::map<std::string, std::string> mapParam;
		XParseHttpParam(strParam, &mapParam);
		if (mapParam.find("uid") != mapParam.end()) {
			stream_info_.strUserId = mapParam["uid"];
		}
		else {
			XGetRandomStr(stream_info_.strUserId, 8);
		}
		if (mapParam.find("sid") != mapParam.end()) {
			stream_info_.strSessionId = mapParam["sid"];
		}
		else {
			XGetRandomStr(stream_info_.strSessionId, 16);
		}

		if (mapParam.find("event") != mapParam.end()) {
			stream_info_.strEventType = mapParam["event"];
		}

		if (mapParam.find("scale") != mapParam.end()) {
			int nScale = atoi(mapParam["scale"].c_str());
			if (nScale > 0 && nScale < 5) {
				vid_recv_status_.n_req_scale_rd_ = nScale;
				vid_recv_status_.b_need_switch_stream_ = true;
			}
		}
		if (mapParam.find("cacheGop") != mapParam.end()) {
			int nCacheGop = atoi(mapParam["cacheGop"].c_str());
			bCacheGop = nCacheGop == 1 ? true : false;
		}
		if (mapParam.find("selfCtrl") != mapParam.end()) {
			int nSelfCtrl = atoi(mapParam["selfCtrl"].c_str());
			b_play_self_ctrl_ = nSelfCtrl == 1 ? true : false;
		}
	}
	if (x2media_suber_ == NULL) {
		x2media_suber_ = new X2MediaSuber();
		x2media_suber_->SetListener(this);
		x2media_suber_->SetProto(x2proto_handler_->GetType());
		x2media_suber_->SetCacheGop(bCacheGop);
		
		if (!b_play_self_ctrl_) {
			X2MediaDispatch::Inst().Subscribe(str_app_id_, str_stream_id_, stream_info_.strEventType, x2media_suber_);
		}
	}
}
void X2NetPeer::OnX2ProtoHandlerPlayStateChanged( bool bPlayed)
{
	if (b_play_self_ctrl_) {
		if (bPlayed) {
			X2MediaDispatch::Inst().Subscribe(str_app_id_, str_stream_id_, stream_info_.strEventType, x2media_suber_);
		}
		else {
			X2MediaDispatch::Inst().UnSubscribe(str_app_id_, str_stream_id_, x2media_suber_);
		}
	}
	
}
void X2NetPeer::OnX2ProtoHandlerClose()
{
	b_notify_close_from_proto_ = true;

	if (x2media_puber_ != NULL) {
		X2RtcPrintf(WARN, "Publish closed by proto remote(%s) app: %s stream: %s", stream_info_.strRemoteAddr.c_str(), str_app_id_.c_str(), str_stream_id_.c_str());
	}
	else if (x2media_suber_ != NULL) {
		X2RtcPrintf(WARN, "Play closed by proto remote(%s) app: %s stream: %s", stream_info_.strRemoteAddr.c_str(), str_app_id_.c_str(), str_stream_id_.c_str());
	}
}
void X2NetPeer::OnX2ProtoHandlerSetPlayCodecType(X2CodecType eAudCodecType, X2CodecType eVidCodecType)
{
	X2MediaDispatch::Inst().ResetSubCodecType(str_app_id_, str_stream_id_, x2media_suber_, eAudCodecType, eVidCodecType);
}
void X2NetPeer::OnX2ProtoHandlerSendData(const uint8_t* pHdr, size_t nHdrLen, const uint8_t* pData, size_t nLen)
{
	if (x2net_connection_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject< X2NetDataRef>();
		if (pHdr != NULL && nHdrLen > 0) {
			x2NetData->SetHdr(pHdr, nHdrLen);
		}
		if (pData != NULL && nLen > 0) {
			x2NetData->SetData(pData, nLen);
		}
		if (x2proto_handler_ != NULL && x2proto_handler_->GetType() == X2Proto_Rtp) {
			//@Eric - 20230811 - fix bug: if send too many data once when use tcp, some divice cannot play data normally.
			x2NetData->bSendOnce = true;
		}

		x2net_connection_->Send(x2NetData);

		stream_stats_.nNetAllBytes += nLen + nHdrLen;
	}
}
void X2NetPeer::OnX2ProtoHandlerRecvScript(const uint8_t* data, size_t bytes, int64_t timestamp)
{

}
void X2NetPeer::OnX2ProtoHandlerRecvCodecData(scoped_refptr<X2CodecDataRef> x2CodecData)
{
	//if(!x2CodecData->bIsAudio)
	//	printf("OnX2ProtoHandlerRecvCodecData isAud: %d codec: %s len: %d keyframe: %d time: %lld\r\n", x2CodecData->bIsAudio, getX2CodeTypeName(x2CodecData->eCodecType), x2CodecData->nLen, x2CodecData->bKeyframe, x2CodecData->lPts);
	if (x2media_puber_ != NULL) {
		x2media_puber_->RecvCodecData(x2CodecData);
	}

	if (x2CodecData->bIsAudio) {
		stream_info_.eAudCodecType = x2CodecData->eCodecType;
		stream_stats_.nAudBytes += x2CodecData->nLen;
		if (stream_info_.nFirstAudInterval == 0) {
			stream_info_.nFirstAudInterval = XGetUtcTimestamp() - n_peer_created_time_;
		}
	}
	else {
		stream_info_.eVidCodecType = x2CodecData->eCodecType;
		stream_stats_.nVidBytes += x2CodecData->nLen;
		if (x2CodecData->bKeyframe) {
			if (n_last_recv_keyframe_time_ != 0) {
				stream_stats_.nVidKeyFrameInterval = XGetUtcTimestamp() - n_last_recv_keyframe_time_;
			}
			n_last_recv_keyframe_time_ = XGetUtcTimestamp();

			if (stream_info_.nFirstVidInterval == 0) {
				stream_info_.nFirstVidInterval = XGetUtcTimestamp() - n_peer_created_time_;
			}
		}
	}
}

//* For X2MediaPuber::Listener
void X2NetPeer::OnX2MediaPuberClosed(X2MediaPuber* x2Puber)
{
	b_notify_close_by_other_ = true;
}
//* For X2MediaSuber::Listener
void X2NetPeer::OnX2MediaSuberGotData(x2rtc::scoped_refptr<X2CodecDataRef> x2CodecData)
{

	JMutexAutoLock l(cs_list_recv_sub_data_);
	list_recv_sub_data_.push_back(x2CodecData);	
}

//* For X2RtcTick
void X2NetPeer::OnX2RtcTick()
{
	if (b_self_closed_) {
		return;
	}

	if (b_set_params_) {
		b_set_params_ = false;
		x2proto_handler_->SetStreamId(str_params_.c_str());
	}
	if (n_next_try_publish_time_ != 0 && n_next_try_publish_time_ <= XGetUtcTimestamp()) {
		n_next_try_publish_time_ = 0;
		if (!X2MediaDispatch::Inst().TryPublish(str_app_id_, str_stream_id_, stream_info_.strEventType, x2media_puber_)) {
			n_next_try_publish_time_ = XGetUtcTimestamp() + 100;
		}
		else {
			x2media_puber_->RtnPublish(str_app_id_.c_str(), str_stream_id_.c_str());
		}
	}
	if (b_notify_close_by_other_ || b_notify_close_from_network_ || b_notify_close_from_proto_) {
		b_notify_close_by_other_ = true;
		b_notify_close_from_network_ = false;
		b_notify_close_from_proto_ = false;
		if (x2net_connection_ != NULL) {
			x2net_connection_->SetListener(NULL);
			x2net_connection_->ForceClose();
			x2net_connection_ = NULL;
		}

		if (x2media_puber_ != NULL) {
			X2MediaDispatch::Inst().UnPublish(str_app_id_, str_stream_id_);
			delete x2media_puber_;
			x2media_puber_ = NULL;
		}

		if (x2media_suber_ != NULL) {
			X2MediaDispatch::Inst().UnSubscribe(str_app_id_, str_stream_id_, x2media_suber_);
			delete x2media_suber_;
			x2media_suber_ = NULL;
		}
		if (x2proto_handler_ != NULL) {
			x2proto_handler_->Close();
			delete x2proto_handler_;
			x2proto_handler_ = NULL;
		}

		if (!b_self_closed_) {
			b_self_closed_ = true;

			if (cb_listener_ != NULL) {
				cb_listener_->OnX2NetPeerClosed(this);
			}
		}
		return;
	}
	while (1) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		{
			JMutexAutoLock l(cs_list_recv_x2net_data_);
			if (list_recv_x2net_data_.size() > 0) {
				x2NetData = list_recv_x2net_data_.front();
				list_recv_x2net_data_.pop_front();
			}
		}

		if (x2NetData != NULL) {
			stream_stats_.nNetAllBytes += x2NetData->nLen;

			if (x2proto_handler_ != NULL) {
				x2proto_handler_->RecvDataFromNetwork(x2NetData->nDataType, x2NetData->pData, x2NetData->nLen);
			}
		}
		else {
			break;
		}
	}

	if (n_send_data_min_gap_time_ <= XGetUtcTimestamp()) {
		n_send_data_min_gap_time_ = XGetUtcTimestamp() + 10;
		int nSendPkt1Time = 5;
		while (1) {
			x2rtc::scoped_refptr<X2CodecDataRef> x2CodecData = NULL;
			{
				JMutexAutoLock l(cs_list_recv_sub_data_);
				if (list_recv_sub_data_.size() > 0) {
					x2CodecData = list_recv_sub_data_.front();
					list_recv_sub_data_.pop_front();
				}
			}

			if (x2CodecData != NULL) {
				bool bCanSend = false;
				if (x2CodecData->bIsAudio) {
					bCanSend = true;
					stream_info_.eAudCodecType = x2CodecData->eCodecType;
					stream_stats_.nAudBytes += x2CodecData->nLen;

					if (stream_info_.nFirstAudInterval == 0) {
						stream_info_.nFirstAudInterval = XGetUtcTimestamp() - n_peer_created_time_;
					}
				}
				else {
					bCanSend = VideoCanSend(x2CodecData->nVidScaleRD, x2CodecData->bKeyframe, &vid_recv_status_);
					if (bCanSend) {
						//printf("Send vid rd: %d key: %d len: %d time: %u\r\n", x2CodecData->nVidScaleRD, x2CodecData->bKeyframe, x2CodecData->nLen, XGetTimestamp());
						stream_info_.eVidCodecType = x2CodecData->eCodecType;
						stream_stats_.nVidBytes += x2CodecData->nLen;

						if (x2CodecData->bKeyframe) {
							if (stream_info_.nFirstVidInterval == 0) {
								stream_info_.nFirstVidInterval = XGetUtcTimestamp() - n_peer_created_time_;
							}
						}
					}
					else {
						//printf("Drop vid rd: %d key: %d len: %d\r\n", x2CodecData->nVidScaleRD, x2CodecData->bKeyframe, x2CodecData->nLen);
					}
				}

				if (bCanSend) {
					if (x2proto_handler_ != NULL) {
						x2proto_handler_->MuxData(x2CodecData->eCodecType, x2CodecData->pData, x2CodecData->nLen, x2CodecData->lDts, x2CodecData->lPts, x2CodecData->nAudSeqn);
					}
				}
			}
			else {
				break;
			}

			nSendPkt1Time--;
			if (nSendPkt1Time <= 0) {
				break;
			}
		}
	}

	if (n_next_stats_time_ != 0 && n_next_stats_time_ <= XGetUtcTimestamp()) {
		n_next_stats_time_ = XGetUtcTimestamp() + 1000;
		stream_stats_.nUsedAllTime = XGetUtcTimestamp() - n_peer_created_time_;

		if (str_app_id_.length() > 0) {
			if (cb_listener_ != NULL) {
				cb_listener_->OnX2NetPeerStreamStats(b_is_publish_, str_app_id_, str_stream_id_, &stream_info_, &stream_stats_);
			}
		}

		stream_stats_.nAudAllBytes += stream_stats_.nAudBytes;
		stream_stats_.nVidAllBytes += stream_stats_.nVidBytes;
		stream_stats_.nAudBytes = 0;
		stream_stats_.nVidBytes = 0;
	}
}

bool X2NetPeer::VideoCanSend(int nScaleRD, bool bKeyFrame, VideoRecvStatus*vidRecvStatus)
{
	if (!vidRecvStatus->b_remote_recv_video_)
		return false;	// If not recv video, return false always.
	if (nScaleRD == vidRecvStatus->n_last_scale_rd_ok_) {
		if (vidRecvStatus->n_req_scale_rd_ == vidRecvStatus->n_last_scale_rd_ok_) {
			vidRecvStatus->b_cur_sub_stream_ = false;	// StreamA switched.
		}

		if (vidRecvStatus->n_req_scale_rd_ == vidRecvStatus->n_last_scale_rd_ok_) {
			if (vidRecvStatus->b_need_switch_stream_ && !bKeyFrame) {
				return false;
			}
			else {
				vidRecvStatus->b_need_switch_stream_ = false;
			}

			if (vidRecvStatus->b_tmp_need_main_keyframe_) {
				if (!bKeyFrame)
					return false;
				vidRecvStatus->b_tmp_need_main_keyframe_ = false;
				vidRecvStatus->n_last_scale_rd_ok_ = nScaleRD;
			}

			return true;
		}
		else {
			if (vidRecvStatus->b_need_switch_stream_) {
				// If need switch to StreamA from FrameB, but the key frame of StreamA isn't coming, so StreamB continue to sending.
				return true;
			}
		}
	}
	else {
		if (nScaleRD != vidRecvStatus->n_req_scale_rd_) {
			return false;
		}
		if (vidRecvStatus->n_req_scale_rd_ != vidRecvStatus->n_last_scale_rd_ok_) {
			vidRecvStatus->b_cur_sub_stream_ = true;	// Switch to FrameA & reved the frame.
		}
		if (vidRecvStatus->n_req_scale_rd_ != vidRecvStatus->n_last_scale_rd_ok_) {
			if (vidRecvStatus->b_need_switch_stream_ && !bKeyFrame) {
				return false;
			}
			else {
				vidRecvStatus->b_need_switch_stream_ = false;
			}
			if (vidRecvStatus->b_tmp_need_sub_keyframe_) {
				if (!bKeyFrame)
					return false;
				vidRecvStatus->b_tmp_need_sub_keyframe_ = false;
				vidRecvStatus->n_last_scale_rd_ok_ = nScaleRD;
				//Switch to main logic
				vidRecvStatus->b_tmp_need_main_keyframe_ = false;
			}

			return true;
		}
		else {
			if (vidRecvStatus->b_need_switch_stream_) {
				// If need switch to StreamB from FrameA, but the key frame of StreamB isn't coming, so StreamA continue to sending.
				return true;
			}
		}
	}
	return false;
}

}	// namespace x2rtc