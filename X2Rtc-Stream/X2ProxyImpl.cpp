#include "X2ProxyImpl.h"
#include "XUtil.h"
#include "X2RtcLog.h"
#include "X2RtcCheck.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"

#define PUB_STATS_TIME 3000
#define SUB_STATS_TIME 10000

namespace x2rtc {

X2Proxy* createX2Proxy()
{
	return new X2ProxyImpl();
}

static void cb_loop_timer(uS::Timer* timer)
{
	X2ProxyImpl* x2UvRun = (X2ProxyImpl*)timer->userData;
	x2UvRun->OnTimer();
}

X2ProxyImpl::X2ProxyImpl(void)
	: b_running_(false)
	, uv_loop_(NULL)
	, uv_timer_(NULL)
	, x2net_tcp_client_(NULL)
	, n_tcp_reconnect_time_(0)
	, n_next_report_pub_stats_time_(0)
	, n_next_report_sub_stats_time_(0)
	, b_registed_(false)
	, b_abort_(false)
	, n_subscribe_report_time_(0)
{

}
X2ProxyImpl::~X2ProxyImpl(void)
{

}

//* For X2Proxy
int X2ProxyImpl::StartTask(const char* strSvrIp, int nSvrPort)
{
	int ret = 0;
	n_subscribe_report_time_ = XGetUtcTimestamp();
	n_next_report_pub_stats_time_ = XGetUtcTimestamp() + PUB_STATS_TIME;
	n_next_report_sub_stats_time_ = XGetUtcTimestamp() + SUB_STATS_TIME;
	if (uv_loop_ == NULL) {
		uv_loop_ = uS::Loop::createLoop(true);
	}

	if (strSvrIp != NULL && strlen(strSvrIp) != 0 && nSvrPort != 0) {
		if (x2net_tcp_client_ == NULL) {
			x2net_tcp_client_ = createX2NetTcpClient();
			x2net_tcp_client_->SetListener(this);
			x2net_tcp_client_->Connect(strSvrIp, nSvrPort);
		}
	}

	if (uv_timer_ == NULL) {
		uv_timer_ = new uS::Timer(uv_loop_);
		uv_timer_->userData = this;
		uv_timer_->start(cb_loop_timer, 10, 1);
	}

	if (!b_running_) {
		b_running_ = true;	
		X2MediaDispatch::Inst().SetListener(this);
		ret = JThread::Start();
	}
	return ret;
}
int X2ProxyImpl::StopTask()
{
	int ret = 0;
	if (b_running_) {
		b_running_ = false;
		X2MediaDispatch::Inst().SetListener(NULL);
		ret = JThread::Kill();
	}

	if (uv_timer_ != NULL) {
		uv_timer_->close();
		uv_timer_ = NULL;
	}

	if (x2net_tcp_client_ != NULL) {
		x2net_tcp_client_->DisConnect();
		delete x2net_tcp_client_;
		x2net_tcp_client_ = NULL;
	}

	if (uv_loop_ != NULL) {
		uv_loop_->destroy();
		uv_loop_ = NULL;
	}

	return 0;
}
int X2ProxyImpl::RunOnce()
{
	int64_t nUtcTime = XGetUtcTimestamp();
	{
		MapX2NetProcess::iterator itnr = map_x2net_process_.begin();
		while (itnr != map_x2net_process_.end()) {
			itnr->second->RunOnce();
			itnr++;
		}
	}

	while(1)
	{
		X2NetPeer* x2NetPeer = NULL;
		{
			JMutexAutoLock l(cs_list_x2net_peer_delete_);
			if (list_x2net_peer_delete_.size() > 0) {
				x2NetPeer = list_x2net_peer_delete_.front();
				list_x2net_peer_delete_.pop_front();
			}
		}

		if (x2NetPeer != NULL) {
			x2NetPeer->DeInit();
			delete x2NetPeer;
		}
		else {
			break;
		}
	}

	x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
	{
		JMutexAutoLock l(cs_list_recv_data_);
		if (list_recv_data_.size() > 0) {
			x2NetData = list_recv_data_.front();
			list_recv_data_.pop_front();
		}
	}
	if (x2NetData != NULL) {
		rapidjson::Document		jsonReqDoc;
		JsonStr sprCopy((char*)x2NetData->pData, x2NetData->nLen);
		X2RtcPrintf(INF, "X2ProxyImpl process msg: %s", sprCopy.Ptr);
		if (!jsonReqDoc.ParseInsitu<0>(sprCopy.Ptr).HasParseError()) {
			const std::string& strCmd = GetJsonStr(jsonReqDoc, "Cmd", F_AT);
			if (strCmd.compare("RtpOpen") == 0 || strCmd.compare("RtpClose") == 0 || strCmd.compare("RtpSendOpen") == 0 || strCmd.compare("RtpSendClose") == 0) {
				if (map_x2net_process_.find(X2Process_Rtp) != map_x2net_process_.end()) {
					map_x2net_process_[X2Process_Rtp]->SetParameter((char*)x2NetData->pData);
				}
			}
		}
	}

	if (n_subscribe_report_time_ != 0 && n_subscribe_report_time_ <= nUtcTime) {
		n_subscribe_report_time_ = nUtcTime + 1000;

		std::string strAppSubers;
		X2MediaDispatch::Inst().StatsSubscribe(strAppSubers);

		if (strAppSubers.length() > 0) {
			rapidjson::Document		jsonAppSubersDoc;
			jsonAppSubersDoc.ParseInsitu<0>((char*)strAppSubers.c_str());

			rapidjson::Document		jsonDoc;
			rapidjson::StringBuffer jsonStr;
			rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
			jsonDoc.SetObject();
			jsonDoc.AddMember("Cmd", "SubscribeList", jsonDoc.GetAllocator());
			rapidjson::Value jsContent(rapidjson::kObjectType);
			jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
			jsContent.AddMember("List", jsonAppSubersDoc["List"], jsonDoc.GetAllocator());
			jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
			jsonDoc.Accept(jsonWriter);

			if (x2net_tcp_client_ != NULL) {
				x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
				x2NetData->SetData((uint8_t*)jsonStr.GetString(), jsonStr.Size());
				x2net_tcp_client_->SendData(x2NetData);
			}
		}
	}

	ReportStreamStats(nUtcTime);
	
	return b_abort_?1:0;
}
int X2ProxyImpl::ImportX2NetProcess(x2rtc::X2ProcessType eType, int nPort)
{
	X2RTC_CHECK(!b_running_);
	if (nPort <= 0) {
		return -1;
	}
	if (map_x2net_process_.find(eType) == map_x2net_process_.end()) {
		X2NetProcess* x2NetProcess = createX2NetProcess(eType);
		if (x2NetProcess != NULL) {
			if (int ret = x2NetProcess->Init(nPort) != 0) {
				X2RtcPrintf(ERR, "NetProcess type(%d) init err: %d", eType, ret);
				return -1;
			}
			x2NetProcess->SetListener(this);
			map_x2net_process_[eType] = x2NetProcess;
		}
		else {
			X2RtcPrintf(ERR, "NetProcess type(%d) not support!", eType);
		}
	}
	return 0;
}

void X2ProxyImpl::ReportStreamStats(int64_t nUtcTime)
{
	if (n_next_report_pub_stats_time_ <= nUtcTime) {
		n_next_report_pub_stats_time_ = nUtcTime + PUB_STATS_TIME;

		JMutexAutoLock l(cs_map_app_pub_stream_);
		if (map_app_pub_stream_.size() > 0) {
			rapidjson::Document		jsonAppDoc;
			rapidjson::StringBuffer jsonAppStr;
			rapidjson::Writer<rapidjson::StringBuffer> jsonAppWriter(jsonAppStr);
			jsonAppDoc.SetObject();
			rapidjson::Value jarrApps(rapidjson::kArrayType);

			MapAppPubStream::iterator itar = map_app_pub_stream_.begin();
			while (itar != map_app_pub_stream_.end()) {
				rapidjson::Value jsApp(rapidjson::kObjectType);
				rapidjson::Value jarrStream(rapidjson::kArrayType);
				jsApp.AddMember("AppId", itar->first.c_str(), jsonAppDoc.GetAllocator());
				MapStreamInfo& mapStreamInfo = itar->second;
				MapStreamInfo::iterator itsr = mapStreamInfo.begin();
				while (itsr != mapStreamInfo.end()) {
					StreamStatsInfo& streamStatsInfo = itsr->second;
					rapidjson::Value jsStream(rapidjson::kObjectType);
					jsStream.AddMember("StreamId", itsr->first.c_str(), jsonAppDoc.GetAllocator());
					jsStream.AddMember("UId", streamStatsInfo.streamInfo.strUserId.c_str(), jsonAppDoc.GetAllocator());
					jsStream.AddMember("SId", streamStatsInfo.streamInfo.strSessionId.c_str(), jsonAppDoc.GetAllocator());
					jsStream.AddMember("ClientAddr", streamStatsInfo.streamInfo.strRemoteAddr.c_str(), jsonAppDoc.GetAllocator());
					jsStream.AddMember("ACodec", streamStatsInfo.streamInfo.eAudCodecType, jsonAppDoc.GetAllocator());
					jsStream.AddMember("VCodec", streamStatsInfo.streamInfo.eVidCodecType, jsonAppDoc.GetAllocator());
					jsStream.AddMember("Proto", streamStatsInfo.streamInfo.eProtoType, jsonAppDoc.GetAllocator());
					jsStream.AddMember("ConnectedTime", streamStatsInfo.streamInfo.nConnectedTime, jsonAppDoc.GetAllocator());
					jsStream.AddMember("FirstAudInterval", streamStatsInfo.streamInfo.nFirstAudInterval, jsonAppDoc.GetAllocator());
					jsStream.AddMember("FirstVidInterval", streamStatsInfo.streamInfo.nFirstVidInterval, jsonAppDoc.GetAllocator());

					rapidjson::Value jarrStats(rapidjson::kArrayType);
					std::list<StreamStats>::iterator itlr = streamStatsInfo.listStreamStats.begin();
					while (itlr != streamStatsInfo.listStreamStats.end()) {
						rapidjson::Value jsStats(rapidjson::kObjectType);
						jsStats.AddMember("AByts", itlr->nAudBytes, jsonAppDoc.GetAllocator());
						jsStats.AddMember("VByts", itlr->nVidBytes, jsonAppDoc.GetAllocator());
						jsStats.AddMember("AllByts", itlr->nNetAllBytes, jsonAppDoc.GetAllocator());
						jsStats.AddMember("LossRate", itlr->nLossRate, jsonAppDoc.GetAllocator());
						jsStats.AddMember("Rtt", itlr->nRtt, jsonAppDoc.GetAllocator());
						jsStats.AddMember("VidKeyGap", itlr->nVidKeyFrameInterval, jsonAppDoc.GetAllocator());
						jsStats.AddMember("AllUsedTime", itlr->nUsedAllTime, jsonAppDoc.GetAllocator());
						jarrStats.PushBack(jsStats, jsonAppDoc.GetAllocator());
						itlr++;
					}
					jsStream.AddMember("Stats", jarrStats, jsonAppDoc.GetAllocator());
					jarrStream.PushBack(jsStream, jsonAppDoc.GetAllocator());
					itsr++;
				}

				jsApp.AddMember("List", jarrStream, jsonAppDoc.GetAllocator());
				jarrApps.PushBack(jsApp, jsonAppDoc.GetAllocator());
				itar++;
			}

			jsonAppDoc.AddMember("Cmd", "PubReport", jsonAppDoc.GetAllocator());
			rapidjson::Value jsContent(rapidjson::kObjectType);
			jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonAppDoc.GetAllocator());
			jsContent.AddMember("UtcTime", nUtcTime, jsonAppDoc.GetAllocator());
			jsContent.AddMember("Apps", jarrApps, jsonAppDoc.GetAllocator());
			jsonAppDoc.AddMember("Content", jsContent, jsonAppDoc.GetAllocator());
			jsonAppDoc.Accept(jsonAppWriter);
			X2RtcPrintf(INF, "PubStats: %s\r\n", jsonAppStr.GetString());

			if (x2net_tcp_client_ != NULL) {
				x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
				x2NetData->SetData((uint8_t*)jsonAppStr.GetString(), jsonAppStr.Size());
				x2net_tcp_client_->SendData(x2NetData);
			}
			map_app_pub_stream_.clear();
		}
	}

	if (n_next_report_sub_stats_time_ <= nUtcTime) {
		n_next_report_sub_stats_time_ = nUtcTime + SUB_STATS_TIME;

		JMutexAutoLock l(cs_map_app_sub_stream_);
		if (map_app_sub_stream_.size() > 0) {
			rapidjson::Document		jsonAppDoc;
			rapidjson::StringBuffer jsonAppStr;
			rapidjson::Writer<rapidjson::StringBuffer> jsonAppWriter(jsonAppStr);
			jsonAppDoc.SetObject();
			rapidjson::Value jarrApps(rapidjson::kArrayType);

			MapAppSubStream::iterator itar = map_app_sub_stream_.begin();
			while (itar != map_app_sub_stream_.end()) {
				rapidjson::Value jsApp(rapidjson::kObjectType);
				rapidjson::Value jarrStream(rapidjson::kArrayType);
				jsApp.AddMember("AppId", itar->first.c_str(), jsonAppDoc.GetAllocator());
				MapSubStreamInfo& mapStreamInfo = itar->second;
				MapSubStreamInfo::iterator itssr = mapStreamInfo.begin();
				while (itssr != mapStreamInfo.end()) {
					rapidjson::Value jsSubStream(rapidjson::kObjectType);
					rapidjson::Value arrSubStream(rapidjson::kArrayType);
					jsSubStream.AddMember("StreamId", itssr->first.c_str(), jsonAppDoc.GetAllocator());
					MapStreamInfo& mapStreamInfo = itssr->second;
					MapStreamInfo::iterator itsr = mapStreamInfo.begin();
					while (itsr != mapStreamInfo.end()) {
						StreamStatsInfo& streamStatsInfo = itsr->second;
						rapidjson::Value jsStream(rapidjson::kObjectType);
						jsStream.AddMember("UId", streamStatsInfo.streamInfo.strUserId.c_str(), jsonAppDoc.GetAllocator());
						jsStream.AddMember("SId", streamStatsInfo.streamInfo.strSessionId.c_str(), jsonAppDoc.GetAllocator());
						jsStream.AddMember("ClientAddr", streamStatsInfo.streamInfo.strRemoteAddr.c_str(), jsonAppDoc.GetAllocator());
						jsStream.AddMember("ACodec", streamStatsInfo.streamInfo.eAudCodecType, jsonAppDoc.GetAllocator());
						jsStream.AddMember("VCodec", streamStatsInfo.streamInfo.eVidCodecType, jsonAppDoc.GetAllocator());
						jsStream.AddMember("Proto", streamStatsInfo.streamInfo.eProtoType, jsonAppDoc.GetAllocator());
						jsStream.AddMember("ConnectedTime", streamStatsInfo.streamInfo.nConnectedTime, jsonAppDoc.GetAllocator());
						jsStream.AddMember("FirstAudInterval", streamStatsInfo.streamInfo.nFirstAudInterval, jsonAppDoc.GetAllocator());
						jsStream.AddMember("FirstVidInterval", streamStatsInfo.streamInfo.nFirstVidInterval, jsonAppDoc.GetAllocator());

						rapidjson::Value jarrStats(rapidjson::kArrayType);
						std::list<StreamStats>::iterator itlr = streamStatsInfo.listStreamStats.begin();
						while (itlr != streamStatsInfo.listStreamStats.end()) {
							rapidjson::Value jsStats(rapidjson::kObjectType);
							jsStats.AddMember("AByts", itlr->nAudBytes, jsonAppDoc.GetAllocator());
							jsStats.AddMember("VByts", itlr->nVidBytes, jsonAppDoc.GetAllocator());
							jsStats.AddMember("AllByts", itlr->nNetAllBytes, jsonAppDoc.GetAllocator());
							jsStats.AddMember("LossRate", itlr->nLossRate, jsonAppDoc.GetAllocator());
							jsStats.AddMember("Rtt", itlr->nRtt, jsonAppDoc.GetAllocator());
							jsStats.AddMember("AllUsedTime", itlr->nUsedAllTime, jsonAppDoc.GetAllocator());
							jarrStats.PushBack(jsStats, jsonAppDoc.GetAllocator());
							itlr++;
						}
						jsStream.AddMember("Stats", jarrStats, jsonAppDoc.GetAllocator());
						arrSubStream.PushBack(jsStream, jsonAppDoc.GetAllocator());
						itsr++;
					}
					jsSubStream.AddMember("Users", arrSubStream, jsonAppDoc.GetAllocator());
					jarrStream.PushBack(jsSubStream, jsonAppDoc.GetAllocator());
					itssr++;
				}

				jsApp.AddMember("List", jarrStream, jsonAppDoc.GetAllocator());
				jarrApps.PushBack(jsApp, jsonAppDoc.GetAllocator());
				itar++;
			}

			jsonAppDoc.AddMember("Cmd", "SubReport", jsonAppDoc.GetAllocator());
			rapidjson::Value jsContent(rapidjson::kObjectType);
			jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonAppDoc.GetAllocator());
			jsContent.AddMember("UtcTime", nUtcTime, jsonAppDoc.GetAllocator());
			jsContent.AddMember("Apps", jarrApps, jsonAppDoc.GetAllocator());
			jsonAppDoc.AddMember("Content", jsContent, jsonAppDoc.GetAllocator());
			jsonAppDoc.Accept(jsonAppWriter);
			X2RtcPrintf(INF, "SubStats: %s\r\n", jsonAppStr.GetString());

			if (x2net_tcp_client_ != NULL) {
				x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
				x2NetData->SetData((uint8_t*)jsonAppStr.GetString(), jsonAppStr.Size());
				x2net_tcp_client_->SendData(x2NetData);
			}
			map_app_sub_stream_.clear();
		}
	}
}

//* For JThread
void* X2ProxyImpl::Thread()
{
	JThread::ThreadStarted();

	while (b_running_) {
		if (uv_loop_ != NULL) {
			uv_loop_->run();
		}
		XSleep(1);
	}
	return NULL;
}

void X2ProxyImpl::OnTimer()
{
	if (n_tcp_reconnect_time_ != 0 && n_tcp_reconnect_time_ <= XGetUtcTimestamp()) {
		n_tcp_reconnect_time_ = 0;
		X2RTC_CHECK(x2net_tcp_client_ != NULL);
		x2net_tcp_client_->DisConnect();
		x2net_tcp_client_->ReConnect();
		X2RtcPrintf(WARN, "X2NetTcpClient re connecting...");
	}
	if (x2net_tcp_client_ != NULL) {
		x2net_tcp_client_->RunOnce();
	}
}

//* For X2NetProcess::Listener
void X2ProxyImpl::OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection)
{
	X2NetPeer* x2NetPeer = new X2NetPeer();
	x2NetPeer->SetListener(this);
	x2NetPeer->SetX2NetConnection(x2NetType, connection);
	x2NetPeer->Init();
}

void X2ProxyImpl::OnX2NetProcessSendMessage(const char* strMsg)
{
	if (x2net_tcp_client_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)strMsg, strlen(strMsg));
		x2net_tcp_client_->SendData(x2NetData);
	}
}

//* For X2NetPeer::Listener
void X2ProxyImpl::OnX2NetPeerEvent(
	X2NetPeer* x2NetPeer, x2rtc::scoped_refptr<X2RtcEventRef> x2RtcEvent)
{

}
void X2ProxyImpl::OnX2NetPeerClosed(X2NetPeer* x2NetPeer)
{
	JMutexAutoLock l(cs_list_x2net_peer_delete_);
	list_x2net_peer_delete_.push_back(x2NetPeer);
}
void X2ProxyImpl::OnX2NetPeerStreamStats(bool isPublish, const std::string& strAppId, const std::string& strStreamId,
	StreamInfo* streamInfo, StreamStats* streamStats)
{
	//printf("OnX2NetPeerStreamStats isPub: %d app: %s stream: %s uid: %s sid: %s adddr: %s aud: %d vid: %d all: %d\r\n", 
	//	isPublish, strAppId.c_str(), strStreamId.c_str(), streamInfo->strUserId.c_str(), streamInfo->strSessionId.c_str(), streamInfo->strRemoteAddr.c_str(), streamStats->nAudBytes,
	//	streamStats->nVidBytes, streamStats->nNetAllBytes);

	if (isPublish) {
		JMutexAutoLock l(cs_map_app_pub_stream_);
		if (map_app_pub_stream_.find(strAppId) == map_app_pub_stream_.end()) {
			map_app_pub_stream_[strAppId];
		}
		MapStreamInfo &mapStreamInfo = map_app_pub_stream_[strAppId];
		if (mapStreamInfo.find(strStreamId) == mapStreamInfo.end()) {
			mapStreamInfo[strStreamId];
		}
		else {
			StreamStatsInfo& streamStatsInfo = mapStreamInfo[strStreamId];
			if (streamStatsInfo.streamInfo.strSessionId.compare(streamInfo->strSessionId) != 0) {
				return;
			}
		}
		StreamStatsInfo&streamStatsInfo = mapStreamInfo[strStreamId];
		streamStatsInfo.streamInfo = *streamInfo;
		streamStatsInfo.listStreamStats.push_front(*streamStats);
	}
	else {
		JMutexAutoLock l(cs_map_app_sub_stream_);
		if (map_app_sub_stream_.find(strAppId) == map_app_sub_stream_.end()) {
			map_app_sub_stream_[strAppId];
		}
		MapSubStreamInfo&mapSubStreamInfo = map_app_sub_stream_[strAppId];
		if (mapSubStreamInfo.find(strStreamId) == mapSubStreamInfo.end()) {
			mapSubStreamInfo[strStreamId];
		}
		MapStreamInfo&mapStreamInfo = mapSubStreamInfo[strStreamId];
		if (mapStreamInfo.find(streamInfo->strUserId) == mapStreamInfo.end()) {
			mapStreamInfo[streamInfo->strUserId];
		}
		else {
			StreamStatsInfo& streamStatsInfo = mapStreamInfo[streamInfo->strUserId];
			if (streamStatsInfo.streamInfo.strSessionId.compare(streamInfo->strSessionId) != 0) {
				return;
			}
		}
		StreamStatsInfo& streamStatsInfo = mapStreamInfo[streamInfo->strUserId];
		streamStatsInfo.streamInfo = *streamInfo;
		streamStatsInfo.listStreamStats.push_front(*streamStats);
	}
}

//* For X2NetTcpClient::Listener
void X2ProxyImpl::OnX2NetTcpClientDataReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	JMutexAutoLock l(cs_list_recv_data_);
	list_recv_data_.push_back(x2NetData);
}
void X2ProxyImpl::OnX2NetTcpClientConnected()
{
	X2RtcPrintf(INF, "X2NetTcpClient conntected!");
}
void X2ProxyImpl::OnX2NetTcpClientClosed()
{
	X2RtcPrintf(INF, "X2NetTcpClient closed!");
	n_tcp_reconnect_time_ = XGetUtcTimestamp() + 3000;
}
void X2ProxyImpl::OnX2NetTcpClientFailed()
{
	X2RtcPrintf(INF, "X2NetTcpClient failed!");
	n_tcp_reconnect_time_ = XGetUtcTimestamp() + 3000;
}

//* For X2MediaDispatch::Listener
void X2ProxyImpl::OnX2MediaDispatchPublishOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType)
{
	{
		rapidjson::Document		jsonDoc;
		rapidjson::StringBuffer jsonStr;
		rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
		jsonDoc.SetObject();
		jsonDoc.AddMember("Cmd", "PublishOpen", jsonDoc.GetAllocator());
		rapidjson::Value jsContent(rapidjson::kObjectType);
		jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("AppId", strAppId.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("StreamId", strStreamId.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("EventType", strEventType.c_str(), jsonDoc.GetAllocator());
		jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
		jsonDoc.Accept(jsonWriter);

		if (x2net_tcp_client_ != NULL) {
			x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
			x2NetData->SetData((uint8_t*)jsonStr.GetString(), jsonStr.Size());
			x2net_tcp_client_->SendData(x2NetData);
		}
	}
}
void X2ProxyImpl::OnX2MediaDispatchPublishClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType)
{
	{
		rapidjson::Document		jsonDoc;
		rapidjson::StringBuffer jsonStr;
		rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
		jsonDoc.SetObject();
		jsonDoc.AddMember("Cmd", "PublishClose", jsonDoc.GetAllocator());
		rapidjson::Value jsContent(rapidjson::kObjectType);
		jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("AppId", strAppId.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("StreamId", strStreamId.c_str(), jsonDoc.GetAllocator());
		jsContent.AddMember("EventType", strEventType.c_str(), jsonDoc.GetAllocator());
		jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
		jsonDoc.Accept(jsonWriter);

		if (x2net_tcp_client_ != NULL) {
			x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
			x2NetData->SetData((uint8_t*)jsonStr.GetString(), jsonStr.Size());
			x2net_tcp_client_->SendData(x2NetData);
		}
	}
}
void X2ProxyImpl::OnX2MediaDispatchSubscribeOpen(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType)
{
	rapidjson::Document		jsonDoc;
	rapidjson::StringBuffer jsonStr;
	rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
	jsonDoc.SetObject();
	jsonDoc.AddMember("Cmd", "SubscribeOpen", jsonDoc.GetAllocator());
	rapidjson::Value jsContent(rapidjson::kObjectType);
	jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("AppId", strAppId.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("StreamId", strStreamId.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("EventType", strEventType.c_str(), jsonDoc.GetAllocator());
	jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
	jsonDoc.Accept(jsonWriter);

	if (x2net_tcp_client_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)jsonStr.GetString(), jsonStr.Size());
		x2net_tcp_client_->SendData(x2NetData);
	}

	X2RtcPrintf(INF, "OnX2MediaDispatchSubscribeOpen: %s", jsonStr.GetString());
}
void X2ProxyImpl::OnX2MediaDispatchSubscribeClose(const std::string& strAppId, const std::string& strStreamId, const std::string& strEventType)
{
	rapidjson::Document		jsonDoc;
	rapidjson::StringBuffer jsonStr;
	rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
	jsonDoc.SetObject();
	jsonDoc.AddMember("Cmd", "SubscribeClose", jsonDoc.GetAllocator());
	rapidjson::Value jsContent(rapidjson::kObjectType);
	jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("AppId", strAppId.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("StreamId", strStreamId.c_str(), jsonDoc.GetAllocator());
	jsContent.AddMember("EventType", strEventType.c_str(), jsonDoc.GetAllocator());
	jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
	jsonDoc.Accept(jsonWriter);

	if (x2net_tcp_client_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)jsonStr.GetString(), jsonStr.Size());
		x2net_tcp_client_->SendData(x2NetData);
	}

	X2RtcPrintf(INF, "OnX2MediaDispatchSubscribeClose: %s", jsonStr.GetString());
}

}	// namespace x2rtc