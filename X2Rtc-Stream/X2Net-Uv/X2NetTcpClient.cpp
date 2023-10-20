#include "X2NetTcpClient.h"
#include "X2PortManager.h"
#include "X2Utils.hpp"
#include "X2MediaSoupErrors.hpp"
#include "X2RpcConnection.hpp"
#include "XUtil.h"
#include "X2RtcLog.h"
#include "X2RtcCheck.h"
#include "Libuv.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"

#define KEEP_ALIVE_TIME 10000	// 10s

namespace x2rtc {

static void cb_async_free_rpc_connection(uS::Async* async) {
	if (async->getData() != NULL) {
		X2RpcConnection* x2RpcConn = (X2RpcConnection*)async->getData();
		delete x2RpcConn;
		async->setData(NULL);
	}

	async->close();
}
class X2NetTcpClientImpl : public X2NetTcpClient, public X2RpcConnection::Listener, public X2TcpConnectionHandler::Listener
{
public:
	X2NetTcpClientImpl(void);
	virtual ~X2NetTcpClientImpl(void);

	virtual int Connect(const std::string& strIp, int nPort);
	virtual int ReConnect();
	virtual int DisConnect();
	virtual int SendData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
	virtual int RunOnce();

	void SetConnectRlt(int nCode);

	//* For X2RpcConnection::Listener
	virtual void OnTcpConnectionPacketReceived(
		x2rtc::X2RpcConnection* connection, const uint8_t* data, size_t len);

	//* For X2TcpConnectionHandler::Listener
	virtual void OnTcpConnectionClosed(X2TcpConnectionHandler* connection);

private:
	X2RpcConnection* x2tcp_connection_;

	bool b_connected_;
	int64_t n_next_keepalive_time_;

	std::string str_svr_ip_;
	int n_svr_port_;

private:
	JMutex cs_list_send_data_;
	std::list< x2rtc::scoped_refptr<X2NetDataRef>> list_send_data_;
};

X2NetTcpClient* createX2NetTcpClient()
{
	return new X2NetTcpClientImpl();
}

void X2NetGetAddressInfo(const struct sockaddr* addr, int* family, std::string& ip, uint16_t* port)
{
	X2Utils::IP::GetAddressInfo(addr, *family, ip, *port);
}
bool X2NetCompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2)
{
	return X2Utils::IP::CompareAddresses(addr1, addr2);
}

static void on_uv_connect_cb(uv_connect_t* req, int status)
{
	if (req->data != NULL)
	{
		X2NetTcpClientImpl* x2NetTcpClient = (X2NetTcpClientImpl*)req->data;
		x2NetTcpClient->SetConnectRlt(status);
	}
	
	free(req);
}

X2NetTcpClientImpl::X2NetTcpClientImpl(void)
	: x2tcp_connection_(NULL)
	, b_connected_(false)
	, n_next_keepalive_time_(0)
	, n_svr_port_(0)
{

}
X2NetTcpClientImpl::~X2NetTcpClientImpl(void)
{
	X2RTC_CHECK(x2tcp_connection_ == NULL);
}

int X2NetTcpClientImpl::Connect(const std::string& strIp, int nPort)
{
	if (x2tcp_connection_ == NULL) {
		str_svr_ip_ = strIp;
		n_svr_port_ = nPort;

		return ReConnect();
	}
	
	return 0;
}

int X2NetTcpClientImpl::ReConnect()
{
	if (n_svr_port_ == 0 || str_svr_ip_.length() == 0) {
		return -1;
	}
	if (x2tcp_connection_ == NULL) {
		struct sockaddr_storage localAddr;
		std::string localIp = "0.0.0.0";
		uint16_t localPort = 0;

		x2tcp_connection_ = new X2RpcConnection(this, 65535);
		x2tcp_connection_->SetSelfUvLoop(uv_default_loop());
		try
		{
			x2tcp_connection_->Setup(this, &localAddr, localIp, localPort);
		}
		catch (const X2MediaSoupError& error)
		{
			uS::Async* uvAsync = new uS::Async((uS::Loop*)uv_default_loop());
			uvAsync->setData(x2tcp_connection_);
			uvAsync->start(cb_async_free_rpc_connection);
			uvAsync->send();

			x2tcp_connection_ = NULL;

			X2RtcPrintf(ERR, "X2TcpConnection Setup err: %s", error.buffer);
			return -2;
		}

		try
		{
			struct sockaddr_in connect_addr;
			uv_ip4_addr(str_svr_ip_.c_str(), n_svr_port_, &connect_addr);

			uv_connect_t* uv_connect_ = NULL;
			if (uv_connect_ == NULL) {
				uv_connect_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
				uv_connect_->data = this;
			}
			int ret = uv_tcp_connect(uv_connect_, x2tcp_connection_->GetUvHandle(), reinterpret_cast<const struct sockaddr*>(&connect_addr), on_uv_connect_cb);
		}
		catch (const X2MediaSoupError& error)
		{
			uS::Async* uvAsync = new uS::Async((uS::Loop*)uv_default_loop());
			uvAsync->setData(x2tcp_connection_);
			uvAsync->start(cb_async_free_rpc_connection);
			uvAsync->send();

			x2tcp_connection_ = NULL;


			X2RtcPrintf(ERR, "X2TcpConnection Connect err: %s", error.buffer);
			return -3;
		}

		n_next_keepalive_time_ = XGetUtcTimestamp() + KEEP_ALIVE_TIME;
	}

	return 0;
}
int X2NetTcpClientImpl::DisConnect()
{
	if (x2tcp_connection_ != NULL) {
		try
		{
			x2tcp_connection_->Close();
		}
		catch (const X2MediaSoupError& error)
		{
			X2RtcPrintf(ERR, "X2TcpConnection DisConnect err: %s", error.buffer);
		}
		uS::Async* uvAsync = new uS::Async((uS::Loop*)uv_default_loop());
		uvAsync->setData(x2tcp_connection_);
		uvAsync->start(cb_async_free_rpc_connection);
		uvAsync->send();

		x2tcp_connection_ = NULL;
	}

	return 0;
}

int X2NetTcpClientImpl::SendData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData)
{
	JMutexAutoLock l(cs_list_send_data_);
	list_send_data_.push_back(x2NetData);
	return 0;
}

int X2NetTcpClientImpl::RunOnce()
{
	while (1) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		{
			JMutexAutoLock l(cs_list_send_data_);
			if (list_send_data_.size() > 0) {
				x2NetData = list_send_data_.front();
				list_send_data_.pop_front();
			}
		}
		if (x2NetData != NULL) {
			if (b_connected_ && x2tcp_connection_ != NULL) {
				x2tcp_connection_->Send((uint8_t*)x2NetData->pData, x2NetData->nLen, NULL);
			}
		}
		else {
			break;
		}
	}
	
	if (!b_connected_) {
		if (n_next_keepalive_time_ != 0 && n_next_keepalive_time_ <= XGetUtcTimestamp()) {
			n_next_keepalive_time_ = 0;

			SetConnectRlt(-2);
		}
	}

	if (n_next_keepalive_time_ != 0 && n_next_keepalive_time_ <= XGetUtcTimestamp()) {
		n_next_keepalive_time_ = XGetUtcTimestamp() + KEEP_ALIVE_TIME;
		rapidjson::Document		jsonDoc;
		rapidjson::StringBuffer jsonStr;
		rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
		jsonDoc.SetObject();
		jsonDoc.AddMember("Cmd", "KeepAlive", jsonDoc.GetAllocator());
		rapidjson::Value jsContent(rapidjson::kObjectType);
		jsContent.AddMember("NodeId", "TcpClient", jsonDoc.GetAllocator());
		jsonDoc.AddMember("Content", jsContent, jsonDoc.GetAllocator());
		jsonDoc.Accept(jsonWriter);

		if (x2tcp_connection_ != NULL) {
			x2tcp_connection_->Send((uint8_t*)jsonStr.GetString(), jsonStr.Size(), NULL);
		}
	}
	return 0;
}

void X2NetTcpClientImpl::SetConnectRlt(int nCode)
{
	if (nCode == 0) {
		try {
			x2tcp_connection_->Start();
		}
		catch (const X2MediaSoupError& error)
		{
		}

		b_connected_ = true;
		n_next_keepalive_time_ = XGetUtcTimestamp();
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2NetTcpClientConnected();
		}
	}
	else {
		bool bFailed = false;
		if (!b_connected_) {
			bFailed = true;
		}
		b_connected_ = false;
		n_next_keepalive_time_ = 0;

		try {
			x2tcp_connection_->Close();
		}
		catch (const X2MediaSoupError& error)
		{
			X2RtcPrintf(ERR, "X2TcpConnection Close err: %s", error.buffer);
		}
		
		if (cb_listener_ != NULL) {
			if (bFailed) {
				cb_listener_->OnX2NetTcpClientFailed();
			}
			else {
				cb_listener_->OnX2NetTcpClientClosed();
			}
		}
	}
}

//* For X2RpcConnection::Listener
void X2NetTcpClientImpl::OnTcpConnectionPacketReceived(
	x2rtc::X2RpcConnection* connection, const uint8_t* data, size_t len)
{
	rapidjson::Document		jsonReqDoc;
	JsonStr sprCopy((char*)data, len);
	if (!jsonReqDoc.ParseInsitu<0>(sprCopy.Ptr).HasParseError()) {
		const std::string& strCmd = GetJsonStr(jsonReqDoc, "Cmd", F_AT);
		if (strCmd.compare("KeepAlive") == 0) {
			return;
		}
	}

	if (cb_listener_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData(data, len);
		cb_listener_->OnX2NetTcpClientDataReceived(x2NetData);
	}
}

//* For X2TcpConnectionHandler::Listener
void X2NetTcpClientImpl::OnTcpConnectionClosed(X2TcpConnectionHandler* connection)
{
	SetConnectRlt(-1);
}

}	// namespace x2rtc