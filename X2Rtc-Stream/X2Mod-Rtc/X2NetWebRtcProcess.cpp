#include "X2NetWebRtcProcess.h"
#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#ifdef ENABLE_WEBRTC
#include "DepLibWebRTC.hpp"
#endif
#include "DepOpenSSL.hpp"
#ifdef ENABLE_SCTP
#include "DepUsrSCTP.hpp"
#endif
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/SrtpSession.hpp"
#include "Log.hpp"
#include "sdp/UtilsC.hpp"
#include "sdp/ortc.hpp"
#include "sdp/RemoteSdp.hpp"
#include "http_parser.h"
#include "X2UrlParser.h"
#include "X2RtcSdp.h"
#include "XUtil.h"
#ifndef _WIN32
# include <unistd.h>  /* close */
#else
#include <fcntl.h>
#include <io.h>
#endif
#define MS_CLASS "WebRtc-Process"
using namespace mediasoup;

static void gen_random(char* s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}

static int ConsumerChannelFd[2] = { 3,4 };
static int ProducerChannelFd[2] = { 5,6 };
extern int mpipe(int* fds);
#ifndef _WIN32
static int mpipe(int* fds) {
	if (pipe(fds) == -1)
		return -1;
	if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 ||
		fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1) {
		close(fds[0]);
		close(fds[1]);
		return -1;
	}
	return 0;
}
#else
static int mpipe(int* fds) {
	SECURITY_ATTRIBUTES attr;
	HANDLE readh, writeh;
	attr.nLength = sizeof(attr);
	attr.lpSecurityDescriptor = NULL;
	attr.bInheritHandle = FALSE;
	if (!CreatePipe(&readh, &writeh, &attr, 0))
		return -1;
	int rd = _open_osfhandle((intptr_t)readh, 0);
	fds[0] = rd;
	int wt = _open_osfhandle((intptr_t)writeh, 0);
	fds[1] = wt;
	if (fds[0] == -1 || fds[1] == -1) {
		CloseHandle(readh);
		CloseHandle(writeh);
		return -1;
	}
	return 0;
}
#endif

extern std::string gStrExtenIp;

namespace x2rtc {

X2NetProcess* createX2NetWebRtcProcess()
{
	return new X2NetWebRtcProcess();
}

struct X2Http
{
	std::string strUrl;
	std::map<std::string, std::string>mapHeaders;
	std::string strBody;

	std::string strLastHdr;
};

static int http_on_url(http_parser* httpParser, const char* at, size_t length)
{
	X2Http* x2Http = (X2Http*)httpParser->data;
	x2Http->strUrl.append(at, length);

	return 0;
}

static int http_on_header_field(http_parser* httpParser, const char* at, size_t length)
{
	X2Http* x2Http = (X2Http*)httpParser->data;
	x2Http->strLastHdr.append(at, length);

	return 0;
}

static int http_on_header_value(http_parser* httpParser, const char* at, size_t length)
{
	X2Http* x2Http = (X2Http*)httpParser->data;
	std::string strVal;
	strVal.append(at, length);
	x2Http->mapHeaders[x2Http->strLastHdr] = strVal;
	x2Http->strLastHdr.clear();

	return 0;
}
static int http_on_body(http_parser* httpParser, const char* at, size_t length)
{
	X2Http* x2Http = (X2Http*)httpParser->data;
	x2Http->strBody.append(at, length);

	return 0;
}

X2NetWebRtcProcess::X2NetWebRtcProcess(void)
	: b_init_(false)
	, n_id_idx_(0)
	, tcp_server_(NULL)
	, n_listen_port_(0)
{

}
X2NetWebRtcProcess::~X2NetWebRtcProcess(void)
{

}

int X2NetWebRtcProcess::Init(int nPort)
{
	if (!b_init_) {
		b_init_ = true;
		if (nPort != 0) {
			n_listen_port_ = nPort;
			UvThread::SetRtcSinglePort(nPort);
		}
		UvThread::StartThread(2);
		SetExternIp(gStrExtenIp.c_str());	// "127.0.0.1,192.168.1.130"
		SetExternPort(nPort);
	}
	return 0;
}
int X2NetWebRtcProcess::DeInit()
{
	if (b_init_) {
		b_init_ = false;
		UvThread::StopThread();

		{
			MapX2NetConnection::iterator itnr = map_x2net_connection_.begin();
			while (itnr != map_x2net_connection_.end()) {
				X2NetWebRtcConnection* x2NetSrtConn = (X2NetWebRtcConnection*)itnr->second;
				x2NetSrtConn->DeInit();

				itnr = map_x2net_connection_.erase(itnr);
				JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
				list_x2net_connection_for_delete_.push_back(x2NetSrtConn);
			}
		}
	}
	return 0;
}

int X2NetWebRtcProcess::SetParameter(const char* strJsParams)
{
	return 0;
}

void X2NetWebRtcProcess::SendRequset(const std::string& strMethod, const std::string& strHandlerId, const std::string& strData)
{
	uint32_t nReqId = ++n_req_id_;

	if (channel_local_ != nullptr) {
		//channel_local->Send(req);
		char strId[64];
		sprintf(strId, "%u", nReqId);
		std::string chReq;
		chReq.append(strId);
		chReq.append(":");
		chReq.append(strMethod);
		chReq.append(":");
		chReq.append(strHandlerId);
		chReq.append(":");
		chReq.append(strData);

		channel_local_->SendLog((char*)chReq.c_str(), chReq.length());
	}
}

//* For UvThread
void X2NetWebRtcProcess::OnUvThreadStart()
{
	{
		mpipe(ConsumerChannelFd);
		MS_lOGD("pipe Create Pair ConsumerChannelFd[0]=%d ConsumerChannelFd[1]=%d ", ConsumerChannelFd[0], ConsumerChannelFd[1]);
		mpipe(ProducerChannelFd);
		MS_lOGD("pipe Create Pair ProducerChannelFd[0]=%d ProducerChannelFd[1]=%d ", ProducerChannelFd[0], ProducerChannelFd[1]);
	}
	try
	{
		channel_rtc_.reset(new Channel::ChannelSocket(ConsumerChannelFd[0], ProducerChannelFd[1]));
		channel_local_.reset(new Channel::ChannelPSocket(ProducerChannelFd[0], ConsumerChannelFd[1]));

		//payload_channel_.reset(new PayloadChannel::PayloadChannelSocket(ProducerChannelFd[0], ConsumerChannelFd[1]));
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("error creating the Channel: %s", error.what());

		DepLibUV::RunLoop();
		DepLibUV::ClassDestroy();

		return;
	}
	channel_rtc_->SetListener(this);
	if (channel_local_ != nullptr) {
		channel_local_->SetListener(this);
	}
	if (payload_channel_ != nullptr) {
		payload_channel_->SetListener(this);
	}

	// Initialize static stuff.
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
#ifdef ENABLE_SCTP
	DepUsrSCTP::ClassInit();
#endif
#ifdef ENABLE_WEBRTC
	DepLibWebRTC::ClassInit();
#endif
	Utils::Crypto::ClassInit();
	RTC::DtlsTransport::ClassInit();
	RTC::SrtpSession::ClassInit();

	// Set up the RTC::Shared singleton.
	this->rtc_shared_.reset(new RTC::Shared(
		/*channelMessageRegistrator*/ new ChannelMessageRegistrator(),
		/*channelNotifier*/ new Channel::ChannelNotifier(this->channel_rtc_.get()),
		/*payloadChannelNotifier*/ new PayloadChannel::PayloadChannelNotifier(this->payload_channel_.get())));

	// Create the Checker instance in DepUsrSCTP.
#ifdef ENABLE_SCTP
	DepUsrSCTP::CreateChecker();
#endif

	if (tcp_server_ == NULL) {
		std::string strIp = "0.0.0.0";
		tcp_server_ = new RTC::TcpServer(this, this, strIp, n_listen_port_);
	}
}
void X2NetWebRtcProcess::OnUvThreadStop()
{
	channel_rtc_ = nullptr;
	channel_local_ = nullptr;
	payload_channel_ = nullptr;
	rtc_shared_ = nullptr;

	if (tcp_server_ != NULL) {
		tcp_server_->Close();
		delete tcp_server_;
		tcp_server_ = NULL;
	}

	// Free static stuff.
	DepLibSRTP::ClassDestroy();
	Utils::Crypto::ClassDestroy();
#ifdef ENABLE_WEBRTC
	DepLibWebRTC::ClassDestroy();
#endif
	RTC::DtlsTransport::ClassDestroy();
#ifdef ENABLE_SCTP
	DepUsrSCTP::ClassDestroy();
#endif

	{
		// Close the Checker instance in DepUsrSCTP.
#ifdef ENABLE_SCTP
		DepUsrSCTP::CloseChecker();
#endif
	}
}
void X2NetWebRtcProcess::OnUvThreadTimer()
{
#if 0
	static bool gII = true;
	if (!gII) {
		gII = true;
		const char* strSdp = "v=0\r\no=- 1269333315708051109 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\na=group:BUNDLE audio video\r\na=msid-semantic: WMS 21255e45-2d6f-458f-b51d-6c123b254a48\r\nm=audio 9 UDP/TLS/RTP/SAVPF 111\r\nc=IN IP4 0.0.0.0\r\na=rtcp:9 IN IP4 0.0.0.0\r\na=ice-ufrag:vgJV\r\na=ice-pwd:Mxf3d14uXEjqmgFHqfI8y9nU\r\na=ice-options:trickle\r\na=fingerprint:sha-256 1B:11:7C:48:58:48:3A:0C:08:6C:8C:0F:5C:72:BD:29:02:BD:A1:A5:14:32:EF:37:78:DB:B5:68:A2:3D:ED:8A\r\na=setup:active\r\na=mid:audio\r\na=extmap:2 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\na=extmap:4 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\na=extmap:7 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=sendonly\r\na=msid:21255e45-2d6f-458f-b51d-6c123b254a48 36868d5e-a21f-43b6-b964-ccc16c8365a0\r\na=rtcp-mux\r\na=rtpmap:111 opus/48000/2\r\na=rtcp-fb:111 transport-cc\r\na=fmtp:111 minptime=10;useinbandfec=1\r\na=ssrc:1602178519 cname:jCeE861ZBhBt6uFZ\r\na=ssrc:1602178519 msid:21255e45-2d6f-458f-b51d-6c123b254a48 36868d5e-a21f-43b6-b964-ccc16c8365a0\r\nm=video 9 UDP/TLS/RTP/SAVPF 96 97\r\nc=IN IP4 0.0.0.0\r\na=rtcp:9 IN IP4 0.0.0.0\r\na=ice-ufrag:vgJV\r\na=ice-pwd:Mxf3d14uXEjqmgFHqfI8y9nU\r\na=ice-options:trickle\r\na=fingerprint:sha-256 1B:11:7C:48:58:48:3A:0C:08:6C:8C:0F:5C:72:BD:29:02:BD:A1:A5:14:32:EF:37:78:DB:B5:68:A2:3D:ED:8A\r\na=setup:active\r\na=mid:video\r\na=extmap:3 urn:ietf:params:rtp-hdrext:toffset\r\na=extmap:4 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\na=extmap:7 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:9 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\na=extmap:10 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\na=extmap:11 http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\na=sendonly\r\na=msid:21255e45-2d6f-458f-b51d-6c123b254a48 2fcfb7a4-8bfb-464e-8a5c-59a53818c440\r\na=rtcp-mux\r\na=rtpmap:96 H264/90000\r\na=rtcp-fb:96 goog-remb\r\na=rtcp-fb:96 transport-cc\r\na=rtcp-fb:96 ccm fir\r\na=rtcp-fb:96 nack\r\na=rtcp-fb:96 nack pli\r\na=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\na=rtpmap:97 rtx/90000\r\na=fmtp:97 apt=96\r\na=ssrc-group:FID 1472902993 1982055093\r\na=ssrc:1472902993 cname:jCeE861ZBhBt6uFZ\r\na=ssrc:1472902993 msid:21255e45-2d6f-458f-b51d-6c123b254a48 2fcfb7a4-8bfb-464e-8a5c-59a53818c440\r\na=ssrc:1982055093 cname:jCeE861ZBhBt6uFZ\r\na=ssrc:1982055093 msid:21255e45-2d6f-458f-b51d-6c123b254a48 2fcfb7a4-8bfb-464e-8a5c-59a53818c440\r\n";
		json jsReq;
		jsReq["Cmd"] = "Subscribe";// "Publish";
		jsReq["Args"] = "app=1212&stream=890xxx&chid=121&chsid=xxx&token=xxx&type=publish";
		jsReq["Ice"]["uname"] = "a9hkjfh3";
		jsReq["Ice"]["pwd"] = "78943d";
		jsReq["Sdp"] = strSdp;
		while (1) {
			uint32_t nId = n_id_idx_++;
			if (map_x2net_connection_.find(nId) == map_x2net_connection_.end()) {
				char strId[64];
				sprintf(strId, "%u", nId);
				jsReq["Id"] = strId;
				X2NetWebRtcConnection* x2NetWebRtcConn = new X2NetWebRtcConnection();
				x2NetWebRtcConn->SetCallback(this);
				map_x2net_connection_[nId] = x2NetWebRtcConn;
				std::string strAnswer;
				if (x2NetWebRtcConn->Init(rtc_shared_.get(), jsReq, strAnswer) != 0) {
					map_x2net_connection_.erase(nId);
					delete x2NetWebRtcConn;
				}
				else {
					if (cb_listener_ != NULL) {
						cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtc, x2NetWebRtcConn);
					}
				}
				break;
			}	
			uv_sleep(1);
		}
	}
#endif

	MapX2NetConnection::iterator itnr = map_x2net_connection_.begin();
	while (itnr != map_x2net_connection_.end()) {
		X2NetWebRtcConnection* x2NetRtcConn = (X2NetWebRtcConnection*)itnr->second;
		x2NetRtcConn->RunOnce();
		itnr++;
	}
}

//* For Channel::ChannelSocket::Listener
void X2NetWebRtcProcess::HandleRequest(Channel::ChannelRequest* request)
{
	std::string strId = request->handlerId;
	uint32_t nId = atoi(strId.c_str());
	if (map_x2net_connection_.find(nId) != map_x2net_connection_.end()) {
		((X2NetWebRtcConnection*)map_x2net_connection_[nId])->Transport()->HandleRequest(request);
	}
	
}
void X2NetWebRtcProcess::OnChannelClosed(Channel::ChannelSocket* channel)
{

}

//* For Channel::ChannelPSocket::Listener
void X2NetWebRtcProcess::OnChannelPResponse(
	Channel::ChannelPSocket* channel, json* response)
{//{"accepted":true,"data":{"dtlsLocalRole":"server"},"id":0}
	//printf("OnChannelPResponse: %s\r\n", response->dump().c_str());

	if (response->find("id") != response->end()) {
		uint32_t idd = (*response)["id"];
		if (map_rtc_request_waitfor_response_.find(idd) != map_rtc_request_waitfor_response_.end()) {
			RtcRequest& rtcReq = map_rtc_request_waitfor_response_[idd];
			rtcReq.x2NetWebRtcConn->RecvResponse(rtcReq.strMethod, (*response));
			map_rtc_request_waitfor_response_.erase(idd);
		}
	}
	else {
		auto itt = response->find("targetId");
		if (itt != response->end())
		{
			std::string strId = itt->get<std::string>();
			uint32_t nId = atoi(strId.c_str());
			if (map_x2net_connection_.find(nId) != map_x2net_connection_.end()) {
				X2NetWebRtcConnection* x2NetWebRtcConn = (X2NetWebRtcConnection*)map_x2net_connection_[nId];
				x2NetWebRtcConn->RecvEvent((*response));
			}
		}
	}

}
void X2NetWebRtcProcess::OnChannelPClosed(Channel::ChannelPSocket* channel)
{

}

void X2NetWebRtcProcess::HandleRequest(PayloadChannel::PayloadChannelRequest* request)
{
	
}
void X2NetWebRtcProcess::HandleNotification(PayloadChannel::PayloadChannelNotification* notification)
{

}
void X2NetWebRtcProcess::OnPayloadChannelClosed(PayloadChannel::PayloadChannelSocket* payloadChannel)
{

}

//* For X2NetWebRtcConnection::Listener
void X2NetWebRtcProcess::OnX2NetWebRtcConnectionSendRequest(X2NetWebRtcConnection* x2NetWebRtcConn, const std::string& strMethod, const std::string& strId, const std::string& strData)
{
	uint32_t nReqId = n_req_id_++;
	map_rtc_request_waitfor_response_[nReqId].strMethod = strMethod;
	map_rtc_request_waitfor_response_[nReqId].x2NetWebRtcConn = x2NetWebRtcConn;

	if (channel_local_ != nullptr) {
		char strReqId[64];
		sprintf(strReqId, "%u", nReqId);
		std::string chReq;
		chReq.append(strReqId);
		chReq.append(":");
		chReq.append(strMethod);
		chReq.append(":");
		chReq.append(strId);
		chReq.append(":");
		chReq.append(strData);

		channel_local_->SendLog((char*)chReq.c_str(), chReq.length());
	}
}

void X2NetWebRtcProcess::OnX2NetWebRtcConnectionLostConnection(X2NetWebRtcConnection* x2NetWebRtcConn, const std::string& strId)
{
	uint32_t nId = atoi(strId.c_str());
	map_x2net_connection_.erase(nId);
	{
		JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
		list_x2net_connection_for_delete_.push_back(x2NetWebRtcConn);
	}
	std::map<uint32_t, RtcRequest>::iterator itrr = map_rtc_request_waitfor_response_.begin();
	while (itrr != map_rtc_request_waitfor_response_.end()) {
		if (itrr->second.x2NetWebRtcConn == x2NetWebRtcConn) {
			itrr = map_rtc_request_waitfor_response_.erase(itrr);
		}
		else {
			itrr++;
		}
	}
}

void X2NetWebRtcProcess::OnX2NetWebRtcConnectionAddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword, ClientIpListener* ipListener)
{
	UvThread::RtcSinglePortPtr()->AddRemoteIdToLocal(strRemoteId, nLocalPort, strPassword, ipListener);
}
void X2NetWebRtcProcess::OnX2NetWebRtcConnectionRemoveRemoteId(const std::string& strRemoteId)
{
	UvThread::RtcSinglePortPtr()->RemoveRemoteId(strRemoteId);
}

//* For RTC::TcpServer::Listener
void X2NetWebRtcProcess::OnRtcTcpConnectionAccept(RTC::TcpServer* tcpServer, RTC::TcpConnection** connection)
{
	// Allocate a new RTC::TcpConnection for the TcpServer to handle it.
	*connection = new X2WhipTcpConnection(this, 65535);
}
void X2NetWebRtcProcess::OnRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection)
{

}

//* For RTC::TcpConnection::Listener
int X2NetWebRtcProcess::OnTcpConnectionPacketProcess(RTC::TcpConnection* connection, const uint8_t* data, size_t len)
{
	X2Http x2Http;
	http_parser httpParser;
	httpParser.data = &x2Http;
	http_parser_init(&httpParser, HTTP_REQUEST);
	http_parser_settings httpSettins;
	http_parser_settings_init(&httpSettins);
	httpSettins.on_url = http_on_url;
	httpSettins.on_header_field = http_on_header_field;
	httpSettins.on_header_value = http_on_header_value;
	httpSettins.on_body = http_on_body;
	size_t parserLen = http_parser_execute(&httpParser, &httpSettins, (char*)data, len);
	if (parserLen <= 0) {
		connection->Close();
	}
	else {
		if (httpParser.method == http_method::HTTP_OPTIONS) {
			char strResp[1024];
			int ret = http_gen_response(strResp, 200, 0, 1, "application/x-www-form-urlencoded");
			printf("Http resp: %s \r\n", strResp);
			((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, NULL, 0);
			return parserLen;
		}
		printf("Recv url: %s sdp: %s\r\n", x2Http.strUrl.c_str(), x2Http.strBody.c_str());
		if (httpParser.content_length != 0 && x2Http.strBody.length() != httpParser.content_length) {
			return 0;
		}

		int nBodyLen = 0;
		if (x2Http.mapHeaders.find("Content-Length") != x2Http.mapHeaders.end()) {
			nBodyLen = atoi(x2Http.mapHeaders["Content-Length"].c_str());
		}
		if (nBodyLen != x2Http.strBody.length()) {
			return 0;
		}

		if (x2Http.strUrl.find("/x2rtc/v1/pushstream") != std::string::npos) {
			json jsSdp = json::parse(x2Http.strBody);
			if (jsSdp.find("localsdp") != jsSdp.end()) {
				std::string strStreamUrl = jsSdp["streamurl"];
				X2UrlParser x2UrlParser;
				int ret = x2UrlParser.parse(strStreamUrl);
				std::string strApp;
				std::string strStream;
				if (x2UrlParser.path().length() > 0) {
					std::vector<std::string> arrParam;
					XSplitChar(x2UrlParser.path().c_str() + 1, '/', &arrParam);
					if (arrParam.size() > 0) {
						strApp = arrParam[0];
					}
					if (arrParam.size() > 1) {
						strStream = arrParam[1];
					}
				}
				if (strApp.length() == 0 || strStream.length() == 0) {
					json jsAnswer;
					jsAnswer["errcode"] = -1000;
					jsAnswer["reason"] = "Url format error";
					std::string strSdpAnswer = jsAnswer.dump();
					char strResp[1024];
					int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
					((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());
					return parserLen;
				}

				std::string strChanId = "x2rtc";
				std::string strToken;
				if (x2UrlParser.query().length() > 0) {
					std::map<std::string, std::string> mapPrama;
					XParseHttpParam(x2UrlParser.query(), &mapPrama);
					if (mapPrama.find("chanId") != mapPrama.end()) {
						strChanId = mapPrama["chanId"];
					}
					if (mapPrama.find("token") != mapPrama.end()) {
						strToken = mapPrama["token"];
					}
				}
				json jsLocalSdp = jsSdp["localsdp"];
				if (jsLocalSdp["type"] == "offer") {
					char strArgs[2048];
					int nPos = sprintf(strArgs, "app=%s&stream=%s&chanId=%s&type=publish", strApp.c_str(), strStream.c_str(), strChanId.c_str());
					if (strToken.length() > 0) {
						nPos += sprintf(strArgs + nPos, "&token=%s", strToken.c_str());
					}
					json jsReq;
					jsReq["Cmd"] = "Publish";// "Publish";
					jsReq["Args"] = strArgs;
					jsReq["Ice"]["uname"] = Utils::Crypto::GetRandomString(16);
					jsReq["Ice"]["pwd"] = Utils::Crypto::GetRandomString(32);
					jsReq["Sdp"] = jsLocalSdp["sdp"];
					while (1) {
						uint32_t nId = n_id_idx_++;
						if (map_x2net_connection_.find(nId) == map_x2net_connection_.end()) {
							char strId[64];
							sprintf(strId, "%u", nId);
							jsReq["Id"] = strId;
							X2NetWebRtcConnection* x2NetWebRtcConn = new X2NetWebRtcConnection();
							x2NetWebRtcConn->SetCallback(this);
							map_x2net_connection_[nId] = x2NetWebRtcConn;
							std::string strAnswer;
							if (x2NetWebRtcConn->Init(rtc_shared_.get(), jsReq, strAnswer) != 0) {
								map_x2net_connection_.erase(nId);
								delete x2NetWebRtcConn;
							}
							else {
								printf("Resp sdp: %s\r\n", strAnswer.c_str());
								json jsAnswer;
								jsAnswer["errcode"] = 0;
								jsAnswer["remotesdp"]["type"] = "answer";
								jsAnswer["remotesdp"]["sdp"] = strAnswer;
								std::string strSdpAnswer = jsAnswer.dump();
								char strResp[1024];
								int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
								((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());

								if (cb_listener_ != NULL) {
									cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtc, x2NetWebRtcConn);
								}
							}
							break;
						}
						uv_sleep(1);
					}
				}
			}
		}
		else if (x2Http.strUrl.find("/x2rtc/v1/pullstream") != std::string::npos) {
			json jsSdp = json::parse(x2Http.strBody);
			if (jsSdp.find("localsdp") != jsSdp.end()) {
				std::string strStreamUrl = jsSdp["streamurl"];
				X2UrlParser x2UrlParser;
				int ret = x2UrlParser.parse(strStreamUrl);
				std::string strApp;
				std::string strStream;
				if (x2UrlParser.path().length() > 0) {
					std::vector<std::string> arrParam;
					XSplitChar(x2UrlParser.path().c_str() + 1, '/', &arrParam);
					if (arrParam.size() > 0) {
						strApp = arrParam[0];
					}
					if (arrParam.size() > 1) {
						strStream = arrParam[1];
					}
				}
				if (strApp.length() == 0 || strStream.length() == 0) {
					json jsAnswer;
					jsAnswer["errcode"] = -1000;
					jsAnswer["reason"] = "Url format error";
					std::string strSdpAnswer = jsAnswer.dump();
					char strResp[1024];
					int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
					((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());
					return parserLen;
				}

				std::string strChanId = "x2rtc";
				std::string strToken;
				std::string strScale;
				if (x2UrlParser.query().length() > 0) {
					std::map<std::string, std::string> mapPrama;
					XParseHttpParam(x2UrlParser.query(), &mapPrama);
					if (mapPrama.find("chanId") != mapPrama.end()) {
						strChanId = mapPrama["chanId"];
					}
					if (mapPrama.find("token") != mapPrama.end()) {
						strToken = mapPrama["token"];
					}
					if (mapPrama.find("scale") != mapPrama.end()) {
						strScale = mapPrama["scale"];
					}
				}
				json jsLocalSdp = jsSdp["localsdp"];
				if (jsLocalSdp["type"] == "offer") {
					char strArgs[2048];
					int nPos = sprintf(strArgs, "app=%s&stream=%s&chanId=%s&type=play", strApp.c_str(), strStream.c_str(), strChanId.c_str());
					if (strToken.length() > 0) {
						nPos += sprintf(strArgs + nPos, "&token=%s", strToken.c_str());
					}
					if (strScale.length() > 0) {
						nPos += sprintf(strArgs + nPos, "&scale=%s", strScale.c_str());
					}
					if (x2UrlParser.query().find("useAAC=true") != std::string::npos) {
						nPos += sprintf(strArgs + nPos, "&useAAC=1");
					}
					if (x2UrlParser.query().find("cacheGop=true") != std::string::npos) {
						nPos += sprintf(strArgs + nPos, "&cacheGop=1");
					}
					else {
						nPos += sprintf(strArgs + nPos, "&cacheGop=0");
					}
					nPos += sprintf(strArgs + nPos, "&selfCtrl=1");

					json jsReq;
					jsReq["Cmd"] = "Play";// 
					jsReq["Args"] = strArgs;
					jsReq["Ice"]["uname"] = Utils::Crypto::GetRandomString(16);
					jsReq["Ice"]["pwd"] = Utils::Crypto::GetRandomString(32);
					jsReq["Sdp"] = jsLocalSdp["sdp"];
					while (1) {
						uint32_t nId = n_id_idx_++;
						if (map_x2net_connection_.find(nId) == map_x2net_connection_.end()) {
							char strId[64];
							sprintf(strId, "%u", nId);
							jsReq["Id"] = strId;
							X2NetWebRtcConnection* x2NetWebRtcConn = new X2NetWebRtcConnection();
							x2NetWebRtcConn->SetCallback(this);
							map_x2net_connection_[nId] = x2NetWebRtcConn;
							if (x2UrlParser.query().find("useH265=true") != std::string::npos) {
								x2NetWebRtcConn->SetUseH265(true);
							}
							if (x2UrlParser.query().find("useAAC=true") != std::string::npos) {
								x2NetWebRtcConn->SetUseAAC(true);
							}
							std::string strAnswer;
							if (x2NetWebRtcConn->Init(rtc_shared_.get(), jsReq, strAnswer) != 0) {
								map_x2net_connection_.erase(nId);
								delete x2NetWebRtcConn;
							}
							else {
								printf("Resp sdp: %s\r\n", strAnswer.c_str());
								json jsAnswer;
								jsAnswer["errcode"] = 0;
								jsAnswer["remotesdp"]["type"] = "answer";
								jsAnswer["remotesdp"]["sdp"] = strAnswer;
								std::string strSdpAnswer = jsAnswer.dump();
								char strResp[1024];
								int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
								((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());

								if (cb_listener_ != NULL) {
									cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtc, x2NetWebRtcConn);
								}
							}
							break;
						}
						uv_sleep(1);
					}
				}
			}
		}
		else if (x2Http.strUrl.find("/rtc/v1/play") != std::string::npos) {
			json jsSdp = json::parse(x2Http.strBody);
			if (jsSdp.find("sdp") != jsSdp.end()  && jsSdp.find("streamurl") != jsSdp.end()) {
				std::string strStreamUrl = jsSdp["streamurl"];
				X2UrlParser x2UrlParser;
				int ret = x2UrlParser.parse(strStreamUrl);
				std::string strApp;
				std::string strStream;
				if (x2UrlParser.path().length() > 0) {
					std::vector<std::string> arrParam;
					XSplitChar(x2UrlParser.path().c_str() + 1, '/', &arrParam);
					if (arrParam.size() > 0) {
						strApp = arrParam[0];
					}
					if (arrParam.size() > 1) {
						strStream = arrParam[1];
					}
				}
				if (strApp.length() == 0 || strStream.length() == 0) {
					json jsAnswer;
					jsAnswer["errcode"] = -1000;
					jsAnswer["reason"] = "Url format error";
					std::string strSdpAnswer = jsAnswer.dump();
					char strResp[1024];
					int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
					((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());
					return parserLen;
				}

				std::string strChanId = "x2rtc";
				std::string strToken;
				std::string strScale;
				if (x2UrlParser.query().length() > 0) {
					std::map<std::string, std::string> mapPrama;
					XParseHttpParam(x2UrlParser.query(), &mapPrama);
					if (mapPrama.find("chanId") != mapPrama.end()) {
						strChanId = mapPrama["chanId"];
					}
					if (mapPrama.find("token") != mapPrama.end()) {
						strToken = mapPrama["token"];
					}
					if (mapPrama.find("scale") != mapPrama.end()) {
						strScale = mapPrama["scale"];
					}
				}

				json& jsLocalSdp = jsSdp;
				{
					json jsReq;
					char strArgs[2048];
					int nPos = sprintf(strArgs, "app=%s&stream=%s&chanId=%s&type=play", strApp.c_str(), strStream.c_str(), strChanId.c_str());
					if (strToken.length() > 0) {
						nPos += sprintf(strArgs + nPos, "&token=%s", strToken.c_str());
					}
					if (strScale.length() > 0) {
						nPos += sprintf(strArgs + nPos, "&scale=%s", strScale.c_str());
					}
					if (x2UrlParser.query().find("useAAC=true") != std::string::npos) {
						nPos += sprintf(strArgs + nPos, "&useAAC=1");
					}
					if (x2UrlParser.query().find("cacheGop=true") != std::string::npos) {
						nPos += sprintf(strArgs + nPos, "&cacheGop=1");
					}
					else {
						nPos += sprintf(strArgs + nPos, "&cacheGop=0");
					}
					nPos += sprintf(strArgs + nPos, "&selfCtrl=1");

					jsReq["Cmd"] = "Play";// 
					jsReq["Args"] = strArgs;
					jsReq["Ice"]["uname"] = Utils::Crypto::GetRandomString(16);
					jsReq["Ice"]["pwd"] = Utils::Crypto::GetRandomString(32);
					jsReq["Sdp"] = jsLocalSdp["sdp"];
					while (1) {
						uint32_t nId = n_id_idx_++;
						if (map_x2net_connection_.find(nId) == map_x2net_connection_.end()) {
							char strId[64];
							sprintf(strId, "%u", nId);
							jsReq["Id"] = strId;
							X2NetWebRtcConnection* x2NetWebRtcConn = new X2NetWebRtcConnection();
							x2NetWebRtcConn->SetCallback(this);
							map_x2net_connection_[nId] = x2NetWebRtcConn;
							if (x2UrlParser.query().find("useH265=true") != std::string::npos) {
								x2NetWebRtcConn->SetUseH265(true);
							}
							if (x2UrlParser.query().find("useAAC=true") != std::string::npos) {
								x2NetWebRtcConn->SetUseAAC(true);
							}
							std::string strAnswer;
							if (x2NetWebRtcConn->Init(rtc_shared_.get(), jsReq, strAnswer) != 0) {
								map_x2net_connection_.erase(nId);
								delete x2NetWebRtcConn;
							}
							else {
								printf("Resp sdp: %s\r\n", strAnswer.c_str());
								json jsAnswer;
								jsAnswer["code"] = 0;
								jsAnswer["sdp"] = strAnswer;
								std::string strSdpAnswer = jsAnswer.dump();
								char strResp[1024];
								int ret = http_gen_response(strResp, 200, strSdpAnswer.length(), 1, "application/x-www-form-urlencoded");
								((X2WhipTcpConnection*)connection)->SendResponse((uint8_t*)strResp, ret, (uint8_t*)strSdpAnswer.c_str(), strSdpAnswer.length());

								if (cb_listener_ != NULL) {
									cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtc, x2NetWebRtcConn);
								}
							}
							break;
						}
						uv_sleep(1);
					}
				}
			}
		}
		else {
			connection->Close();
		}
		return parserLen;
	}

	return len;
}


}	// namespace x2rtc