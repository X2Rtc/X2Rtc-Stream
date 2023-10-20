#include "X2NetHttpProcess.h"

namespace x2rtc {

extern X2NetProcess* createX2NetTcpProcess();
X2NetProcess* createX2NetHttpProcess()
{
	return new X2NetHttpProcess();
}
X2NetHttpProcess::X2NetHttpProcess(void)
	: x2net_tcp_process_(NULL)
{

}
X2NetHttpProcess::~X2NetHttpProcess(void)
{

}

int X2NetHttpProcess::Init(int nPort)
{
	if (x2net_tcp_process_ == NULL) {
		x2net_tcp_process_ = createX2NetTcpProcess();
		x2net_tcp_process_->SetListener(this);
		return x2net_tcp_process_->Init(nPort);
	}

	return 0;
}
int X2NetHttpProcess::DeInit()
{
	if (x2net_tcp_process_ != NULL) {
		x2net_tcp_process_->DeInit();
		delete x2net_tcp_process_;
		x2net_tcp_process_ = NULL;
	}

	return 0;
}

//* For X2NetProcess
int X2NetHttpProcess::RunOnce()
{
	if (x2net_tcp_process_ != NULL) {
		x2net_tcp_process_->RunOnce();
	}
	return 0;
}

//* For X2NetProcess::Listener
void X2NetHttpProcess::OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetProcessNewConnection(X2Net_Http, connection);
	}
}

}	// namespace x2rtc