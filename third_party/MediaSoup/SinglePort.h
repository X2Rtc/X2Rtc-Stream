#ifndef __SINGLE_PORT_H__
#define __SINGLE_PORT_H__
#include <list>
#include <map>
#include "RTC/UdpSocket.hpp"
#include <mutex>

class ClientIpListener
{
public:
	ClientIpListener(void) {};
	virtual ~ClientIpListener(void) {};

	virtual void OnClientIpListenerChanged(const std::string& strIpPort) = 0;
};


class SinglePort : public RTC::UdpSocket::Listener
{
public:
	SinglePort(std::string& ip, uint16_t port);
	virtual ~SinglePort(void);

	uint16_t LocalMapPort();
	void AddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword, ClientIpListener* ipListener = NULL);
	void RemoveRemoteId(const std::string& strRemoteId);

	void SendToLocal(const uint8_t* data, size_t len, int nPort);

	//* For RTC::UdpSocket::Listener
	virtual void OnUdpSocketPacketReceived(
		RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr);
private:
	RTC::UdpSocket* udpSocketSingle;
	RTC::UdpSocket* udpSocketLocal;

	struct MapRemote{
		//std::string strCurRemoteAddr;
		std::list<std::string>lstRemoteAddr;
		struct sockaddr curRemoteAdd;
		int nLocalPort{0};
		std::string strPassword;

		ClientIpListener* ipListener{ NULL };
	};

	typedef std::map<std::string/*RemoteId*/, MapRemote> MapRemoteIdToLocal;
	typedef std::map<std::string/*RemoteAddr*/, MapRemote*>MapRemoteAddrToLocal;
	typedef std::map<int/*LocalPort*/, MapRemote*> MapLocalPortToRemote;

	std::mutex cs_map_local_or_remote_;
	MapRemoteIdToLocal	map_remote_id_to_local_;
	MapRemoteAddrToLocal map_remote_addr_to_local_;
	MapLocalPortToRemote map_local_port_to_remote_;
};

#endif	// __SINGLE_PORT_H__