﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_RTPPROXY)
#include "Util/uv_errno.h"
#include "RtpServer.h"
#include "RtpSelector.h"
#include "Rtcp/RtcpContext.h"
#include "Common/config.h"

using namespace std;
using namespace toolkit;

namespace mediakit{

RtpServer::~RtpServer() {
    if (_on_cleanup) {
        _on_cleanup();
    }
}

class RtcpHelper: public std::enable_shared_from_this<RtcpHelper> {
public:
    using Ptr = std::shared_ptr<RtcpHelper>;

    RtcpHelper(Socket::Ptr rtcp_sock, std::string stream_id) {
        _rtcp_sock = std::move(rtcp_sock);
        _stream_id = std::move(stream_id);
    }

    ~RtcpHelper() {
        if (_process) {
            // 删除rtp处理器
            RtpSelector::Instance().delProcess(_stream_id, _process.get());
        }
    }

    void setRtpServerInfo(uint16_t local_port,RtpServer::TcpMode mode,bool re_use_port,uint32_t ssrc, bool only_audio) {
        _local_port = local_port;
        _tcp_mode = mode;
        _re_use_port = re_use_port;
        _ssrc = ssrc;
        _only_audio = only_audio;
    }

    void setOnDetach(function<void()> cb) {
        if (_process) {
            _process->setOnDetach(std::move(cb));
        } else {
            _on_detach = std::move(cb);
        }
    }

    void onRecvRtp(const Socket::Ptr &sock, const Buffer::Ptr &buf, struct sockaddr *addr) {
        if (!_process) {
            _process = RtpSelector::Instance().getProcess(_stream_id, true);
            _process->setOnlyAudio(_only_audio);
            _process->setOnDetach(std::move(_on_detach));
            cancelDelayTask();
        }
        _process->inputRtp(true, sock, buf->data(), buf->size(), addr);

        // 统计rtp接受情况，用于发送rr包
        auto header = (RtpHeader *)buf->data();
        sendRtcp(ntohl(header->ssrc), addr);
    }

    void startRtcp() {
        weak_ptr<RtcpHelper> weak_self = shared_from_this();
        _rtcp_sock->setOnRead([weak_self](const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
            // 用于接受rtcp打洞包
            auto strong_self = weak_self.lock();
            if (!strong_self || !strong_self->_process) {
                return;
            }
            if (!strong_self->_rtcp_addr) {
                // 只设置一次rtcp对端端口
                strong_self->_rtcp_addr = std::make_shared<struct sockaddr_storage>();
                memcpy(strong_self->_rtcp_addr.get(), addr, addr_len);
            }
            auto rtcps = RtcpHeader::loadFromBytes(buf->data(), buf->size());
            for (auto &rtcp : rtcps) {
                strong_self->_process->onRtcp(rtcp);
            }
        });

        GET_CONFIG(uint64_t, timeoutSec, RtpProxy::kTimeoutSec);
        _delay_task = _rtcp_sock->getPoller()->doDelayTask(timeoutSec * 1000, [weak_self]() {
            if (auto strong_self = weak_self.lock()) {
                auto process = RtpSelector::Instance().getProcess(strong_self->_stream_id, false);
                if (!process && strong_self->_on_detach) {
                    strong_self->_on_detach();
                }
                if(process && strong_self->_on_detach){// tcp 链接防止断开不删除rtpServer
                    process->setOnDetach(std::move(strong_self->_on_detach));
                }
                if (!process) { // process 未创建，触发rtp server 超时事件
                    NoticeCenter::Instance().emitEvent(Broadcast::KBroadcastRtpServerTimeout, strong_self->_local_port, strong_self->_stream_id,
                                                       (int)strong_self->_tcp_mode, strong_self->_re_use_port, strong_self->_ssrc);
                }
            }
            return 0;
        });
    }

    void cancelDelayTask() {
        if (_delay_task) {
            _delay_task->cancel();
            _delay_task = nullptr;
        }
    }

private:
    void sendRtcp(uint32_t rtp_ssrc, struct sockaddr *addr) {
        // 每5秒发送一次rtcp
        if (_ticker.elapsedTime() < 5000 || !_process) {
            return;
        }
        _ticker.resetTime();

        auto rtcp_addr = (struct sockaddr *)_rtcp_addr.get();
        if (!rtcp_addr) {
            // 默认的，rtcp端口为rtp端口+1
            switch (addr->sa_family) {
                case AF_INET: ((sockaddr_in *)addr)->sin_port = htons(ntohs(((sockaddr_in *)addr)->sin_port) + 1); break;
                case AF_INET6: ((sockaddr_in6 *)addr)->sin6_port = htons(ntohs(((sockaddr_in6 *)addr)->sin6_port) + 1); break;
            }
            // 未收到rtcp打洞包时，采用默认的rtcp端口
            rtcp_addr = addr;
        }
        _rtcp_sock->send(_process->createRtcpRR(rtp_ssrc + 1, rtp_ssrc), rtcp_addr);
    }

private:
    bool _re_use_port = false;
    bool _only_audio = false;
    uint16_t _local_port = 0;
    uint32_t _ssrc = 0;
    RtpServer::TcpMode _tcp_mode = RtpServer::NONE;

    Ticker _ticker;
    Socket::Ptr _rtcp_sock;
    RtpProcess::Ptr _process;
    std::string _stream_id;
    function<void()> _on_detach;
    std::shared_ptr<struct sockaddr_storage> _rtcp_addr;
    EventPoller::DelayTask::Ptr _delay_task;
};

void RtpServer::start(uint16_t local_port, const string &stream_id, TcpMode tcp_mode, const char *local_ip, bool re_use_port, uint32_t ssrc, bool only_audio) {
    //创建udp服务器
    Socket::Ptr rtp_socket = Socket::createSocket(nullptr, true);
    Socket::Ptr rtcp_socket = Socket::createSocket(nullptr, true);
    if (local_port == 0) {
        //随机端口，rtp端口采用偶数
        auto pair = std::make_pair(rtp_socket, rtcp_socket);
        makeSockPair(pair, local_ip, re_use_port);
        local_port = rtp_socket->get_local_port();
    } else if (!rtp_socket->bindUdpSock(local_port, local_ip, re_use_port)) {
        //用户指定端口
        throw std::runtime_error(StrPrinter << "创建rtp端口 " << local_ip << ":" << local_port << " 失败:" << get_uv_errmsg(true));
    } else if (!rtcp_socket->bindUdpSock(local_port + 1, local_ip, re_use_port)) {
        // rtcp端口
        throw std::runtime_error(StrPrinter << "创建rtcp端口 " << local_ip << ":" << local_port + 1 << " 失败:" << get_uv_errmsg(true));
    }

    //设置udp socket读缓存
    SockUtil::setRecvBuf(rtp_socket->rawFD(), 4 * 1024 * 1024);

    TcpServer::Ptr tcp_server;
    _tcp_mode = tcp_mode;
    if (tcp_mode == PASSIVE || tcp_mode == ACTIVE) {
        //创建tcp服务器
        tcp_server = std::make_shared<TcpServer>(rtp_socket->getPoller());
        (*tcp_server)[RtpSession::kStreamID] = stream_id;
        (*tcp_server)[RtpSession::kSSRC] = ssrc;
        (*tcp_server)[RtpSession::kOnlyAudio] = only_audio;
        if (tcp_mode == PASSIVE) {
            tcp_server->start<RtpSession>(local_port, local_ip);
        } else if (stream_id.empty()) {
            // tcp主动模式时只能一个端口一个流，必须指定流id; 创建TcpServer对象也仅用于传参
            throw std::runtime_error(StrPrinter << "tcp主动模式时必需指定流id");
        }
    }

    //创建udp服务器
    UdpServer::Ptr udp_server;
    RtcpHelper::Ptr helper;
    if (!stream_id.empty()) {
        //指定了流id，那么一个端口一个流(不管是否包含多个ssrc的多个流，绑定rtp源后，会筛选掉ip端口不匹配的流)
        helper = std::make_shared<RtcpHelper>(std::move(rtcp_socket), stream_id);
        helper->startRtcp();
        helper->setRtpServerInfo(local_port, tcp_mode, re_use_port, ssrc, only_audio);
        bool bind_peer_addr = false;
        auto ssrc_ptr = std::make_shared<uint32_t>(ssrc);
        _ssrc = ssrc_ptr;
        rtp_socket->setOnRead([rtp_socket, helper, ssrc_ptr, bind_peer_addr](const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) mutable {
            RtpHeader *header = (RtpHeader *)buf->data();
            auto rtp_ssrc = ntohl(header->ssrc);
            auto ssrc = *ssrc_ptr;
            if (ssrc && rtp_ssrc != ssrc) {
                WarnL << "ssrc mismatched, rtp dropped: " << rtp_ssrc << " != " << ssrc;
            } else {
                if (!bind_peer_addr) {
                    //绑定对方ip+端口，防止多个设备或一个设备多次推流从而日志报ssrc不匹配问题
                    bind_peer_addr = true;
                    rtp_socket->bindPeerAddr(addr, addr_len);
                }
                helper->onRecvRtp(rtp_socket, buf, addr);
            }
        });
    } else {
        //单端口多线程接收多个流，根据ssrc区分流
        udp_server = std::make_shared<UdpServer>(rtp_socket->getPoller());
        (*udp_server)[RtpSession::kOnlyAudio] = only_audio;
        udp_server->start<RtpSession>(local_port, local_ip);
        rtp_socket = nullptr;
    }

    _on_cleanup = [rtp_socket, stream_id]() {
        if (rtp_socket) {
            //去除循环引用
            rtp_socket->setOnRead(nullptr);
        }
    };

    _tcp_server = tcp_server;
    _udp_server = udp_server;
    _rtp_socket = rtp_socket;
    _rtcp_helper = helper;
}

void RtpServer::setOnDetach(function<void()> cb) {
    if (_rtcp_helper) {
        _rtcp_helper->setOnDetach(std::move(cb));
    }
}

uint16_t RtpServer::getPort() {
    return _udp_server ? _udp_server->getPort() : _rtp_socket->get_local_port();
}

void RtpServer::connectToServer(const std::string &url, uint16_t port, const function<void(const SockException &ex)> &cb) {
    if (_tcp_mode != ACTIVE || !_rtp_socket) {
        cb(SockException(Err_other, "仅支持tcp主动模式"));
        return;
    }
    weak_ptr<RtpServer> weak_self = shared_from_this();
    _rtp_socket->connect(url, port, [url, port, cb, weak_self](const SockException &err) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            cb(SockException(Err_other, "服务对象已释放"));
            return;
        }
        if (err) {
            WarnL << "连接到服务器 " << url << ":" << port << " 失败 " << err;
        } else {
            InfoL << "连接到服务器 " << url << ":" << port << " 成功";
            strong_self->onConnect();
        }
        cb(err);
    },
    5.0F, "::", _rtp_socket->get_local_port());
}

void RtpServer::onConnect() {
    auto rtp_session = std::make_shared<RtpSession>(_rtp_socket);
    rtp_session->attachServer(*_tcp_server);
    _rtp_socket->setOnRead([rtp_session](const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
        rtp_session->onRecv(buf);
    });
    weak_ptr<RtpServer> weak_self = shared_from_this();
    _rtp_socket->setOnErr([weak_self](const SockException &err) {
        if (auto strong_self = weak_self.lock()) {
            strong_self->_rtp_socket->setOnRead(nullptr);
        }
    });
}

void RtpServer::updateSSRC(uint32_t ssrc) {
    if (_ssrc) {
        *_ssrc = ssrc;
    }

    if (_tcp_server) {
        (*_tcp_server)[RtpSession::kSSRC] = ssrc;
    }
}

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)
