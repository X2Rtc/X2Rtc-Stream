#define MS_CLASS "Channel::ChannelSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelSocket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cmath>   // std::ceil()
#include <cstring> // std::memcpy(), std::memmove()

namespace Channel
{
	// Binary length for a 4194304 bytes payload.
	static constexpr size_t MessageMaxLen{ 4194308 };
	static constexpr size_t PayloadMaxLen{ 4194304 };

	/* Static methods for UV callbacks. */

	inline static void onAsync(uv_handle_t* handle)
	{
		while (static_cast<ChannelSocket*>(handle->data)->CallbackRead())
		{
			// Read while there are new messages.
		}
	}

	inline static void onCloseAsync(uv_handle_t* handle)
	{
		delete reinterpret_cast<uv_async_t*>(handle);
	}

	/* Instance methods. */

	ChannelSocket::ChannelSocket(int consumerFd, int producerFd)
	  : consumerSocket(new ConsumerSocket(consumerFd, MessageMaxLen, this)),
	    producerSocket(new ProducerSocket(producerFd, MessageMaxLen)),
	    writeBuffer(static_cast<uint8_t*>(std::malloc(MessageMaxLen)))
	{
		MS_TRACE_STD();
	}

	ChannelSocket::ChannelSocket(
	  ChannelReadFn channelReadFn,
	  ChannelReadCtx channelReadCtx,
	  ChannelWriteFn channelWriteFn,
	  ChannelWriteCtx channelWriteCtx)
	  : channelReadFn(channelReadFn), channelReadCtx(channelReadCtx), channelWriteFn(channelWriteFn),
	    channelWriteCtx(channelWriteCtx)
	{
		MS_TRACE_STD();

		int err;

		this->uvReadHandle       = new uv_async_t;
		this->uvReadHandle->data = static_cast<void*>(this);

		err =
		  uv_async_init(DepLibUV::GetLoop(), this->uvReadHandle, reinterpret_cast<uv_async_cb>(onAsync));

		if (err != 0)
		{
			delete this->uvReadHandle;
			this->uvReadHandle = nullptr;

			MS_THROW_ERROR_STD("uv_async_init() failed: %s", uv_strerror(err));
		}

		err = uv_async_send(this->uvReadHandle);

		if (err != 0)
		{
			delete this->uvReadHandle;
			this->uvReadHandle = nullptr;

			MS_THROW_ERROR_STD("uv_async_send() failed: %s", uv_strerror(err));
		}
	}

	ChannelSocket::~ChannelSocket()
	{
		MS_TRACE_STD();

		std::free(this->writeBuffer);

		if (!this->closed)
			Close();

		delete this->consumerSocket;
		delete this->producerSocket;
	}

	void ChannelSocket::Close()
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		this->closed = true;

		if (this->uvReadHandle)
		{
			uv_close(
			  reinterpret_cast<uv_handle_t*>(this->uvReadHandle), static_cast<uv_close_cb>(onCloseAsync));
		}

		if (this->consumerSocket)
		{
			this->consumerSocket->Close();
		}

		if (this->producerSocket)
		{
			this->producerSocket->Close();
		}
	}

	void ChannelSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void ChannelSocket::Send(json& jsonMessage)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		std::string message = jsonMessage.dump();

		if (message.length() > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(
		  reinterpret_cast<const uint8_t*>(message.c_str()), static_cast<uint32_t>(message.length()));
	}

	void ChannelSocket::Send(const std::string& message)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		if (message.length() > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(
		  reinterpret_cast<const uint8_t*>(message.c_str()), static_cast<uint32_t>(message.length()));
	}

	void ChannelSocket::SendLog(const char* message, uint32_t messageLen)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		if (messageLen > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(reinterpret_cast<const uint8_t*>(message), messageLen);
	}

	bool ChannelSocket::CallbackRead()
	{
		MS_TRACE_STD();

		if (this->closed)
			return false;

		uint8_t* message{ nullptr };
		uint32_t messageLen;
		size_t messageCtx;

		// Try to read next message using `channelReadFn`, message, its length and context will be
		// stored in provided arguments.
		auto free = this->channelReadFn(
		  &message, &messageLen, &messageCtx, this->uvReadHandle, this->channelReadCtx);

		// Non-null free function pointer means message was successfully read above and will need to be
		// freed later.
		if (free)
		{
			try
			{
				char* charMessage{ reinterpret_cast<char*>(message) };

				auto* request = new Channel::ChannelRequest(this, charMessage, messageLen);

				// Notify the listener.
				try
				{
					this->listener->HandleRequest(request);
				}
				catch (const MediaSoupTypeError& error)
				{
					request->TypeError(error.what());
				}
				catch (const MediaSoupError& error)
				{
					request->Error(error.what());
				}

				// Delete the Request.
				delete request;
			}
			catch (const json::parse_error& error)
			{
				MS_ERROR_STD("message parsing error: %s", error.what());
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR_STD("discarding wrong Channel request: %s", error.what());
			}

			// Message needs to be freed using stored function pointer.
			free(message, messageLen, messageCtx);
		}

		// Return `true` if something was processed.
		return free != nullptr;
	}

	inline void ChannelSocket::SendImpl(const uint8_t* payload, uint32_t payloadLen)
	{
		MS_TRACE_STD();

		// Write using function call if provided.
		if (this->channelWriteFn)
		{
			this->channelWriteFn(payload, payloadLen, this->channelWriteCtx);
		}
		else
		{
			std::memcpy(this->writeBuffer, &payloadLen, sizeof(uint32_t));

			if (payloadLen != 0)
			{
				std::memcpy(this->writeBuffer + sizeof(uint32_t), payload, payloadLen);
			}

			size_t len = sizeof(uint32_t) + payloadLen;

			this->producerSocket->Write(this->writeBuffer, len);
		}
	}

	void ChannelSocket::OnConsumerSocketMessage(ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
		MS_TRACE_STD();

		try
		{
			auto* request = new Channel::ChannelRequest(this, msg, msgLen);

			// Notify the listener.
			try
			{
				this->listener->HandleRequest(request);
			}
			catch (const MediaSoupTypeError& error)
			{
				request->TypeError(error.what());
			}
			catch (const MediaSoupError& error)
			{
				request->Error(error.what());
			}

			// Delete the Request.
			delete request;
		}
		catch (const json::parse_error& error)
		{
			MS_ERROR_STD("JSON parsing error: %s", error.what());
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR_STD("discarding wrong Channel request: %s", error.what());
		}
	}

	void ChannelSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
		MS_TRACE_STD();

		this->listener->OnChannelClosed(this);
	}

	/* Instance methods. */

	ConsumerSocket::ConsumerSocket(int fd, size_t bufferSize, Listener* listener)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::CONSUMER), listener(listener)
	{
		MS_TRACE_STD();
	}

	ConsumerSocket::~ConsumerSocket()
	{
		MS_TRACE_STD();
	}

	void ConsumerSocket::UserOnUnixStreamRead()
	{
		MS_TRACE_STD();

		size_t msgStart{ 0 };

		// Be ready to parse more than a single message in a single chunk.
		while (true)
		{
			if (IsClosed())
				return;

			size_t readLen = this->bufferDataLen - msgStart;

			if (readLen < sizeof(uint32_t))
			{
				// Incomplete data.
				break;
			}

			uint32_t msgLen;
			// Read message length.
			std::memcpy(&msgLen, this->buffer + msgStart, sizeof(uint32_t));

			if (readLen < sizeof(uint32_t) + static_cast<size_t>(msgLen))
			{
				// Incomplete data.
				break;
			}

			this->listener->OnConsumerSocketMessage(
			  this,
			  reinterpret_cast<char*>(this->buffer + msgStart + sizeof(uint32_t)),
			  static_cast<size_t>(msgLen));

			msgStart += sizeof(uint32_t) + static_cast<size_t>(msgLen);
		}

		if (msgStart != 0)
		{
			this->bufferDataLen = this->bufferDataLen - msgStart;

			if (this->bufferDataLen != 0)
			{
				std::memmove(this->buffer, this->buffer + msgStart, this->bufferDataLen);
			}
		}
	}

	void ConsumerSocket::UserOnUnixStreamSocketClosed()
	{
		MS_TRACE_STD();

		// Notify the listener.
		this->listener->OnConsumerSocketClosed(this);
	}

	/* Instance methods. */

	ProducerSocket::ProducerSocket(int fd, size_t bufferSize)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::PRODUCER)
	{
		MS_TRACE_STD();
	}

	/* Instance methods. */
	ChannelPSocket::ChannelPSocket(int consumerFd, int producerFd)
		: consumerSocket(consumerFd, MessageMaxLen, this), producerSocket(producerFd, MessageMaxLen)
	{
		MS_TRACE_STD();

		this->writeBuffer = static_cast<uint8_t*>(std::malloc(MessageMaxLen));
	}

	ChannelPSocket::~ChannelPSocket()
	{
		MS_TRACE_STD();

		std::free(this->writeBuffer);

		if (!this->closed)
			Close();
	}

	void ChannelPSocket::Close()
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		this->closed = true;

		this->consumerSocket.Close();
		this->producerSocket.Close();
	}

	void ChannelPSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void ChannelPSocket::Send(json& jsonMessage)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		std::string message = jsonMessage.dump();

		if (message.length() > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(message.c_str(), static_cast<uint32_t>(message.length()));
	}

	void ChannelPSocket::SendLog(char* message, uint32_t messageLen)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		if (messageLen > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(message, messageLen);
	}

	inline void ChannelPSocket::SendImpl(const void* payload, uint32_t payloadLen)
	{
		MS_TRACE_STD();

		std::memcpy(this->writeBuffer, &payloadLen, sizeof(uint32_t));

		if (payloadLen != 0)
		{
			std::memcpy(this->writeBuffer + sizeof(uint32_t), payload, payloadLen);
		}

		size_t len = sizeof(uint32_t) + payloadLen;

		this->producerSocket.Write(this->writeBuffer, len);
	}

	void ChannelPSocket::OnConsumerSocketMessage(ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
		MS_TRACE_STD();

		try
		{
			json jsonMessage = json::parse(msg, msg + msgLen);

			// Notify the listener.
			{
				this->listener->OnChannelPResponse(this, &jsonMessage);
			}
		}
		catch (const json::parse_error& error)
		{
			MS_ERROR_STD("JSON parsing error: %s msg: %s", error.what(), msg);
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR_STD("discarding wrong Channel request");
		}
	}

	void ChannelPSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
		MS_TRACE_STD();

		this->listener->OnChannelPClosed(this);
	}
} // namespace Channel
