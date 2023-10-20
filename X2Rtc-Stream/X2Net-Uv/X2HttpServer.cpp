#define MS_CLASS "x2rtc::X2HttpServer"
// #define MS_LOG_DEV_LEVEL 3

#include "X2HttpServer.h"
#include "X2Logger.hpp"
#include "X2PortManager.h"
#include <string>
#include "X2MediaSoupErrors.hpp"

namespace x2rtc
{
	/* Instance methods. */

	X2HttpServer::X2HttpServer(Listener* listener, x2rtc::X2HttpConnection::Listener* connListener)
	  : // This may throw.
		listener(listener),
	    connListener(connListener)
	{
		MS_TRACE();
	}

	X2HttpServer::~X2HttpServer()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			x2rtc::X2PortManager::UnbindTcp(this->localIp, this->localPort);
		}
	}

	bool X2HttpServer::Listen(std::string& ip, uint16_t port)
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

	void X2HttpServer::UserOnTcpConnectionAlloc()
	{
		MS_TRACE();

		// Allocate a new x2rtc::X2HttpConnection for the X2HttpServer to handle it.
		x2rtc::X2HttpConnection* connection = NULL;
		//@Eric - 20230612 - Can use self defined tcp connection.
		this->listener->OnRtcTcpConnectionAccept(this, &connection);
		if (connection == NULL) {
			connection =new x2rtc::X2HttpConnection(this->connListener, 65536);
		}

		// Accept it.
		AcceptTcpConnection(connection);
	}

	void X2HttpServer::UserOnTcpConnectionClosed(::X2TcpConnectionHandler* connection)
	{
		MS_TRACE();

		this->listener->OnRtcTcpConnectionClosed(this, static_cast<x2rtc::X2HttpConnection*>(connection));
	}
} // namespace x2rtc
