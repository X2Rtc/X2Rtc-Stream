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
#ifndef __X2_RTC_SDP_H__
#define __X2_RTC_SDP_H__
#include <stdint.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <set>
#ifndef WIN32
#include <stdlib.h>
#include <strings.h>
#else
#if !defined(strcasecmp)
#define strcasecmp _stricmp
#endif
#endif

namespace x2rtc {
struct StrCaseCompare {
    bool operator()(const std::string& __x, const std::string& __y) const {
        return strcasecmp(__x.data(), __y.data()) < 0;
    }
};
//Session description
//         v=  (protocol version)
//         o=  (originator and session identifier)
//         s=  (session name)
//         i=* (session information)
//         u=* (URI of description)
//         e=* (email address)
//         p=* (phone number)
//         c=* (connection information -- not required if included in
//              all media)
//         b=* (zero or more bandwidth information lines)
//         One or more time descriptions ("t=" and "r=" lines; see below)
//         z=* (time zone adjustments)
//         k=* (encryption key)
//         a=* (zero or more session attribute lines)
//         Zero or more media descriptions
//
//      Time description
//         t=  (time the session is active)
//         r=* (zero or more repeat times)
//
//      Media description, if present
//         m=  (media name and transport address)
//         i=* (media title)
//         c=* (connection information -- optional if included at
//              session level)
//         b=* (zero or more bandwidth information lines)
//         k=* (encryption key)
//         a=* (zero or more media attribute lines)
typedef enum {
    TrackInvalid = -1,
    TrackVideo = 0,
    TrackAudio,
    TrackTitle,
    TrackApplication,
    TrackMax
} TrackType;

enum class RtpDirection {
    invalid = -1,
    //只发送
    sendonly,
    //只接收
    recvonly,
    //同时发送接收
    sendrecv,
    //禁止发送数据
    inactive
};

enum class DtlsRole {
    invalid = -1,
    //客户端
    active,
    //服务端
    passive,
    //既可作做客户端也可以做服务端
    actpass,
};

enum class SdpType {
    invalid = -1,
    offer,
    answer
};


#define RTP_EXT_MAP(XX) \
    XX(ssrc_audio_level,            "urn:ietf:params:rtp-hdrext:ssrc-audio-level") \
    XX(abs_send_time,               "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time") \
    XX(transport_cc,                "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") \
    XX(sdes_mid,                    "urn:ietf:params:rtp-hdrext:sdes:mid") \
    XX(sdes_rtp_stream_id,          "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id") \
    XX(sdes_repaired_rtp_stream_id, "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id") \
    XX(video_timing,                "http://www.webrtc.org/experiments/rtp-hdrext/video-timing") \
    XX(color_space,                 "http://www.webrtc.org/experiments/rtp-hdrext/color-space") \
    XX(csrc_audio_level,            "urn:ietf:params:rtp-hdrext:csrc-audio-level") \
    XX(framemarking,                "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07") \
    XX(video_content_type,          "http://www.webrtc.org/experiments/rtp-hdrext/video-content-type") \
    XX(playout_delay,               "http://www.webrtc.org/experiments/rtp-hdrext/playout-delay") \
    XX(video_orientation,           "urn:3gpp:video-orientation") \
    XX(toffset,                     "urn:ietf:params:rtp-hdrext:toffset") \
    XX(encrypt,                     "urn:ietf:params:rtp-hdrext:encrypt") \
    XX(abs_capture_time,            "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time")

enum class RtpExtType : uint8_t {
    padding = 0,
#define XX(type, uri) type,
    RTP_EXT_MAP(XX)
#undef XX
    reserved = encrypt,
};


DtlsRole getDtlsRole(const std::string& str);
const char* getDtlsRoleString(DtlsRole role);
RtpDirection getRtpDirection(const std::string& str);
const char* getRtpDirectionString(RtpDirection val);

class SdpItem {
public:
    using Ptr = std::shared_ptr<SdpItem>;
    virtual ~SdpItem() = default;
    virtual void parse(const std::string& str) {
        value = str;
    }
    virtual std::string toString() const {
        return value;
    }
    virtual const char* getKey() const = 0;

    void reset() {
        value.clear();
    }

protected:
    mutable std::string value;
};

template <char KEY>
class SdpString : public SdpItem {
public:
    SdpString() = default;
    SdpString(std::string val) { value = std::move(val); }
    // *=*
    const char* getKey() const override { static std::string key(1, KEY); return key.data(); }
};

class SdpCommon : public SdpItem {
public:
    std::string key;
    SdpCommon(std::string key) { this->key = std::move(key); }
    SdpCommon(std::string key, std::string val) {
        this->key = std::move(key);
        this->value = std::move(val);
    }

    const char* getKey() const override { return key.data(); }
};

class SdpTime : public SdpItem {
public:
    //5.9.  Timing ("t=")
    // t=<start-time> <stop-time>
    uint64_t start{ 0 };
    uint64_t stop{ 0 };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "t"; }
};

class SdpOrigin : public SdpItem {
public:
    // 5.2.  Origin ("o=")
    // o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5
    // o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
    std::string username{ "-" };
    std::string session_id;
    std::string session_version;
    std::string nettype{ "IN" };
    std::string addrtype{ "IP4" };
    std::string address{ "0.0.0.0" };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "o"; }
    bool empty() const {
        return username.empty() || session_id.empty() || session_version.empty()
            || nettype.empty() || addrtype.empty() || address.empty();
    }
};

class SdpConnection : public SdpItem {
public:
    // 5.7.  Connection Data ("c=")
    // c=IN IP4 224.2.17.12/127
    // c=<nettype> <addrtype> <connection-address>
    std::string nettype{ "IN" };
    std::string addrtype{ "IP4" };
    std::string address{ "0.0.0.0" };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "c"; }
    bool empty() const { return address.empty(); }
};

class SdpBandwidth : public SdpItem {
public:
    //5.8.  Bandwidth ("b=")
    //b=<bwtype>:<bandwidth>

    //AS、CT
    std::string bwtype{ "AS" };
    uint32_t bandwidth{ 0 };

    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "b"; }
    bool empty() const { return bandwidth == 0; }
};

class SdpMedia : public SdpItem {
public:
    // 5.14.  Media Descriptions ("m=")
    // m=<media> <port> <proto> <fmt> ...
    TrackType type;
    uint16_t port;
    //RTP/AVP：应用场景为视频/音频的 RTP 协议。参考 RFC 3551
    //RTP/SAVP：应用场景为视频/音频的 SRTP 协议。参考 RFC 3711
    //RTP/AVPF: 应用场景为视频/音频的 RTP 协议，支持 RTCP-based Feedback。参考 RFC 4585
    //RTP/SAVPF: 应用场景为视频/音频的 SRTP 协议，支持 RTCP-based Feedback。参考 RFC 5124
    std::string proto;
    std::vector<std::string> fmts;

    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "m"; }
};

class SdpAttr : public SdpItem {
public:
    using Ptr = std::shared_ptr<SdpAttr>;
    //5.13.  Attributes ("a=")
    //a=<attribute>
    //a=<attribute>:<value>
    SdpItem::Ptr detail;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "a"; }
};

class SdpAttrGroup : public SdpItem {
public:
    //a=group:BUNDLE line with all the 'mid' identifiers part of the
    //  BUNDLE group is included at the session-level.
    //a=group:LS session level attribute MUST be included wth the 'mid'
    //  identifiers that are part of the same lip sync group.
    std::string type{ "BUNDLE" };
    std::vector<std::string> mids;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "group"; }
};

class SdpAttrMsidSemantic : public SdpItem {
public:
    //https://tools.ietf.org/html/draft-alvestrand-rtcweb-msid-02#section-3
    //3.  The Msid-Semantic Attribute
    //
    //   In order to fully reproduce the semantics of the SDP and SSRC
    //   grouping frameworks, a session-level attribute is defined for
    //   signalling the semantics associated with an msid grouping.
    //
    //   This OPTIONAL attribute gives the message ID and its group semantic.
    //     a=msid-semantic: examplefoo LS
    //
    //
    //   The ABNF of msid-semantic is:
    //
    //     msid-semantic-attr = "msid-semantic:" " " msid token
    //     token = <as defined in RFC 4566>
    //
    //   The semantic field may hold values from the IANA registries
    //   "Semantics for the "ssrc-group" SDP Attribute" and "Semantics for the
    //   "group" SDP Attribute".
    //a=msid-semantic: WMS 616cfbb1-33a3-4d8c-8275-a199d6005549
    std::string msid{ "WMS" };
    std::string token;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "msid-semantic"; }
    bool empty() const {
        return msid.empty();
    }
};

class SdpAttrRtcp : public SdpItem {
public:
    // a=rtcp:9 IN IP4 0.0.0.0
    uint16_t port{ 0 };
    std::string nettype{ "IN" };
    std::string addrtype{ "IP4" };
    std::string address{ "0.0.0.0" };
    void parse(const std::string& str) override;;
    std::string toString() const override;
    const char* getKey() const override { return "rtcp"; }
    bool empty() const {
        return address.empty() || !port;
    }
};

class SdpAttrIceUfrag : public SdpItem {
public:
    SdpAttrIceUfrag() = default;
    SdpAttrIceUfrag(std::string str) { value = std::move(str); }
    //a=ice-ufrag:sXJ3
    const char* getKey() const override { return "ice-ufrag"; }
};

class SdpAttrIcePwd : public SdpItem {
public:
    SdpAttrIcePwd() = default;
    SdpAttrIcePwd(std::string str) { value = std::move(str); }
    //a=ice-pwd:yEclOTrLg1gEubBFefOqtmyV
    const char* getKey() const override { return "ice-pwd"; }
};

class SdpAttrIceOption : public SdpItem {
public:
    //a=ice-options:trickle
    bool trickle{ false };
    bool renomination{ false };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "ice-options"; }
};

class SdpAttrFingerprint : public SdpItem {
public:
    //a=fingerprint:sha-256 22:14:B5:AF:66:12:C7:C7:8D:EF:4B:DE:40:25:ED:5D:8F:17:54:DD:88:33:C0:13:2E:FD:1A:FA:7E:7A:1B:79
    std::string algorithm;
    std::string hash;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "fingerprint"; }
    bool empty() const { return algorithm.empty() || hash.empty(); }
};

class SdpAttrSetup : public SdpItem {
public:
    //a=setup:actpass
    SdpAttrSetup() = default;
    SdpAttrSetup(DtlsRole r) { role = r; }
    DtlsRole role{ DtlsRole::actpass };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "setup"; }
};

class SdpAttrMid : public SdpItem {
public:
    SdpAttrMid() = default;
    SdpAttrMid(std::string val) { value = std::move(val); }
    //a=mid:audio
    const char* getKey() const override { return "mid"; }
};

class SdpAttrExtmap : public SdpItem {
public:
    //https://aggresss.blog.csdn.net/article/details/106436703
    //a=extmap:1[/sendonly] urn:ietf:params:rtp-hdrext:ssrc-audio-level
    uint8_t id;
    RtpDirection direction{ RtpDirection::invalid };
    std::string ext;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "extmap"; }
};

class SdpAttrRtpMap : public SdpItem {
public:
    //a=rtpmap:111 opus/48000/2
    uint8_t pt;
    std::string codec;
    uint32_t sample_rate;
    uint32_t channel{ 0 };
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "rtpmap"; }
};

class SdpAttrRtcpFb : public SdpItem {
public:
    //a=rtcp-fb:* nack pli
    //a=rtcp-fb:98 nack pli
    //a=rtcp-fb:120 nack 支持 nack 重传，nack (Negative-Acknowledgment) 。
    //a=rtcp-fb:120 nack pli 支持 nack 关键帧重传，PLI (Picture Loss Indication) 。
    //a=rtcp-fb:120 ccm fir 支持编码层关键帧请求，CCM (Codec Control Message)，FIR (Full Intra Request )，通常与 nack pli 有同样的效果，但是 nack pli 是用于重传时的关键帧请求。
    //a=rtcp-fb:120 goog-remb 支持 REMB (Receiver Estimated Maximum Bitrate) 。
    //a=rtcp-fb:120 transport-cc 支持 TCC (Transport Congest Control) 。
    int16_t pt; //@Eric - uint8_t => int16_t
    std::string rtcp_type;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "rtcp-fb"; }
};

class SdpAttrFmtp : public SdpItem {
public:
    //fmtp:96 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f
    uint8_t pt;
    std::map<std::string/*key*/, std::string/*value*/, StrCaseCompare> fmtp;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "fmtp"; }
};

class SdpAttrSSRC : public SdpItem {
public:
    //a=ssrc:3245185839 cname:Cx4i/VTR51etgjT7
    //a=ssrc:3245185839 msid:cb373bff-0fea-4edb-bc39-e49bb8e8e3b9 0cf7e597-36a2-4480-9796-69bf0955eef5
    //a=ssrc:3245185839 mslabel:cb373bff-0fea-4edb-bc39-e49bb8e8e3b9
    //a=ssrc:3245185839 label:0cf7e597-36a2-4480-9796-69bf0955eef5
    //a=ssrc:<ssrc-id> <attribute>
    //a=ssrc:<ssrc-id> <attribute>:<value>
    //cname 是必须的，msid/mslabel/label 这三个属性都是 WebRTC 自创的，或者说 Google 自创的，可以参考 https://tools.ietf.org/html/draft-ietf-mmusic-msid-17，
    // 理解它们三者的关系需要先了解三个概念：RTP stream / MediaStreamTrack / MediaStream ：
    //一个 a=ssrc 代表一个 RTP stream ；
    //一个 MediaStreamTrack 通常包含一个或多个 RTP stream，例如一个视频 MediaStreamTrack 中通常包含两个 RTP stream，一个用于常规传输，一个用于 nack 重传；
    //一个 MediaStream 通常包含一个或多个 MediaStreamTrack ，例如 simulcast 场景下，一个 MediaStream 通常会包含三个不同编码质量的 MediaStreamTrack ；
    //这种标记方式并不被 Firefox 认可，在 Firefox 生成的 SDP 中一个 a=ssrc 通常只有一行，例如：
    //a=ssrc:3245185839 cname:Cx4i/VTR51etgjT7

    uint32_t ssrc;
    std::string attribute;
    std::string attribute_value;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "ssrc"; }
};

class SdpAttrSSRCGroup : public SdpItem {
public:
    //a=ssrc-group 定义参考 RFC 5576(https://tools.ietf.org/html/rfc5576) ，用于描述多个 ssrc 之间的关联，常见的有两种：
    //a=ssrc-group:FID 2430709021 3715850271
    // FID (Flow Identification) 最初用在 FEC 的关联中，WebRTC 中通常用于关联一组常规 RTP stream 和 重传 RTP stream 。
    //a=ssrc-group:SIM 360918977 360918978 360918980
    // 在 Chrome 独有的 SDP munging 风格的 simulcast 中使用，将三组编码质量由低到高的 MediaStreamTrack 关联在一起。
    std::string type{ "FID" };
    std::vector<uint32_t> ssrcs;

    bool isFID() const { return type == "FID"; }
    bool isSIM() const { return type == "SIM"; }
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "ssrc-group"; }
};

class SdpAttrSctpMap : public SdpItem {
public:
    //https://tools.ietf.org/html/draft-ietf-mmusic-sctp-sdp-05
    //a=sctpmap:5000 webrtc-datachannel 1024
    //a=sctpmap: sctpmap-number media-subtypes [streams]
    uint16_t port = 0;
    std::string subtypes;
    uint32_t streams = 0;
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "sctpmap"; }
    bool empty() const { return port == 0 && subtypes.empty() && streams == 0; }
};

class SdpAttrCandidate : public SdpItem {
public:
    using Ptr = std::shared_ptr<SdpAttrCandidate>;
    //https://tools.ietf.org/html/rfc5245
    //15.1.  "candidate" Attribute
    //a=candidate:4 1 udp 2 192.168.1.7 58107 typ host
    //a=candidate:<foundation> <component-id> <transport> <priority> <address> <port> typ <cand-type>
    std::string foundation;
    //传输媒体的类型,1代表RTP;2代表 RTCP。
    uint32_t component;
    std::string transport{ "udp" };
    uint32_t priority;
    std::string address;
    uint16_t port;
    std::string type;
    std::vector<std::pair<std::string, std::string> > arr;

    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "candidate"; }
};

class SdpAttrMsid : public SdpItem {
public:
    const char* getKey() const override { return "msid"; }
};

class SdpAttrExtmapAllowMixed : public SdpItem {
public:
    const char* getKey() const override { return "extmap-allow-mixed"; }
};

class SdpAttrSimulcast : public SdpItem {
public:
    //https://www.meetecho.com/blog/simulcast-janus-ssrc/
    //https://tools.ietf.org/html/draft-ietf-mmusic-sdp-simulcast-14
    const char* getKey() const override { return "simulcast"; }
    void parse(const std::string& str) override;
    std::string toString() const override;
    bool empty() const { return rids.empty(); }
    std::string direction;
    std::vector<std::string> rids;
};

class SdpAttrRid : public SdpItem {
public:
    void parse(const std::string& str) override;
    std::string toString() const override;
    const char* getKey() const override { return "rid"; }
    std::string direction;
    std::string rid;
};

class RtcSdpBase {
public:
    void addItem(SdpItem::Ptr item) { items.push_back(std::move(item)); }
    void addAttr(SdpItem::Ptr attr) {
        auto item = std::make_shared<SdpAttr>();
        item->detail = std::move(attr);
        items.push_back(std::move(item));
    }

    virtual ~RtcSdpBase() = default;
    virtual std::string toString() const;
    void toRtsp();

    RtpDirection getDirection() const;

    template<typename cls>
    cls getItemClass(char key, const char* attr_key = nullptr) const {
        auto item = std::dynamic_pointer_cast<cls>(getItem(key, attr_key));
        if (!item) {
            return cls();
        }
        return *item;
    }

    std::string getStringItem(char key, const char* attr_key = nullptr) const {
        auto item = getItem(key, attr_key);
        if (!item) {
            return "";
        }
        return item->toString();
    }

    SdpItem::Ptr getItem(char key, const char* attr_key = nullptr) const;

    template<typename cls>
    std::vector<cls> getAllItem(char key_c, const char* attr_key = nullptr) const {
        std::vector<cls> ret;
        std::string key(1, key_c);
        for (auto item : items) {
            if (strcasecmp(item->getKey(), key.data()) == 0) {
                if (!attr_key) {
                    auto c = std::dynamic_pointer_cast<cls>(item);
                    if (c) {
                        ret.emplace_back(*c);
                    }
                }
                else {
                    auto attr = std::dynamic_pointer_cast<SdpAttr>(item);
                    if (attr && !strcasecmp(attr->detail->getKey(), attr_key)) {
                        auto c = std::dynamic_pointer_cast<cls>(attr->detail);
                        if (c) {
                            ret.emplace_back(*c);
                        }
                    }
                }
            }
        }
        return ret;
    }

private:
    std::vector<SdpItem::Ptr> items;
};

class RtcSessionSdp : public RtcSdpBase {
public:
    using Ptr = std::shared_ptr<RtcSessionSdp>;
    int getVersion() const;
    SdpOrigin getOrigin() const;
    std::string getSessionName() const;
    std::string getSessionInfo() const;
    SdpTime getSessionTime() const;
    SdpConnection getConnection() const;
    SdpBandwidth getBandwidth() const;

    std::string getUri() const;
    std::string getEmail() const;
    std::string getPhone() const;
    std::string getTimeZone() const;
    std::string getEncryptKey() const;
    std::string getRepeatTimes() const;

    std::vector<RtcSdpBase> medias;
    void parse(const std::string& str);
    std::string toString() const override;
};

//////////////////////////////////////////////////////////////////

//ssrc相关信息
class RtcSSRC {
public:
    uint32_t ssrc{ 0 };
    uint32_t rtx_ssrc{ 0 };
    std::string cname;
    std::string msid;
    std::string mslabel;
    std::string label;

    bool empty() const { return ssrc == 0 && cname.empty(); }
};

//rtc传输编码方案
class RtcCodecPlan {
public:
    using Ptr = std::shared_ptr<RtcCodecPlan>;
    uint8_t pt;
    std::string codec;
    uint32_t sample_rate;
    //音频时有效
    uint32_t channel = 0;
    //rtcp反馈
    std::set<std::string> rtcp_fb;
    std::map<std::string/*key*/, std::string/*value*/, StrCaseCompare> fmtp;

    std::string getFmtp(const char* key) const;
};

//rtc 媒体描述
class RtcMedia {
public:
    TrackType type{ TrackType::TrackInvalid };
    std::string mid;
    uint16_t port{ 0 };
    SdpConnection addr;
    SdpBandwidth bandwidth;
    std::string proto;
    RtpDirection direction{ RtpDirection::invalid };
    std::vector<RtcCodecPlan> plan;

    //////// rtp ////////
    std::vector<RtcSSRC> rtp_rtx_ssrc;

    //////// simulcast ////////
    std::vector<RtcSSRC> rtp_ssrc_sim;
    std::vector<std::string> rtp_rids;

    ////////  rtcp  ////////
    bool rtcp_mux{ false };
    bool rtcp_rsize{ false };
    SdpAttrRtcp rtcp_addr;

    //////// ice ////////
    bool ice_trickle{ false };
    bool ice_lite{ false };
    bool ice_renomination{ false };
    std::string ice_ufrag;
    std::string ice_pwd;
    std::vector<SdpAttrCandidate> candidate;

    //////// dtls ////////
    DtlsRole role{ DtlsRole::invalid };
    SdpAttrFingerprint fingerprint;

    //////// extmap ////////
    std::vector<SdpAttrExtmap> extmap;

    //////// sctp ////////////
    SdpAttrSctpMap sctpmap;
    uint32_t sctp_port{ 0 };

    void checkValid() const;
    const RtcCodecPlan* getPlan(uint8_t pt) const;
    const RtcCodecPlan* getPlan(const char* codec) const;
    const RtcCodecPlan* getRelatedRtxPlan(uint8_t pt) const;
    uint32_t getRtpSSRC() const;
    uint32_t getRtxSSRC() const;
    bool supportSimulcast() const;
};


#define CODEC_MAP(XX) \
    XX(CodecH264,  TrackVideo, 0, "H264", PSI_STREAM_H264)          \
    XX(CodecH265,  TrackVideo, 1, "H265", PSI_STREAM_H265)          \
    XX(CodecAAC,   TrackAudio, 2, "mpeg4-generic", PSI_STREAM_AAC)  \
    XX(CodecG711A, TrackAudio, 3, "PCMA", PSI_STREAM_AUDIO_G711A)   \
    XX(CodecG711U, TrackAudio, 4, "PCMU", PSI_STREAM_AUDIO_G711U)   \
    XX(CodecOpus,  TrackAudio, 5, "opus", PSI_STREAM_AUDIO_OPUS)    \
    XX(CodecL16,   TrackAudio, 6, "L16", PSI_STREAM_RESERVED)       \
    XX(CodecVP8,   TrackVideo, 7, "VP8", PSI_STREAM_VP8)            \
    XX(CodecVP9,   TrackVideo, 8, "VP9", PSI_STREAM_VP9)            \
    XX(CodecAV1,   TrackVideo, 9, "AV1", PSI_STREAM_AV1)            \
    XX(CodecJPEG,  TrackVideo, 10, "JPEG", PSI_STREAM_JPEG_2000)    \
    XX(CodecAAC_A, TrackAudio, 11, "MP4A-ADTS", PSI_STREAM_AAC)       \

typedef enum {
    CodecInvalid = -1,
#define XX(name, type, value, str, mpeg_id) name = value,
    CODEC_MAP(XX)
#undef XX
    CodecMax
} CodecId;

class RtpPayload {
public:
    static int getClockRate(int pt);
    static TrackType getTrackType(int pt);
    static int getAudioChannel(int pt);
    static const char* getName(int pt);
    static CodecId getCodecId(int pt);

private:
    RtpPayload() = delete;
    ~RtpPayload() = delete;
};

class X2RtcSdp
{
public:
    using Ptr = std::shared_ptr<X2RtcSdp>;

    uint32_t version;
    SdpOrigin origin;
    std::string session_name;
    std::string session_info;
    SdpTime time;
    SdpConnection connection;
    SdpAttrMsidSemantic msid_semantic;
    std::vector<RtcMedia> media;
    SdpAttrGroup group;

    void loadFrom(const std::string& sdp);
    void checkValid() const;
    std::string toString() const;
    std::string toRtspSdp() const;
    const  RtcMedia* getMedia(TrackType type) const;
    bool supportRtcpFb(const std::string& name, TrackType type = TrackType::TrackVideo) const;
    bool supportSimulcast() const;
    bool isOnlyDatachannel() const;

    bool isIceLite() const;//@Eric - ice-lite 要加在media的外面

private:
    RtcSessionSdp::Ptr toRtcSessionSdp() const;
};

class SdpConst {
public:
    static std::string const kTWCCRtcpFb;
    static std::string const kRembRtcpFb;

private:
    SdpConst() = delete;
    ~SdpConst() = delete;
};

class X2RtcConfigure {
public:
    using Ptr = std::shared_ptr<X2RtcConfigure>;
    class RtcTrackConfigure {
    public:
        bool rtcp_mux;
        bool rtcp_rsize;
        bool group_bundle;
        bool support_rtx;
        bool support_red;
        bool support_ulpfec;
        bool ice_lite;
        bool ice_trickle;
        bool ice_renomination;
        std::string ice_ufrag;
        std::string ice_pwd;

        RtpDirection direction{ RtpDirection::invalid };
        SdpAttrFingerprint fingerprint;

        std::set<std::string> rtcp_fb;
        std::map<RtpExtType, RtpDirection> extmap;
        std::vector<CodecId> preferred_codec;
        std::vector<SdpAttrCandidate> candidate;

        void setDefaultSetting(TrackType type, const std::string&strPreferCodec);
        void clearAll();
        void enableTWCC(bool enable = true);
        void enableREMB(bool enable = true);
    };

    RtcTrackConfigure video;
    RtcTrackConfigure audio;
    RtcTrackConfigure application;

    void setDefaultSetting(std::string ice_ufrag, std::string ice_pwd, RtpDirection direction, const SdpAttrFingerprint& fingerprint);
    void addCandidate(const SdpAttrCandidate& candidate, TrackType type = TrackInvalid);

    void createOffer(const X2RtcSdp& offer) const;
    std::shared_ptr<X2RtcSdp> createAnswer(const X2RtcSdp& offer) const;

    void setPlayRtspInfo(const std::string& sdp);

    void enableTWCC(bool enable = true, TrackType type = TrackInvalid);
    void enableREMB(bool enable = true, TrackType type = TrackInvalid);

    void setAudioPreferCodec(const std::string& strCodec);
    void setVideoPreferCodec(const std::string& strCodec);
    
private:
    void matchMedia(const std::shared_ptr<X2RtcSdp>& ret, const RtcMedia& media) const;
    bool onCheckCodecProfile(const RtcCodecPlan& plan, CodecId codec) const;
    void onSelectPlan(RtcCodecPlan& plan, CodecId codec) const;

private:
    RtcCodecPlan::Ptr _rtsp_video_plan;
    RtcCodecPlan::Ptr _rtsp_audio_plan;

    std::string str_audio_codec_;
    std::string str_video_codec_;
};

void SetExternIp(const std::string& strExternIp);
void SetExternPort(int nPort);
void SdpCheckAnswer(X2RtcSdp::Ptr sdp);

}	// namespace x2rtc
#endif	// __X2_RTC_SDP_H__