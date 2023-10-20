#include "WastXSdp.h"
#include "Common/config.h"
#include "Util/function_traits.h"

#define RTP_SSRC_OFFSET 1
#define RTX_SSRC_OFFSET 2
#define RTP_CNAME "zlmediakit-rtp"
#define RTP_LABEL "zlmediakit-label"
#define RTP_MSLABEL "zlmediakit-mslabel"
#define RTP_MSID RTP_MSLABEL " " RTP_LABEL

using namespace std;
using namespace mediakit;
using namespace toolkit;

WastXSdp::WastXSdp(void)
	: _local_port(0)
	, _b_use_local_port(false)
{

}
WastXSdp::~WastXSdp(void)
{

}

void WastXSdp::SetLocalPort(int nPort)
{
	_local_port = nPort;
}

int WastXSdp::GetLocalPort()
{
	return _local_port;
}

void WastXSdp::SetUseLocalPort(bool bUse)
{
	_b_use_local_port = bUse;
}

std::string WastXSdp::getAnswerSdp(const std::string &offer, const std::string &ufrag, const std::string &password)
{
	try {
		//// 解析offer sdp ////
		_offer_sdp = std::make_shared<RtcSession>();
		_offer_sdp->loadFrom(offer);
		onCheckSdp(SdpType::offer, *_offer_sdp);
		_offer_sdp->checkValid();
		setRemoteDtlsFingerprint(*_offer_sdp);

		//// sdp 配置 ////
		SdpAttrFingerprint fingerprint;
		fingerprint.algorithm = _offer_sdp->media[0].fingerprint.algorithm;
		fingerprint.hash = getFingerprint(fingerprint.algorithm);
		RtcConfigure configure;
		configure.setDefaultSetting(
			ufrag, password, RtpDirection::sendrecv, fingerprint);
		onRtcConfigure(configure);

		//// 生成answer sdp ////
		_answer_sdp = configure.createAnswer(*_offer_sdp);
		onCheckSdp(SdpType::answer, *_answer_sdp);
		_answer_sdp->checkValid();
		return _answer_sdp->toString();
	}
	catch (exception &ex) {
		//onShutdown(SockException(Err_shutdown, ex.what()));
		return "";
	}
}

std::string WastXSdp::getOfferSdp(const std::string &offer, const std::string &ufrag, const std::string &password)
{
	try {
		//// 解析offer sdp ////
		_offer_sdp = std::make_shared<RtcSession>();
		_offer_sdp->loadFrom(offer);
		onCheckSdp(SdpType::offer, *_offer_sdp);
		_offer_sdp->checkValid();

		//// sdp 配置 ////
		SdpAttrFingerprint fingerprint;
		fingerprint.algorithm = "sha-256";
		fingerprint.hash = getFingerprint(fingerprint.algorithm);
		RtcConfigure configure;
		configure.setDefaultSetting(
			ufrag, password, RtpDirection::sendrecv, fingerprint);
		onRtcConfigure(configure);

		configure.createOffer(*_offer_sdp);
		_offer_sdp->checkValid();
		return _offer_sdp->toString();
	}
	catch (exception &ex) {
		//onShutdown(SockException(Err_shutdown, ex.what()));
		return "";
	}
}
void WastXSdp::recvAnswerSdp(const std::string &answer)
{
	try {
		//// 解析offer sdp ////
		_answer_sdp = std::make_shared<RtcSession>();
		_answer_sdp->loadFrom(answer);
		onCheckSdp(SdpType::answer, *_answer_sdp);
		_answer_sdp->checkValid();
		setRemoteDtlsFingerprint(*_answer_sdp);
	}
	catch (exception &ex) {
		//onShutdown(SockException(Err_shutdown, ex.what()));
		//throw;
	}

}

bool WastXSdp::canSendRtp() const
{
	for (auto &m : _answer_sdp->media) {
		if (m.direction == RtpDirection::sendrecv || m.direction == RtpDirection::sendonly) {
			return true;
		}
	}
	return false;
}
bool WastXSdp::canRecvRtp() const
{
	for (auto &m : _answer_sdp->media) {
		if (m.direction == RtpDirection::sendrecv || m.direction == RtpDirection::recvonly) {
			return true;
		}
	}
	return false;
}

void WastXSdp::onCheckSdp(SdpType type, RtcSession &sdp)
{
	switch (type) {
	case SdpType::answer:
		onCheckAnswer(sdp);
		break;
	case SdpType::offer:
		break;
	default: /*不可达*/
		assert(0);
		break;
	}
}
void  WastXSdp::onCheckAnswer(mediakit::RtcSession& sdp)
{
	// 修改answer sdp的ip、端口信息
	GET_CONFIG_FUNC(std::vector<std::string>, extern_ips, Rtc::kExternIP, [](string str) {
		std::vector<std::string> ret;
		if (str.length()) {
			ret = split(str, ",");
		}
		translateIPFromEnv(ret);
		return ret;
	});
	for (auto& m : sdp.media) {
		m.addr.reset();
		m.addr.address = extern_ips.empty() ? SockUtil::get_local_ip() : extern_ips[0];
		m.rtcp_addr.reset();
		m.rtcp_addr.address = m.addr.address;

		GET_CONFIG(uint16_t, local_port, Rtc::kPort);
		int nLocalPort = local_port;
		if (_b_use_local_port && _local_port != 0) {
			nLocalPort = _local_port;
		}
		m.rtcp_addr.port = nLocalPort;
		m.port = m.rtcp_addr.port;
		sdp.origin.address = m.addr.address;
	}

	if (!canSendRtp()) {
		// 设置我们发送的rtp的ssrc
		return;
	}

	for (auto& m : sdp.media) {
		if (m.type == TrackApplication) {
			continue;
		}
		if (!m.rtp_rtx_ssrc.empty()) {
			// 已经生成了ssrc
			continue;
		}
		// 添加answer sdp的ssrc信息
		m.rtp_rtx_ssrc.emplace_back();
		auto& ssrc = m.rtp_rtx_ssrc.back();
		// 发送的ssrc我们随便定义，因为在发送rtp时会修改为此值
		ssrc.ssrc = m.type + RTP_SSRC_OFFSET;
		ssrc.cname = RTP_CNAME;
		ssrc.label = RTP_LABEL;
		ssrc.mslabel = RTP_MSLABEL;
		ssrc.msid = RTP_MSID;

		if (m.getRelatedRtxPlan(m.plan[0].pt)) {
			// rtx ssrc
			ssrc.rtx_ssrc = ssrc.ssrc + RTX_SSRC_OFFSET;
		}
	}
}

SdpAttrCandidate::Ptr
makeIceCandidate(std::string ip, uint16_t port, uint32_t priority = 100, std::string proto = "udp") {
	auto candidate = std::make_shared<SdpAttrCandidate>();
	// rtp端口
	candidate->component = 1;
	candidate->transport = proto;
	candidate->foundation = proto + "candidate";
	// 优先级，单candidate时随便
	candidate->priority = priority;
	candidate->address = ip;
	candidate->port = port;
	candidate->type = "host";
	return candidate;
}
void WastXSdp::onRtcConfigure(RtcConfigure &configure) const
{
	// 开启remb后关闭twcc，因为开启twcc后remb无效
	GET_CONFIG(size_t, remb_bit_rate, Rtc::kRembBitRate);
	configure.enableTWCC(!remb_bit_rate);

	GET_CONFIG(uint16_t, local_port, Rtc::kPort);
	// 添加接收端口candidate信息
	GET_CONFIG_FUNC(std::vector<std::string>, extern_ips, Rtc::kExternIP, [](string str) {
		std::vector<std::string> ret;
		if (str.length()) {
			ret = split(str, ",");
		}
		translateIPFromEnv(ret);
		return ret;
	});
	int nLocalPort = local_port;
	if (_b_use_local_port && _local_port != 0) {
		nLocalPort = _local_port;
	}
	if (nLocalPort != 0) {
		if (extern_ips.empty()) {
			std::string localIp = SockUtil::get_local_ip();
			configure.addCandidate(*makeIceCandidate(localIp, nLocalPort, 120, "udp"));
		}
		else {
			const uint32_t delta = 10;
			uint32_t priority = 100 + delta * extern_ips.size();
			for (auto ip : extern_ips) {
				configure.addCandidate(*makeIceCandidate(ip, nLocalPort, priority, "udp"));
				priority -= delta;
			}
		}
	}
}