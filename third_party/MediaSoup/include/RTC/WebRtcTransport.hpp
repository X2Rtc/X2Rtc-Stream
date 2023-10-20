#ifndef MS_RTC_WEBRTC_TRANSPORT_HPP
#define MS_RTC_WEBRTC_TRANSPORT_HPP

#include "RTC/DtlsTransport.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/Shared.hpp"
#include "RTC/SrtpSession.hpp"
#include "RTC/StunPacket.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <vector>

namespace RTC
{
	class WebRtcTransport : public RTC::Transport,
	                        public RTC::UdpSocket::Listener,
	                        public RTC::TcpServer::Listener,
	                        public RTC::TcpConnection::Listener,
	                        public RTC::IceServer::Listener,
	                        public RTC::DtlsTransport::Listener
	{
	private:
		struct ListenIp
		{
			std::string ip;
			std::string announcedIp;
		};

	public:
		class WebRtcTransportListener
		{
		public:
			virtual ~WebRtcTransportListener() = default;

		public:
			virtual void OnWebRtcTransportCreated(RTC::WebRtcTransport* webRtcTransport) = 0;
			virtual void OnWebRtcTransportClosed(RTC::WebRtcTransport* webRtcTransport)  = 0;
			virtual void OnWebRtcTransportLocalIceUsernameFragmentAdded(
			  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment) = 0;
			virtual void OnWebRtcTransportLocalIceUsernameFragmentRemoved(
			  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment) = 0;
			virtual void OnWebRtcTransportTransportTupleAdded(
			  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple) = 0;
			virtual void OnWebRtcTransportTransportTupleRemoved(
			  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple) = 0;
		};

	public:
		WebRtcTransport(
		  RTC::Shared* shared, const std::string& id, RTC::Transport::Listener* listener, json& data);
		WebRtcTransport(
		  RTC::Shared* shared,
		  const std::string& id,
		  RTC::Transport::Listener* listener,
		  WebRtcTransportListener* webRtcTransportListener,
		  std::vector<RTC::IceCandidate>& iceCandidates,
		  json& data);
		~WebRtcTransport() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) override;
		void ProcessStunPacketFromWebRtcServer(RTC::TransportTuple* tuple, RTC::StunPacket* packet);
		void ProcessNonStunPacketFromWebRtcServer(
		  RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void RemoveTuple(RTC::TransportTuple* tuple);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

		/* Methods inherited from PayloadChannel::PayloadChannelSocket::NotificationHandler. */
	public:
		void HandleNotification(PayloadChannel::PayloadChannelNotification* notification) override;

	private:
		bool IsConnected() const override;
		void MayRunDtlsTransport();
		void SendRtpPacket(
		  RTC::Consumer* consumer,
		  RTC::RtpPacket* packet,
		  RTC::Transport::onSendCallback* cb = nullptr) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void SendMessage(
		  RTC::DataConsumer* dataConsumer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len,
		  onQueuedCallback* cb = nullptr) override;
		void SendSctpData(const uint8_t* data, size_t len) override;
		void RecvStreamClosed(uint32_t ssrc) override;
		void SendStreamClosed(uint32_t ssrc) override;
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnDtlsDataReceived(const RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

		/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		void OnRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection) override;

		/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		void OnTcpConnectionPacketReceived(
		  RTC::TcpConnection* connection, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		void OnIceServerSendStunPacket(
		  const RTC::IceServer* iceServer,
		  const RTC::StunPacket* packet,
		  RTC::TransportTuple* tuple) override;
		void OnIceServerLocalUsernameFragmentAdded(
		  const RTC::IceServer* iceServer, const std::string& usernameFragment) override;
		void OnIceServerLocalUsernameFragmentRemoved(
		  const RTC::IceServer* iceServer, const std::string& usernameFragment) override;
		void OnIceServerTupleAdded(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		void OnIceServerTupleRemoved(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		void OnIceServerSelectedTuple(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		void OnIceServerConnected(const RTC::IceServer* iceServer) override;
		void OnIceServerCompleted(const RTC::IceServer* iceServer) override;
		void OnIceServerDisconnected(const RTC::IceServer* iceServer) override;

		/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		void OnDtlsTransportConnecting(const RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportConnected(
		  const RTC::DtlsTransport* dtlsTransport,
		  RTC::SrtpSession::CryptoSuite srtpCryptoSuite,
		  uint8_t* srtpLocalKey,
		  size_t srtpLocalKeyLen,
		  uint8_t* srtpRemoteKey,
		  size_t srtpRemoteKeyLen,
		  std::string& remoteCert) override;
		void OnDtlsTransportFailed(const RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportClosed(const RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportSendData(
		  const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		void OnDtlsTransportApplicationDataReceived(
		  const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;

	private:
		// Passed by argument.
		WebRtcTransportListener* webRtcTransportListener{ nullptr };
		// Allocated by this.
		RTC::IceServer* iceServer{ nullptr };
		// Map of UdpSocket/TcpServer and local announced IP (if any).
		absl::flat_hash_map<RTC::UdpSocket*, std::string> udpSockets;
		absl::flat_hash_map<RTC::TcpServer*, std::string> tcpServers;
		RTC::DtlsTransport* dtlsTransport{ nullptr };
		RTC::SrtpSession* srtpRecvSession{ nullptr };
		RTC::SrtpSession* srtpSendSession{ nullptr };
		// Others.
		bool connectCalled{ false }; // Whether connect() was succesfully called.
		std::vector<RTC::IceCandidate> iceCandidates;
		RTC::DtlsTransport::Role dtlsRole{ RTC::DtlsTransport::Role::AUTO };
	};
} // namespace RTC

#endif
