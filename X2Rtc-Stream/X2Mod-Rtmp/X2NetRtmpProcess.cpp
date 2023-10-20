#include "X2NetRtmpProcess.h"

namespace x2rtc {
extern X2NetProcess* createX2NetTcpProcess();
X2NetProcess* createX2NetRtmpProcess()
{
	return new X2NetRtmpProcess();
}
X2NetRtmpProcess::X2NetRtmpProcess(void)
	: x2net_tcp_process_(NULL)
{

}
X2NetRtmpProcess::~X2NetRtmpProcess(void)
{

}

int X2NetRtmpProcess::Init(int nPort)
{
	if (x2net_tcp_process_ == NULL) {
		x2net_tcp_process_ = createX2NetTcpProcess();
		x2net_tcp_process_->SetListener(this);
		return x2net_tcp_process_->Init(nPort);
	}
	
	return 0;
}
int X2NetRtmpProcess::DeInit()
{
	if (x2net_tcp_process_ != NULL) {
		x2net_tcp_process_->DeInit();
		delete x2net_tcp_process_;
		x2net_tcp_process_ = NULL;
	}
	
	return 0;
}

//* For X2NetProcess
int X2NetRtmpProcess::RunOnce()
{
	if (x2net_tcp_process_ != NULL) {
		x2net_tcp_process_->RunOnce();
	}
	return 0;
}

//* For X2NetProcess::Listener
void X2NetRtmpProcess::OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetProcessNewConnection(X2Net_Rtmp, connection);
	}
}

}	// namespace x2rtc