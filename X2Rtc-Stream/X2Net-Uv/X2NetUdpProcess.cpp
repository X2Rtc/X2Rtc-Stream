#include "X2NetUdpProcess.h"
#include "X2PortManager.h"

namespace x2rtc {

X2NetProcess* createX2NetUdpProcess() {
	X2NetUdpProcess* x2NetUdpProcess = new X2NetUdpProcess();
	return x2NetUdpProcess;
}

X2NetUdpProcess::X2NetUdpProcess(void)
	: x2net_udp_connection_(NULL)
{

}
X2NetUdpProcess::~X2NetUdpProcess(void)
{

}

int X2NetUdpProcess::Init(int nPort)
{
	if (x2net_udp_connection_ == NULL) {
		std::string strListenIp = "0.0.0.0";
		uv_udp_t* uvHandle = NULL;
		try {
			uvHandle = ::X2PortManager::BindUdp(strListenIp, nPort);
		}
		catch (std::exception&) {
			return -1;
		}
		
		x2net_udp_connection_ = new X2NetUdpConnection(uvHandle);
		x2net_udp_connection_->SetRecvBufferSize(8 * 1024 * 1024);
		x2net_udp_connection_->SetSendBufferSize(4 * 1024 * 1024);
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2NetProcessNewConnection(X2NetType::X2Net_Udp, x2net_udp_connection_);
		}
	}

	return 0;
}
int X2NetUdpProcess::DeInit()
{
	if (x2net_udp_connection_ != NULL) {
		try {
			x2net_udp_connection_->Close();
		}
		catch (std::exception&) {
		}
		delete x2net_udp_connection_;
		x2net_udp_connection_ = NULL;
	}

	return 0;
}

//* For X2NetProcess
int X2NetUdpProcess::RunOnce()
{
	if (x2net_udp_connection_ != NULL) {
		x2net_udp_connection_->RunOnce();
	}

	return 0;
}

}	// namespace x2rtc