#ifndef __WS_CLIENT_H__
#define __WS_CLIENT_H__
#include <string>

class WsClientEvent
{
public:
	WsClientEvent() {};
	virtual ~WsClientEvent(void) {};

	virtual void OnWsClientOpen() {};
	virtual void OnWsClientFail() {};
	virtual void OnWsClientClose() {};
	virtual void OnWsClientRecvMessage(const std::string&strMsg) {};
};

class WsClient
{
public:
	WsClient(WsClientEvent*pEvent): ws_event_(pEvent){};
	virtual ~WsClient(void) {};

	virtual int Connect(const std::string&strUri) = 0;
	virtual void Disconnect() = 0;

	virtual void DoTick(int64_t curUtcTime) = 0;

	virtual void SendWsMessage(const std::string&strMsg) = 0;

protected:
	WsClientEvent*	ws_event_;
};

WsClient* createWsClient(WsClientEvent*pEvent);

#endif	// __WS_CLIENT_H__