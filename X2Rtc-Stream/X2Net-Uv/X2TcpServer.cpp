#define MS_CLASS "RTC::X2TcpServer"
// #define MS_LOG_DEV_LEVEL 3

#include "X2TcpServer.h"
#include "X2Logger.hpp"
#include "X2PortManager.h"
#include <string>
#include "X2MediaSoupErrors.hpp"

namespace x2rtc {
	/* Static. */

	static constexpr size_t MaxTcpConnectionsPerServer{ 4096 };

	/* Instance methods. */

	X2TcpServer::X2TcpServer(
	  Listener* listener)
	  :listener(listener), fixedPort(true)
	{
		MS_TRACE();
	}

	X2TcpServer::~X2TcpServer()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			::X2PortManager::UnbindTcp(this->localIp, this->localPort);
		}
	}

	bool X2TcpServer::Listen(std::string& ip, uint16_t port)
	{// This may throw.
		try {
			uv_tcp_t* uvHandle = ::X2PortManager::BindTcp(ip, port);
			if (uvHandle != NULL)
			{
				bool ret = ::X2TcpServerHandler::X2TcpServerHandler::Init(uvHandle);
			}
			else {
				return false;
			}
		}
		catch (X2MediaSoupError error) {
			::X2TcpServerHandler::X2TcpServerHandler::Close();

			return false;
		}
		
		return true;
	}

	void X2TcpServer::UserOnTcpConnectionAlloc()
	{
		MS_TRACE();

		// Allow just MaxTcpConnectionsPerServer.
		if (GetNumConnections() >= MaxTcpConnectionsPerServer)
		{
			MS_ERROR("cannot handle more than %zu connections", MaxTcpConnectionsPerServer);

			return;
		}

		// Allocate a new RTC::X2TcpConnection for the X2TcpServer to handle it.
		X2TcpDataHanlder* x2TcpDataHanlder = new X2TcpDataHanlder(65536);
		
		// Accept it.
		AcceptTcpConnection(x2TcpDataHanlder);

		X2NetTcpConnection* x2NetTcpConnection = new x2rtc::X2NetTcpConnection();
		x2TcpDataHanlder->SetUserData(x2NetTcpConnection);
		x2NetTcpConnection->SetX2TcpDataHanlder(x2TcpDataHanlder);

		this->listener->OnRtcTcpConnectionOpen(this, x2NetTcpConnection);
	}

	void X2TcpServer::UserOnTcpConnectionClosed(::X2TcpConnectionHandler* connection)
	{
		MS_TRACE();

		X2NetTcpConnection* x2NetTcpConnection = ((x2rtc::X2NetTcpConnection*)static_cast<x2rtc::X2TcpDataHanlder*>(connection)->GetUserData());
		static_cast<x2rtc::X2TcpDataHanlder*>(connection)->SetClosed();
		x2NetTcpConnection->SetX2TcpDataHanlder(NULL);

		this->listener->OnRtcTcpConnectionClose(this, x2NetTcpConnection);
	}
} // namespace RTC
