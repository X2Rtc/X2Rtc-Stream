#define MS_CLASS "RTC::X2UdpSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/X2UdpSocket.hpp"
#include "X2Logger.hpp"
#include "X2PortManager.h"
#include <string>

namespace x2rtc
{
	/* Instance methods. */

	X2UdpSocket::X2UdpSocket(Listener* listener, std::string& ip)
	  : // This may throw.
	    ::X2UdpSocketHandler::X2UdpSocketHandler(x2rtc::X2PortManager::BindUdp(ip)), listener(listener)
	{
		MS_TRACE();
	}

	X2UdpSocket::X2UdpSocket(Listener* listener, std::string& ip, uint16_t port)
	  : // This may throw.
	    ::X2UdpSocketHandler::X2UdpSocketHandler(x2rtc::X2PortManager::BindUdp(ip, port)), listener(listener),
	    fixedPort(true)
	{
		MS_TRACE();
	}

	X2UdpSocket::~X2UdpSocket()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			x2rtc::X2PortManager::UnbindUdp(this->localIp, this->localPort);
		}
	}

	void X2UdpSocket::UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->OnUdpSocketPacketReceived(this, data, len, addr);
	}
} // namespace x2rtc
