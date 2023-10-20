#ifndef MS_UNIX_STREAM_SOCKET_HPP
#define MS_UNIX_STREAM_SOCKET_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class UnixStreamSocket
{
public:
	/* Struct for the data field of uv_req_t when writing data. */
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
		}

		uv_write_t req;
		uint8_t* store{ nullptr };
	};

	enum class Role
	{
		PRODUCER = 1,
		CONSUMER
	};

public:
	UnixStreamSocket(int fd, size_t bufferSize, UnixStreamSocket::Role role);
	UnixStreamSocket& operator=(const UnixStreamSocket&) = delete;
	UnixStreamSocket(const UnixStreamSocket&)            = delete;
	virtual ~UnixStreamSocket();

public:
	void Close();
	bool IsClosed() const
	{
		return this->closed;
	}
	void Write(const uint8_t* data, size_t len);

	/* Callbacks fired by UV events. */
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRead(ssize_t nread, const uv_buf_t* buf);
	void OnUvWriteError(int error);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnUnixStreamRead()         = 0;
	virtual void UserOnUnixStreamSocketClosed() = 0;

private:
	// Allocated by this.
	uv_pipe_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	bool isClosedByPeer{ false };
	bool hasError{ false };

protected:
	// Passed by argument.
	size_t bufferSize{ 0u };
	UnixStreamSocket::Role role;
	// Allocated by this.
	uint8_t* buffer{ nullptr };
	// Others.
	size_t bufferDataLen{ 0u };
};

#endif
