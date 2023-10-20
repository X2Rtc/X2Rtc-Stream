#include "SinglePort.h"
#include "Utils.hpp"
#include "RTC/StunPacket.hpp"
#include "RTC/RTCP/Packet.hpp"

#define LOG_STUN 0

static std::string getPeerAddress(const struct sockaddr* tuple) {
	char strAddr[256];
	std::string ip;
	int nFamily;
	uint16_t nPort;
	Utils::IP::GetAddressInfo(tuple, nFamily, ip, nPort);
	sprintf(strAddr, "%s:%d", ip.c_str(), nPort);
	return  strAddr;
}
static uint16_t getPeerPort(const struct sockaddr* tuple) {
	return ntohs(((struct sockaddr_in*)tuple)->sin_port);
}

SinglePort::SinglePort(std::string& ip, uint16_t port)
{
	udpSocketSingle = new RTC::UdpSocket(this, ip, port);
	udpSocketLocal = new RTC::UdpSocket(this, ip);

	//uv_recv_buffer_size()
	udpSocketSingle->SetRecvBufferSize(4 * 1024 * 1024);
	udpSocketSingle->SetSendBufferSize(8 * 1024 * 1024);

	udpSocketLocal->SetRecvBufferSize(8 * 1024 * 1024);
	udpSocketLocal->SetSendBufferSize(4 * 1024 * 1024);
}
SinglePort::~SinglePort(void)
{
	delete udpSocketSingle;
	delete udpSocketLocal;
}

uint16_t SinglePort::LocalMapPort()
{
	return udpSocketLocal->GetLocalPort();
}

void SinglePort::AddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword, ClientIpListener* ipListener)
{
	std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
	if (map_remote_id_to_local_.find(strRemoteId) == map_remote_id_to_local_.end()) {
		MapRemote& mapRemote = map_remote_id_to_local_[strRemoteId];
		mapRemote.nLocalPort = nLocalPort;
		mapRemote.strPassword = strPassword;
		mapRemote.ipListener = ipListener;
		map_local_port_to_remote_[nLocalPort] = &mapRemote;
	}
}
void SinglePort::RemoveRemoteId(const std::string& strRemoteId)
{
	std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
	if (map_remote_id_to_local_.find(strRemoteId) != map_remote_id_to_local_.end()) {
		MapRemote& mapRemote = map_remote_id_to_local_[strRemoteId];
		map_local_port_to_remote_.erase(mapRemote.nLocalPort);
		std::list<std::string>::iterator itar = mapRemote.lstRemoteAddr.begin();
		while (itar != mapRemote.lstRemoteAddr.end()) {
			std::string& strCurRemoteAddr = *itar;
			//printf("this(%p) RemoveRemoteId :%s addr : %s \r\n", this, strRemoteId.c_str(), strCurRemoteAddr.c_str());
			map_remote_addr_to_local_.erase(strCurRemoteAddr);
			itar = mapRemote.lstRemoteAddr.erase(itar);
		}
		map_remote_id_to_local_.erase(strRemoteId);
	}
}

void SinglePort::SendToLocal(const uint8_t* data, size_t len, int nPort)
{
	std::string strIP = "127.0.0.1";
	Utils::IP::NormalizeIp(strIP);
	int err;
	struct sockaddr_storage localAddr;
	switch (Utils::IP::GetFamily(strIP))
	{
	case AF_INET:
	{
		err = uv_ip4_addr(
			strIP.c_str(),
			static_cast<int>(nPort),
			reinterpret_cast<struct sockaddr_in*>(&localAddr));

		//if (err != 0)
		//	MS_THROW_ERROR("uv_ip4_addr() failed: %s", uv_strerror(err));

		break;
	}

	case AF_INET6:
	{
		err = uv_ip6_addr(
			strIP.c_str(),
			static_cast<int>(nPort),
			reinterpret_cast<struct sockaddr_in6*>(&localAddr));

		//if (err != 0)
		//	MS_THROW_ERROR("uv_ip6_addr() failed: %s", uv_strerror(err));

		break;
	}

	default:
	{
		err = -1;
	}
	}
	if (err == 0) {
		udpSocketLocal->Send(data, len, (struct sockaddr*)&localAddr, NULL);
	}
}

//* For RTC::UdpSocket::Listener
void SinglePort::OnUdpSocketPacketReceived(
	RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
{
	if (udpSocketSingle == socket) {
		//Remote 发过来的数据
		if (RTC::StunPacket::IsStun(data, len))
		{
			RTC::StunPacket* packet = RTC::StunPacket::Parse(data, len);
			if (packet != NULL) {
#if LOG_STUN
				std::string strRemoteAddr = getPeerAddress(remoteAddr);
				printf("Recv stun type: %d from: %s\r\n", packet->GetClass(), strRemoteAddr.c_str());
#endif
				if (packet->GetClass() == RTC::StunPacket::Class::REQUEST) {
					std::string username = packet->GetUsername();
					auto size = username.find(":");
					if (size != std::string::npos)
					{
						username = username.substr(0, size);
					}
					std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
					if (map_remote_id_to_local_.find(username) != map_remote_id_to_local_.end()) {
						MapRemote& mapRemote = map_remote_id_to_local_[username];
						if (!Utils::IP::CompareAddresses(&mapRemote.curRemoteAdd, remoteAddr)) {
							std::string strRemoteAddr = getPeerAddress(remoteAddr);

							mapRemote.curRemoteAdd = *remoteAddr;
							if (map_remote_addr_to_local_.find(strRemoteAddr) == map_remote_addr_to_local_.end()) {
								mapRemote.lstRemoteAddr.push_back(strRemoteAddr);
								map_remote_addr_to_local_[strRemoteAddr] = &mapRemote;
								//printf("this(%p) AddRemoteId username(%s) addr : %s \r\n", this, username.c_str(), strRemoteAddr.c_str());
								if (mapRemote.ipListener != NULL) {
									mapRemote.ipListener->OnClientIpListenerChanged(strRemoteAddr);
								}
							}
						}
						SendToLocal(data, len, mapRemote.nLocalPort);
					}
					else {
						// Reply 400.
						uint8_t strStunBuffer[1500];
						RTC::StunPacket* response = packet->CreateErrorResponse(400);
						response->Serialize(strStunBuffer);
						socket->Send(response->GetData(), response->GetSize(), remoteAddr, NULL);
						delete response;
					}
				}
				else if (packet->GetClass() == RTC::StunPacket::Class::INDICATION) {
					std::string strRemoteAddr = getPeerAddress(remoteAddr);
					std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
					if (map_remote_addr_to_local_.find(strRemoteAddr) != map_remote_addr_to_local_.end()) {
						MapRemote* mapRemote = map_remote_addr_to_local_[strRemoteAddr];
						SendToLocal(data, len, mapRemote->nLocalPort);
					}
				}
				else if (packet->GetClass() == RTC::StunPacket::Class::SUCCESS_RESPONSE) {
					std::string strRemoteAddr = getPeerAddress(remoteAddr);
					std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
					if (map_remote_addr_to_local_.find(strRemoteAddr) != map_remote_addr_to_local_.end()) {
						MapRemote* mapRemote = map_remote_addr_to_local_[strRemoteAddr];
						SendToLocal(data, len, mapRemote->nLocalPort);
					}
				}
				delete packet;
			}
		}
		else {
			std::string strRemoteAddr = getPeerAddress(remoteAddr);
			std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
			if (map_remote_addr_to_local_.find(strRemoteAddr) != map_remote_addr_to_local_.end()) {
				MapRemote* mapRemote = map_remote_addr_to_local_[strRemoteAddr];
				SendToLocal(data, len, mapRemote->nLocalPort);
			}
			else {
				printf("[err] this(%p) map_remote_addr_to_local_ not found: %s \r\n", this, strRemoteAddr.c_str());
			}
		}
	}
	else if (udpSocketLocal == socket) {
		//Local 往外发的数据
		uint16_t localPort = getPeerPort(remoteAddr);
		std::string strLocalAddr = getPeerAddress(remoteAddr);
		std::lock_guard<std::mutex> lock(cs_map_local_or_remote_);
		if (map_local_port_to_remote_.find(localPort) != map_local_port_to_remote_.end()) {
			MapRemote* mapRemote = map_local_port_to_remote_[localPort];
			if (RTC::StunPacket::IsStun(data, len) && mapRemote->strPassword.length() > 0)
			{
				RTC::StunPacket* packet = RTC::StunPacket::Parse(data, len);
				if (packet != NULL) {//* 如果是Stun的Response则需要替换XorMappedAddress为真实地址
#if LOG_STUN
					std::string strRemoteAddr = getPeerAddress(&(mapRemote->curRemoteAdd));
					printf("Send stun type: %d to: %s\r\n", packet->GetClass(), strRemoteAddr.c_str());
#endif
					if (packet->GetClass() == RTC::StunPacket::Class::SUCCESS_RESPONSE) {
						packet->SetXorMappedAddress(&(mapRemote->curRemoteAdd));
						packet->Authenticate(mapRemote->strPassword);	//这里必须要用password重新生成Stun的Response包，否则只换address客户端会验证失败
						uint8_t strDump[1500];
						packet->Serialize(strDump);
						udpSocketSingle->Send(packet->GetData(), packet->GetSize(), &(mapRemote->curRemoteAdd), NULL);
						delete packet;
						return;
					}
					delete packet;
				}
			}

			udpSocketSingle->Send(data, len, &(mapRemote->curRemoteAdd), NULL);
		}
		else {
			//printf("map_local_port_to_remote_ not found: %s port: %d \r\n", strLocalAddr.c_str(), localPort);
		}
	}
}

