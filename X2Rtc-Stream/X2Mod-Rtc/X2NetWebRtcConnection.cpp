#include "X2NetWebRtcConnection.h"
#include "rtc_base/time_utils.h"
#include "modules/rtp_rtcp/include/rtp_header_extension_map.h"
#include "RTC/Codecs/Tools.hpp"
#include "X2RtcLog.h"
#include "X2RtcSdp.h"

static void AddEncoding(std::string& kind, json& sendingRtpParameters, std::string mid, json& offerMediaObject)
{
	sendingRtpParameters["mid"] = mid;

	sendingRtpParameters["rtcp"]["cname"] = mediasoupclient::Sdp::Utils::getCname(offerMediaObject);
	//sendingRtpParameters["rtcp"]["cname"] = peer->participantName;

	// sendingRtpParameters["encodings"] = Sdp::Utils::getRtpEncodings(offerMediaObject);
	if (kind == "application")
	{
		int x = 0; // in case if you need to proccess data channel in future
	}
	else
	{
		json encodings = json::array();

		if (offerMediaObject.find("rids") != offerMediaObject.end())
		{
			int size = offerMediaObject["rids"].size();

			encodings.push_back({
				{"rid", "h"},
				{"scaleResolutionDownBy", 1.0}
				});

			encodings.push_back({
				{"rid", "l"},
				{"scaleResolutionDownBy", 4.0}
				});

			if (size > 2)
				encodings.push_back({
					{"rid", "m"},
					{"scaleResolutionDownBy", 2.0}
					});

			if (size > 3)
				encodings.push_back({
					{"rid", "s"},
					{"scaleResolutionDownBy", 3.0}
					});
		}

		if (!encodings.size())
		{
			sendingRtpParameters["encodings"] = mediasoupclient::Sdp::Utils::getRtpEncodings(offerMediaObject);
		}// Set RTP encodings by parsing the SDP offer and complete them with given
			// one if just a single encoding has been given.
		else if (encodings.size() == 1)
		{
			auto newEncodings = mediasoupclient::Sdp::Utils::getRtpEncodings(offerMediaObject);
			//  Object.assign(newEncodings[0], encodings[0]);  // TBD // this is for FID most probabily we need to check
			sendingRtpParameters["encodings"] = newEncodings;
		}// Otherwise if more than 1 encoding are given use them verbatim.
		else
		{
			sendingRtpParameters["encodings"] = encodings;
		}

		// If VP8 and there is effective simulcast, add scalabilityMode to each encoding.
		auto mimeType = sendingRtpParameters["codecs"][0]["mimeType"].get<std::string>();

		std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), ::tolower);

		if (sendingRtpParameters["encodings"].size() > 1 && (mimeType == "video/vp8" || mimeType == "video/h264"))
		{
			for (auto& encoding : sendingRtpParameters["encodings"])
			{
				encoding["scalabilityMode"] = "S1T3";
			}
		}
	}
}

/* Simulcast sdp
offer:
   m=video 49300 RTP/AVP 97 98 99
   a=rtpmap:97 H264/90000
   a=rtpmap:98 H264/90000
   a=rtpmap:99 VP8/90000
   a=fmtp:97 profile-level-id=42c01f;max-fs=3600;max-mbps=108000
   a=fmtp:98 profile-level-id=42c00b;max-fs=240;max-mbps=3600
   a=fmtp:99 max-fs=240; max-fr=30
   a=rid:1 send pt=97;max-width=1280;max-height=720
   a=rid:2 send pt=98;max-width=320;max-height=180
   a=rid:3 send pt=99;max-width=320;max-height=180
   a=rid:4 recv pt=97
   a=simulcast:send 1;2,3 recv 4
   a=extmap:1 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id

answer:
   m=video 49674 RTP/AVP 97 98
   a=rtpmap:97 H264/90000
   a=rtpmap:98 H264/90000
   a=fmtp:97 profile-level-id=42c01f;max-fs=3600;max-mbps=108000
   a=fmtp:98 profile-level-id=42c00b;max-fs=240;max-mbps=3600
   a=rid:1 recv pt=97;max-width=1280;max-height=720
   a=rid:2 recv pt=98;max-width=320;max-height=180
   a=rid:4 send pt=97
   a=simulcast:recv 1;2 send 4
   a=extmap:1 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
*/

namespace x2rtc{
X2NetWebRtcConnection::X2NetWebRtcConnection(void)
	: cb_callback_(NULL)
	, webrtc_transport_(NULL)
	, b_do_publish_(false)
	, b_lost_connection_(false)
	, b_dtls_connected_(false)
	, b_ice_connected_(false)
	, b_use_h265_(false)
	, b_use_aac_(false)
	, audio_payload_type_(0)
	, video_payload_type_(0)
	, audio_producer_(NULL)
	, video_producer_(NULL)
	, fake_producer_rtp_stream_(NULL)
	, audio_consumer_(NULL)
	, video_consumer_(NULL)
	, p_timer_(NULL)
	, n_last_recv_bytes_(0)
	, n_connection_lost_time_(0)
	, n_next_req_keyframe_time_(0)
	, n_next_stats_time_(0)
{
	producerRtpStreamScores.push_back(10);
	n_connection_lost_time_ = rtc::TimeUTCMillis() + 15000;
}
X2NetWebRtcConnection::~X2NetWebRtcConnection(void)
{

}

int X2NetWebRtcConnection::Init(RTC::Shared* rtcShared, json& jsReq, std::string& strAnswer)
{
	str_id_ = jsReq["Id"];
	str_stream_id_ = jsReq["Args"];
	if (jsReq["Cmd"] == "Publish") {
		b_do_publish_ = true;
	}
	else {
		if (fake_producer_rtp_stream_ == NULL) {
			RTC::RtpStream::Params params;
			params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;	//@Eric - 20230324 如果不设置，使用默认值为UNSET，会ASSERT
			fake_producer_rtp_stream_ = new RTC::RtpStreamRecv(NULL, params, 10, false);
		}
	}
	if (webrtc_transport_ == NULL) {
		try {
			json jsData = json::parse("{\"listenIps\":[{\"ip\":\"0.0.0.0\"}], \"port\":0, \"enableUdp\":true, \"enableTcp\":false, \"preferUdp\":false, \"preferTcp\":false, \"initialAvailableOutgoingBitrate\":600000, \"enableSctp\":false, \"numSctpStreams\":{ \"OS\": 1024, \"MIS\": 1024 },\"maxSctpMessageSize\":262144, \"sctpSendBufferSize\":262144, \"appData\":{}}");  //, \"announcedIp\":\"192.168.1.130\"
			jsData["Ice"] = jsReq["Ice"];

			webrtc_transport_ = new RTC::WebRtcTransport(rtcShared, str_id_, this, jsData);
			json fillData;
			webrtc_transport_->FillJson(fillData);
			std::string strFingerprint;
			if (fillData.find("dtlsParameters") != fillData.end()) {
				json jsFingerprints = fillData["dtlsParameters"]["fingerprints"];
				for (json::iterator fingerprint_it = jsFingerprints.begin(); fingerprint_it != jsFingerprints.end(); ++fingerprint_it) {
					if ((*fingerprint_it)["algorithm"] == "sha-256") {
						strFingerprint = (*fingerprint_it)["value"];
						break;
					}
				}
			}

			{
				json jsIceCandidates = fillData["iceCandidates"];
				for (json::iterator candidate_it = jsIceCandidates.begin(); candidate_it != jsIceCandidates.end(); ++candidate_it) {
					json& cand = *candidate_it;
					int localPort = cand["port"];
					str_single_port_id_ = jsReq["Ice"]["uname"];
					if (cb_callback_ != NULL) {
						cb_callback_->OnX2NetWebRtcConnectionAddRemoteIdToLocal(str_single_port_id_, localPort, jsReq["Ice"]["pwd"], this);
					}
					break;
				}
			}

			try {
				X2RtcSdp x2RtcSdp;
				x2RtcSdp.loadFrom(jsReq["Sdp"]);
				x2RtcSdp.checkValid();

				X2RtcConfigure x2RtcConfig;
				SdpAttrFingerprint sdpFrin;
				sdpFrin.algorithm = "sha-256";
				sdpFrin.hash = strFingerprint;
				if (b_use_h265_) {
					x2RtcConfig.setVideoPreferCodec("H265");
				}
				if (b_use_aac_) {
					x2RtcConfig.setAudioPreferCodec("MP4A-ADTS");
				}
				x2RtcConfig.setDefaultSetting(jsReq["Ice"]["uname"], jsReq["Ice"]["pwd"], 
					b_do_publish_?x2rtc::RtpDirection::recvonly: x2rtc::RtpDirection::sendonly, sdpFrin);
				if (!b_do_publish_) {
					x2RtcConfig.enableTWCC(false, TrackMax);
					x2RtcConfig.enableREMB(false, TrackMax);
				}
				X2RtcSdp::Ptr x2RtcAnswer = x2RtcConfig.createAnswer(x2RtcSdp);
				//* Need catch exception
				SdpCheckAnswer(x2RtcAnswer);
				strAnswer = x2RtcAnswer->toString();
			}
			catch (std::runtime_error err) {
				return -1;
			}

			offer_sdp_ = sdptransform::parse(jsReq["Sdp"]);
			answer_sdp_ = sdptransform::parse(strAnswer);
			// Get our local DTLS parameters.
			json dtlsParameters = mediasoupclient::Sdp::Utils::extractDtlsParameters(offer_sdp_);
			dtlsParameters["role"] = "client";	//@Eric - force to set the role to "client": remote is "client" local is "server"
			json jsDtls;
			jsDtls["dtlsParameters"] = dtlsParameters;
			//printf("Offer dtlsParameters: %s\r\n", dtlsParameters.dump().c_str());
			if (cb_callback_ != NULL) {
				cb_callback_->OnX2NetWebRtcConnectionSendRequest(this, "transport.connect", str_id_, jsDtls.dump().c_str());
			}
		}
		catch (MediaSoupError err) {
			GotMediaSoupError("X2NetWebRtcConnection::Init", err);
			return -1;
		};
	}

	if (p_timer_ == NULL) {
		p_timer_ = new Timer(this);
		p_timer_->Start(1000, 0);
	}
	return 0;
}

int X2NetWebRtcConnection::DeInit()
{
	if (p_timer_ != NULL) {
		delete p_timer_;
		p_timer_ = NULL;
	}

	if (str_single_port_id_.length() > 0) {
		if (cb_callback_ != NULL) {
			cb_callback_->OnX2NetWebRtcConnectionRemoveRemoteId(str_single_port_id_);
		}
		str_single_port_id_.clear();
	}

	if (webrtc_transport_ != NULL) {
		delete webrtc_transport_;
		webrtc_transport_ = NULL;
	}

	audio_producer_ = NULL;
	video_producer_ = NULL;

	audio_consumer_ = NULL;
	video_consumer_ = NULL;

	if (fake_producer_rtp_stream_ != NULL) {
		delete fake_producer_rtp_stream_;
		fake_producer_rtp_stream_ = NULL;
	}

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetConnectionClosed(this);
	}

	return 0;
}

int X2NetWebRtcConnection::X2NetWebRtcConnection::RunOnce()
{
	if (!b_ice_connected_ || !b_dtls_connected_) {
		return 0;
	}
	int nIdx = 16;	//@Eric - 10 -> 15
	while ((nIdx--) > 0) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = NULL;
		x2NetData = GetX2NetData();
		if (x2NetData != NULL) {
			RTC::RtpPacket* rtpPkt = RTC::RtpPacket::Parse(x2NetData->pData, x2NetData->nLen);
			if (rtpPkt != NULL) {
				if (rtpPkt->GetPayloadType() == audio_payload_type_) {
					if (audio_consumer_ != NULL) {
						const std::vector<uint32_t>& ssrcs = audio_consumer_->GetMediaSsrcs();
						std::vector<uint32_t>::const_iterator itvr = ssrcs.begin();
						if (itvr != ssrcs.end()) {
							rtpPkt->SetSsrc(*itvr);
							rtpPkt->SetMarker(true);
							std::shared_ptr<RTC::RtpPacket> sharedPkt;
							audio_consumer_->SendRtpPacket(rtpPkt, sharedPkt);
						}
					}
				}
				else if (rtpPkt->GetPayloadType() == video_payload_type_) {
					if (video_consumer_ != NULL) {
						RTC::RtpCodecMimeType mimType;
						mimType.type = RTC::RtpCodecMimeType::Type::VIDEO;
						if (b_use_h265_) {
							mimType.subtype = RTC::RtpCodecMimeType::Subtype::H265;
						}
						else {
							mimType.subtype = RTC::RtpCodecMimeType::Subtype::H264;
						}
						RTC::Codecs::Tools::ProcessRtpPacket(rtpPkt, mimType);
						{
							const auto& mid = video_consumer_->GetRtpParameters().mid;

							if (!mid.empty())
								rtpPkt->UpdateMid(mid);
						}
						const std::vector<uint32_t>& ssrcs = video_consumer_->GetMediaSsrcs();
						std::vector<uint32_t>::const_iterator itvr = ssrcs.begin();
						if (itvr != ssrcs.end()) {
							rtpPkt->SetSsrc(*itvr);

							std::shared_ptr<RTC::RtpPacket> sharedPkt;
							video_consumer_->SendRtpPacket(rtpPkt, sharedPkt);

							//printf("Send vid pkt len: %d seqn: %d mark: %d curTime: %u \r\n", rtpPkt->GetSize(), rtpPkt->GetSequenceNumber(), rtpPkt->HasMarker(), rtc::Time32());
						}
					}
				}
				delete rtpPkt;
			}
		}
		else {
			break;
		}
	}
	return 0;
}

void X2NetWebRtcConnection::RecvResponse(const std::string& strMethod, json& jsResp)
{
	if (strMethod.compare("producer.getStats") == 0) {
		GetStats(jsResp["data"]);
	}
	else if (strMethod.compare("consumer.getStats") == 0) {
		GetStats(jsResp["data"]);
	}
	else if (strMethod.compare("transport.connect") == 0) {
		if (jsResp["accepted"] == true) {
			try {
				auto nativeRtpCapabilities = mediasoupclient::Sdp::Utils::extractRtpCapabilities(answer_sdp_);
				auto routerRtpCapabilities = mediasoupclient::Sdp::Utils::extractRtpCapabilities(offer_sdp_);
				json extendedRtpCapabilities =
					mediasoupclient::ortc::getExtendedRtpCapabilities(nativeRtpCapabilities, routerRtpCapabilities);
				json* targetSdpObject = &answer_sdp_;	
				if (b_do_publish_) {
					targetSdpObject = &offer_sdp_;// Media(ssrc)是客户端发送来的
				}
				int sizeofMid = (*targetSdpObject)["media"].size();
				for (int i = 0; i < sizeofMid; ++i)
				{
					json offerMediaObject = (*targetSdpObject)["media"][i];
					std::string strKind = offerMediaObject["type"].get<std::string>();
					for (const auto& extendedCodec : extendedRtpCapabilities["codecs"])
					{
						if (extendedCodec["kind"].get<std::string>() == strKind) {
							auto midIt = offerMediaObject.find("mid");
							json sendingRtpParameters = mediasoupclient::ortc::getSendingRtpParameters(strKind, extendedRtpCapabilities);
							AddEncoding(strKind, sendingRtpParameters, *midIt, offerMediaObject);
							//printf("sendingRtpParameters: %s \r\n", sendingRtpParameters.dump().c_str());
							// This may throw.
							auto rtpMapping = mediasoupclient::ortc::getProducerRtpParametersMapping(sendingRtpParameters, routerRtpCapabilities);
							//printf("rtpMapping: %s \r\n", rtpMapping.dump().c_str());
							if (b_do_publish_) {	
								json param;
								param["producerId"] = str_id_ + "*" + strKind;
								param["kind"] = strKind;
								param["rtpMapping"] = rtpMapping;
								param["rtpParameters"] = sendingRtpParameters;
								std::string strParam = param.dump();

								//printf("param: %s \r\n", param.dump().c_str());
								if (cb_callback_ != NULL) {
									cb_callback_->OnX2NetWebRtcConnectionSendRequest(this, "transport.produce", str_id_, strParam.c_str());
								}
							}
							else {
								auto consumableRtpParameters = mediasoupclient::ortc::getConsumableRtpParameters(strKind, sendingRtpParameters, routerRtpCapabilities, rtpMapping);
								json param;
								param["kind"] = strKind;
								param["producerId"] = "x2rtc_pullstream";
								param["consumerId"] = str_id_ + "*" + strKind;
								param["consumableRtpEncodings"] = consumableRtpParameters["encodings"];
								param["paused"] = false;
								param["rtpParameters"] = sendingRtpParameters;
								param["type"] = "simple";
								std::string strParam = param.dump();

								//printf("param: %s \r\n", param.dump().c_str());
								if (cb_callback_ != NULL) {
									cb_callback_->OnX2NetWebRtcConnectionSendRequest(this, "transport.consume", str_id_, strParam.c_str());
								}
							}
						}
					}
				}
			}
			catch (MediaSoupError err) {
				GotMediaSoupError("transport.connect accepted", err);
			};
		}
		else {
			MediaSoupError err("accepted failed");
			GotMediaSoupError("transport.connect accepted", err);
		}
	}
	else if (strMethod.compare("transport.produce") == 0) {
		if (jsResp["accepted"] == true) {

		}
		else {
			MediaSoupError err("produce failed");
			GotMediaSoupError("transport.produce accepted", err);
		}
	}
	else if (strMethod.compare("transport.consume") == 0) {
		if (jsResp["accepted"] == true) {

		}
		else {
			MediaSoupError err("produce failed");
			GotMediaSoupError("transport.consume accepted", err);
		}
	}
}

void X2NetWebRtcConnection::RecvEvent(json& jsEvent)
{
	std::string strEvent = jsEvent["event"];
	//printf("RecvEvent idd(%s) : %s \r\n", str_id_.c_str(), jsEvent.dump().c_str());
	if (strEvent.compare("icestatechange") == 0) {
		std::string strIceState = jsEvent["data"]["iceState"];
		if (strIceState.compare("new") == 0) {
		}
		else if (strIceState.compare("connected") == 0) {
			if (!b_ice_connected_) {
				b_ice_connected_ = true;

				if (b_dtls_connected_) {
					//WastStateCallback(kXRtcConnecting, kXRtcConnected);
					if (cb_listener_ != NULL) {
						json jsEvent;
						jsEvent["Event"] = "StatueChanged";
						jsEvent["Connected"] = true;
						std::string strData = jsEvent.dump();
						x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
						x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
						cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
					}

					n_next_stats_time_ = rtc::TimeUTCMillis() + 1000;
				}
			}
		}
		else if (strIceState.compare("completed") == 0) {
			if (!b_ice_connected_) {
				//WastStateCallback(kXRtcConnecting, kXRtcFailed);
				if (cb_listener_ != NULL) {
					json jsEvent;
					jsEvent["Event"] = "StatueChanged";
					jsEvent["Connected"] = false;
					std::string strData = jsEvent.dump();
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
					cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
				}
			}
		}
		else if (strIceState.compare("disconnected") == 0) {
			if (b_ice_connected_) {
				//WastStateCallback(kXRtcConnected, kXRtcDisconnected);
				if (cb_listener_ != NULL) {
					json jsEvent;
					jsEvent["Event"] = "StatueChanged";
					jsEvent["Connected"] = false;
					std::string strData = jsEvent.dump();
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
					cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
				}
			}
			else {
				//WastStateCallback(kXRtcConnecting, kXRtcDisconnected);
				if (cb_listener_ != NULL) {
					json jsEvent;
					jsEvent["Event"] = "StatueChanged";
					jsEvent["Connected"] = false;
					std::string strData = jsEvent.dump();
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
					cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
				}
			}
			b_ice_connected_ = false;
		}
	}
	else if (strEvent.compare("dtlsstatechange") == 0) {
		std::string strDtlsState = jsEvent["data"]["dtlsState"];
		if (strDtlsState.compare("new") == 0) {
		}
		else if (strDtlsState.compare("connecting") == 0) {
		}
		else if (strDtlsState.compare("connected") == 0) {
			if (!b_dtls_connected_) {
				b_dtls_connected_ = true;

				if (b_ice_connected_) {
					//WastStateCallback(kXRtcConnecting, kXRtcConnected);
					if (cb_listener_ != NULL) {
						json jsEvent;
						jsEvent["Event"] = "StatueChanged";
						jsEvent["Connected"] = true;
						std::string strData = jsEvent.dump();
						x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
						x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
						cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
					}

					n_next_stats_time_ = rtc::TimeUTCMillis() + 1000;
				}
			}
		}
		else if (strDtlsState.compare("failed") == 0) {
			//WastStateCallback(kXRtcConnecting, kXRtcFailed);
			if (cb_listener_ != NULL) {
				json jsEvent;
				jsEvent["Event"] = "StatueChanged";
				jsEvent["Connected"] = false;
				std::string strData = jsEvent.dump();
				x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
				x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
				cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
			}
			
			SetConnectionLost("Dtls failed");
		}
		else if (strDtlsState.compare("closed") == 0) {
			if (b_dtls_connected_) {
				//WastStateCallback(kXRtcConnected, kXRtcDisconnected);
				if (cb_listener_ != NULL) {
					json jsEvent;
					jsEvent["Event"] = "StatueChanged";
					jsEvent["Connected"] = false;
					std::string strData = jsEvent.dump();
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
					cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
				}
			}
			else {
				//WastStateCallback(kXRtcConnecting, kXRtcDisconnected);
				if (cb_listener_ != NULL) {
					json jsEvent;
					jsEvent["Event"] = "StatueChanged";
					jsEvent["Connected"] = false;
					std::string strData = jsEvent.dump();
					x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
					x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
					cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
				}
			}
			b_dtls_connected_ = false;

			SetConnectionLost("Dtls closed");
		}
	}
}

void X2NetWebRtcConnection::GetStats(json& jsData)
{
	float fRoundTripTime = 0.0;
	uint32_t nVidPacketsReceived = 0;
	uint32_t nVidPacketsLost = 0;
	uint32_t nVidBytesReceived = 0;
	uint32_t nAudPacketsReceived = 0;
	uint32_t nAudPacketsLost = 0;
	uint32_t nAudBytesReceived = 0;
	uint32_t nVidPacketsSent = 0;
	uint32_t nVidPacketsReSent = 0;
	uint32_t nVidBytesSent = 0;
	uint32_t nAudPacketsSent = 0;
	uint32_t nAudPacketsReSent = 0;
	uint32_t nAudBytesSent = 0;

	for (auto& jsItem : jsData)
	{
		if (jsItem["type"] == "inbound-rtp" || jsItem["type"] == "outbound-rtp") {
			bool bAudio = false;
			uint32_t nPacketsReceived = 0;
			uint32_t nPacketsLost = 0;
			uint32_t nBytesReceived = 0;
			if (jsItem["kind"] == "audio") {
				bAudio = true;
			}
			else if (jsItem["kind"] == "video") {

			}
			else {
				continue;
			}
			//fRoundTripTime = jsItem["roundTripTime"];

			nPacketsReceived = jsItem["packetCount"];
			nPacketsLost = jsItem["packetsLost"];
			nBytesReceived = jsItem["byteCount"];
			int nFractionLost = jsItem["fractionLost"];

			uint32_t nRtt = 0; // fRoundTripTime * 1000;
			if (jsItem.find("roundTripTime") != jsItem.end()) {
				nRtt = jsItem["roundTripTime"]; // fRoundTripTime * 1000;
			}
			if (nRtt == 0) {
				nRtt = 1;
			}

			if (bAudio) {
				nAudPacketsReceived = nPacketsReceived;
				nAudPacketsLost = nPacketsLost;
				nAudBytesReceived = nBytesReceived;

				//wrtc_stats_.UpdateAudRecvStats(nRtt, nAudPacketsReceived, nAudPacketsLost, nAudBytesReceived);
			}
			else {
				nVidPacketsReceived = nPacketsReceived;
				nVidPacketsLost = nPacketsLost;
				nVidBytesReceived = nBytesReceived;

				//wrtc_stats_.UpdateVidRecvStats(nRtt, nVidPacketsReceived, nVidPacketsLost, nVidBytesReceived);
			}

			int nLossRate = (int)(((float)nFractionLost * 100) / 256);
			
			if (cb_listener_ != NULL) {
				cb_listener_->OnX2NetGotNetStats(this, bAudio, nRtt, nLossRate, 0);
			}
		}
	}
}

void X2NetWebRtcConnection::GotMediaSoupError(const char* strFrom, MediaSoupError err)
{
	SetConnectionLost(err.buffer);
}

void X2NetWebRtcConnection::SetConnectionLost(const char* strReason)
{
	if (!b_lost_connection_) {
		if (cb_callback_ != NULL) {
			cb_callback_->OnX2NetWebRtcConnectionLostConnection(this, str_id_);
		}
		X2RtcPrintf(WARN, "X2NetWebRtcConnection(%s) lost connection at: %lld resason: %s", str_id_.c_str(), rtc::TimeUTCMillis(), strReason);

		b_lost_connection_ = true;
		DeInit();
	}
}

//* For X2NetConnection
bool X2NetWebRtcConnection::CanDelete()
{
	return IsForceClosed() && b_lost_connection_;
}
//* For RTC::Transport::Listener
void X2NetWebRtcConnection::OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer)
{
	RTC::RtpParameters& rtpParam = (RTC::RtpParameters&)producer->GetRtpParameters();
	auto& encoding = rtpParam.encodings[0];
	auto* mediaCodec = rtpParam.GetCodecForEncoding(encoding);
	json jsRtpParam;
	rtpParam.FillJson(jsRtpParam);

	json jsEvent;
	jsEvent["Event"] = "NewProducer";

	if (producer->GetKind() == RTC::Media::Kind::AUDIO) {
		audio_producer_ = producer;

		audio_payload_type_ = mediaCodec->payloadType;
		int sample_rate_ = mediaCodec->clockRate;

		jsEvent["Type"] = "audio";
		jsEvent["PayloadType"] = audio_payload_type_;
		jsEvent["SampleRate"] = sample_rate_;
	}
	else if (producer->GetKind() == RTC::Media::Kind::VIDEO) {
		video_producer_ = producer;
		n_next_req_keyframe_time_ = rtc::TimeUTCMillis() + 3000;

		video_payload_type_ = mediaCodec->payloadType;

		jsEvent["Type"] = "video";
		jsEvent["PayloadType"] = video_payload_type_;

	}
	else {
		return;
	}

	if (cb_listener_ != NULL) {
		jsEvent["RtpParam"] = jsRtpParam;
		std::string strData = jsEvent.dump();
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}
void X2NetWebRtcConnection::OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer)
{
	json jsEvent;
	jsEvent["Event"] = "CloseProducer";

	if (producer->GetKind() == RTC::Media::Kind::AUDIO) {
		audio_producer_ = NULL;

		jsEvent["Type"] = "audio";
	}
	else if (producer->GetKind() == RTC::Media::Kind::VIDEO) {
		video_producer_ = NULL;

		jsEvent["Type"] = "video";
	}
	else {
		return;
	}

	if (cb_listener_ != NULL) {
		std::string strData = jsEvent.dump();
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

void X2NetWebRtcConnection::OnTransportProducerRtcpSenderReport(
	RTC::Transport* transport, RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, bool first)
{
	if (!first) {
		int rtt = rtpStream->GetRtt();
		Utils::Time::Ntp ntp = Utils::Time::TimeMs2Ntp(rtpStream->GetSenderReportNtpMs());
		json jsEvent;
		jsEvent["Event"] = "ProducerRtcpSenderReport";
		if (producer->GetKind() == RTC::Media::Kind::VIDEO) {
			jsEvent["Type"] = "video";

			//printf("*Vid ntp time rtt:%d sec: %u frac:%u rtp: %u\r\n", rtt, ntp.seconds, ntp.fractions, rtpStream->GetSenderReportTs());

		}
		else if (producer->GetKind() == RTC::Media::Kind::AUDIO) {
			jsEvent["Type"] = "audio";

			//printf("*Aud ntp time rtt:%d sec: %u frac:%u rtp: %u\r\n", rtt, ntp.seconds, ntp.fractions, rtpStream->GetSenderReportTs());

		}
		else {
			return;
		}

		jsEvent["Rtt"] = rtt;
		jsEvent["NtpSeconds"] = ntp.seconds;
		jsEvent["NtpFractions"] = ntp.fractions;
		jsEvent["SenderReportTs"] = rtpStream->GetSenderReportTs();

		if (cb_listener_ != NULL) {
			std::string strData = jsEvent.dump();
			x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
			x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
			cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
		}
	}
}
void X2NetWebRtcConnection::OnTransportProducerRtpPacketReceived(
	RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet)
{
	if (producer->GetKind() == RTC::Media::Kind::VIDEO) {
		//printf("Video rtp type: %d size: %d\r\n", packet->GetPayloadType(), packet->GetPayloadLength());
		if (packet->IsKeyFrame()) {
			n_next_req_keyframe_time_ = rtc::TimeUTCMillis() + 3000;
		}
		//printf("Recv rtp ssrc: %u type: %d len: %d\r\n", packet->GetSsrc(), packet->GetPayloadType(), packet->GetPayloadLength());
	}
	else if (producer->GetKind() == RTC::Media::Kind::AUDIO) {
	}
	else {
		return;
	}

	if (cb_listener_ != NULL) {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)packet->GetData(), packet->GetSize());
		x2NetData->nDataType = (int)producer->GetKind();
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

void X2NetWebRtcConnection::OnTransportNewConsumer(
	RTC::Transport* transport, RTC::Consumer* consumer, std::string& producerId)
{
	// Check if Transport Congestion Control client must be created.
	const auto& rtpHeaderExtensionIds = consumer->GetRtpHeaderExtensionIds();
	RTC::RtpParameters& rtpParam = (RTC::RtpParameters&)consumer->GetRtpParameters();
	auto& encoding = rtpParam.encodings[0];
	auto* mediaCodec = rtpParam.GetCodecForEncoding(encoding);
	json jsRtpParam;
	rtpParam.FillJson(jsRtpParam);

	json jsEvent;
	jsEvent["Event"] = "NewConsumer";

	consumer->ProducerRtpStreamScores(&producerRtpStreamScores);
	if (consumer->GetKind() == RTC::Media::Kind::AUDIO) {
		audio_payload_type_ = mediaCodec->payloadType;
		int sample_rate_ = mediaCodec->clockRate;

		jsEvent["Type"] = "audio";
		jsEvent["PayloadType"] = audio_payload_type_;
		jsEvent["SampleRate"] = sample_rate_;

		audio_consumer_ = consumer;
		audio_consumer_->ProducerRtpStream(fake_producer_rtp_stream_, 0);
	}
	else if (consumer->GetKind() == RTC::Media::Kind::VIDEO) {
		video_payload_type_ = mediaCodec->payloadType;

		jsEvent["Type"] = "video";
		jsEvent["PayloadType"] = video_payload_type_;

		video_consumer_ = consumer;
		video_consumer_->ProducerRtpStream(fake_producer_rtp_stream_, 0);
	}
	else {
		return;
	}
	
	if (cb_listener_ != NULL) {
		jsEvent["RtpParam"] = jsRtpParam;
		std::string strData = jsEvent.dump();
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}
void X2NetWebRtcConnection::OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer)
{
	json jsEvent;
	jsEvent["Event"] = "CloseConsumer";

	if (consumer->GetKind() == RTC::Media::Kind::AUDIO) {
		audio_consumer_ = NULL;
		jsEvent["Type"] = "audio";
	}
	else if (consumer->GetKind() == RTC::Media::Kind::VIDEO) {
		video_consumer_ = NULL;
		jsEvent["Type"] = "video";
	}
	else {
		return;
	}

	if (cb_listener_ != NULL) {
		std::string strData = jsEvent.dump();
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
		x2NetData->SetData((uint8_t*)strData.c_str(), strData.length());
		cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
	}
}

//* For Timer::Listener
void X2NetWebRtcConnection::OnTimer(Timer* timer)
{
	if (webrtc_transport_ != NULL) {
		json jsStats;
		webrtc_transport_->FillJsonStats(jsStats);
		//printf("Stats : %s\r\n", jsStats.dump().c_str());

		for (auto& jsstat : jsStats) {
			if (jsstat["transportId"] == str_id_) {
				int nBytesRecved = jsstat["bytesReceived"];
				if (n_last_recv_bytes_ != nBytesRecved) {
					n_last_recv_bytes_ = nBytesRecved;
					n_connection_lost_time_ = rtc::TimeUTCMillis() + 15000;
				}
			}
		}
	}

	if (b_ice_connected_ && b_dtls_connected_) {
		bool bGetStats = false;
		if (n_next_stats_time_ <= rtc::TimeUTCMillis()) {
			n_next_stats_time_ = rtc::TimeUTCMillis() + 1000;
			bGetStats = true;
		}
		if (video_producer_ != NULL) {
			if (n_next_req_keyframe_time_ != 0 && n_next_req_keyframe_time_ <= rtc::TimeUTCMillis()) {
				n_next_req_keyframe_time_ = rtc::TimeUTCMillis() + 3000;

				absl::flat_hash_map<RTC::RtpStreamRecv*, uint32_t>rtpStreams = video_producer_->GetRtpStreams();
				absl::flat_hash_map<RTC::RtpStreamRecv*, uint32_t>::iterator itrr = rtpStreams.begin();
				while (itrr != rtpStreams.end()) {
					video_producer_->RequestKeyFrame(itrr->second);
					itrr++;
				}
			}

			if (bGetStats) {
				json jsStats;
				video_producer_->FillJsonStats(jsStats);
				GetStats(jsStats);
			}
		}
		if (audio_producer_ != NULL) {
			if (bGetStats) {
				json jsStats;
				audio_producer_->FillJsonStats(jsStats);
				GetStats(jsStats);
			}
		}

		if (video_consumer_ != NULL) {
			if (bGetStats) {
				json jsStats;
				video_consumer_->FillJsonStats(jsStats);
				GetStats(jsStats);
			}
		}
		if (audio_consumer_ != NULL) {
			if (bGetStats) {
				json jsStats;
				audio_consumer_->FillJsonStats(jsStats);
				GetStats(jsStats);
			}
		}
	}

	if (n_connection_lost_time_ <= rtc::TimeUTCMillis()) {
		if (b_ice_connected_ && b_dtls_connected_) {
			//WastStateCallback(kXRtcConnected, kXRtcDisconnected);
		}
		else {
			//WastStateCallback(kXRtcConnecting, kXRtcFailed);
		}
		
		SetConnectionLost("Not recvied data by long time.");
	}
	else {
		p_timer_->Restart();
	}
}

//* For ClientIpListener
void X2NetWebRtcConnection::OnClientIpListenerChanged(const std::string& strIpPort)
{
	str_remote_ip_port_ = strIpPort;

	if (cb_listener_ != NULL) {
		cb_listener_->OnX2NetRemoteIpPortChanged(this, NULL);
	}
}
}	// namespace x2rtc