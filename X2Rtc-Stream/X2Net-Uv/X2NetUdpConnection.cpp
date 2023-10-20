#include "X2NetUdpConnection.h"

namespace x2rtc {

X2NetUdpConnection::X2NetUdpConnection(uv_udp_t* uvHandle)
	: X2UdpSocketHandler(uvHandle)
{

}
X2NetUdpConnection::~X2NetUdpConnection(void)
{

}

int X2NetUdpConnection::RunOnce()
{
	while (1) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		x2NetData = GetX2NetData();
		if (x2NetData != NULL) {
			if (x2NetData->pHdr != NULL && x2NetData->nHdrLen > 0) {
				::X2UdpSocketHandler::Send(x2NetData->pHdr, x2NetData->nHdrLen, &x2NetData->peerAddr, NULL);
			}

			if(x2NetData->pData != NULL && x2NetData->nLen > 0) {
				::X2UdpSocketHandler::Send(x2NetData->pData, x2NetData->nLen, &x2NetData->peerAddr, NULL);
			}
		}
		else {
			break;
		}
	}

	return 0;
}

//* For UdpSocketHandler
void X2NetUdpConnection::UserOnUdpDatagramReceived(
	const uint8_t* data, size_t len, const struct sockaddr* addr)
{
	RecvDataWithAddr(data, len, addr);
}

}	// namespace x2rtc 