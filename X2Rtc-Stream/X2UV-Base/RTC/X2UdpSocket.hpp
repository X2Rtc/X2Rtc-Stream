#ifndef MS_RTC_UDP_SOCKET_HPP
#define MS_RTC_UDP_SOCKET_HPP

#include "X2common.hpp"
#include "handles/X2UdpSocketHandler.hpp"
#include <string>

namespace x2rtc
{
	class X2UdpSocket : public ::X2UdpSocketHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnUdpSocketPacketReceived(
			  x2rtc::X2UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) = 0;
		};

	public:
		X2UdpSocket(Listener* listener, std::string& ip);
		X2UdpSocket(Listener* listener, std::string& ip, uint16_t port);
		~X2UdpSocket() override;

		/* Pure virtual methods inherited from ::X2UdpSocketHandler. */
	public:
		void UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		bool fixedPort{ false };
	};
} // namespace x2rtc

#endif
