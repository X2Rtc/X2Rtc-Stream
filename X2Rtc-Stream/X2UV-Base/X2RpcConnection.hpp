#ifndef X2_MS_RTC_RPC_CONNECTION_HPP
#define X2_MS_RTC_RPC_CONNECTION_HPP

#include "X2common.hpp"
#include "handles/X2TcpConnectionHandler.hpp"

namespace x2rtc
{
	class X2RpcConnection : public ::X2TcpConnectionHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTcpConnectionPacketReceived(
				x2rtc::X2RpcConnection* connection, const uint8_t* data, size_t len) = 0;
		};

	public:
		X2RpcConnection(Listener* listener, size_t bufferSize);
		~X2RpcConnection() override;

	public:
		void Send(const uint8_t* data, size_t len, ::X2TcpConnectionHandler::onSendCallback* cb);

		/* Pure virtual methods inherited from ::TcpConnectionHandler. */
	public:
		void UserOnTcpConnectionRead() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		size_t frameStart{ 0u }; // Where the latest frame starts.
	};
} // namespace x2rtc

#endif
