#ifndef MS_TCP_CONNECTION_HPP
#define MS_TCP_CONNECTION_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class TcpConnectionHandler
{
protected:
	using onSendCallback = const std::function<void(bool sent)>;

public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnTcpConnectionClosed(TcpConnectionHandler* connection) = 0;
	};

public:
	/* Struct for the data field of uv_req_t when writing into the connection. */
	struct UvWriteData
	{
		explicit UvWriteData(size_t storeSize)
		{
			this->store = new uint8_t[storeSize];
		}

		// Disable copy constructor because of the dynamically allocated data (store).
		UvWriteData(const UvWriteData&) = delete;

		~UvWriteData()
		{
			delete[] this->store;
			delete this->cb;
		}

		uv_write_t req;
		uint8_t* store{ nullptr };
		TcpConnectionHandler::onSendCallback* cb{ nullptr };
	};

public:
	explicit TcpConnectionHandler(size_t bufferSize);
	TcpConnectionHandler& operator=(const TcpConnectionHandler&) = delete;
	TcpConnectionHandler(const TcpConnectionHandler&)            = delete;
	virtual ~TcpConnectionHandler();

public:
	void Close();
	bool IsClosed() const
	{
		return this->closed;
	}
	virtual void Dump() const;
	void Setup(
	  Listener* listener,
	  struct sockaddr_storage* localAddr,
	  const std::string& localIp,
	  uint16_t localPort);
	uv_tcp_t* GetUvHandle() const
	{
		return this->uvHandle;
	}
	void Start();
	void Write(
	  const uint8_t* data1,
	  size_t len1,
	  const uint8_t* data2,
	  size_t len2,
	  TcpConnectionHandler::onSendCallback* cb);
	void ErrorReceiving();
	const struct sockaddr* GetLocalAddress() const
	{
		return reinterpret_cast<const struct sockaddr*>(this->localAddr);
	}
	int GetLocalFamily() const
	{
		return reinterpret_cast<const struct sockaddr*>(this->localAddr)->sa_family;
	}
	const std::string& GetLocalIp() const
	{
		return this->localIp;
	}
	uint16_t GetLocalPort() const
	{
		return this->localPort;
	}
	const struct sockaddr* GetPeerAddress() const
	{
		return reinterpret_cast<const struct sockaddr*>(&this->peerAddr);
	}
	const std::string& GetPeerIp() const
	{
		return this->peerIp;
	}
	uint16_t GetPeerPort() const
	{
		return this->peerPort;
	}
	size_t GetRecvBytes() const
	{
		return this->recvBytes;
	}
	size_t GetSentBytes() const
	{
		return this->sentBytes;
	}

private:
	bool SetPeerAddress();

	/* Callbacks fired by UV events. */
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRead(ssize_t nread, const uv_buf_t* buf);
	void OnUvWrite(int status, onSendCallback* cb);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnTcpConnectionRead() = 0;

protected:
	// Passed by argument.
	size_t bufferSize{ 0u };
	// Allocated by this.
	uint8_t* buffer{ nullptr };
	// Others.
	size_t bufferDataLen{ 0u };
	std::string localIp;
	uint16_t localPort{ 0u };
	struct sockaddr_storage peerAddr;
	std::string peerIp;
	uint16_t peerPort{ 0u };

private:
	// Passed by argument.
	Listener* listener{ nullptr };
	// Allocated by this.
	uv_tcp_t* uvHandle{ nullptr };
	// Others.
	struct sockaddr_storage* localAddr{ nullptr };
	bool closed{ false };
	size_t recvBytes{ 0u };
	size_t sentBytes{ 0u };
	bool isClosedByPeer{ false };
	bool hasError{ false };

public:
	//@Eric - 20230609 - used for uv_loop from user self defined
	void SetSelfUvLoop(uv_loop_t* loop) {
		uv_loop_ = loop;
	}
private:
	uv_loop_t* uv_loop_{nullptr};
};

#endif
