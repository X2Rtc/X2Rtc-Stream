#include "WsClient.h"
#include <iostream>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> ws_client_t;
// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class WsClientImpl : public WsClient
{
public:
	WsClientImpl(WsClientEvent*pEvent);
	virtual ~WsClientImpl(void);

	virtual int Connect(const std::string&strUri);
	virtual void Disconnect();

	virtual void DoTick(int64_t curUtcTime);

	virtual void SendWsMessage(const std::string&strMsg);

	void OnWsOpen(websocketpp::connection_hdl hdl);
	void OnWsFail(websocketpp::connection_hdl hdl);
	void OnWsClose(websocketpp::connection_hdl hdl);
	void OnWsPong(websocketpp::connection_hdl hdl, std::string const &strMsg);
	void OnWsPongTimout(websocketpp::connection_hdl hdl, std::string const &strMsg);
	void OnWsMessage(websocketpp::connection_hdl hdl, message_ptr msg);

private:

	ws_client_t	*ws_client_;
	websocketpp::connection_hdl ws_hdl_;
	ws_client_t::connection_ptr connection_;

	int64_t n_next_ping_time_;
};

WsClient* createWsClient(WsClientEvent*pEvent)
{
	return new WsClientImpl(pEvent);
}

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
static void on_open(void* c, websocketpp::connection_hdl hdl)
{
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsOpen(hdl);
}
static void on_fail(void* c, websocketpp::connection_hdl hdl)
{
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsFail(hdl);
}
static void on_close(void* c, websocketpp::connection_hdl hdl)
{
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsClose(hdl);
}
static bool on_pong(void* c, websocketpp::connection_hdl hdl, std::string strMsg)
{
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsPong(hdl, strMsg);
	return true;
}
static bool on_pong_timeout(void* c, websocketpp::connection_hdl hdl, std::string strMsg)
{
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsPongTimout(hdl, strMsg);
	return true;
}
static void on_message(void* c, websocketpp::connection_hdl hdl, message_ptr msg) {
#if 0
	std::cout << "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;
#endif
	WsClientImpl* wsClient = (WsClientImpl*)c;
	wsClient->OnWsMessage(hdl, msg);


	
}

WsClientImpl::WsClientImpl(WsClientEvent*pEvent)
	: WsClient(pEvent)
	, ws_client_(NULL)
	, connection_(NULL)
	, n_next_ping_time_(0)
{
	
}
WsClientImpl::~WsClientImpl(void)
{
	assert(ws_client_ == NULL);
}

int WsClientImpl::Connect(const std::string&strUri)
{
	if (ws_client_ == NULL) {
		ws_client_ = new ws_client_t();
		ws_client_->set_access_channels(websocketpp::log::alevel::all);
		ws_client_->clear_access_channels(websocketpp::log::alevel::frame_payload);

		// Initialize ASIO
		ws_client_->init_asio();

		// Register our message handler
		ws_client_->set_open_handler(bind(&on_open, this, ::_1));
		ws_client_->set_fail_handler(bind(&on_fail, this, ::_1));
		ws_client_->set_close_handler(bind(&on_close, this, ::_1));
		ws_client_->set_message_handler(bind(&on_message, this, ::_1, ::_2));
		ws_client_->set_pong_handler(bind(&on_pong, this, ::_1, ::_2));
		ws_client_->set_pong_timeout_handler(bind(&on_pong_timeout, this, ::_1, ::_2));

		websocketpp::lib::error_code ec;
		connection_ = ws_client_->get_connection(strUri, ec);
		if (ec) {
			std::cout << "could not create connection because: " << ec.message() << std::endl;
			return -1;
		}
		ws_client_->set_error_channels(websocketpp::log::elevel::none);
		ws_client_->set_access_channels(websocketpp::log::alevel::none);

		// Note that connect here only requests a connection. No network messages are
		// exchanged until the event loop starts running in the next line.
		ws_client_->connect(connection_);
		ws_hdl_ = connection_->get_handle();
	}

	return 0;
}
void WsClientImpl::Disconnect()
{
	if (ws_client_ != NULL) {
		try {
			connection_->close(1001, "");
			ws_client_->close(ws_hdl_, 1001, "");
		}
		catch (websocketpp::exception ec) {
			printf("websocketpp::exception: %s\r\n", ec.what());
		}

		connection_ = NULL;
		delete ws_client_;
		ws_client_ = NULL;
		n_next_ping_time_ = 0;
	}
}
void WsClientImpl::DoTick(int64_t curUtcTime)
{
	if (ws_client_ != NULL) {
		ws_client_->poll_one();
	}

	if (n_next_ping_time_ != 0 && n_next_ping_time_ <= curUtcTime) {
		n_next_ping_time_ = curUtcTime + 10000;
		if (connection_ != NULL) {
			websocketpp::lib::error_code ec;
			connection_->ping("--heartbeat--", ec);
		}
	}
}

void WsClientImpl::SendWsMessage(const std::string&strMsg)
{
	websocketpp::lib::error_code ec;

	ws_client_->send(ws_hdl_, strMsg, websocketpp::frame::opcode::text, ec);
	if (ec) {
		std::cout << "Send failed because: " << ec.message() << std::endl;
	}
}

void WsClientImpl::OnWsOpen(websocketpp::connection_hdl hdl)
{
	if (ws_event_ != NULL) {
		ws_event_->OnWsClientOpen();
	}

	n_next_ping_time_ = 1;
}
void WsClientImpl::OnWsFail(websocketpp::connection_hdl hdl)
{
	if (ws_event_ != NULL) {
		ws_event_->OnWsClientFail();
	}
}
void WsClientImpl::OnWsClose(websocketpp::connection_hdl hdl)
{
	if (ws_event_ != NULL) {
		ws_event_->OnWsClientClose();
	}
}
void WsClientImpl::OnWsPong(websocketpp::connection_hdl hdl, std::string const &strMsg)
{
	websocketpp::lib::error_code ec;

	//SendWsMessage(strMsg);
}
void WsClientImpl::OnWsPongTimout(websocketpp::connection_hdl hdl, std::string const &strMsg)
{
	if (ws_event_ != NULL) {
		ws_event_->OnWsClientClose();
	}
}
void WsClientImpl::OnWsMessage(websocketpp::connection_hdl hdl, message_ptr msg)
{
#if 0
	websocketpp::lib::error_code ec;

	ws_client_->send(hdl, msg->get_payload(), msg->get_opcode(), ec);
	if (ec) {
		std::cout << "Echo failed because: " << ec.message() << std::endl;
	}
#endif

	if (ws_event_ != NULL) {
		ws_event_->OnWsClientRecvMessage(msg->get_payload());
	}
}
