#include "X2NetRtpProcess.h"
#include "X2NetRtpConnection.h"
#include "Libuv.h"
#include "Rtcp/Rtcp.h"
#include "Rtsp/Rtsp.h"
#include "X2RtcCheck.h"
#include "X2RtcLog.h"
#include "XUtil.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"

namespace x2rtc {
extern X2NetProcess* createX2NetUdpProcess();
extern void X2NetGetAddressInfo(const struct sockaddr* addr, int* family, std::string& ip, uint16_t* port);

X2NetProcess* createX2NetRtpProcess()
{
	return new X2NetRtpProcess();
}


X2NetRtpProcess::X2NetRtpProcess(void)
	: x2net_rtp_process_(NULL)
	, x2net_rtcp_process_(NULL)
	, x2net_rtp_connection_(NULL)
	, x2net_rtcp_connection_(NULL)
	, n_id_idx_(0)
{


}
X2NetRtpProcess::~X2NetRtpProcess(void)
{

}

int X2NetRtpProcess::Init(int nPort)
{
	if (x2net_rtp_process_ == NULL) {
		x2net_rtp_process_ = createX2NetUdpProcess();
		x2net_rtp_process_->SetListener(this);
		if (x2net_rtp_process_->Init(nPort) != 0) {
			x2net_rtp_process_->DeInit();
			delete x2net_rtp_process_;
			x2net_rtp_process_ = NULL;
			return -1;
		}
	}

	if (x2net_rtcp_process_ == NULL) {
		x2net_rtcp_process_ = createX2NetUdpProcess();
		x2net_rtcp_process_->SetListener(this);
		if (x2net_rtcp_process_->Init(nPort + 1) != 0) {
			{
				x2net_rtp_process_->DeInit();
				delete x2net_rtp_process_;
				x2net_rtp_process_ = NULL;
			}
			x2net_rtcp_process_->DeInit();
			delete x2net_rtcp_process_;
			x2net_rtcp_process_ = NULL;
			return -2;
		}
	}

	return 0;
}
int X2NetRtpProcess::DeInit()
{
	if (x2net_rtp_connection_ != NULL) {
		x2net_rtp_connection_->ForceClose();
		x2net_rtp_connection_ = NULL;
	}

	if (x2net_rtcp_connection_ != NULL) {
		x2net_rtcp_connection_->ForceClose();
		x2net_rtcp_connection_ = NULL;
	}

	if (x2net_rtp_process_ != NULL) {
		x2net_rtp_process_->DeInit();
		delete x2net_rtp_process_;
		x2net_rtp_process_ = NULL;
	}

	if (x2net_rtcp_process_ != NULL) {
		x2net_rtcp_process_->DeInit();
		delete x2net_rtcp_process_;
		x2net_rtcp_process_ = NULL;
	}

	return 0;
}

int X2NetRtpProcess::SetParameter(const char* strJsParams)
{//Run on main thread
	rapidjson::Document		jsonReqDoc;
	JsonStr sprCopy((char*)strJsParams, strlen(strJsParams));
	if (!jsonReqDoc.ParseInsitu<0>(sprCopy.Ptr).HasParseError()) {
		const std::string& strCmd = GetJsonStr(jsonReqDoc, "Cmd", F_AT);
		if (jsonReqDoc.HasMember("Content")) {
			rapidjson::Value& jsContent = jsonReqDoc["Content"];
			if (jsContent.HasMember("Data")) {
				rapidjson::Value& jsData = jsContent["Data"];
				std::string strSsrc = GetJsonStr(jsData, "Ssrc", F_AT);
				int32_t nSsrc = atoi(strSsrc.c_str());

				int ret = 0;
				if (strCmd.compare("RtpOpen") == 0) {
					const std::string& strAppId = GetJsonStr(jsData, "AppId", F_AT);
					const std::string& strStreamId = GetJsonStr(jsData, "StreamId", F_AT);
					bool bSsrcClient = GetJsonBool(jsData, "SsrcClient", F_AT);
					int nSsrcPort = GetJsonInt(jsData, "SsrcPort", F_AT);

					if (map_x2net_connection_.find(nSsrc) == map_x2net_connection_.end()) {
						X2NetRtpConnection* x2NetRtpConn = new X2NetRtpConnection();
						char strUrl[1024];
						sprintf(strUrl, "x2::r=%s/%s,m=publish", strAppId.c_str(), strStreamId.c_str());
						if (bSsrcClient) {
							ret = x2NetRtpConn->InitUdp(strUrl, nSsrcPort, nSsrcPort+1);
						}
						else {
							ret = x2NetRtpConn->Init(strUrl, x2net_rtp_connection_, x2net_rtcp_connection_);
						}
						if (ret == 0) {
							map_x2net_connection_[nSsrc] = x2NetRtpConn;

							if (cb_listener_ != NULL) {
								cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtp, x2NetRtpConn);
							}
						}
						else {
							x2NetRtpConn->DeInit();
							delete x2NetRtpConn;
							x2NetRtpConn = NULL;
						}
					}
					else {
						ret = 201;
					}
				}
				else if (strCmd.compare("RtpSendOpen") == 0) {
					const std::string& strAppId = GetJsonStr(jsData, "AppId", F_AT);
					const std::string& strStreamId = GetJsonStr(jsData, "StreamId", F_AT);
					const std::string& strRemoteIp = GetJsonStr(jsData, "DstHost", F_AT);
					int nRemotePort = GetJsonInt(jsData, "DstPort", F_AT);
					const std::string& strNet = GetJsonStr(jsData, "Net", F_AT);
					
					int ret = 0;
					if (map_x2net_connection_.find(nSsrc) == map_x2net_connection_.end()) {
						X2NetRtpConnection* x2NetRtpConn = new X2NetRtpConnection();
						char strUrl[1024];
						sprintf(strUrl, "x2::r=%s/%s,m=play&t=raw&ssrc=%d&event=talk", strAppId.c_str(), strStreamId.c_str(), nSsrc);
						int ret = 0;
						if (strNet.compare("tcp") == 0) {
							sprintf(strUrl + strlen(strUrl), "&net=tcp");
							if (x2NetRtpConn->InitTcp(strUrl, nRemotePort) != 0) {
								ret = 501;
							}
						}
						else {
							x2NetRtpConn->Init(strUrl, x2net_rtp_connection_, x2net_rtcp_connection_);
							{
								struct sockaddr_in remoteAddr;
								uv_ip4_addr(strRemoteIp.c_str(), nRemotePort, &remoteAddr);
								x2NetRtpConn->UpdateRemoteAddr((struct sockaddr*)&remoteAddr);
							}
						}
						if (ret == 0) {
							map_x2net_connection_[nSsrc] = x2NetRtpConn;

							if (cb_listener_ != NULL) {
								cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtp, x2NetRtpConn);
							}
						}
						else {
							x2NetRtpConn->DeInit();
							delete x2NetRtpConn;
							x2NetRtpConn = NULL;
						}
					}
					else {
						ret = 201;
					}
				}
				else if (strCmd.compare("RtpClose") == 0 || strCmd.compare("RtpSendClose") == 0) {
					int ret = 0;
					if (map_x2net_connection_.find(nSsrc) != map_x2net_connection_.end()) {
						X2NetRtpConnection* x2NetRtpConn = (X2NetRtpConnection*)map_x2net_connection_[nSsrc];
						map_x2net_connection_.erase(nSsrc);

						x2NetRtpConn->DeInit();

						JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
						list_x2net_connection_for_delete_.push_back(x2NetRtpConn);
					}
					else {
						ret = 404;
					}
				}
				else {
					ret = 406;
				}

				if (cb_listener_ != NULL) {
					char strCmdRlt[64];
					sprintf(strCmdRlt, "%sRlt", strCmd.c_str());
					rapidjson::Document		jsonDoc;
					rapidjson::StringBuffer jsonStr;
					rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
					jsonDoc.SetObject();
					jsonDoc.AddMember("Cmd", strCmdRlt, jsonDoc.GetAllocator());
					rapidjson::Value jsContent(rapidjson::kObjectType);
					jsContent.AddMember("NodeId", str_node_id_.c_str(), jsonDoc.GetAllocator());
					jsContent.AddMember("Code", ret, jsonDoc.GetAllocator());
					jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
					jsonDoc.Accept(jsonWriter);

					cb_listener_->OnX2NetProcessSendMessage(jsonStr.GetString());
				}
			}
		}
	}
	return 0;
}

//* For X2NetProcess
int X2NetRtpProcess::RunOnce()
{//Run on main thread
	X2NetProcess::RunOnce();

	if (x2net_rtp_process_ != NULL) {
		x2net_rtp_process_->RunOnce();
	}
	if (x2net_rtcp_process_ != NULL) {
		x2net_rtcp_process_->RunOnce();
	}
	{
		std::map<int32_t, SsrcFrom>::iterator itsr = map_ssrc_not_found_.begin();
		while (itsr != map_ssrc_not_found_.end()) {
			SsrcFrom& ssrcFrom = itsr->second;
			if (ssrcFrom.nReportTime <= XGetUtcTimestamp()) {
				X2RtcPrintf(ERR, "RtpHeader ssrc: %d from: %s not found!\r\n", itsr->first, ssrcFrom.strIpPort.c_str());
				itsr = map_ssrc_not_found_.erase(itsr);
				continue;
			}
			itsr++;
		}
	}
	// Process recved rtp data
	while (1) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		{
			JMutexAutoLock l(cs_list_recv_data_);
			if (list_recv_data_.size() > 0) {
				x2NetData = list_recv_data_.front();
				list_recv_data_.pop_front();
			}
		}
		if (x2NetData != NULL) {
			if (mediakit::isRtp((char*)x2NetData->pData, x2NetData->nLen)) {
				mediakit::RtpHeader* header = (mediakit::RtpHeader*)x2NetData->pData;
				int32_t nSsrc = ntohl(header->ssrc);
				//printf("RtpHeader ssrc: %d\r\n", nSsrc);
				if (map_x2net_connection_.find(nSsrc) != map_x2net_connection_.end()) {
					X2NetRtpConnection* x2NetRtpConn = (X2NetRtpConnection*)map_x2net_connection_[nSsrc];
					x2NetRtpConn->RecvData(x2NetData);
				}
				else {
					//printf("RtpHeader ssrc: %d not found!\r\n", nSsrc);
					if (map_ssrc_not_found_.find(nSsrc) == map_ssrc_not_found_.end()) {
						SsrcFrom&ssrcFrom = map_ssrc_not_found_[nSsrc];
						int family = 0;
						std::string remoteIp;
						uint16_t remotePort = 0;
						X2NetGetAddressInfo(reinterpret_cast<const struct sockaddr*>(&x2NetData->peerAddr), &family, remoteIp, &remotePort);

						char ipPort[128];
						sprintf(ipPort, "%s:%d", remoteIp.c_str(), remotePort);
						ssrcFrom.strIpPort = ipPort;
						ssrcFrom.nReportTime = XGetUtcTimestamp() + 1000;
					}
				}
			}
			else {
				bool bFind = false;
				std::vector<mediakit::RtcpHeader*> rtcps = mediakit::RtcpHeader::loadFromBytes((char*)x2NetData->pData, x2NetData->nLen);
				for (auto& rtcp : rtcps) {
					switch ((mediakit::RtcpType)rtcp->pt) {
					case mediakit::RtcpType::RTCP_SR: {
						auto rtcp_sr = (mediakit::RtcpSR*)rtcp;
						int32_t nSsrc = rtcp_sr->ssrc;
						if (map_x2net_connection_.find(nSsrc) != map_x2net_connection_.end()) {
							X2NetRtpConnection* x2NetRtpConn = (X2NetRtpConnection*)map_x2net_connection_[nSsrc];
							x2NetRtpConn->RecvRtcpData(x2NetData);
						}
						bFind = true;
					}break;
					}
					if (bFind) {
						break;
					}
				}
			}
		}
		else {
			break;
		}
	}
	
	MapX2NetConnection::iterator itnr = map_x2net_connection_.begin();
	while (itnr != map_x2net_connection_.end()) {
		X2NetRtpConnection* x2NetRtpConn = (X2NetRtpConnection*)itnr->second;
		if (x2NetRtpConn->RunOnce() != 0) {
			itnr = map_x2net_connection_.erase(itnr);
			x2NetRtpConn->DeInit();

			JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
			list_x2net_connection_for_delete_.push_back(x2NetRtpConn);
		}
		else {
			itnr++;
		}
	}

	return 0;
}

//* For X2NetProcess::Listener
void X2NetRtpProcess::OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection)
{
	if (x2NetType == X2Net_Udp) {
		if (x2net_rtp_connection_ == NULL) {
			x2net_rtp_connection_ = connection;
			if (x2net_rtp_connection_ != NULL) {
				x2net_rtp_connection_->SetListener(this);
			}
		}
		else if (x2net_rtcp_connection_ == NULL) {
			x2net_rtcp_connection_ = connection;
			if (x2net_rtcp_connection_ != NULL) {
				x2net_rtcp_connection_->SetListener(this);
			}
		}
	}
	
}

//* For X2NetConnection::Listener
void X2NetRtpProcess::OnX2NetConnectionPacketReceived(
	X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{	
	JMutexAutoLock l(cs_list_recv_data_);
	list_recv_data_.push_back(x2NetData);
}
void X2NetRtpProcess::OnX2NetConnectionClosed(X2NetConnection* connection)
{

}

}	// namespace x2rtc
