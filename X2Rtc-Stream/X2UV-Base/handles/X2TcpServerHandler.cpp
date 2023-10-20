#define MS_CLASS "X2TcpServerHandler"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/X2TcpServerHandler.hpp"
#include "X2Logger.hpp"
#include "X2MediaSoupErrors.hpp"
#include "X2Utils.hpp"

/* Static. */

static constexpr int ListenBacklog{ 512 };

/* Static methods for UV callbacks. */

inline static void onConnection(uv_stream_t* handle, int status)
{
	auto* server = static_cast<X2TcpServerHandler*>(handle->data);

	if (server)
		server->OnUvConnection(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
X2TcpServerHandler::X2TcpServerHandler()
{
	
}

X2TcpServerHandler::~X2TcpServerHandler()
{
	MS_TRACE();

	if (!this->closed)
		Close();
}

bool X2TcpServerHandler::Init(uv_tcp_t* uvHandle)
{
	this->uvHandle = uvHandle;

	MS_TRACE();

	int err;

	this->uvHandle->data = static_cast<void*>(this);

	err = uv_listen(
		reinterpret_cast<uv_stream_t*>(this->uvHandle),
		ListenBacklog,
		static_cast<uv_connection_cb>(onConnection));

	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

		MS_THROW_ERROR("error setting local IP and port");
	}

	return true;
}

void X2TcpServerHandler::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	this->closed = true;

	// Tell the UV handle that the X2TcpServerHandler has been closed.
	if (this->uvHandle != NULL) {
		this->uvHandle->data = nullptr;
	}

	MS_DEBUG_DEV("closing %zu active connections", this->connections.size());

	for (auto* connection : this->connections)
	{
		// Notify the subclass.
		UserOnTcpConnectionClosed(connection);		//@Eric - Add for notity connection closed

		delete connection;
	}

	if (this->uvHandle != NULL) {
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
	}
}

void X2TcpServerHandler::Dump() const
{
	MS_DUMP("<X2TcpServerHandler>");
	MS_DUMP(
	  "  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
	  this->localIp.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->closed) ? "open" : "closed",
	  this->connections.size());
	MS_DUMP("</X2TcpServerHandler>");
}

void X2TcpServerHandler::AcceptTcpConnection(X2TcpConnectionHandler* connection)
{
	MS_TRACE();

	MS_ASSERT(connection != nullptr, "X2TcpConnectionHandler pointer was not allocated by the user");

	try
	{
		connection->Setup(this, &(this->localAddr), this->localIp, this->localPort);
	}
	catch (const X2MediaSoupError& error)
	{
		delete connection;

		return;
	}

	// Accept the connection.
	const int err = uv_accept(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  reinterpret_cast<uv_stream_t*>(connection->GetUvHandle()));

	if (err != 0)
		MS_THROW_ERROR("uv_accept() failed: %s", uv_strerror(err));

	// Start receiving data.
	try
	{
		// NOTE: This may throw.
		connection->Start();
	}
	catch (const X2MediaSoupError& error)
	{
		delete connection;

		return;
	}

	// Store it.
	this->connections.insert(connection);
}

bool X2TcpServerHandler::SetLocalAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->localAddr);

	err =
	  uv_tcp_getsockname(this->uvHandle, reinterpret_cast<struct sockaddr*>(&this->localAddr), &len);

	if (err != 0)
	{
		MS_ERROR("uv_tcp_getsockname() failed: %s", uv_strerror(err));

		return false;
	}

	int family;

	X2Utils::IP::GetAddressInfo(
	  reinterpret_cast<const struct sockaddr*>(&this->localAddr), family, this->localIp, this->localPort);

	return true;
}

inline void X2TcpServerHandler::OnUvConnection(int status)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (status != 0)
	{
		MS_ERROR("error while receiving a new TCP connection: %s", uv_strerror(status));

		return;
	}

	// Notify the subclass about a new TCP connection attempt.
	UserOnTcpConnectionAlloc();
}

inline void X2TcpServerHandler::OnTcpConnectionClosed(X2TcpConnectionHandler* connection)
{
	MS_TRACE();

	MS_DEBUG_DEV("TCP connection closed");

	// Remove the X2TcpConnectionHandler from the set.
	this->connections.erase(connection);

	// Notify the subclass.
	UserOnTcpConnectionClosed(connection);

	// Delete it.
	delete connection;
}
