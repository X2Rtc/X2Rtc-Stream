#ifndef __X2_SINGLE_PORT_H__
#define __X2_SINGLE_PORT_H__
#include <list>
#include <map>
#include "RTC/X2UdpSocket.hpp"
#include <mutex>

class X2SinglePort : public x2rtc::X2UdpSocket::Listener
{
public:
	X2SinglePort(std::string& ip, uint16_t port);
	virtual ~X2SinglePort(void);

	uint16_t LocalMapPort();
	void AddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword);
	void RemoveRemoteId(const std::string& strRemoteId);

	void SendToLocal(const uint8_t* data, size_t len, int nPort);

	//* For RTC::X2UdpSocket::Listener
	virtual void OnUdpSocketPacketReceived(
		x2rtc::X2UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr);
private:
	x2rtc::X2UdpSocket* udpSocketSingle;
	x2rtc::X2UdpSocket* udpSocketLocal;

	struct MapRemote{
		//std::string strCurRemoteAddr;
		std::list<std::string>lstRemoteAddr;
		struct sockaddr curRemoteAdd;
		int nLocalPort{0};
		std::string strPassword;
	};

	typedef std::map<std::string/*RemoteId*/, MapRemote> MapRemoteIdToLocal;
	typedef std::map<std::string/*RemoteAddr*/, MapRemote*>MapRemoteAddrToLocal;
	typedef std::map<int/*LocalPort*/, MapRemote*> MapLocalPortToRemote;

	std::mutex cs_map_local_or_remote_;
	MapRemoteIdToLocal	map_remote_id_to_local_;
	MapRemoteAddrToLocal map_remote_addr_to_local_;
	MapLocalPortToRemote map_local_port_to_remote_;
};

#endif	// __X2_SINGLE_PORT_H__