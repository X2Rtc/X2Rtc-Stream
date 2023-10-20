#include "X2NetSrtProcess.h"
#include "XUtil.h"
#include "X2RtcLog.h"
#include "X2NetSrtConnection.h"

#define MAX_WORKERS 512 //Max number of connections to deal with each epoll

namespace x2rtc {

X2NetProcess* createX2NetSrtProcess()
{
    return new X2NetSrtProcess();
}

X2NetSrtProcess::X2NetSrtProcess(void)
	: b_running_(false)
    , srt_socket_(-1)
    , srt_poller_(-1)
{
    srt_startup();
}
X2NetSrtProcess::~X2NetSrtProcess(void)
{
    srt_cleanup();
}

int X2NetSrtProcess::Init(int nPort)
{
    std::string lIp = "0.0.0.0";
    struct sockaddr_in lSaV4 = { 0 };
    struct sockaddr_in6 lSaV6 = { 0 };

    int lResult = 0;
    int lIpType = AF_INET;
    if (XIsIPv4(lIp)) {
        lIpType = AF_INET;
    }
    else if (XIsIPv6(lIp)) {
        lIpType = AF_INET6;
    }
    else {
        X2RtcLog(ERR, "[srt] Provided IP-Address not valid.");
        return -1;
    }

    srt_socket_ = srt_create_socket();
    if (srt_socket_ == SRT_ERROR) {
        X2RtcPrintf(ERR, "[srt] srt_create_socket err: %s", srt_getlasterror_str());
        return -1001;
    }

    if (lIpType == AF_INET) {
        lSaV4.sin_family = AF_INET;
        lSaV4.sin_port = htons(nPort);
        if (inet_pton(AF_INET, lIp.c_str(), &lSaV4.sin_addr) != 1) {
            X2RtcLog(ERR, "inet_pton AF_INET failed ");
            srt_close(srt_socket_);
            return -1002;
        }
    }

    if (lIpType == AF_INET6) {
        lSaV6.sin6_family = AF_INET6;
        lSaV6.sin6_port = htons(nPort);
        if (inet_pton(AF_INET6, lIp.c_str(), &lSaV6.sin6_addr) != 1) {
            X2RtcLog(ERR, "inet_pton AF_INET6 failed ");
            srt_close(srt_socket_);
            return -1003;
        }
    }

    if (SetNonBlock(srt_socket_) != 0) {
        srt_close(srt_socket_);
        return -1004;
    }
    
    if(0)
    {//Set Mtu
        int lMtu = 1200;
        SET_SRT_OPT(srt_socket_, SRTO_PAYLOADSIZE, lMtu);
    }

    {// Bind & Listen
        if (lIpType == AF_INET) {
            lResult = srt_bind(srt_socket_, (struct sockaddr*)&lSaV4, sizeof(lSaV4));
            if (lResult == SRT_ERROR) {
                X2RtcPrintf(ERR, "[srt] srt_bind AF_INET err: %s", srt_getlasterror_str());
                srt_close(srt_socket_);
                return -1005;
            }
        }

        if (lIpType == AF_INET6) {
            lResult = srt_bind(srt_socket_, (struct sockaddr*)&lSaV6, sizeof(lSaV6));
            if (lResult == SRT_ERROR) {
                X2RtcPrintf(ERR, "[srt] srt_bind AF_INET6 err: %s", srt_getlasterror_str());
                srt_close(srt_socket_);
                return -1006;
            }
        }

        lResult = srt_listen(srt_socket_, 2);
        if (lResult == SRT_ERROR) {
            X2RtcPrintf(ERR, "[srt] srt_listen err: %s", srt_getlasterror_str());
            srt_close(srt_socket_);
            return -1007;
        }
    }


    {// Create srt epoll for High perfermance
        srt_poller_ = srt_epoll_create();
        srt_epoll_set(srt_poller_, SRT_EPOLL_ENABLE_EMPTY);
    }

    {// Add main socket to epoll
        const int lEvents = SRT_EPOLL_IN | SRT_EPOLL_ERR;
        lResult = srt_epoll_add_usock(srt_poller_, srt_socket_, &lEvents);
    }

    if (!b_running_) {
        b_running_ = true;
        JThread::Start();
    }

	return 0;
}
int X2NetSrtProcess::DeInit()
{
    if (!b_running_) {
        b_running_ = false;
        JThread::Kill();
    }
    {
        MapX2NetConnection::iterator itnr = map_x2net_connection_.begin();
        while (itnr != map_x2net_connection_.end()) {
            SRTSOCKET lThisSocket = itnr->first;
            X2NetSrtConnection* x2NetSrtConn = (X2NetSrtConnection*)itnr->second;
            x2NetSrtConn->SetClosed();

            srt_epoll_remove_usock(srt_poller_, lThisSocket);
            srt_close(lThisSocket);

            itnr = map_x2net_connection_.erase(itnr);
            JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
            list_x2net_connection_for_delete_.push_back(x2NetSrtConn);
        }
        
    }
    if (srt_poller_ != -1) {
        srt_epoll_release(srt_poller_);
        srt_poller_ = -1;
    }
    if (srt_socket_ != -1) {
        srt_close(srt_socket_);
        srt_socket_ = -1;
    }
	return 0;
}

//* For JThread
void* X2NetSrtProcess::Thread()
{
    JThread::ThreadStarted();

    while (b_running_) {
        {
            MapX2NetConnection::iterator itmr = map_x2net_connection_.begin();
            while (itmr != map_x2net_connection_.end()) {
                X2NetSrtConnection* x2NetSrtConn = (X2NetSrtConnection*)itmr->second;
                x2NetSrtConn->RunOnce();
                itmr++;
            }
        }
#ifdef WIN32
        int runTime = 10;
#else
        int runTime = 2;
#endif
        while((runTime--) > 0)
        {
            SRT_EPOLL_EVENT lReady[MAX_WORKERS];
            int lRet = srt_epoll_uwait(srt_poller_, lReady, MAX_WORKERS, 10);
            if (lRet == MAX_WORKERS + 1) {
                lRet--;
            }
            //printf("Time; %u\r\n", XGetTimestamp());
            if (lRet > 0) {
                for (int i = 0; i < lRet; i++) {
                    SRTSOCKET lThisSocket = lReady[i].fd;
                    SRT_EPOLL_EVENT& eEvent = lReady[i];
                    int lResult = 0;
                    if (eEvent.events & SRT_EPOLL_ERR) {
                        lResult = SRT_ERROR;
                    }
                    else {
                        if (eEvent.events & SRT_EPOLL_IN) {
                            if (lThisSocket == srt_socket_) {
                                DoAccept();
                            }
                            else {
                                if (map_x2net_connection_.find(lThisSocket) != map_x2net_connection_.end()) {
                                    X2NetSrtConnection* x2NetSrtConn = (X2NetSrtConnection*)map_x2net_connection_[lThisSocket];
                                    lResult = x2NetSrtConn->DoRead();
                                }

                            }
                        }
                        if (eEvent.events & SRT_EPOLL_OUT) {
                            if (map_x2net_connection_.find(lThisSocket) != map_x2net_connection_.end()) {
                                X2NetSrtConnection* x2NetSrtConn = (X2NetSrtConnection*)map_x2net_connection_[lThisSocket];
                                x2NetSrtConn->NotifyWritable(true);
                            }
                        }
                    }
                    if (lResult == SRT_ERROR) {
                        srt_epoll_remove_usock(srt_poller_, lThisSocket);
                        srt_close(lThisSocket);
                        if (map_x2net_connection_.find(lThisSocket) != map_x2net_connection_.end()) {
                            X2NetSrtConnection* x2NetSrtConn = (X2NetSrtConnection*)map_x2net_connection_[lThisSocket];
                            x2NetSrtConn->SetClosed();
                            map_x2net_connection_.erase(lThisSocket);
                            JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
                            list_x2net_connection_for_delete_.push_back(x2NetSrtConn);
                        }
                    }
                }
            }
            else {
                if (lRet == -1) {
                    X2RtcPrintf(ERR, "[srt] epoll error: %s", srt_getlasterror_str());
                }
                break;
            }
        }
        XSleep(1);
    }
    return NULL;
}

void X2NetSrtProcess::DoAccept()
{
    struct sockaddr_storage lTheir_addr = { 0 };
    int lAddr_size = sizeof(lTheir_addr);
    SRTSOCKET lNewSocketCandidate = srt_accept(srt_socket_, (struct sockaddr*)&lTheir_addr, &lAddr_size);
    if (lNewSocketCandidate != -1) {
        const int lEvents = SRT_EPOLL_IN | SRT_EPOLL_OUT | SRT_EPOLL_ERR;
        int lResult = srt_epoll_add_usock(srt_poller_, lNewSocketCandidate, &lEvents);
        // notify error, don't notify read/write event.

        if (lResult == SRT_ERROR) {
            X2RtcPrintf(ERR, "[srt] srt_epoll_add_usock error: %s", srt_getlasterror_str());
            srt_close(lNewSocketCandidate);
        }
        else {
            SetNonBlock(lNewSocketCandidate);
            {
                int srtt = SRTT_LIVE;
                srt_setsockflag(lNewSocketCandidate, SRTO_TRANSTYPE, &srtt, sizeof(srtt));
            }
            if (map_x2net_connection_.find(lNewSocketCandidate) == map_x2net_connection_.end()) {
                X2NetSrtConnection* x2NetSrtConn = new X2NetSrtConnection();
                if (x2NetSrtConn->SetSocket(srt_poller_, lNewSocketCandidate) == 0) {
                    map_x2net_connection_[lNewSocketCandidate] = x2NetSrtConn;

                    if (cb_listener_ != NULL) {
                        cb_listener_->OnX2NetProcessNewConnection(X2Net_Srt, x2NetSrtConn);
                    }
                }
                else {
                    srt_close(lNewSocketCandidate);
                    delete x2NetSrtConn;
                    x2NetSrtConn = NULL;
                }
            }
        }
    }
}

int X2NetSrtProcess::SetNonBlock(SRTSOCKET sock)
{//Set non-block
    int sync = 0;
    SET_SRT_OPT(sock, SRTO_SNDSYN, sync);
    SET_SRT_OPT(sock, SRTO_RCVSYN, sync);

    return 0;
}

}	// namespace x2rtc