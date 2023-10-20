#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/ActiveSpeakerObserver.hpp"
#include "RTC/AudioLevelObserver.hpp"
#include "RTC/DirectTransport.hpp"
#include "RTC/PipeTransport.hpp"
#include "RTC/PlainTransport.hpp"
#include "RTC/WebRtcTransport.hpp"

namespace RTC
{
	/* Instance methods. */

	Router::Router(RTC::Shared* shared, const std::string& id, Listener* listener)
	  : id(id), shared(shared), listener(listener)
	{
		MS_TRACE();

		// NOTE: This may throw.
		this->shared->channelMessageRegistrator->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*payloadChannelRequestHandler*/ nullptr,
		  /*payloadChannelNotificationHandler*/ nullptr);
	}

	Router::~Router()
	{
		MS_TRACE();

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);

		// Close all Transports.
		for (auto& kv : this->mapTransports)
		{
			auto* transport = kv.second;

			delete transport;
		}
		this->mapTransports.clear();

		// Close all RtpObservers.
		for (auto& kv : this->mapRtpObservers)
		{
			auto* rtpObserver = kv.second;

			delete rtpObserver;
		}
		this->mapRtpObservers.clear();

		// Clear other maps.
		this->mapProducerConsumers.clear();
		this->mapConsumerProducer.clear();
		this->mapProducerRtpObservers.clear();
		this->mapProducers.clear();
		this->mapDataProducerDataConsumers.clear();
		this->mapDataConsumerDataProducer.clear();
		this->mapDataProducers.clear();
	}

	void Router::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add transportIds.
		jsonObject["transportIds"] = json::array();
		auto jsonTransportIdsIt    = jsonObject.find("transportIds");

		for (const auto& kv : this->mapTransports)
		{
			const auto& transportId = kv.first;

			jsonTransportIdsIt->emplace_back(transportId);
		}

		// Add rtpObserverIds.
		jsonObject["rtpObserverIds"] = json::array();
		auto jsonRtpObserverIdsIt    = jsonObject.find("rtpObserverIds");

		for (const auto& kv : this->mapRtpObservers)
		{
			const auto& rtpObserverId = kv.first;

			jsonRtpObserverIdsIt->emplace_back(rtpObserverId);
		}

		// Add mapProducerIdConsumerIds.
		jsonObject["mapProducerIdConsumerIds"] = json::object();
		auto jsonMapProducerConsumersIt        = jsonObject.find("mapProducerIdConsumerIds");

		for (const auto& kv : this->mapProducerConsumers)
		{
			auto* producer        = kv.first;
			const auto& consumers = kv.second;

			(*jsonMapProducerConsumersIt)[producer->id] = json::array();
			auto jsonProducerIdIt                       = jsonMapProducerConsumersIt->find(producer->id);

			for (auto* consumer : consumers)
			{
				jsonProducerIdIt->emplace_back(consumer->id);
			}
		}

		// Add mapConsumerIdProducerId.
		jsonObject["mapConsumerIdProducerId"] = json::object();
		auto jsonMapConsumerProducerIt        = jsonObject.find("mapConsumerIdProducerId");

		for (const auto& kv : this->mapConsumerProducer)
		{
			auto* consumer = kv.first;
			auto* producer = kv.second;

			(*jsonMapConsumerProducerIt)[consumer->id] = producer->id;
		}

		// Add mapProducerIdObserverIds.
		jsonObject["mapProducerIdObserverIds"] = json::object();
		auto jsonMapProducerRtpObserversIt     = jsonObject.find("mapProducerIdObserverIds");

		for (const auto& kv : this->mapProducerRtpObservers)
		{
			auto* producer           = kv.first;
			const auto& rtpObservers = kv.second;

			(*jsonMapProducerRtpObserversIt)[producer->id] = json::array();
			auto jsonProducerIdIt = jsonMapProducerRtpObserversIt->find(producer->id);

			for (auto* rtpObserver : rtpObservers)
			{
				jsonProducerIdIt->emplace_back(rtpObserver->id);
			}
		}

		// Add mapDataProducerIdDataConsumerIds.
		jsonObject["mapDataProducerIdDataConsumerIds"] = json::object();
		auto jsonMapDataProducerDataConsumersIt = jsonObject.find("mapDataProducerIdDataConsumerIds");

		for (const auto& kv : this->mapDataProducerDataConsumers)
		{
			auto* dataProducer        = kv.first;
			const auto& dataConsumers = kv.second;

			(*jsonMapDataProducerDataConsumersIt)[dataProducer->id] = json::array();
			auto jsonDataProducerIdIt = jsonMapDataProducerDataConsumersIt->find(dataProducer->id);

			for (auto* dataConsumer : dataConsumers)
			{
				jsonDataProducerIdIt->emplace_back(dataConsumer->id);
			}
		}

		// Add mapDataConsumerIdDataProducerId.
		jsonObject["mapDataConsumerIdDataProducerId"] = json::object();
		auto jsonMapDataConsumerDataProducerIt = jsonObject.find("mapDataConsumerIdDataProducerId");

		for (const auto& kv : this->mapDataConsumerDataProducer)
		{
			auto* dataConsumer = kv.first;
			auto* dataProducer = kv.second;

			(*jsonMapDataConsumerDataProducerIt)[dataConsumer->id] = dataProducer->id;
		}
	}

	void Router::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::ROUTER_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
			{
				std::string transportId;

				// This may throw.
				SetNewTransportIdFromData(request->data, transportId);

				// This may throw.
				auto* webRtcTransport =
				  new RTC::WebRtcTransport(this->shared, transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = webRtcTransport;

				MS_DEBUG_DEV("WebRtcTransport created [transportId:%s]", transportId.c_str());

				json data = json::object();

				webRtcTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER:
			{
				std::string transportId;

				// This may throw.
				SetNewTransportIdFromData(request->data, transportId);

				auto jsonWebRtcServerIdIt = request->data.find("webRtcServerId");

				if (jsonWebRtcServerIdIt == request->data.end() || !jsonWebRtcServerIdIt->is_string())
				{
					MS_THROW_TYPE_ERROR("missing webRtcServerId");
				}

				std::string webRtcServerId = jsonWebRtcServerIdIt->get<std::string>();

				auto* webRtcServer = this->listener->OnRouterNeedWebRtcServer(this, webRtcServerId);

				if (!webRtcServer)
					MS_THROW_ERROR("wrong webRtcServerId (no associated WebRtcServer found)");

				bool enableUdp{ true };
				auto jsonEnableUdpIt = request->data.find("enableUdp");

				if (jsonEnableUdpIt != request->data.end())
				{
					if (!jsonEnableUdpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong enableUdp (not a boolean)");

					enableUdp = jsonEnableUdpIt->get<bool>();
				}

				bool enableTcp{ false };
				auto jsonEnableTcpIt = request->data.find("enableTcp");

				if (jsonEnableTcpIt != request->data.end())
				{
					if (!jsonEnableTcpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong enableTcp (not a boolean)");

					enableTcp = jsonEnableTcpIt->get<bool>();
				}

				bool preferUdp{ false };
				auto jsonPreferUdpIt = request->data.find("preferUdp");

				if (jsonPreferUdpIt != request->data.end())
				{
					if (!jsonPreferUdpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong preferUdp (not a boolean)");

					preferUdp = jsonPreferUdpIt->get<bool>();
				}

				bool preferTcp{ false };
				auto jsonPreferTcpIt = request->data.find("preferTcp");

				if (jsonPreferTcpIt != request->data.end())
				{
					if (!jsonPreferTcpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong preferTcp (not a boolean)");

					preferTcp = jsonPreferTcpIt->get<bool>();
				}

				auto iceCandidates =
				  webRtcServer->GetIceCandidates(enableUdp, enableTcp, preferUdp, preferTcp);

				// This may throw.
				auto* webRtcTransport = new RTC::WebRtcTransport(
				  this->shared, transportId, this, webRtcServer, iceCandidates, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = webRtcTransport;

				MS_DEBUG_DEV(
				  "WebRtcTransport with WebRtcServer created [transportId:%s]", transportId.c_str());

				json data = json::object();

				webRtcTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_PLAIN_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromData(request->data, transportId);

				auto* plainTransport =
				  new RTC::PlainTransport(this->shared, transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = plainTransport;

				MS_DEBUG_DEV("PlainTransport created [transportId:%s]", transportId.c_str());

				json data = json::object();

				plainTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_PIPE_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromData(request->data, transportId);

				auto* pipeTransport = new RTC::PipeTransport(this->shared, transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = pipeTransport;

				MS_DEBUG_DEV("PipeTransport created [transportId:%s]", transportId.c_str());

				json data = json::object();

				pipeTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_DIRECT_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromData(request->data, transportId);

				auto* directTransport =
				  new RTC::DirectTransport(this->shared, transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = directTransport;

				MS_DEBUG_DEV("DirectTransport created [transportId:%s]", transportId.c_str());

				json data = json::object();

				directTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER:
			{
				std::string rtpObserverId;

				// This may throw.
				SetNewRtpObserverIdFromData(request->data, rtpObserverId);

				auto* activeSpeakerObserver =
				  new RTC::ActiveSpeakerObserver(this->shared, rtpObserverId, this, request->data);

				// Insert into the map.
				this->mapRtpObservers[rtpObserverId] = activeSpeakerObserver;

				MS_DEBUG_DEV("ActiveSpeakerObserver created [rtpObserverId:%s]", rtpObserverId.c_str());

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CREATE_AUDIO_LEVEL_OBSERVER:
			{
				std::string rtpObserverId;

				// This may throw
				SetNewRtpObserverIdFromData(request->data, rtpObserverId);

				auto* audioLevelObserver =
				  new RTC::AudioLevelObserver(this->shared, rtpObserverId, this, request->data);

				// Insert into the map.
				this->mapRtpObservers[rtpObserverId] = audioLevelObserver;

				MS_DEBUG_DEV("AudioLevelObserver created [rtpObserverId:%s]", rtpObserverId.c_str());

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CLOSE_TRANSPORT:
			{
				// This may throw.
				RTC::Transport* transport = GetTransportFromData(request->data);

				// Tell the Transport to close all its Producers and Consumers so it will
				// notify us about their closures.
				transport->CloseProducersAndConsumers();

				// Remove it from the map.
				this->mapTransports.erase(transport->id);

				MS_DEBUG_DEV("Transport closed [transportId:%s]", transport->id.c_str());

				// Delete it.
				delete transport;

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::ROUTER_CLOSE_RTP_OBSERVER:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromData(request->data);

				// Remove it from the map.
				this->mapRtpObservers.erase(rtpObserver->id);

				// Iterate all entries in mapProducerRtpObservers and remove the closed one.
				for (auto& kv : this->mapProducerRtpObservers)
				{
					auto& rtpObservers = kv.second;

					rtpObservers.erase(rtpObserver);
				}

				MS_DEBUG_DEV("RtpObserver closed [rtpObserverId:%s]", rtpObserver->id.c_str());

				// Delete it.
				delete rtpObserver;

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void Router::SetNewTransportIdFromData(json& data, std::string& transportId) const
	{
		MS_TRACE();

		auto jsonTransportIdIt = data.find("transportId");

		if (jsonTransportIdIt == data.end() || !jsonTransportIdIt->is_string())
		{
			MS_THROW_TYPE_ERROR("missing transportId");
		}

		transportId.assign(jsonTransportIdIt->get<std::string>());

		if (this->mapTransports.find(transportId) != this->mapTransports.end())
		{
			MS_THROW_ERROR("a Transport with same transportId already exists");
		}
	}

	RTC::Transport* Router::GetTransportFromData(json& data) const
	{
		MS_TRACE();

		auto jsonTransportIdIt = data.find("transportId");

		if (jsonTransportIdIt == data.end() || !jsonTransportIdIt->is_string())
		{
			MS_THROW_TYPE_ERROR("missing transportId");
		}

		auto it = this->mapTransports.find(jsonTransportIdIt->get<std::string>());

		if (it == this->mapTransports.end())
			MS_THROW_ERROR("Transport not found");

		RTC::Transport* transport = it->second;

		return transport;
	}

	void Router::SetNewRtpObserverIdFromData(json& data, std::string& rtpObserverId) const
	{
		MS_TRACE();

		auto jsonRtpObserverIdIt = data.find("rtpObserverId");

		if (jsonRtpObserverIdIt == data.end() || !jsonRtpObserverIdIt->is_string())
		{
			MS_THROW_TYPE_ERROR("missing rtpObserverId");
		}

		rtpObserverId.assign(jsonRtpObserverIdIt->get<std::string>());

		if (this->mapRtpObservers.find(rtpObserverId) != this->mapRtpObservers.end())
		{
			MS_THROW_ERROR("an RtpObserver with same rtpObserverId already exists");
		}
	}

	RTC::RtpObserver* Router::GetRtpObserverFromData(json& data) const
	{
		MS_TRACE();

		auto jsonRtpObserverIdIt = data.find("rtpObserverId");

		if (jsonRtpObserverIdIt == data.end() || !jsonRtpObserverIdIt->is_string())
		{
			MS_THROW_TYPE_ERROR("missing rtpObserverId");
		}

		auto it = this->mapRtpObservers.find(jsonRtpObserverIdIt->get<std::string>());

		if (it == this->mapRtpObservers.end())
			MS_THROW_ERROR("RtpObserver not found");

		RTC::RtpObserver* rtpObserver = it->second;

		return rtpObserver;
	}

	inline void Router::OnTransportNewProducer(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) == this->mapProducerConsumers.end(),
		  "Producer already present in mapProducerConsumers");

		if (this->mapProducers.find(producer->id) != this->mapProducers.end())
		{
			MS_THROW_ERROR("Producer already present in mapProducers [producerId:%s]", producer->id.c_str());
		}

		// Insert the Producer in the maps.
		this->mapProducers[producer->id] = producer;
		this->mapProducerConsumers[producer];
		this->mapProducerRtpObservers[producer];
	}

	inline void Router::OnTransportProducerClosed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto mapProducerConsumersIt    = this->mapProducerConsumers.find(producer);
		auto mapProducersIt            = this->mapProducers.find(producer->id);
		auto mapProducerRtpObserversIt = this->mapProducerRtpObservers.find(producer);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(mapProducersIt != this->mapProducers.end(), "Producer not present in mapProducers");
		MS_ASSERT(
		  mapProducerRtpObserversIt != this->mapProducerRtpObservers.end(),
		  "Producer not present in mapProducerRtpObservers");

		// Close all Consumers associated to the closed Producer.
		auto& consumers = mapProducerConsumersIt->second;

		// NOTE: While iterating the set of Consumers, we call ProducerClosed() on each
		// one, which will end calling Router::OnTransportConsumerProducerClosed(),
		// which will remove the Consumer from mapConsumerProducer but won't remove the
		// closed Consumer from the set of Consumers in mapProducerConsumers (here will
		// erase the complete entry in that map).
		for (auto* consumer : consumers)
		{
			// Call consumer->ProducerClosed() so the Consumer will notify the Node process,
			// will notify its Transport, and its Transport will delete the Consumer.
			consumer->ProducerClosed();
		}

		// Tell all RtpObservers that the Producer has been closed.
		auto& rtpObservers = mapProducerRtpObserversIt->second;

		for (auto* rtpObserver : rtpObservers)
		{
			rtpObserver->RemoveProducer(producer);
		}

		// Remove the Producer from the maps.
		this->mapProducers.erase(mapProducersIt);
		this->mapProducerConsumers.erase(mapProducerConsumersIt);
		this->mapProducerRtpObservers.erase(mapProducerRtpObserversIt);
	}

	inline void Router::OnTransportProducerPaused(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerPaused();
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ProducerPaused(producer);
			}
		}
	}

	inline void Router::OnTransportProducerResumed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerResumed();
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ProducerResumed(producer);
			}
		}
	}

	inline void Router::OnTransportProducerNewRtpStream(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  RTC::RtpStreamRecv* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerNewRtpStream(rtpStream, mappedSsrc);
		}
	}

	inline void Router::OnTransportProducerRtpStreamScore(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  RTC::RtpStreamRecv* rtpStream,
	  uint8_t score,
	  uint8_t previousScore)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerRtpStreamScore(rtpStream, score, previousScore);
		}
	}

	inline void Router::OnTransportProducerRtcpSenderReport(
	  RTC::Transport* /*transport*/, RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, bool first)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerRtcpSenderReport(rtpStream, first);
		}
	}

	inline void Router::OnTransportProducerRtpPacketReceived(
	  RTC::Transport* /*transport*/, RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		packet->logger.routerId = this->id;

		auto& consumers = this->mapProducerConsumers.at(producer);

		if (!consumers.empty())
		{
			// Cloned ref-counted packet that RtpStreamSend will store for as long as
			// needed avoiding multiple allocations unless absolutely necessary.
			// Clone only happens if needed.
			std::shared_ptr<RTC::RtpPacket> sharedPacket;

			for (auto* consumer : consumers)
			{
				// Update MID RTP extension value.
				const auto& mid = consumer->GetRtpParameters().mid;

				if (!mid.empty())
					packet->UpdateMid(mid);

				consumer->SendRtpPacket(packet, sharedPacket);
			}
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ReceiveRtpPacket(producer, packet);
			}
		}
	}

	inline void Router::OnTransportNeedWorstRemoteFractionLost(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  uint32_t mappedSsrc,
	  uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->NeedWorstRemoteFractionLost(mappedSsrc, worstRemoteFractionLost);
		}
	}

	inline void Router::OnTransportNewConsumer(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, std::string& producerId)
	{
		MS_TRACE();

		auto mapProducersIt = this->mapProducers.find(producerId);

		if (mapProducersIt == this->mapProducers.end())
			MS_THROW_ERROR("Producer not found [producerId:%s]", producerId.c_str());

		auto* producer              = mapProducersIt->second;
		auto mapProducerConsumersIt = this->mapProducerConsumers.find(producer);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(
		  this->mapConsumerProducer.find(consumer) == this->mapConsumerProducer.end(),
		  "Consumer already present in mapConsumerProducer");

		// Update the Consumer status based on the Producer status.
		if (producer->IsPaused())
			consumer->ProducerPaused();

		// Insert the Consumer in the maps.
		auto& consumers = mapProducerConsumersIt->second;

		consumers.insert(consumer);
		this->mapConsumerProducer[consumer] = producer;

		// Get all streams in the Producer and provide the Consumer with them.
		for (const auto& kv : producer->GetRtpStreams())
		{
			auto* rtpStream           = kv.first;
			const uint32_t mappedSsrc = kv.second;

			consumer->ProducerRtpStream(rtpStream, mappedSsrc);
		}

		// Provide the Consumer with the scores of all streams in the Producer.
		consumer->ProducerRtpStreamScores(producer->GetRtpStreamScores());
	}

	inline void Router::OnTransportConsumerClosed(RTC::Transport* /*transport*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the Consumer has been closed but its Producer
		// remains alive, so the entry in mapProducerConsumers still exists and must
		// be removed.

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);

		MS_ASSERT(
		  mapConsumerProducerIt != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		// Get the associated Producer.
		auto* producer = mapConsumerProducerIt->second;

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		// Remove the Consumer from the set of Consumers of the Producer.
		auto& consumers = this->mapProducerConsumers.at(producer);

		consumers.erase(consumer);

		// Remove the Consumer from the map.
		this->mapConsumerProducer.erase(mapConsumerProducerIt);
	}

	inline void Router::OnTransportConsumerProducerClosed(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the Consumer has been closed because its
		// Producer was closed, so the entry in mapProducerConsumers has already been
		// removed.

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);

		MS_ASSERT(
		  mapConsumerProducerIt != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		// Remove the Consumer from the map.
		this->mapConsumerProducer.erase(mapConsumerProducerIt);
	}

	inline void Router::OnTransportConsumerKeyFrameRequested(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto* producer = this->mapConsumerProducer.at(consumer);

		producer->RequestKeyFrame(mappedSsrc);
	}

	inline void Router::OnTransportNewDataProducer(
	  RTC::Transport* /*transport*/, RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapDataProducerDataConsumers.find(dataProducer) ==
		    this->mapDataProducerDataConsumers.end(),
		  "DataProducer already present in mapDataProducerDataConsumers");

		if (this->mapDataProducers.find(dataProducer->id) != this->mapDataProducers.end())
		{
			MS_THROW_ERROR(
			  "DataProducer already present in mapDataProducers [dataProducerId:%s]",
			  dataProducer->id.c_str());
		}

		// Insert the DataProducer in the maps.
		this->mapDataProducers[dataProducer->id] = dataProducer;
		this->mapDataProducerDataConsumers[dataProducer];
	}

	inline void Router::OnTransportDataProducerClosed(
	  RTC::Transport* /*transport*/, RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		auto mapDataProducerDataConsumersIt = this->mapDataProducerDataConsumers.find(dataProducer);
		auto mapDataProducersIt             = this->mapDataProducers.find(dataProducer->id);

		MS_ASSERT(
		  mapDataProducerDataConsumersIt != this->mapDataProducerDataConsumers.end(),
		  "DataProducer not present in mapDataProducerDataConsumers");
		MS_ASSERT(
		  mapDataProducersIt != this->mapDataProducers.end(),
		  "DataProducer not present in mapDataProducers");

		// Close all DataConsumers associated to the closed DataProducer.
		auto& dataConsumers = mapDataProducerDataConsumersIt->second;

		// NOTE: While iterating the set of DataConsumers, we call DataProducerClosed()
		// on each one, which will end calling
		// Router::OnTransportDataConsumerDataProducerClosed(), which will remove the
		// DataConsumer from mapDataConsumerDataProducer but won't remove the closed
		// DataConsumer from the set of DataConsumers in mapDataProducerDataConsumers
		// (here will erase the complete entry in that map).
		for (auto* dataConsumer : dataConsumers)
		{
			// Call dataConsumer->DataProducerClosed() so the DataConsumer will notify the Node
			// process, will notify its Transport, and its Transport will delete the DataConsumer.
			dataConsumer->DataProducerClosed();
		}

		// Remove the DataProducer from the maps.
		this->mapDataProducers.erase(mapDataProducersIt);
		this->mapDataProducerDataConsumers.erase(mapDataProducerDataConsumersIt);
	}

	inline void Router::OnTransportDataProducerMessageReceived(
	  RTC::Transport* /*transport*/,
	  RTC::DataProducer* dataProducer,
	  uint32_t ppid,
	  const uint8_t* msg,
	  size_t len)
	{
		MS_TRACE();

		auto& dataConsumers = this->mapDataProducerDataConsumers.at(dataProducer);

		for (auto* consumer : dataConsumers)
		{
			consumer->SendMessage(ppid, msg, len);
		}
	}

	inline void Router::OnTransportNewDataConsumer(
	  RTC::Transport* /*transport*/, RTC::DataConsumer* dataConsumer, std::string& dataProducerId)
	{
		MS_TRACE();

		auto mapDataProducersIt = this->mapDataProducers.find(dataProducerId);

		if (mapDataProducersIt == this->mapDataProducers.end())
			MS_THROW_ERROR("DataProducer not found [dataProducerId:%s]", dataProducerId.c_str());

		auto* dataProducer                  = mapDataProducersIt->second;
		auto mapDataProducerDataConsumersIt = this->mapDataProducerDataConsumers.find(dataProducer);

		MS_ASSERT(
		  mapDataProducerDataConsumersIt != this->mapDataProducerDataConsumers.end(),
		  "DataProducer not present in mapDataProducerDataConsumers");
		MS_ASSERT(
		  this->mapDataConsumerDataProducer.find(dataConsumer) == this->mapDataConsumerDataProducer.end(),
		  "DataConsumer already present in mapDataConsumerDataProducer");

		// Insert the DataConsumer in the maps.
		auto& dataConsumers = mapDataProducerDataConsumersIt->second;

		dataConsumers.insert(dataConsumer);
		this->mapDataConsumerDataProducer[dataConsumer] = dataProducer;
	}

	inline void Router::OnTransportDataConsumerClosed(
	  RTC::Transport* /*transport*/, RTC::DataConsumer* dataConsumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the DataConsumer has been closed but its DataProducer
		// remains alive, so the entry in mapDataProducerDataConsumers still exists and must
		// be removed.

		auto mapDataConsumerDataProducerIt = this->mapDataConsumerDataProducer.find(dataConsumer);

		MS_ASSERT(
		  mapDataConsumerDataProducerIt != this->mapDataConsumerDataProducer.end(),
		  "DataConsumer not present in mapDataConsumerDataProducer");

		// Get the associated DataProducer.
		auto* dataProducer = mapDataConsumerDataProducerIt->second;

		MS_ASSERT(
		  this->mapDataProducerDataConsumers.find(dataProducer) !=
		    this->mapDataProducerDataConsumers.end(),
		  "DataProducer not present in mapDataProducerDataConsumers");

		// Remove the DataConsumer from the set of DataConsumers of the DataProducer.
		auto& dataConsumers = this->mapDataProducerDataConsumers.at(dataProducer);

		dataConsumers.erase(dataConsumer);

		// Remove the DataConsumer from the map.
		this->mapDataConsumerDataProducer.erase(mapDataConsumerDataProducerIt);
	}

	inline void Router::OnTransportDataConsumerDataProducerClosed(
	  RTC::Transport* /*transport*/, RTC::DataConsumer* dataConsumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the DataConsumer has been closed because its
		// DataProducer was closed, so the entry in mapDataProducerDataConsumers has already
		// been removed.

		auto mapDataConsumerDataProducerIt = this->mapDataConsumerDataProducer.find(dataConsumer);

		MS_ASSERT(
		  mapDataConsumerDataProducerIt != this->mapDataConsumerDataProducer.end(),
		  "DataConsumer not present in mapDataConsumerDataProducer");

		// Remove the DataConsumer from the map.
		this->mapDataConsumerDataProducer.erase(mapDataConsumerDataProducerIt);
	}

	inline void Router::OnTransportListenServerClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapTransports.find(transport->id) != this->mapTransports.end(),
		  "Transport not present in mapTransports");

		// Tell the Transport to close all its Producers and Consumers so it will
		// notify us about their closures.
		transport->CloseProducersAndConsumers();

		// Remove it from the map.
		this->mapTransports.erase(transport->id);

		// Delete it.
		delete transport;
	}

	void Router::OnRtpObserverAddProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer)
	{
		// Add to the map.
		this->mapProducerRtpObservers[producer].insert(rtpObserver);
	}

	void Router::OnRtpObserverRemoveProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer)
	{
		// Remove from the map.
		this->mapProducerRtpObservers[producer].erase(rtpObserver);
	}

	RTC::Producer* Router::RtpObserverGetProducer(
	  RTC::RtpObserver* /* rtpObserver */, const std::string& id)
	{
		auto it = this->mapProducers.find(id);

		if (it == this->mapProducers.end())
			MS_THROW_ERROR("Producer not found");

		RTC::Producer* producer = it->second;

		return producer;
	}
} // namespace RTC
