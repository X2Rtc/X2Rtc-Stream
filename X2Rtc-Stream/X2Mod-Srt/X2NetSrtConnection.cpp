#include "X2NetSrtConnection.h"
#include "X2RtcLog.h"

namespace x2rtc {

X2NetSrtConnection::X2NetSrtConnection(void)
	: b_sock_closed_(false)
    , srt_poller_(-1)
	, srt_socket_(-1)
{

}
X2NetSrtConnection::~X2NetSrtConnection(void)
{

}

//* For X2NetConnection
bool X2NetSrtConnection::CanDelete()
{
	return X2NetConnection::IsForceClosed() && b_sock_closed_;
}

int X2NetSrtConnection::SetSocket(int poll, SRTSOCKET sock)
{
    srt_poller_ = poll;
	srt_socket_ = sock;

	if (srt_socket_ != -1) {
        std::string ip;
        int port = 0;
        // discovery client information
        sockaddr_storage addr;
        int addrlen = sizeof(addr);
        if (srt_getpeername(srt_socket_, (sockaddr*)&addr, &addrlen) == -1) {
            X2RtcPrintf(ERR, "[srt] srt_getpeername");
            return -1;
        }

        char saddr[64];
        char* h = (char*)saddr;
        socklen_t nbh = (socklen_t)sizeof(saddr);
        const int r0 = getnameinfo((const sockaddr*)&addr, addrlen, h, nbh, NULL, 0, NI_NUMERICHOST);
        if (r0) {
            X2RtcPrintf(ERR, "[srt] getnameinfo");
            return -1;
        }

        switch (addr.ss_family) {
        case AF_INET:
            port = ntohs(((sockaddr_in*)&addr)->sin_port);
            break;
        case AF_INET6:
            port = ntohs(((sockaddr_in6*)&addr)->sin6_port);
            break;
        }

        ip.assign(saddr);

        // SRT max streamid length is 512.
        char sid[512] = { 0 };
        GET_SRT_OPT(srt_socket_, SRTO_STREAMID, sid);
        // If streamid empty, using default streamid instead.
        if (strlen(sid) == 0) {
            sprintf(sid, "#!::r=live/livestream,m=publish");
            X2RtcPrintf(WARN, "[srt] srt get empty streamid, using default steramid %s instead", sid);
        }

        char ipPort[128];
        sprintf(ipPort, "%s:%d", ip.c_str(), port);
        str_remote_ip_port_ = ipPort;
        str_stream_id_ = sid;
        X2RtcPrintf(INF, "[srt] new connection: %s streamId: %s", ipPort, sid);
	}

    return 0;
}

SRTSOCKET X2NetSrtConnection::GetSocket()
{
	return srt_socket_;
}

void X2NetSrtConnection::SetClosed()
{
	b_sock_closed_ = true;

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionClosed(this);
	}
}

int X2NetSrtConnection::RunOnce()
{
#if 0
    if (IsForceClosed()) {
        if (srt_socket_ != -1) {
            srt_close(srt_socket_);
            srt_socket_ = -1;
        }
    }
#endif
	if (!b_sock_closed_ && IsWritable()) {
        while (1) {
            if (n_write_cache_len_ > 0) {
                int lResult = srt_sendmsg(srt_socket_, (char*)write_cache_data_, n_write_cache_len_, -1, true);
                if (lResult > 0) {
                    if (n_write_cache_len_ != lResult) {
                        ResetCacheData(lResult);
                        break;
                    }
                    else {
                        ResetCacheData(n_write_cache_len_);
                    }
                }
                else {
                    break;
                }
            }
            x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
            x2NetData = GetX2NetData();
            if (x2NetData != NULL) {
                if (x2NetData->pHdr != NULL && x2NetData->nHdrLen > 0) {
                    int lResult = srt_sendmsg(srt_socket_, (char*)x2NetData->pHdr, x2NetData->nHdrLen, -1, true);
                    if (lResult > 0) {
                        if (x2NetData->nHdrLen != lResult) {
                            CacheData(x2NetData->pHdr + lResult, x2NetData->nHdrLen - lResult);
                            if (x2NetData->pData != NULL && x2NetData->nLen > 0) {
                                CacheData(x2NetData->pData, x2NetData->nLen);
                            }
                            break;
                        }
                        else {
                            if (x2NetData->pData != NULL && x2NetData->nLen > 0) {
                                lResult = srt_sendmsg(srt_socket_, (char*)x2NetData->pData, x2NetData->nLen, -1, true);
                                if (lResult > 0) {
                                    if (x2NetData->nLen != lResult) {
                                        CacheData(x2NetData->pData + lResult, x2NetData->nLen - lResult);
                                        break;
                                    }
                                    //* Send all data OK
                                }
                                else {
                                    CacheData(x2NetData->pData, x2NetData->nLen);
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        CacheData(x2NetData->pHdr, x2NetData->nHdrLen);
                        if (x2NetData->pData != NULL && x2NetData->nLen > 0) {
                            CacheData(x2NetData->pData, x2NetData->nLen);
                        }
                        break;
                    }
                }
                
            }
            else {
                break;
            }
        }
	}
	return 0;
}

int X2NetSrtConnection::DoRead()
{
    uint8_t lMsg[2048];
    int lResult = 0;
    lResult = srt_recvmsg(srt_socket_, (char*)lMsg, sizeof(lMsg));
    if (lResult > 0) {
        RecvData(lMsg, lResult);
    }
    else {
        if (lResult < 0) {
            X2RtcPrintf(ERR, "[srt] srt_recvmsg rlt: %d error: %s", lResult, srt_getlasterror_str());
        }

    }
   

    return lResult;
}

}	// namespace x2rtc