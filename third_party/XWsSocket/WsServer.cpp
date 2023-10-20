#include "WsServer.h"
#include <map>
#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/core.hpp>
#include <websocketpp/endpoint.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/logger/levels.hpp>

typedef websocketpp::server<websocketpp::config::asio> ws_server_t;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::mutex;
using websocketpp::lib::unique_lock;

class WsServerImpl : public WsServer
{
public:
	WsServerImpl(WsServerEvent*pEvent);
	virtual ~WsServerImpl(void);

	//* For WsServerImpl
	virtual int StartSvr(int nListenPort);
	virtual int StopSvr();
	virtual void DoTick();

	virtual void ForceClose(void* pdl, int nCode);

public:
	void OnWsOpen(websocketpp::connection_hdl hdl);
	void OnWsClose(websocketpp::connection_hdl hdl);
	void OnWsMessage(websocketpp::connection_hdl hdl, ws_server_t::message_ptr msg);
	void OnWsPing(websocketpp::connection_hdl hdl, std::string const &strMsg);

private:
	ws_server_t		*ws_server_;

	struct HdlPeer
	{
		HdlPeer(void) {
			wsClient = NULL;
		}
		websocketpp::connection_hdl hdl;
		void*wsClient;
	};
	typedef std::map<void*, HdlPeer> MapHdlWsClients;


	mutex				cs_hdl_ws_clients_;
	MapHdlWsClients		map_hdl_ws_clients_;


};

WsServer* createWsServer(WsServerEvent*pEvent)
{
	return new WsServerImpl(pEvent);
}

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
static void on_open(void* c, websocketpp::connection_hdl hdl)
{
	WsServerImpl* wsServer = (WsServerImpl*)c;
	wsServer->OnWsOpen(hdl);
}
static void on_close(void* c, websocketpp::connection_hdl hdl)
{
	WsServerImpl* wsServer = (WsServerImpl*)c;
	wsServer->OnWsClose(hdl);
}
static void on_message(void* c, websocketpp::connection_hdl hdl, ws_server_t::message_ptr msg) {
	/*std::cout << "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;*/

	WsServerImpl* wsServer = (WsServerImpl*)c;
	wsServer->OnWsMessage(hdl, msg);
}
static bool on_ping(void* c, websocketpp::connection_hdl hdl, std::string strMsg)
{
	WsServerImpl* wsServer = (WsServerImpl*)c;
	wsServer->OnWsPing(hdl, strMsg);
	return true;
}


WsServerImpl::WsServerImpl(WsServerEvent*pEvent)
	: WsServer(pEvent)
	, ws_server_(NULL)
{
	//websocketpp::config::core::elog_level = websocketpp::log::elevel::none;
	//websocketpp::config::core::alog_level = websocketpp::log::alevel::none;
}
WsServerImpl::~WsServerImpl(void)
{

}

//* For WsServerImpl
int WsServerImpl::StartSvr(int nListenPort)
{
	if (ws_server_ == NULL) {
		ws_server_ = new ws_server_t();
		ws_server_->set_access_channels(websocketpp::log::alevel::all);
		ws_server_->clear_access_channels(websocketpp::log::alevel::frame_payload);
		ws_server_->set_reuse_addr(true);

		// Initialize ASIO
		ws_server_->init_asio();

		ws_server_->set_open_handler(bind(&on_open, this, ::_1));
		ws_server_->set_close_handler(bind(&on_close, this, ::_1));
		ws_server_->set_message_handler(bind(&on_message, this, ::_1, ::_2));
		ws_server_->set_ping_handler(bind(&on_ping, this, ::_1, ::_2));

		ws_server_->listen(nListenPort);
		websocketpp::lib::error_code ec;
		ws_server_->start_accept(ec);
		if (ec) {
			delete ws_server_;
			ws_server_ = NULL;
			return -1;
		}
	}
	return 0;
}
int WsServerImpl::StopSvr()
{
	if (ws_server_ != NULL) {
		//ws_server_->
		delete ws_server_;
		ws_server_ = NULL;
	}
	return 0;
}

void WsServerImpl::DoTick()
{
	if (ws_server_ != NULL) {
		ws_server_->poll_one();
	}

	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	MapHdlWsClients::iterator ithr = map_hdl_ws_clients_.begin();
	while (ithr != map_hdl_ws_clients_.end()) {
		websocketpp::lib::error_code ec;
		ws_server_t::connection_ptr con =  ws_server_->get_con_from_hdl(ithr->second.hdl, ec);

		if (ec || con->get_buffered_amount() > 10) 
		{//* 如果缓冲区有消息没发完，则不能继续发。否则会导致堆栈堆积，信令断开
			ithr++;
			continue; 
		}


		std::string strMsg;
		if (ws_svr_event_ != NULL) {
			ws_svr_event_->OnSendMssage(ithr->second.wsClient, strMsg);
		}
		if (strMsg.length() > 0) {
			websocketpp::lib::error_code ec;

			websocketpp::connection_hdl hdl = ithr->second.hdl;
			ws_server_->send(hdl, strMsg.c_str(), websocketpp::frame::opcode::text, ec);
			if (ec) {
				std::cout << "Send failed because: " << ec.message() << std::endl;
			}
		}

		ithr++;
	}
}

void WsServerImpl::ForceClose(void* ptr, int nCode)
{
	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	MapHdlWsClients::iterator ithr = map_hdl_ws_clients_.find(ptr);
	if (ithr != map_hdl_ws_clients_.end()) {
		HdlPeer& hdlPeer = map_hdl_ws_clients_[ptr];
		websocketpp::lib::error_code ec;
		ws_server_->close(hdlPeer.hdl, nCode, "", ec);
		map_hdl_ws_clients_.erase(ithr);
	}
}

void WsServerImpl::OnWsOpen(websocketpp::connection_hdl hdl)
{
	void* ptr = hdl.lock().get();
	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	if (map_hdl_ws_clients_.find(ptr) == map_hdl_ws_clients_.end()) {
		void*wsClient = NULL;
		if (ws_svr_event_ != NULL) {
			ws_server_t::connection_ptr con = ws_server_->get_con_from_hdl(hdl);
			std::string strRealIp = con->get_request_header("x-real-ip");	//X-Real-IP => WS统一使用小写的Header
			if (strRealIp.length() == 0)
			{
				strRealIp = con->get_request_header("X-Real-IP");
			}
			wsClient = ws_svr_event_->OnConnection(ptr, strRealIp);
			
			//printf("XWs socket get real ip: %s \r\n", strRealIp.c_str());
		}
		if (wsClient != NULL) {
			map_hdl_ws_clients_[ptr].hdl = hdl;
			map_hdl_ws_clients_[ptr].wsClient = wsClient;
		}
	}
}
void WsServerImpl::OnWsClose(websocketpp::connection_hdl hdl)
{
	void* ptr = hdl.lock().get();
	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	if (map_hdl_ws_clients_.find(ptr) != map_hdl_ws_clients_.end()) {
		if (ws_svr_event_ != NULL) {
			ws_svr_event_->OnDisConnection(map_hdl_ws_clients_[ptr].wsClient, ptr);
		}
		map_hdl_ws_clients_.erase(ptr);
	}
}
void WsServerImpl::OnWsMessage(websocketpp::connection_hdl hdl, ws_server_t::message_ptr msg)
{
	void* ptr = hdl.lock().get();
	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	if (map_hdl_ws_clients_.find(ptr) != map_hdl_ws_clients_.end()) {
		if (ws_svr_event_ != NULL) {
			ws_svr_event_->OnDisMessage(map_hdl_ws_clients_[ptr].wsClient, msg->get_payload());
		}
	}
}

void WsServerImpl::OnWsPing(websocketpp::connection_hdl hdl, std::string const &strMsg)
{
	void* ptr = hdl.lock().get();
	unique_lock<mutex> lock(cs_hdl_ws_clients_);
	if (map_hdl_ws_clients_.find(ptr) != map_hdl_ws_clients_.end()) {
		websocketpp::lib::error_code ec;
		ws_server_t::connection_ptr con = ws_server_->get_con_from_hdl(hdl);
		if (con != NULL) {
			con->pong(strMsg, ec);
		}
	}
	//SendWsMessage(strMsg);
}