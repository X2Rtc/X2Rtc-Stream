#ifndef __WS_SERVER_H__
#define __WS_SERVER_H__
#include <string>

class WsServerEvent
{
public:
	WsServerEvent(void) {}
	virtual ~WsServerEvent(void) {};

	virtual void* OnConnection(void*conn, const std::string&strRealIp) = 0;

	virtual void OnDisMessage(void*wsClient, const std::string&strMsg) = 0;

	virtual void OnDisConnection(void*wsClient, void*conn) = 0;

	virtual void OnSendMssage(void*wsClient, std::string&strMsg) = 0;

};

class WsServer
{
public:
	WsServer(WsServerEvent*pEvent) : ws_svr_event_(pEvent){};
	virtual ~WsServer(void) {};

	virtual int StartSvr(int nListenPort) = 0;
	virtual int StopSvr() = 0;

	virtual void DoTick() = 0;

	virtual void ForceClose(void* pdl, int nCode) = 0;

protected:
	WsServerEvent	*ws_svr_event_;
};

WsServer* createWsServer(WsServerEvent*pEvent);

#endif	// __WS_SERVER_H__