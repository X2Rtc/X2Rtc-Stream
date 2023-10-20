/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __X2_NET_WEBRTC_CONNECTION_H__ 
#define __X2_NET_WEBRTC_CONNECTION_H__
#include "X2NetConnection.h"
#include "RTC/WebRtcTransport.hpp"
#include "RTC/Producer.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/DataProducer.hpp"
#include "RTC/DataConsumer.hpp"
#include "sdp/UtilsC.hpp"
#include "sdp/ortc.hpp"
#include "sdp/RemoteSdp.hpp"
#include "SinglePort.h"
#include "MediaSoupErrors.hpp"

namespace x2rtc {
class X2NetWebRtcConnection : public X2NetConnection, public RTC::Transport::Listener, public Timer::Listener, public ClientIpListener
{
public:
	class Callback
	{
	public:
		virtual ~Callback() = default;

	public:
		virtual void OnX2NetWebRtcConnectionSendRequest(X2NetWebRtcConnection*x2NetWebRtcConn, const std::string& strMethod, const std::string& strId, const std::string& strData) = 0;
		virtual void OnX2NetWebRtcConnectionLostConnection(X2NetWebRtcConnection* x2NetWebRtcConn, const std::string& strId) = 0;
		virtual void OnX2NetWebRtcConnectionAddRemoteIdToLocal(const std::string& strRemoteId, int nLocalPort, const std::string& strPassword, ClientIpListener* ipListener) = 0;
		virtual void OnX2NetWebRtcConnectionRemoveRemoteId(const std::string& strRemoteId) = 0;
	};
public:
	X2NetWebRtcConnection(void);
	virtual ~X2NetWebRtcConnection(void);

	int Init(RTC::Shared*rtcShared, json& jsReq, std::string&strAnswer);
	int DeInit();

	int RunOnce();

	void SetCallback(Callback* callback) {
		cb_callback_ = callback;
	};

	void SetUseH265(bool bUse) {
		b_use_h265_ = bUse;
	}

	void SetUseAAC(bool bUse) {
		b_use_aac_ = bUse;
	}

	RTC::WebRtcTransport* Transport() { return webrtc_transport_; };

	void RecvResponse(const std::string& strMethod, json& jsResp);
	void RecvEvent(json& jsEvent);
	void GetStats(json& jsData);
	void GotMediaSoupError(const char* strFrom, MediaSoupError err);

	void SetConnectionLost(const char* strReason);

public:
	//* For X2NetConnection
	virtual bool CanDelete();

	//* For RTC::Transport::Listener
	virtual void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer);
	virtual void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer);
	virtual void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) {};
	virtual void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) {};
	virtual void OnTransportProducerNewRtpStream(
		RTC::Transport* transport,
		RTC::Producer* producer,
		RTC::RtpStreamRecv* rtpStream,
		uint32_t mappedSsrc) {};
	virtual void OnTransportProducerRtpStreamScore(
		RTC::Transport* transport,
		RTC::Producer* producer,
		RTC::RtpStreamRecv* rtpStream,
		uint8_t score,
		uint8_t previousScore) {};
	virtual void OnTransportProducerRtcpSenderReport(
		RTC::Transport* transport, RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, bool first);
	virtual void OnTransportProducerRtpPacketReceived(
		RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet);
	virtual void OnTransportNeedWorstRemoteFractionLost(
		RTC::Transport* transport,
		RTC::Producer* producer,
		uint32_t mappedSsrc,
		uint8_t& worstRemoteFractionLost) {};
	virtual void OnTransportNewConsumer(
		RTC::Transport* transport, RTC::Consumer* consumer, std::string& producerId);
	virtual void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer);
	virtual void OnTransportConsumerProducerClosed(
		RTC::Transport* transport, RTC::Consumer* consumer) {};
	virtual void OnTransportConsumerKeyFrameRequested(
		RTC::Transport* transport, RTC::Consumer* consumer, uint32_t mappedSsrc) {};
	virtual void OnTransportNewDataProducer(
		RTC::Transport* transport, RTC::DataProducer* dataProducer) {};
	virtual void OnTransportDataProducerClosed(
		RTC::Transport* transport, RTC::DataProducer* dataProducer) {};
	virtual void OnTransportDataProducerMessageReceived(
		RTC::Transport* transport,
		RTC::DataProducer* dataProducer,
		uint32_t ppid,
		const uint8_t* msg,
		size_t len) {};
	virtual void OnTransportNewDataConsumer(
		RTC::Transport* transport, RTC::DataConsumer* dataConsumer, std::string& dataProducerId) {};
	virtual void OnTransportDataConsumerClosed(
		RTC::Transport* transport, RTC::DataConsumer* dataConsumer) {};
	virtual void OnTransportDataConsumerDataProducerClosed(
		RTC::Transport* transport, RTC::DataConsumer* dataConsumer) {};
	virtual void OnTransportListenServerClosed(RTC::Transport* transport) {};

	//* For Timer::Listener
	virtual void OnTimer(Timer* timer);

	//* For ClientIpListener
	virtual void OnClientIpListenerChanged(const std::string& strIpPort);

private:
	Callback* cb_callback_;
	RTC::WebRtcTransport* webrtc_transport_;

	std::string str_id_;
	json offer_sdp_;
	json answer_sdp_;

	bool			b_do_publish_;
	bool			b_lost_connection_;
	bool			b_dtls_connected_;
	bool			b_ice_connected_;
	bool			b_use_h265_;
	bool			b_use_aac_;
	int audio_payload_type_;
	int video_payload_type_;

	std::string		str_single_port_id_;			// Single port ID

	RTC::Producer* audio_producer_;
	RTC::Producer* video_producer_;
	std::vector<uint8_t> producerRtpStreamScores;

	RTC::RtpStreamRecv* fake_producer_rtp_stream_;
	RTC::Consumer* audio_consumer_;
	RTC::Consumer* video_consumer_;

	Timer* p_timer_;
	int64_t				n_last_recv_bytes_;
	int64_t				n_connection_lost_time_;	// If there is no data for a long time, disconnect directly
	int64_t				n_next_req_keyframe_time_;
	int64_t				n_next_stats_time_;

};

}	// namespace x2rtc

#endif	// __X2_NET_WEBRTC_CONNECTION_H__