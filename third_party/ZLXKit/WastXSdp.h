#ifndef __WAST_X_SDP_H__
#define __WAST_X_SDP_H__
#include "Sdp.h"
#include "Util/util.h"
#include "Util/onceToken.h"
#include "Util/mini.h"

namespace mediakit {
// RTC配置项目
namespace Rtc {
#define RTC_FIELD "rtc."
	// rtp和rtcp接受超时时间
	const std::string kTimeOutSec = RTC_FIELD "timeoutSec";
	// 服务器外网ip
	const std::string kExternIP = RTC_FIELD "externIP";
	// 设置remb比特率，非0时关闭twcc并开启remb。该设置在rtc推流时有效，可以控制推流画质
	const std::string kRembBitRate = RTC_FIELD "rembBitRate";
	// webrtc单端口udp服务器
	const std::string kPort = RTC_FIELD "port";

	static toolkit::onceToken token([]() {
		toolkit::mINI::Instance()[kTimeOutSec] = 15;
		toolkit::mINI::Instance()[kExternIP] = "";
		toolkit::mINI::Instance()[kRembBitRate] = 0;
		toolkit::mINI::Instance()[kPort] = 0;
	});

	static void SetExternIp(const std::string& strExternIp) {
		static std::string gStrExternIp;
		gStrExternIp = strExternIp;
		toolkit::mINI::Instance()[kExternIP] = gStrExternIp.c_str();

	}
	static void SetSinglePort(int nPort) {
		toolkit::mINI::Instance()[kPort] = nPort;
	}
} // namespace RTC

static std::atomic<uint64_t> s_key{ 0 };

static void translateIPFromEnv(std::vector<std::string>& v) {
	for (auto iter = v.begin(); iter != v.end();) {
		if (toolkit::start_with(*iter, "$")) {
			auto ip = toolkit::getEnv(*iter);
			if (ip.empty()) {
				iter = v.erase(iter);
			}
			else {
				*iter++ = ip;
			}
		}
		else {
			++iter;
		}
	}
}
}

class WastXSdp
{
public:
	WastXSdp(void);
	virtual ~WastXSdp(void);

	void SetLocalPort(int nPort);
	int GetLocalPort();
	void SetUseLocalPort(bool bUse);

	/**
	* 创建webrtc answer sdp
	* @param offer offer sdp
	* @return answer sdp
	*/
	std::string getAnswerSdp(const std::string &offer, const std::string &ufrag, const std::string &password);

	std::string getOfferSdp(const std::string &offer, const std::string &ufrag, const std::string &password);
	void recvAnswerSdp(const std::string &answer);

	bool canSendRtp() const;
	bool canRecvRtp() const;

	void onCheckSdp(mediakit::SdpType type, mediakit::RtcSession &sdp);
	void onCheckAnswer(mediakit::RtcSession& sdp);
	void onRtcConfigure(mediakit::RtcConfigure &configure) const;

public:
	virtual std::string getFingerprint(const std::string &algorithm_str) = 0;
	virtual void setRemoteDtlsFingerprint(const mediakit::RtcSession &remote) = 0;

protected:
	mediakit::RtcSession::Ptr _offer_sdp;
	mediakit::RtcSession::Ptr _answer_sdp;

	bool _b_use_local_port;
	int _local_port;
};

#endif	// __WAST_X_SDP_H__