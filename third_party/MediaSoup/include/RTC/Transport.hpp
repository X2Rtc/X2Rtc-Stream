#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP
// #define ENABLE_RTC_SENDER_BANDWIDTH_ESTIMATOR

#include "common.hpp"
#include "DepLibUV.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelNotification.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/DataConsumer.hpp"
#include "RTC/DataProducer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SctpAssociation.hpp"
#include "RTC/SctpListener.hpp"
#include "RTC/Shared.hpp"
#ifdef ENABLE_RTC_SENDER_BANDWIDTH_ESTIMATOR
#include "RTC/SenderBandwidthEstimator.hpp"
#endif
#include "RTC/TransportCongestionControlClient.hpp"
#include "RTC/TransportCongestionControlServer.hpp"
#include "handles/Timer.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace RTC
{
	class Transport : public RTC::Producer::Listener,
	                  public RTC::Consumer::Listener,
	                  public RTC::DataProducer::Listener,
	                  public RTC::DataConsumer::Listener,
	                  public RTC::SctpAssociation::Listener,
	                  public RTC::TransportCongestionControlClient::Listener,
	                  public RTC::TransportCongestionControlServer::Listener,
	                  public Channel::ChannelSocket::RequestHandler,
	                  public PayloadChannel::PayloadChannelSocket::RequestHandler,
	                  public PayloadChannel::PayloadChannelSocket::NotificationHandler,
#ifdef ENABLE_RTC_SENDER_BANDWIDTH_ESTIMATOR
	                  public RTC::SenderBandwidthEstimator::Listener,
#endif
	                  public Timer::Listener
	{
	protected:
		using onSendCallback   = const std::function<void(bool sent)>;
		using onQueuedCallback = const std::function<void(bool queued, bool sctpSendBufferFull)>;

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerNewRtpStream(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  RTC::RtpStreamRecv* rtpStream,
			  uint32_t mappedSsrc) = 0;
			virtual void OnTransportProducerRtpStreamScore(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  RTC::RtpStreamRecv* rtpStream,
			  uint8_t score,
			  uint8_t previousScore) = 0;
			virtual void OnTransportProducerRtcpSenderReport(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  RTC::RtpStreamRecv* rtpStream,
			  bool first) = 0;
			virtual void OnTransportProducerRtpPacketReceived(
			  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnTransportNeedWorstRemoteFractionLost(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  uint32_t mappedSsrc,
			  uint8_t& worstRemoteFractionLost) = 0;
			virtual void OnTransportNewConsumer(
			  RTC::Transport* transport, RTC::Consumer* consumer, std::string& producerId) = 0;
			virtual void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) = 0;
			virtual void OnTransportConsumerProducerClosed(
			  RTC::Transport* transport, RTC::Consumer* consumer) = 0;
			virtual void OnTransportConsumerKeyFrameRequested(
			  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;
			virtual void OnTransportNewDataProducer(
			  RTC::Transport* transport, RTC::DataProducer* dataProducer) = 0;
			virtual void OnTransportDataProducerClosed(
			  RTC::Transport* transport, RTC::DataProducer* dataProducer) = 0;
			virtual void OnTransportDataProducerMessageReceived(
			  RTC::Transport* transport,
			  RTC::DataProducer* dataProducer,
			  uint32_t ppid,
			  const uint8_t* msg,
			  size_t len) = 0;
			virtual void OnTransportNewDataConsumer(
			  RTC::Transport* transport, RTC::DataConsumer* dataConsumer, std::string& dataProducerId) = 0;
			virtual void OnTransportDataConsumerClosed(
			  RTC::Transport* transport, RTC::DataConsumer* dataConsumer) = 0;
			virtual void OnTransportDataConsumerDataProducerClosed(
			  RTC::Transport* transport, RTC::DataConsumer* dataConsumer)         = 0;
			virtual void OnTransportListenServerClosed(RTC::Transport* transport) = 0;
		};

	private:
		struct TraceEventTypes
		{
			bool probation{ false };
			bool bwe{ false };
		};

	public:
		Transport(RTC::Shared* shared, const std::string& id, Listener* listener, json& data);
		virtual ~Transport();

	public:
		void CloseProducersAndConsumers();
		void ListenServerClosed();
		// Subclasses must also invoke the parent Close().
		virtual void FillJson(json& jsonObject) const;
		virtual void FillJsonStats(json& jsonArray);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

		/* Methods inherited from PayloadChannel::PayloadChannelSocket::RequestHandler. */
	public:
		void HandleRequest(PayloadChannel::PayloadChannelRequest* request) override;

		/* Methods inherited from PayloadChannel::PayloadChannelSocket::NotificationHandler. */
	public:
		void HandleNotification(PayloadChannel::PayloadChannelNotification* notification) override;

	protected:
		// Must be called from the subclass.
		void Destroying();
		void Connected();
		void Disconnected();
		void DataReceived(size_t len)
		{
			this->recvTransmission.Update(len, DepLibUV::GetTimeMs());
		}
		void DataSent(size_t len)
		{
			this->sendTransmission.Update(len, DepLibUV::GetTimeMs());
		}
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpPacket(RTC::RTCP::Packet* packet);
		void ReceiveSctpData(const uint8_t* data, size_t len);
		void SetNewProducerIdFromData(json& data, std::string& producerId) const;
		RTC::Producer* GetProducerFromData(json& data) const;
		void SetNewConsumerIdFromData(json& data, std::string& consumerId) const;
		RTC::Consumer* GetConsumerFromData(json& data) const;
		RTC::Consumer* GetConsumerByMediaSsrc(uint32_t ssrc) const;
		RTC::Consumer* GetConsumerByRtxSsrc(uint32_t ssrc) const;
		void SetNewDataProducerIdFromData(json& data, std::string& dataProducerId) const;
		RTC::DataProducer* GetDataProducerFromData(json& data) const;
		void SetNewDataConsumerIdFromData(json& data, std::string& dataConsumerId) const;
		RTC::DataConsumer* GetDataConsumerFromData(json& data) const;

	private:
		virtual bool IsConnected() const = 0;
		virtual void SendRtpPacket(
		  RTC::Consumer* consumer, RTC::RtpPacket* packet, onSendCallback* cb = nullptr) = 0;
		void HandleRtcpPacket(RTC::RTCP::Packet* packet);
		void SendRtcp(uint64_t nowMs);
		virtual void SendRtcpPacket(RTC::RTCP::Packet* packet)                 = 0;
		virtual void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) = 0;
		virtual void SendMessage(
		  RTC::DataConsumer* dataConsumer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len,
		  onQueuedCallback* = nullptr)                             = 0;
		virtual void SendSctpData(const uint8_t* data, size_t len) = 0;
		virtual void RecvStreamClosed(uint32_t ssrc)               = 0;
		virtual void SendStreamClosed(uint32_t ssrc)               = 0;
		void DistributeAvailableOutgoingBitrate();
		void ComputeOutgoingDesiredBitrate(bool forceBitrate = false);
		void EmitTraceEventProbationType(RTC::RtpPacket* packet) const;
		void EmitTraceEventBweType(RTC::TransportCongestionControlClient::Bitrates& bitrates) const;

		/* Pure virtual methods inherited from RTC::Producer::Listener. */
	public:
		void OnProducerReceiveData(RTC::Producer* /*producer*/, size_t len) override
		{
			this->DataReceived(len);
		}
		void OnProducerReceiveRtpPacket(RTC::Producer* /*producer*/, RTC::RtpPacket* packet) override
		{
			this->ReceiveRtpPacket(packet);
		}
		void OnProducerPaused(RTC::Producer* producer) override;
		void OnProducerResumed(RTC::Producer* producer) override;
		void OnProducerNewRtpStream(
		  RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void OnProducerRtpStreamScore(
		  RTC::Producer* producer,
		  RTC::RtpStreamRecv* rtpStream,
		  uint8_t score,
		  uint8_t previousScore) override;
		void OnProducerRtcpSenderReport(
		  RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, bool first) override;
		void OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnProducerSendRtcpPacket(RTC::Producer* producer, RTC::RTCP::Packet* packet) override;
		void OnProducerNeedWorstRemoteFractionLost(
		  RTC::Producer* producer, uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) override;
		void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) override;
		void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) override;
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) override;
		void OnConsumerNeedZeroBitrate(RTC::Consumer* consumer) override;
		void OnConsumerProducerClosed(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from RTC::DataProducer::Listener. */
	public:
		void OnDataProducerReceiveData(RTC::DataProducer* /*dataProducer*/, size_t len) override
		{
			this->DataReceived(len);
		}
		void OnDataProducerMessageReceived(
		  RTC::DataProducer* dataProducer, uint32_t ppid, const uint8_t* msg, size_t len) override;

		/* Pure virtual methods inherited from RTC::DataConsumer::Listener. */
	public:
		void OnDataConsumerSendMessage(
		  RTC::DataConsumer* dataConsumer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len,
		  onQueuedCallback* = nullptr) override;
		void OnDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer) override;

		/* Pure virtual methods inherited from RTC::SctpAssociation::Listener. */
	public:
		void OnSctpAssociationConnecting(RTC::SctpAssociation* sctpAssociation) override;
		void OnSctpAssociationConnected(RTC::SctpAssociation* sctpAssociation) override;
		void OnSctpAssociationFailed(RTC::SctpAssociation* sctpAssociation) override;
		void OnSctpAssociationClosed(RTC::SctpAssociation* sctpAssociation) override;
		void OnSctpAssociationSendData(
		  RTC::SctpAssociation* sctpAssociation, const uint8_t* data, size_t len) override;
		void OnSctpAssociationMessageReceived(
		  RTC::SctpAssociation* sctpAssociation,
		  uint16_t streamId,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len) override;
		void OnSctpAssociationBufferedAmount(
		  RTC::SctpAssociation* sctpAssociation, uint32_t bufferedAmount) override;

		/* Pure virtual methods inherited from RTC::TransportCongestionControlClient::Listener. */
	public:
		void OnTransportCongestionControlClientBitrates(
		  RTC::TransportCongestionControlClient* tccClient,
		  RTC::TransportCongestionControlClient::Bitrates& bitrates) override;
		void OnTransportCongestionControlClientSendRtpPacket(
		  RTC::TransportCongestionControlClient* tccClient,
		  RTC::RtpPacket* packet,
		  const webrtc::PacedPacketInfo& pacingInfo) override;

		/* Pure virtual methods inherited from RTC::TransportCongestionControlServer::Listener. */
	public:
		void OnTransportCongestionControlServerSendRtcpPacket(
		  RTC::TransportCongestionControlServer* tccServer, RTC::RTCP::Packet* packet) override;

#ifdef ENABLE_RTC_SENDER_BANDWIDTH_ESTIMATOR
		/* Pure virtual methods inherited from RTC::SenderBandwidthEstimator::Listener. */
	public:
		void OnSenderBandwidthEstimatorAvailableBitrate(
		  RTC::SenderBandwidthEstimator* senderBwe,
		  uint32_t availableBitrate,
		  uint32_t previousAvailableBitrate) override;
#endif

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		const std::string id;

	protected:
		RTC::Shared* shared{ nullptr };
		size_t maxMessageSize{ 262144u };
		// Allocated by this.
		RTC::SctpAssociation* sctpAssociation{ nullptr };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		absl::flat_hash_map<std::string, RTC::Producer*> mapProducers;
		absl::flat_hash_map<std::string, RTC::Consumer*> mapConsumers;
		absl::flat_hash_map<std::string, RTC::DataProducer*> mapDataProducers;
		absl::flat_hash_map<std::string, RTC::DataConsumer*> mapDataConsumers;
		absl::flat_hash_map<uint32_t, RTC::Consumer*> mapSsrcConsumer;
		absl::flat_hash_map<uint32_t, RTC::Consumer*> mapRtxSsrcConsumer;
		Timer* rtcpTimer{ nullptr };
		std::shared_ptr<RTC::TransportCongestionControlClient> tccClient{ nullptr };
		std::shared_ptr<RTC::TransportCongestionControlServer> tccServer{ nullptr };
#ifdef ENABLE_RTC_SENDER_BANDWIDTH_ESTIMATOR
		std::shared_ptr<RTC::SenderBandwidthEstimator> senderBwe{ nullptr };
#endif
		// Others.
		bool direct{ false }; // Whether this Transport allows PayloadChannel comm.
		bool destroying{ false };
		struct RTC::RtpHeaderExtensionIds recvRtpHeaderExtensionIds;
		RTC::RtpListener rtpListener;
		RTC::SctpListener sctpListener;
		RTC::RateCalculator recvTransmission;
		RTC::RateCalculator sendTransmission;
		RTC::RtpDataCounter recvRtpTransmission;
		RTC::RtpDataCounter sendRtpTransmission;
		RTC::RtpDataCounter recvRtxTransmission;
		RTC::RtpDataCounter sendRtxTransmission;
		RTC::RtpDataCounter sendProbationTransmission;
		uint16_t transportWideCcSeq{ 0u };
		uint32_t initialAvailableOutgoingBitrate{ 600000u };
		uint32_t maxIncomingBitrate{ 0u };
		uint32_t maxOutgoingBitrate{ 0u };
		uint32_t minOutgoingBitrate{ 0u };
		struct TraceEventTypes traceEventTypes;
	};
} // namespace RTC

#endif
