#include "X2NetTcpProcess.h"

namespace x2rtc {

X2NetProcess* createX2NetTcpProcess() {
	X2NetTcpProcess* x2NetTcpProcess = new X2NetTcpProcess();
	return x2NetTcpProcess;
}

X2NetTcpProcess::X2NetTcpProcess(void)
	: x2tcp_server_(NULL)
{

}
X2NetTcpProcess::~X2NetTcpProcess(void)
{
	
}

int X2NetTcpProcess::Init(int nPort)
{
	if (x2tcp_server_ == NULL) {
		std::string strListenIp = "0.0.0.0";
		x2tcp_server_ = new X2TcpServer(this);
		if (!x2tcp_server_->Listen(strListenIp, nPort)) {
			delete x2tcp_server_;
			x2tcp_server_ = NULL;
			return -1;
		}
	}
	return 0;
}
int X2NetTcpProcess::DeInit()
{
	{
		JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
		ListX2NetConnection::iterator itlr = list_x2net_connection_for_delete_.begin();
		while (itlr != list_x2net_connection_for_delete_.end()) {
			delete (*itlr);
			itlr = list_x2net_connection_for_delete_.erase(itlr);
		}
	}

	if (x2tcp_server_ != NULL) {
		x2tcp_server_->Close();
		delete x2tcp_server_;
		x2tcp_server_ = NULL;
	}
	return 0;
}

//* For X2NetProcess
int X2NetTcpProcess::RunOnce()
{
	X2NetProcess::RunOnce();

	return 0;
}

//* For RTC::X2TcpServer::Listener
void X2NetTcpProcess::OnRtcTcpConnectionOpen(
	X2TcpServer* tcpServer, x2rtc::X2NetTcpConnection* connection)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetProcessNewConnection(X2NetType::X2Net_Tcp, connection);
	}
}

void X2NetTcpProcess::OnRtcTcpConnectionClose(
	X2TcpServer* tcpServer, x2rtc::X2NetTcpConnection* connection)
{
	JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
	list_x2net_connection_for_delete_.push_back(connection);
}

}	// namespace x2rtc