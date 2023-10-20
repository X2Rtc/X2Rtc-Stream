#ifndef MS_RTC_DATA_CONSUMER_HPP
#define MS_RTC_DATA_CONSUMER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include "RTC/SctpDictionaries.hpp"
#include "RTC/Shared.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace RTC
{
	// Define class here such that we can use it even though we don't know what it looks like yet
	// (this is to avoid circular dependencies).
	class SctpAssociation;

	class DataConsumer : public Channel::ChannelSocket::RequestHandler,
	                     public PayloadChannel::PayloadChannelSocket::RequestHandler
	{
	protected:
		using onQueuedCallback = const std::function<void(bool queued, bool sctpSendBufferFull)>;

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnDataConsumerSendMessage(
			  RTC::DataConsumer* dataConsumer,
			  uint32_t ppid,
			  const uint8_t* msg,
			  size_t len,
			  onQueuedCallback* cb)                                                        = 0;
			virtual void OnDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer) = 0;
		};

	public:
		enum class Type : uint8_t
		{
			SCTP = 0,
			DIRECT
		};

	public:
		DataConsumer(
		  RTC::Shared* shared,
		  const std::string& id,
		  const std::string& dataProducerId,
		  RTC::SctpAssociation* sctpAssociation,
		  RTC::DataConsumer::Listener* listener,
		  json& data,
		  size_t maxMessageSize);
		virtual ~DataConsumer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		Type GetType() const
		{
			return this->type;
		}
		const RTC::SctpStreamParameters& GetSctpStreamParameters() const
		{
			return this->sctpStreamParameters;
		}
		bool IsActive() const
		{
			// clang-format off
			return (
				this->transportConnected &&
				(this->type == DataConsumer::Type::DIRECT || this->sctpAssociationConnected) &&
				!this->dataProducerClosed
			);
			// clang-format on
		}
		void TransportConnected();
		void TransportDisconnected();
		void SctpAssociationConnected();
		void SctpAssociationClosed();
		void SctpAssociationBufferedAmount(uint32_t bufferedAmount);
		void SctpAssociationSendBufferFull();
		void DataProducerClosed();
		void SendMessage(uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* = nullptr);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

		/* Methods inherited from PayloadChannel::PayloadChannelSocket::RequestHandler. */
	public:
		void HandleRequest(PayloadChannel::PayloadChannelRequest* request) override;

	public:
		// Passed by argument.
		const std::string id;
		const std::string dataProducerId;

	private:
		// Passed by argument.
		RTC::Shared* shared{ nullptr };
		RTC::SctpAssociation* sctpAssociation{ nullptr };
		RTC::DataConsumer::Listener* listener{ nullptr };
		size_t maxMessageSize{ 0u };
		// Others.
		Type type;
		std::string typeString;
		RTC::SctpStreamParameters sctpStreamParameters;
		std::string label;
		std::string protocol;
		bool transportConnected{ false };
		bool sctpAssociationConnected{ false };
		bool dataProducerClosed{ false };
		size_t messagesSent{ 0u };
		size_t bytesSent{ 0u };
		uint32_t bufferedAmount{ 0u };
		uint32_t bufferedAmountLowThreshold{ 0u };
		bool forceTriggerBufferedAmountLow{ false };
	};
} // namespace RTC

#endif
