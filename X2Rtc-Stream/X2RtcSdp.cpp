#include "X2RtcSdp.h"
#include <cinttypes>
#include <functional>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include "X2RtcLog.h"

using namespace std;
namespace x2rtc {
#define RTC_FIELD "rtc."
#define RTP_SSRC_OFFSET 1
#define RTX_SSRC_OFFSET 2
#define RTP_CNAME "x2rtc-rtp"
#define RTP_LABEL "x2rtc-label"
#define RTP_MSLABEL "x2rtc-mslabel"
#define RTP_MSID RTP_MSLABEL " " RTP_LABEL

//////////////////////////////////////////////////////////////////////////////////////////
class AssertFailedException : public std::runtime_error {
public:
    template<typename ...T>
    AssertFailedException(T && ...args) : std::runtime_error(std::forward<T>(args)...) {}
    ~AssertFailedException() override = default;
};
void Assert_Throw(int failed, const char* exp, const char* func, const char* file, int line, const char* str) {
    if (failed) {
        std::stringstream printer;
        printer << "Assertion failed: (" << exp;
        if (str && *str) {
            printer << ", " << str;
        }
        printer << "), function " << func << ", file " << file << ", line " << line << ".";
        throw AssertFailedException(printer.str());
    }
}
template <typename... ARGS>
void Assert_ThrowCpp(int failed, const char* exp, const char* func, const char* file, int line, ARGS &&...args) {
    if (failed) {
        std::stringstream ss;
        //toolkit::LoggerWrapper::appendLog(ss, std::forward<ARGS>(args)...);
        Assert_Throw(failed, exp, func, file, line, ss.str().data());
    }
}
#ifndef CHECK
#define CHECK(exp, ...) Assert_ThrowCpp(!(exp), #exp, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif // CHECK
#define CHECK_SDP(exp) CHECK(exp, "����sdp ", getKey(), " �ֶ�ʧ��:", str)

//////////////////////////////////////////////////////////////////////////////////////////

using onCreateSdpItem = function<SdpItem::Ptr(const string& key, const string& value)>;
static map<string, onCreateSdpItem, StrCaseCompare> sdpItemCreator;

template <typename Item>
void registerSdpItem() {
    onCreateSdpItem func = [](const string& key, const string& value) {
        auto ret = std::make_shared<Item>();
        ret->parse(value);
        return ret;
    };
    Item item;
    sdpItemCreator.emplace(item.getKey(), std::move(func));
}

vector<string> split(const string& s, const char* delim) {
    vector<string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}
#define TRIM(s, chars) \
do{ \
    string map(0xFF, '\0'); \
    for (auto &ch : chars) { \
        map[(unsigned char &)ch] = '\1'; \
    } \
    while( s.size() && map.at((unsigned char &)s.back())) s.pop_back(); \
    while( s.size() && map.at((unsigned char &)s.front())) s.erase(0,1); \
}while(0);
//ȥ��ǰ��Ŀո񡢻س������Ʊ��
std::string& trim(std::string& s, const string& chars) {
    TRIM(s, chars);
    return s;
}

static map<string, TrackType, StrCaseCompare> track_str_map = {
        {"video",       TrackVideo},
        {"audio",       TrackAudio},
        {"application", TrackApplication}
};

TrackType getTrackType(const string& str) {
    auto it = track_str_map.find(str);
    return it == track_str_map.end() ? TrackInvalid : it->second;
}

const char* getTrackString(TrackType type) {
    switch (type) {
    case TrackVideo: return "video";
    case TrackAudio: return "audio";
    case TrackApplication: return "application";
    default: return "invalid";
    }
}

#define XX(name, type, value, str, mpeg_id) {str, name},
static map<string, CodecId, StrCaseCompare> codec_map = { CODEC_MAP(XX) };
#undef XX

CodecId getCodecId(const string& str) {
    auto it = codec_map.find(str);
    return it == codec_map.end() ? CodecInvalid : it->second;
}

TrackType getTrackType(CodecId codecId) {
    switch (codecId) {
#define XX(name, type, value, str, mpeg_id) case name : return type;
        CODEC_MAP(XX)
#undef XX
    default: return TrackInvalid;
    }
}

const char* getCodecName(CodecId codec) {
    switch (codec) {
#define XX(name, type, value, str, mpeg_id) case name : return str;
        CODEC_MAP(XX)
#undef XX
    default: return "invalid";
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
#define RTP_PT_MAP(XX) \
    XX(PCMU, TrackAudio, 0, 8000, 1, CodecG711U) \
    XX(GSM, TrackAudio , 3, 8000, 1, CodecInvalid) \
    XX(G723, TrackAudio, 4, 8000, 1, CodecInvalid) \
    XX(DVI4_8000, TrackAudio, 5, 8000, 1, CodecInvalid) \
    XX(DVI4_16000, TrackAudio, 6, 16000, 1, CodecInvalid) \
    XX(LPC, TrackAudio, 7, 8000, 1, CodecInvalid) \
    XX(PCMA, TrackAudio, 8, 8000, 1, CodecG711A) \
    XX(G722, TrackAudio, 9, 8000, 1, CodecInvalid) \
    XX(L16_Stereo, TrackAudio, 10, 44100, 2, CodecInvalid) \
    XX(L16_Mono, TrackAudio, 11, 44100, 1, CodecInvalid) \
    XX(QCELP, TrackAudio, 12, 8000, 1, CodecInvalid) \
    XX(CN, TrackAudio, 13, 8000, 1, CodecInvalid) \
    XX(MPA, TrackAudio, 14, 90000, 1, CodecInvalid) \
    XX(G728, TrackAudio, 15, 8000, 1, CodecInvalid) \
    XX(DVI4_11025, TrackAudio, 16, 11025, 1, CodecInvalid) \
    XX(DVI4_22050, TrackAudio, 17, 22050, 1, CodecInvalid) \
    XX(G729, TrackAudio, 18, 8000, 1, CodecInvalid) \
    XX(CelB, TrackVideo, 25, 90000, 1, CodecInvalid) \
    XX(JPEG, TrackVideo, 26, 90000, 1, CodecJPEG) \
    XX(nv, TrackVideo, 28, 90000, 1, CodecInvalid) \
    XX(H261, TrackVideo, 31, 90000, 1, CodecInvalid) \
    XX(MPV, TrackVideo, 32, 90000, 1, CodecInvalid) \
    XX(MP2T, TrackVideo, 33, 90000, 1, CodecInvalid) \
    XX(H263, TrackVideo, 34, 90000, 1, CodecInvalid) \

typedef enum {
#define ENUM_DEF(name, type, value, clock_rate, channel, codec_id) PT_ ## name = value,
    RTP_PT_MAP(ENUM_DEF)
#undef ENUM_DEF
    PT_MAX = 128
} PayloadType;

int RtpPayload::getClockRate(int pt) {
    switch (pt) {
#define SWITCH_CASE(name, type, value, clock_rate, channel, codec_id) case value :  return clock_rate;
        RTP_PT_MAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return 90000;
    }
}

TrackType RtpPayload::getTrackType(int pt) {
    switch (pt) {
#define SWITCH_CASE(name, type, value, clock_rate, channel, codec_id) case value :  return type;
        RTP_PT_MAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return TrackInvalid;
    }
}

int RtpPayload::getAudioChannel(int pt) {
    switch (pt) {
#define SWITCH_CASE(name, type, value, clock_rate, channel, codec_id) case value :  return channel;
        RTP_PT_MAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return 1;
    }
}

const char* RtpPayload::getName(int pt) {
    switch (pt) {
#define SWITCH_CASE(name, type, value, clock_rate, channel, codec_id) case value :  return #name;
        RTP_PT_MAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return "unknown payload type";
    }
}

CodecId RtpPayload::getCodecId(int pt) {
    switch (pt) {
#define SWITCH_CASE(name, type, value, clock_rate, channel, codec_id) case value :  return codec_id;
        RTP_PT_MAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return CodecInvalid;
    }
}

static std::string gStrExternIp;
static int gExternPort = 0;
void SetExternIp(const std::string& strExternIp) {
    gStrExternIp = strExternIp;
}
void SetExternPort(int nPort) {
    gExternPort = nPort;
}
SdpAttrCandidate::Ptr makeIceCandidate(std::string ip, uint16_t port, uint32_t priority = 100, std::string proto = "udp") {
    auto candidate = std::make_shared<SdpAttrCandidate>();
    // rtp�˿�
    candidate->component = 1;
    candidate->transport = proto;
    candidate->foundation = proto + "candidate";
    // ���ȼ�����candidateʱ���
    candidate->priority = priority;
    candidate->address = ip;
    candidate->port = port;
    candidate->type = "host";
    return candidate;
}

//////////////////////////////////////////////////////////////////////////////////////////
class DirectionInterface {
public:
    virtual RtpDirection getDirection() const = 0;
};

class SdpDirectionSendonly : public SdpItem, public DirectionInterface {
public:
    const char* getKey() const override { return getRtpDirectionString(getDirection()); }
    RtpDirection getDirection() const override { return RtpDirection::sendonly; }
};

class SdpDirectionRecvonly : public SdpItem, public DirectionInterface {
public:
    const char* getKey() const override { return getRtpDirectionString(getDirection()); }
    RtpDirection getDirection() const override { return RtpDirection::recvonly; }
};

class SdpDirectionSendrecv : public SdpItem, public DirectionInterface {
public:
    const char* getKey() const override { return getRtpDirectionString(getDirection()); }
    RtpDirection getDirection() const override { return RtpDirection::sendrecv; }
};

class SdpDirectionInactive : public SdpItem, public DirectionInterface {
public:
    const char* getKey() const override { return getRtpDirectionString(getDirection()); }
    RtpDirection getDirection() const override { return RtpDirection::inactive; }
};

class DirectionInterfaceImp : public SdpItem, public DirectionInterface {
public:
    DirectionInterfaceImp(RtpDirection direct) {
        direction = direct;
    }
    const char* getKey() const override { return getRtpDirectionString(getDirection()); }
    RtpDirection getDirection() const override { return direction; }

private:
    RtpDirection direction;
};

static bool registerAllItem() {
    registerSdpItem<SdpString<'v'> >();
    registerSdpItem<SdpString<'s'> >();
    registerSdpItem<SdpString<'i'> >();
    registerSdpItem<SdpString<'u'> >();
    registerSdpItem<SdpString<'e'> >();
    registerSdpItem<SdpString<'p'> >();
    registerSdpItem<SdpString<'z'> >();
    registerSdpItem<SdpString<'k'> >();
    registerSdpItem<SdpString<'r'> >();
    registerSdpItem<SdpTime>();
    registerSdpItem<SdpOrigin>();
    registerSdpItem<SdpConnection>();
    registerSdpItem<SdpBandwidth>();
    registerSdpItem<SdpMedia>();
    registerSdpItem<SdpAttr>();
    registerSdpItem<SdpAttrGroup>();
    registerSdpItem<SdpAttrMsidSemantic>();
    registerSdpItem<SdpAttrRtcp>();
    registerSdpItem<SdpAttrIceUfrag>();
    registerSdpItem<SdpAttrIcePwd>();
    registerSdpItem<SdpAttrIceOption>();
    registerSdpItem<SdpAttrFingerprint>();
    registerSdpItem<SdpAttrSetup>();
    registerSdpItem<SdpAttrMid>();
    registerSdpItem<SdpAttrExtmap>();
    registerSdpItem<SdpAttrRtpMap>();
    registerSdpItem<SdpAttrRtcpFb>();
    registerSdpItem<SdpAttrFmtp>();
    registerSdpItem<SdpAttrSSRC>();
    registerSdpItem<SdpAttrSSRCGroup>();
    registerSdpItem<SdpAttrSctpMap>();
    registerSdpItem<SdpAttrCandidate>();
    registerSdpItem<SdpDirectionSendonly>();
    registerSdpItem<SdpDirectionRecvonly>();
    registerSdpItem<SdpDirectionSendrecv>();
    registerSdpItem<SdpDirectionInactive>();
    registerSdpItem<SdpAttrMsid>();
    registerSdpItem<SdpAttrExtmapAllowMixed>();
    registerSdpItem<SdpAttrRid>();
    registerSdpItem<SdpAttrSimulcast>();
    return true;
}

static map<string, DtlsRole, StrCaseCompare> dtls_role_map = {
        {"active",  DtlsRole::active},
        {"passive", DtlsRole::passive},
        {"actpass", DtlsRole::actpass}
};

DtlsRole getDtlsRole(const string& str) {
    auto it = dtls_role_map.find(str);
    return it == dtls_role_map.end() ? DtlsRole::invalid : it->second;
}

const char* getDtlsRoleString(DtlsRole role) {
    switch (role) {
    case DtlsRole::active: return "active";
    case DtlsRole::passive: return "passive";
    case DtlsRole::actpass: return "actpass";
    default: return "invalid";
    }
}

static map<string, RtpDirection, StrCaseCompare> direction_map = {
        {"sendonly", RtpDirection::sendonly},
        {"recvonly", RtpDirection::recvonly},
        {"sendrecv", RtpDirection::sendrecv},
        {"inactive", RtpDirection::inactive}
};

RtpDirection getRtpDirection(const string& str) {
    auto it = direction_map.find(str);
    return it == direction_map.end() ? RtpDirection::invalid : it->second;
}

const char* getRtpDirectionString(RtpDirection val) {
    switch (val) {
    case RtpDirection::sendonly: return "sendonly";
    case RtpDirection::recvonly: return "recvonly";
    case RtpDirection::sendrecv: return "sendrecv";
    case RtpDirection::inactive: return "inactive";
    default: return "invalid";
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

string RtcSdpBase::toString() const {
    std::stringstream printer;
    for (auto& item : items) {
        printer << item->getKey() << "=" << item->toString() << "\r\n";
    }
    return printer.str();
}

RtpDirection RtcSdpBase::getDirection() const {
    for (auto& item : items) {
        auto attr = dynamic_pointer_cast<SdpAttr>(item);
        if (attr) {
            auto dir = dynamic_pointer_cast<DirectionInterface>(attr->detail);
            if (dir) {
                return dir->getDirection();
            }
        }
    }
    return RtpDirection::invalid;
}

SdpItem::Ptr RtcSdpBase::getItem(char key_c, const char* attr_key) const {
    std::string key(1, key_c);
    for (auto item : items) {
        if (strcasecmp(item->getKey(), key.data()) == 0) {
            if (!attr_key) {
                return item;
            }
            auto attr = dynamic_pointer_cast<SdpAttr>(item);
            if (attr && !strcasecmp(attr->detail->getKey(), attr_key)) {
                return attr->detail;
            }
        }
    }
    return SdpItem::Ptr();
}

//////////////////////////////////////////////////////////////////////////
int RtcSessionSdp::getVersion() const {
    return atoi(getStringItem('v').data());
}

SdpOrigin RtcSessionSdp::getOrigin() const {
    return getItemClass<SdpOrigin>('o');
}

string RtcSessionSdp::getSessionName() const {
    return getStringItem('s');
}

string RtcSessionSdp::getSessionInfo() const {
    return getStringItem('i');
}

SdpTime RtcSessionSdp::getSessionTime() const {
    return getItemClass<SdpTime>('t');
}

SdpConnection RtcSessionSdp::getConnection() const {
    return getItemClass<SdpConnection>('c');
}

SdpBandwidth RtcSessionSdp::getBandwidth() const {
    return getItemClass<SdpBandwidth>('b');
}

string RtcSessionSdp::getUri() const {
    return getStringItem('u');
}

string RtcSessionSdp::getEmail() const {
    return getStringItem('e');
}

string RtcSessionSdp::getPhone() const {
    return getStringItem('p');
}

string RtcSessionSdp::getTimeZone() const {
    return getStringItem('z');
}

string RtcSessionSdp::getEncryptKey() const {
    return getStringItem('k');
}

string RtcSessionSdp::getRepeatTimes() const {
    return getStringItem('r');
}

//////////////////////////////////////////////////////////////////////

void RtcSessionSdp::parse(const string& str) {
    static auto flag = registerAllItem();
    RtcSdpBase* media = nullptr;
    auto lines = split(str, "\n");
    for (auto& line : lines) {
        trim(line, " \r\n\t");
        if (line.size() < 3 || line[1] != '=') {
            continue;
        }
        auto key = line.substr(0, 1);
        auto value = line.substr(2);
        if (!strcasecmp(key.data(), "m")) {
            medias.emplace_back(RtcSdpBase());
            media = &medias.back();
        }

        SdpItem::Ptr item;
        auto it = sdpItemCreator.find(key);
        if (it != sdpItemCreator.end()) {
            item = it->second(key, value);
        }
        else {
            item = std::make_shared<SdpCommon>(key);
            item->parse(value);
        }
        if (media) {
            media->addItem(std::move(item));
        }
        else {
            addItem(std::move(item));
        }
    }
}

string RtcSessionSdp::toString() const {
    std::stringstream printer;
    printer << RtcSdpBase::toString();
    for (auto& media : medias) {
        printer << media.toString();
    }

    return printer.str();
}

//////////////////////////////////////////////////////////////////////////////////////////
void SdpTime::parse(const string& str) {
    CHECK_SDP(sscanf(str.data(), "%" SCNu64 " %" SCNu64, &start, &stop) == 2);
}

string SdpTime::toString() const {
    if (value.empty()) {
        value = to_string(start) + " " + to_string(stop);
    }
    return SdpItem::toString();
}

void SdpOrigin::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() == 6);
    username = vec[0];
    session_id = vec[1];
    session_version = vec[2];
    nettype = vec[3];
    addrtype = vec[4];
    address = vec[5];
}

string SdpOrigin::toString() const {
    if (value.empty()) {
        value = username + " " + session_id + " " + session_version + " " + nettype + " " + addrtype + " " + address;
    }
    return SdpItem::toString();
}

void SdpConnection::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() == 3);
    nettype = vec[0];
    addrtype = vec[1];
    address = vec[2];
}

string SdpConnection::toString() const {
    if (value.empty()) {
        value = nettype + " " + addrtype + " " + address;
    }
    return SdpItem::toString();
}

void SdpBandwidth::parse(const string& str) {
    auto vec = split(str, ":");
    CHECK_SDP(vec.size() == 2);
    bwtype = vec[0];
    bandwidth = atoi(vec[1].data());
}

string SdpBandwidth::toString() const {
    if (value.empty()) {
        value = bwtype + ":" + to_string(bandwidth);
    }
    return SdpItem::toString();
}

void SdpMedia::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() >= 4);
    type = getTrackType(vec[0]);
    CHECK_SDP(type != TrackInvalid);
    port = atoi(vec[1].data());
    proto = vec[2];
    for (size_t i = 3; i < vec.size(); ++i) {
        fmts.emplace_back(vec[i]);
    }
}

string SdpMedia::toString() const {
    if (value.empty()) {
        value = string(getTrackString(type)) + " " + to_string(port) + " " + proto;
        for (auto fmt : fmts) {
            value += ' ';
            value += fmt;
        }
    }
    return SdpItem::toString();
}

void SdpAttr::parse(const string& str) {
    auto pos = str.find(':');
    auto key = pos == string::npos ? str : str.substr(0, pos);
    auto value = pos == string::npos ? string() : str.substr(pos + 1);
    auto it = sdpItemCreator.find(key);
    if (it != sdpItemCreator.end()) {
        detail = it->second(key, value);
    }
    else {
        detail = std::make_shared<SdpCommon>(key);
        detail->parse(value);
    }
}

string SdpAttr::toString() const {
    if (value.empty()) {
        auto detail_value = detail->toString();
        if (detail_value.empty()) {
            value = detail->getKey();
        }
        else {
            value = string(detail->getKey()) + ":" + detail_value;
        }
    }
    return SdpItem::toString();
}

void SdpAttrGroup::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() >= 2);
    type = vec[0];
    vec.erase(vec.begin());
    mids = std::move(vec);
}

string SdpAttrGroup::toString() const {
    if (value.empty()) {
        value = type;
        for (auto mid : mids) {
            value += ' ';
            value += mid;
        }
    }
    return SdpItem::toString();
}

void SdpAttrMsidSemantic::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() >= 1);
    msid = vec[0];
    token = vec.size() > 1 ? vec[1] : "";
}

string SdpAttrMsidSemantic::toString() const {
    if (value.empty()) {
        if (token.empty()) {
            value = string(" ") + msid;
        }
        else {
            value = string(" ") + msid + " " + token;
        }
    }
    return SdpItem::toString();
}

void SdpAttrRtcp::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() == 4);
    port = atoi(vec[0].data());
    nettype = vec[1];
    addrtype = vec[2];
    address = vec[3];
}

string SdpAttrRtcp::toString() const {
    if (value.empty()) {
        value = to_string(port) + " " + nettype + " " + addrtype + " " + address;
    }
    return SdpItem::toString();
}

void SdpAttrIceOption::parse(const string& str) {
    auto vec = split(str, " ");
    for (auto& v : vec) {
        if (!strcasecmp(v.data(), "trickle")) {
            trickle = true;
            continue;
        }
        if (!strcasecmp(v.data(), "renomination")) {
            renomination = true;
            continue;
        }
    }
}

string SdpAttrIceOption::toString() const {
    if (value.empty()) {
        if (trickle && renomination) {
            value = "trickle renomination";
        }
        else if (trickle) {
            value = "trickle";
        }
        else if (renomination) {
            value = "renomination";
        }
    }
    return value;
}

void SdpAttrFingerprint::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() == 2);
    algorithm = vec[0];
    hash = vec[1];
}

string SdpAttrFingerprint::toString() const {
    if (value.empty()) {
        value = algorithm + " " + hash;
    }
    return SdpItem::toString();
}

void SdpAttrSetup::parse(const string& str) {
    role = getDtlsRole(str);
    CHECK_SDP(role != DtlsRole::invalid);
}

string SdpAttrSetup::toString() const {
    if (value.empty()) {
        value = getDtlsRoleString(role);
    }
    return SdpItem::toString();
}

void SdpAttrExtmap::parse(const string& str) {
    char buf[128] = { 0 };
    char direction_buf[32] = { 0 };
    if (sscanf(str.data(), "%" SCNd8 "/%31[^ ] %127s", &id, direction_buf, buf) != 3) {
        CHECK_SDP(sscanf(str.data(), "%" SCNd8 " %127s", &id, buf) == 2);
        direction = RtpDirection::sendrecv;
    }
    else {
        direction = getRtpDirection(direction_buf);
    }
    ext = buf;
}

string SdpAttrExtmap::toString() const {
    if (value.empty()) {
        if (direction == RtpDirection::invalid || direction == RtpDirection::sendrecv) {
            value = to_string((int)id) + " " + ext;
        }
        else {
            value = to_string((int)id) + "/" + getRtpDirectionString(direction) + " " + ext;
        }
    }
    return SdpItem::toString();
}

void SdpAttrRtpMap::parse(const string& str) {
    char buf[32] = { 0 };
    if (sscanf(str.data(), "%" SCNu8 " %31[^/]/%" SCNd32 "/%" SCNd32, &pt, buf, &sample_rate, &channel) != 4) {
        CHECK_SDP(sscanf(str.data(), "%" SCNu8 " %31[^/]/%" SCNd32, &pt, buf, &sample_rate) == 3);
        if (getTrackType(getCodecId(buf)) == TrackAudio) {
            //δָ��ͨ����ʱ����Ϊ��Ƶʱ����ôͨ����Ĭ��Ϊ1
            channel = 1;
        }
    }
    codec = buf;
}

string SdpAttrRtpMap::toString() const {
    if (value.empty()) {
        value = to_string((int)pt) + " " + codec + "/" + to_string(sample_rate);
        if (channel) {
            value += '/';
            value += to_string(channel);
        }
    }
    return SdpItem::toString();
}

void SdpAttrRtcpFb::parse(const string& str_in) {
    auto str = str_in + "\n";
    char rtcp_type_buf[32] = { 0 };
    std::string::size_type pos;
    if ((pos = str.find("*")) != std::string::npos) {
        str = str.replace(pos, 1, "-1");
    }
    CHECK_SDP(sscanf(str.data(), "%" SCNi16 " %31[^\n]", &pt, rtcp_type_buf) == 2); //@Eric - SCNu8 => SCNo16
    rtcp_type = rtcp_type_buf;
}

string SdpAttrRtcpFb::toString() const {
    if (value.empty()) {
        if (pt == -1) {
            value = "* " + rtcp_type;
        }
        else {
            value = to_string((int)pt) + " " + rtcp_type;
        }
    }
    return SdpItem::toString();
}

void SdpAttrFmtp::parse(const string& str) {
    auto pos = str.find(' ');
    CHECK_SDP(pos != string::npos);
    pt = atoi(str.substr(0, pos).data());
    auto vec = split(str.substr(pos + 1), ";");
    for (auto& item : vec) {
        trim(item, " \r\n\t");
        auto pos = item.find('=');
        if (pos == string::npos) {
            fmtp.emplace(std::make_pair(item, ""));
        }
        else {
            fmtp.emplace(std::make_pair(item.substr(0, pos), item.substr(pos + 1)));
        }
    }
    CHECK_SDP(!fmtp.empty());
}

string SdpAttrFmtp::toString() const {
    if (value.empty()) {
        value = to_string((int)pt);
        int i = 0;
        for (auto& pr : fmtp) {
            value += (i++ ? ';' : ' ');
            value += pr.first + "=" + pr.second;
        }
    }
    return SdpItem::toString();
}

void SdpAttrSSRC::parse(const string& str_in) {
    auto str = str_in + '\n';
    char attr_buf[32] = { 0 };
    char attr_val_buf[128] = { 0 };
    if (3 == sscanf(str.data(), "%" SCNu32 " %31[^:]:%127[^\n]", &ssrc, attr_buf, attr_val_buf)) {
        attribute = attr_buf;
        attribute_value = attr_val_buf;
    }
    else if (2 == sscanf(str.data(), "%" SCNu32 " %31s[^\n]", &ssrc, attr_buf)) {
        attribute = attr_buf;
    }
    else {
        CHECK_SDP(0);
    }
}

string SdpAttrSSRC::toString() const {
    if (value.empty()) {
        value = to_string(ssrc) + ' ';
        value += attribute;
        if (!attribute_value.empty()) {
            value += ':';
            value += attribute_value;
        }
    }
    return SdpItem::toString();
}

void SdpAttrSSRCGroup::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() >= 3);
    type = std::move(vec[0]);
    CHECK(isFID() || isSIM());
    vec.erase(vec.begin());
    for (auto ssrc : vec) {
        ssrcs.emplace_back((uint32_t)atoll(ssrc.data()));
    }
}

string SdpAttrSSRCGroup::toString() const {
    if (value.empty()) {
        value = type;
        //����Ҫ��2��ssrc
        CHECK(ssrcs.size() >= 2);
        for (auto& ssrc : ssrcs) {
            value += ' ';
            value += to_string(ssrc);
        }
    }
    return SdpItem::toString();
}

void SdpAttrSctpMap::parse(const string& str) {
    char subtypes_buf[64] = { 0 };
    CHECK_SDP(3 == sscanf(str.data(), "%" SCNu16 " %63[^ ] %" SCNd32, &port, subtypes_buf, &streams));
    subtypes = subtypes_buf;
}

string SdpAttrSctpMap::toString() const {
    if (value.empty()) {
        value = to_string(port);
        value += ' ';
        value += subtypes;
        value += ' ';
        value += to_string(streams);
    }
    return SdpItem::toString();
}

void SdpAttrCandidate::parse(const string& str) {
    char foundation_buf[40] = { 0 };
    char transport_buf[16] = { 0 };
    char address_buf[64] = { 0 };
    char type_buf[16] = { 0 };

    // https://datatracker.ietf.org/doc/html/rfc5245#section-15.1
    CHECK_SDP(sscanf(str.data(), "%32[^ ] %" SCNu32 " %15[^ ] %" SCNu32 " %63[^ ] %" SCNu16 " typ %15[^ ]",
        foundation_buf, &component, transport_buf, &priority, address_buf, &port, type_buf) == 7);
    foundation = foundation_buf;
    transport = transport_buf;
    address = address_buf;
    type = type_buf;
    auto pos = str.find(type);
    if (pos != string::npos) {
        auto remain = str.substr(pos + type.size());
        trim(remain, " \r\n\t");
        if (!remain.empty()) {
            auto vec = split(remain, " ");
            string key;
            for (auto& item : vec) {
                if (key.empty()) {
                    key = item;
                }
                else {
                    arr.emplace_back(std::make_pair(std::move(key), std::move(item)));
                }
            }
        }
    }
}

string SdpAttrCandidate::toString() const {
    if (value.empty()) {
        value = foundation + " " + to_string(component) + " " + transport + " " + to_string(priority) +
            " " + address + " " + to_string(port) + " typ " + type;
        for (auto& pr : arr) {
            value += ' ';
            value += pr.first;
            value += ' ';
            value += pr.second;
        }
    }
    return SdpItem::toString();
}

void SdpAttrSimulcast::parse(const string& str) {
    //https://www.meetecho.com/blog/simulcast-janus-ssrc/
    //a=simulcast:send/recv q;h;f
    //a=simulcast:send/recv [rid=]q;h;f
    //a=simulcast: recv h;m;l
    //
    auto vec = split(str, " ");
    CHECK_SDP(vec.size() == 2);
    direction = vec[0];
    rids = split(vec[1], ";");
}

string SdpAttrSimulcast::toString() const {
    if (value.empty()) {
        value = direction + " ";
        bool first = true;
        for (auto& rid : rids) {
            if (first) {
                first = false;
            }
            else {
                value += ';';
            }
            value += rid;
        }
    }
    return SdpItem::toString();
}

void SdpAttrRid::parse(const string& str) {
    auto vec = split(str, " ");
    CHECK(vec.size() >= 2);
    rid = vec[0];
    direction = vec[1];
}

string SdpAttrRid::toString() const {
    if (value.empty()) {
        value = rid + " " + direction;
    }
    return SdpItem::toString();
}

void X2RtcSdp::loadFrom(const string& str) {
    RtcSessionSdp sdp;
    sdp.parse(str);

    version = sdp.getVersion();
    origin = sdp.getOrigin();
    session_name = sdp.getSessionName();
    session_info = sdp.getSessionInfo();
    connection = sdp.getConnection();
    time = sdp.getSessionTime();
    msid_semantic = sdp.getItemClass<SdpAttrMsidSemantic>('a', "msid-semantic");
    for (auto& media : sdp.medias) {
        auto mline = media.getItemClass<SdpMedia>('m');
        this->media.emplace_back();
        auto& rtc_media = this->media.back();
        rtc_media.mid = media.getStringItem('a', "mid");
        rtc_media.proto = mline.proto;
        rtc_media.type = mline.type;
        rtc_media.port = mline.port;
        rtc_media.addr = media.getItemClass<SdpConnection>('c');
        rtc_media.bandwidth = media.getItemClass<SdpBandwidth>('b');
        rtc_media.ice_ufrag = media.getStringItem('a', "ice-ufrag");
        rtc_media.ice_pwd = media.getStringItem('a', "ice-pwd");
        rtc_media.role = media.getItemClass<SdpAttrSetup>('a', "setup").role;
        rtc_media.fingerprint = media.getItemClass<SdpAttrFingerprint>('a', "fingerprint");
        if (rtc_media.fingerprint.empty()) {
            rtc_media.fingerprint = sdp.getItemClass<SdpAttrFingerprint>('a', "fingerprint");
        }
        rtc_media.ice_lite = media.getItem('a', "ice-lite").operator bool();
        auto ice_options = media.getItemClass<SdpAttrIceOption>('a', "ice-options");
        rtc_media.ice_trickle = ice_options.trickle;
        rtc_media.ice_renomination = ice_options.renomination;
        rtc_media.candidate = media.getAllItem<SdpAttrCandidate>('a', "candidate");

        if (mline.type == TrackType::TrackApplication) {
            rtc_media.sctp_port = atoi(media.getStringItem('a', "sctp-port").data());
            rtc_media.sctpmap = media.getItemClass<SdpAttrSctpMap>('a', "sctpmap");
            continue;
        }
        rtc_media.rtcp_addr = media.getItemClass<SdpAttrRtcp>('a', "rtcp");
        rtc_media.direction = media.getDirection();
        rtc_media.extmap = media.getAllItem<SdpAttrExtmap>('a', "extmap");
        rtc_media.rtcp_mux = media.getItem('a', "rtcp-mux").operator bool();
        rtc_media.rtcp_rsize = media.getItem('a', "rtcp-rsize").operator bool();

        map<uint32_t, RtcSSRC> rtc_ssrc_map;
        auto ssrc_attr = media.getAllItem<SdpAttrSSRC>('a', "ssrc");
        for (auto& ssrc : ssrc_attr) {
            auto& rtc_ssrc = rtc_ssrc_map[ssrc.ssrc];
            rtc_ssrc.ssrc = ssrc.ssrc;
            if (!strcasecmp(ssrc.attribute.data(), "cname")) {
                rtc_ssrc.cname = ssrc.attribute_value;
                continue;
            }
            if (!strcasecmp(ssrc.attribute.data(), "msid")) {
                rtc_ssrc.msid = ssrc.attribute_value;
                continue;
            }
            if (!strcasecmp(ssrc.attribute.data(), "mslabel")) {
                rtc_ssrc.mslabel = ssrc.attribute_value;
                continue;
            }
            if (!strcasecmp(ssrc.attribute.data(), "label")) {
                rtc_ssrc.label = ssrc.attribute_value;
                continue;
            }
        }

        auto ssrc_groups = media.getAllItem<SdpAttrSSRCGroup>('a', "ssrc-group");
        bool have_rtx_ssrc = false;
        SdpAttrSSRCGroup* ssrc_group_sim = nullptr;
        for (auto& group : ssrc_groups) {
            if (group.isFID()) {
                have_rtx_ssrc = true;
                //ssrc-group:FID�ֶα������rtp/rtx��ssrc
                CHECK(group.ssrcs.size() == 2);
                //����rtp ssrc�ҵ�����
                auto it = rtc_ssrc_map.find(group.ssrcs[0]);
                CHECK(it != rtc_ssrc_map.end());
                //����rtx ssrc
                it->second.rtx_ssrc = group.ssrcs[1];
                rtc_media.rtp_rtx_ssrc.emplace_back(it->second);
            }
            else if (group.isSIM()) {
                CHECK(!ssrc_group_sim);
                ssrc_group_sim = &group;
            }
        }

        if (!have_rtx_ssrc) {
            //����sdp˳���������ssrc
            for (auto& attr : ssrc_attr) {
                if (attr.attribute == "cname") {
                    rtc_media.rtp_rtx_ssrc.emplace_back(rtc_ssrc_map[attr.ssrc]);
                }
            }
        }

        auto simulcast = media.getItemClass<SdpAttrSimulcast>('a', "simulcast");
        if (!simulcast.empty()) {
            // a=rid:h send
            // a=rid:m send
            // a=rid:l send
            // a=simulcast:send h;m;l
            // ����simulcast
            unordered_set<string> rid_map;
            for (auto& rid : simulcast.rids) {
                rid_map.emplace(rid);
            }
            for (auto& rid : media.getAllItem<SdpAttrRid>('a', "rid")) {
                CHECK(rid.direction == simulcast.direction);
                CHECK(rid_map.find(rid.rid) != rid_map.end());
            }
            //simulcast����Ҫ��2�ַ���
            CHECK(simulcast.rids.size() >= 2);
            rtc_media.rtp_rids = simulcast.rids;
        }

        if (ssrc_group_sim) {
            //ָ����a=ssrc-group:SIM
            for (auto ssrc : ssrc_group_sim->ssrcs) {
                auto it = rtc_ssrc_map.find(ssrc);
                CHECK(it != rtc_ssrc_map.end());
                rtc_media.rtp_ssrc_sim.emplace_back(it->second);
            }
        }
        else if (!rtc_media.rtp_rids.empty()) {
            //δָ��a=ssrc-group:SIM, ����ָ����a=simulcast, ��ôֻ�ܸ���ssrc˳������Ӧrid˳��
            rtc_media.rtp_ssrc_sim = rtc_media.rtp_rtx_ssrc;
        }

        if (!rtc_media.supportSimulcast()) {
            //��֧��simulcast������£����һ��ssrc
            CHECK(rtc_media.rtp_rtx_ssrc.size() <= 1);
        }
        else {
            //simulcast������£�Ҫôû��ָ��ssrc��Ҫôָ����ssrc������rid����һ��
            //CHECK(rtc_media.rtp_ssrc_sim.empty() || rtc_media.rtp_ssrc_sim.size() == rtc_media.rtp_rids.size());
        }

        auto rtpmap_arr = media.getAllItem<SdpAttrRtpMap>('a', "rtpmap");
        auto rtcpfb_arr = media.getAllItem<SdpAttrRtcpFb>('a', "rtcp-fb");
        auto fmtp_aar = media.getAllItem<SdpAttrFmtp>('a', "fmtp");
        //�������pt����rtpmap,һ��pt����һ��
        map<uint8_t, SdpAttrRtpMap&> rtpmap_map;
        //�������pt����rtcp-fb,һ��pt�����ж�����0��
        multimap<uint8_t, SdpAttrRtcpFb&> rtcpfb_map;
        //�������pt����fmtp��һ��pt���һ��
        map<uint8_t, SdpAttrFmtp&> fmtp_map;

        for (auto& rtpmap : rtpmap_arr) {
            //���ʧ�ܣ��ж���
            CHECK(rtpmap_map.emplace(rtpmap.pt, rtpmap).second, "��pt���ڶ���a=rtpmap:", (int)rtpmap.pt);
        }
        for (auto& rtpfb : rtcpfb_arr) {
            rtcpfb_map.emplace(rtpfb.pt, rtpfb);
        }
        for (auto& fmtp : fmtp_aar) {
            //���ʧ�ܣ��ж���
            CHECK(fmtp_map.emplace(fmtp.pt, fmtp).second, "��pt���ڶ���a=fmtp:", (int)fmtp.pt);
        }
        for (auto& item : mline.fmts) {
            auto pt = atoi(item.c_str());
            CHECK(pt < 0xFF, "invalid payload type: ", item);
            //�������б��뷽����pt
            rtc_media.plan.emplace_back();
            auto& plan = rtc_media.plan.back();
            auto rtpmap_it = rtpmap_map.find(pt);
            if (rtpmap_it == rtpmap_map.end()) {
                plan.pt = pt;
                plan.codec = RtpPayload::getName(pt);
                plan.sample_rate = RtpPayload::getClockRate(pt);
                plan.channel = RtpPayload::getAudioChannel(pt);
            }
            else {
                plan.pt = rtpmap_it->second.pt;
                plan.codec = rtpmap_it->second.codec;
                plan.sample_rate = rtpmap_it->second.sample_rate;
                plan.channel = rtpmap_it->second.channel;
            }

            auto fmtp_it = fmtp_map.find(pt);
            if (fmtp_it != fmtp_map.end()) {
                plan.fmtp = fmtp_it->second.fmtp;
            }
            for (auto rtpfb_it = rtcpfb_map.find(pt);
                rtpfb_it != rtcpfb_map.end() && rtpfb_it->second.pt == pt; ++rtpfb_it) {
                plan.rtcp_fb.emplace(rtpfb_it->second.rtcp_type);
            }
        }
    }

    group = sdp.getItemClass<SdpAttrGroup>('a', "group");
}

void RtcSdpBase::toRtsp() {
    for (auto it = items.begin(); it != items.end();) {
        switch ((*it)->getKey()[0]) {
        case 'v':
        case 'o':
        case 's':
        case 'i':
        case 't':
        case 'c':
        case 'b': {
            ++it;
            break;
        }

        case 'm': {
            auto m = dynamic_pointer_cast<SdpMedia>(*it);
            CHECK(m);
            m->proto = "RTP/AVP";
            ++it;
            break;
        }
        case 'a': {
            auto attr = dynamic_pointer_cast<SdpAttr>(*it);
            CHECK(attr);
            if (!strcasecmp(attr->detail->getKey(), "rtpmap")
                || !strcasecmp(attr->detail->getKey(), "fmtp")) {
                ++it;
                break;
            }
        }
        default: {
            it = items.erase(it);
            break;
        }
        }
    }
}

string X2RtcSdp::toRtspSdp() const {
    X2RtcSdp copy = *this;
    copy.media.clear();
    for (auto& m : media) {
        switch (m.type) {
        case TrackAudio:
        case TrackVideo: {
            if (m.direction != RtpDirection::inactive) {
                copy.media.emplace_back(m);
                copy.media.back().plan.resize(1);
            }
            break;
        }
        default: continue;
        }
    }

    CHECK(!copy.media.empty());
    auto sdp = copy.toRtcSessionSdp();
    sdp->toRtsp();
    int i = 0;
    for (auto& m : sdp->medias) {
        m.toRtsp();
        m.addAttr(std::make_shared<SdpCommon>("control", string("trackID=") + to_string(i++)));
    }
    return sdp->toString();
}

void addSdpAttrSSRC(const RtcSSRC& rtp_ssrc, RtcSdpBase& media, uint32_t ssrc_num) {
    //assert(ssrc_num);
    SdpAttrSSRC ssrc;
    ssrc.ssrc = ssrc_num;

    ssrc.attribute = "cname";
    ssrc.attribute_value = rtp_ssrc.cname;
    media.addAttr(std::make_shared<SdpAttrSSRC>(ssrc));

    if (!rtp_ssrc.msid.empty()) {
        ssrc.attribute = "msid";
        ssrc.attribute_value = rtp_ssrc.msid;
        media.addAttr(std::make_shared<SdpAttrSSRC>(ssrc));
    }

    if (!rtp_ssrc.mslabel.empty()) {
        ssrc.attribute = "mslabel";
        ssrc.attribute_value = rtp_ssrc.mslabel;
        media.addAttr(std::make_shared<SdpAttrSSRC>(ssrc));
    }

    if (!rtp_ssrc.label.empty()) {
        ssrc.attribute = "label";
        ssrc.attribute_value = rtp_ssrc.label;
        media.addAttr(std::make_shared<SdpAttrSSRC>(ssrc));
    }
}

RtcSessionSdp::Ptr X2RtcSdp::toRtcSessionSdp() const {
    RtcSessionSdp::Ptr ret = std::make_shared<RtcSessionSdp>();
    auto& sdp = *ret;
    sdp.addItem(std::make_shared<SdpString<'v'> >(to_string(version)));
    sdp.addItem(std::make_shared<SdpOrigin>(origin));
    sdp.addItem(std::make_shared<SdpString<'s'> >(session_name));
    if (!session_info.empty()) {
        sdp.addItem(std::make_shared<SdpString<'i'> >(session_info));
    }
    sdp.addItem(std::make_shared<SdpTime>(time));
    if (connection.empty()) {
        sdp.addItem(std::make_shared<SdpConnection>(connection));
    }
    sdp.addAttr(std::make_shared<SdpAttrGroup>(group));
    sdp.addAttr(std::make_shared<SdpAttrMsidSemantic>(msid_semantic));
    if (isIceLite()) {
        sdp.addAttr(std::make_shared<SdpCommon>("ice-lite"));   //@Eric - ice-lite Ҫ����media�����棬��ΪMediaSoup��������Stun binding�����û��ice-lite webrtcҳ��һֱ��ice:Controlled��ݷ�������
    }
    for (auto& m : media) {
        sdp.medias.emplace_back();
        auto& sdp_media = sdp.medias.back();
        auto mline = std::make_shared<SdpMedia>();
        mline->type = m.type;
        mline->port = m.port;
        mline->proto = m.proto;
        for (auto& p : m.plan) {
            mline->fmts.emplace_back(to_string((int)p.pt));
        }
        if (m.type == TrackApplication) {
            mline->fmts.emplace_back("webrtc-datachannel");
        }
        sdp_media.addItem(std::move(mline));
        sdp_media.addItem(std::make_shared<SdpConnection>(m.addr));
        if (!m.bandwidth.empty() && m.type != TrackAudio) {
            sdp_media.addItem(std::make_shared<SdpBandwidth>(m.bandwidth));
        }
        if (!m.rtcp_addr.empty()) {
            sdp_media.addAttr(std::make_shared<SdpAttrRtcp>(m.rtcp_addr));
        }

        sdp_media.addAttr(std::make_shared<SdpAttrIceUfrag>(m.ice_ufrag));
        sdp_media.addAttr(std::make_shared<SdpAttrIcePwd>(m.ice_pwd));
        if (m.ice_trickle || m.ice_renomination) {
            auto attr = std::make_shared<SdpAttrIceOption>();
            attr->trickle = m.ice_trickle;
            attr->renomination = m.ice_renomination;
            sdp_media.addAttr(attr);
        }
        sdp_media.addAttr(std::make_shared<SdpAttrFingerprint>(m.fingerprint));
        sdp_media.addAttr(std::make_shared<SdpAttrSetup>(m.role));
        sdp_media.addAttr(std::make_shared<SdpAttrMid>(m.mid));
        if (m.ice_lite) {
            //sdp_media.addAttr(std::make_shared<SdpCommon>("ice-lite"));
        }
        for (auto& ext : m.extmap) {
            sdp_media.addAttr(std::make_shared<SdpAttrExtmap>(ext));
        }
        if (m.direction != RtpDirection::invalid) {
            sdp_media.addAttr(std::make_shared<DirectionInterfaceImp>(m.direction));
        }
        if (m.rtcp_mux) {
            sdp_media.addAttr(std::make_shared<SdpCommon>("rtcp-mux"));
        }
        if (m.rtcp_rsize) {
            sdp_media.addAttr(std::make_shared<SdpCommon>("rtcp-rsize"));
        }

        if (m.type != TrackApplication) {
            for (auto& p : m.plan) {
                auto rtp_map = std::make_shared<SdpAttrRtpMap>();
                rtp_map->pt = p.pt;
                rtp_map->codec = p.codec;
                rtp_map->sample_rate = p.sample_rate;
                rtp_map->channel = p.channel;
                //���a=rtpmap
                sdp_media.addAttr(std::move(rtp_map));

                for (auto& fb : p.rtcp_fb) {
                    auto rtcp_fb = std::make_shared<SdpAttrRtcpFb>();
                    rtcp_fb->pt = p.pt;
                    rtcp_fb->rtcp_type = fb;
                    //���a=rtcp-fb
                    sdp_media.addAttr(std::move(rtcp_fb));
                }

                if (!p.fmtp.empty()) {
                    auto fmtp = std::make_shared<SdpAttrFmtp>();
                    fmtp->pt = p.pt;
                    fmtp->fmtp = p.fmtp;
                    //���a=fmtp
                    sdp_media.addAttr(std::move(fmtp));
                }
            }

            {
                //���a=msid�ֶ�
                if (!m.rtp_rtx_ssrc.empty()) {
                    if (!m.rtp_rtx_ssrc[0].msid.empty()) {
                        auto msid = std::make_shared<SdpAttrMsid>();
                        msid->parse(m.rtp_rtx_ssrc[0].msid);
                        sdp_media.addAttr(std::move(msid));
                    }
                }
            }

            {
                for (auto& ssrc : m.rtp_rtx_ssrc) {
                    //���a=ssrc�ֶ�
                    CHECK(!ssrc.empty());
                    addSdpAttrSSRC(ssrc, sdp_media, ssrc.ssrc);
                    if (ssrc.rtx_ssrc) {
                        addSdpAttrSSRC(ssrc, sdp_media, ssrc.rtx_ssrc);

                        //����a=ssrc-group:FID�ֶ�
                        //��rtx ssrc
                        auto group = std::make_shared<SdpAttrSSRCGroup>();
                        group->type = "FID";
                        group->ssrcs.emplace_back(ssrc.ssrc);
                        group->ssrcs.emplace_back(ssrc.rtx_ssrc);
                        sdp_media.addAttr(std::move(group));
                    }
                }
            }

            {
                if (m.rtp_ssrc_sim.size() >= 2) {
                    //simulcast Ҫ�� 2~3·
                    auto group = std::make_shared<SdpAttrSSRCGroup>();
                    for (auto& ssrc : m.rtp_ssrc_sim) {
                        group->ssrcs.emplace_back(ssrc.ssrc);
                    }
                    //���a=ssrc-group:SIM�ֶ�
                    group->type = "SIM";
                    sdp_media.addAttr(std::move(group));
                }

                if (m.rtp_rids.size() >= 2) {
                    auto simulcast = std::make_shared<SdpAttrSimulcast>();
                    simulcast->direction = "recv";
                    simulcast->rids = m.rtp_rids;
                    sdp_media.addAttr(std::move(simulcast));

                    for (auto& rid : m.rtp_rids) {
                        auto attr_rid = std::make_shared<SdpAttrRid>();
                        attr_rid->rid = rid;
                        attr_rid->direction = "recv";
                        sdp_media.addAttr(std::move(attr_rid));
                    }
                }
            }

        }
        else {
            if (!m.sctpmap.empty()) {
                sdp_media.addAttr(std::make_shared<SdpAttrSctpMap>(m.sctpmap));
            }
            sdp_media.addAttr(std::make_shared<SdpCommon>("sctp-port", to_string(m.sctp_port)));
        }

        for (auto& cand : m.candidate) {
            if (cand.port) {
                sdp_media.addAttr(std::make_shared<SdpAttrCandidate>(cand));
            }
        }
    }
    return ret;
}

string X2RtcSdp::toString() const {
    return toRtcSessionSdp()->toString();
}

string RtcCodecPlan::getFmtp(const char* key) const {
    for (auto& item : fmtp) {
        if (strcasecmp(item.first.data(), key) == 0) {
            return item.second;
        }
    }
    return "";
}

const RtcCodecPlan* RtcMedia::getPlan(uint8_t pt) const {
    for (auto& item : plan) {
        if (item.pt == pt) {
            return &item;
        }
    }
    return nullptr;
}

const RtcCodecPlan* RtcMedia::getPlan(const char* codec) const {
    for (auto& item : plan) {
        if (strcasecmp(item.codec.data(), codec) == 0) {
            return &item;
        }
    }
    return nullptr;
}

const RtcCodecPlan* RtcMedia::getRelatedRtxPlan(uint8_t pt) const {
    for (auto& item : plan) {
        if (strcasecmp(item.codec.data(), "rtx") == 0) {
            auto apt = atoi(item.getFmtp("apt").data());
            if (pt == apt) {
                return &item;
            }
        }
    }
    return nullptr;
}

uint32_t RtcMedia::getRtpSSRC() const {
    if (rtp_rtx_ssrc.size()) {
        return rtp_rtx_ssrc[0].ssrc;
    }
    return 0;
}

uint32_t RtcMedia::getRtxSSRC() const {
    if (rtp_rtx_ssrc.size()) {
        return rtp_rtx_ssrc[0].rtx_ssrc;
    }
    return 0;
}

bool RtcMedia::supportSimulcast() const {
    if (!rtp_rids.empty()) {
        return true;
    }
    if (!rtp_ssrc_sim.empty()) {
        return true;
    }
    return false;
}

void RtcMedia::checkValid() const {
    CHECK(type != TrackInvalid);
    CHECK(!mid.empty());
    CHECK(!proto.empty());
    CHECK(direction != RtpDirection::invalid || type == TrackApplication);
    CHECK(!plan.empty() || type == TrackApplication);
    CHECK(type == TrackApplication || rtcp_mux, "ֻ֧��rtcp-muxģʽ");

    bool send_rtp = (direction == RtpDirection::sendonly || direction == RtpDirection::sendrecv);
    if (!supportSimulcast()) {
        //��simulcastʱ�������û��ָ��rtp ssrc
        CHECK(!rtp_rtx_ssrc.empty() || !send_rtp);
    }

#if 0
    //todo ����Firefox(88.0)��macƽ̨�£�����rtx��û��ָ��ssrc
    auto rtx_plan = getPlan("rtx");
    if (rtx_plan) {
        //����rtx�����ָ��rtx_ssrc
        CHECK(rtp_rtx_ssrc.size() >= 2 || !send_rtp);
    }
#endif
}

void X2RtcSdp::checkValid() const {
    CHECK(version == 0);
    CHECK(!origin.empty());
    CHECK(!session_name.empty());
    CHECK(!msid_semantic.empty());
    CHECK(!media.empty());
    CHECK(!group.mids.empty() && group.mids.size() <= media.size(), "ֻ֧��group BUNDLEģʽ");

    bool have_active_media = false;
    for (auto& item : media) {
        item.checkValid();

        if (TrackApplication == item.type) {
            have_active_media = true;
        }
        switch (item.direction) {
        case RtpDirection::sendrecv:
        case RtpDirection::sendonly:
        case RtpDirection::recvonly: have_active_media = true; break;
        default: break;
        }
    }
    CHECK(have_active_media, "����ȷ��������һ����Ծ��track");
}

const RtcMedia* X2RtcSdp::getMedia(TrackType type) const {
    for (auto& m : media) {
        if (m.type == type) {
            return &m;
        }
    }
    return nullptr;
}

bool X2RtcSdp::supportRtcpFb(const string& name, TrackType type) const {
    auto media = getMedia(type);
    if (!media) {
        return false;
    }
    auto& ref = media->plan[0].rtcp_fb;
    return ref.find(name) != ref.end();
}

bool X2RtcSdp::supportSimulcast() const {
    for (auto& m : media) {
        if (m.supportSimulcast()) {
            return true;
        }
    }
    return false;
}

bool X2RtcSdp::isOnlyDatachannel() const {
    return 1 == media.size() && TrackApplication == media[0].type;
}

bool X2RtcSdp::isIceLite() const
{
    for (auto& m : media) {
        if (m.ice_lite) {
            return true;
        }
    }
    return false;
}

string const SdpConst::kTWCCRtcpFb = "transport-cc";
string const SdpConst::kRembRtcpFb = "goog-remb";

void X2RtcConfigure::RtcTrackConfigure::enableTWCC(bool enable) {
    if (!enable) {
        rtcp_fb.erase(SdpConst::kTWCCRtcpFb);
        extmap.erase(RtpExtType::transport_cc);
    }
    else {
        rtcp_fb.emplace(SdpConst::kTWCCRtcpFb);
        extmap.emplace(RtpExtType::transport_cc, RtpDirection::sendrecv);
    }
}

void X2RtcConfigure::RtcTrackConfigure::enableREMB(bool enable) {
    if (!enable) {
        rtcp_fb.erase(SdpConst::kRembRtcpFb);
        extmap.erase(RtpExtType::abs_send_time);
    }
    else {
        rtcp_fb.emplace(SdpConst::kRembRtcpFb);
        extmap.emplace(RtpExtType::abs_send_time, RtpDirection::sendrecv);
    }
}

static vector<CodecId> toCodecArray(const string& str) {
    vector<CodecId> ret;
    auto vec = split(str, ",");
    for (auto& s : vec) {
        auto codec = getCodecId(trim(s, " \r\n\t"));
        if (codec != CodecInvalid) {
            ret.emplace_back(codec);
        }
    }
    return ret;
}

void X2RtcConfigure::RtcTrackConfigure::setDefaultSetting(TrackType type, const std::string& strPreferCodec) {
    rtcp_mux = true;
    rtcp_rsize = false;
    group_bundle = true;
    support_rtx = true;
    support_red = false;
    support_ulpfec = false;
    ice_lite = true;
    ice_trickle = true;
    ice_renomination = false;
    switch (type) {
    case TrackAudio: {
        //�˴�����ƫ�õı����ʽ���ȼ�
        preferred_codec = toCodecArray(strPreferCodec);

        rtcp_fb = { SdpConst::kTWCCRtcpFb, SdpConst::kRembRtcpFb };
        extmap = {
                {RtpExtType::ssrc_audio_level,            RtpDirection::sendrecv},
                {RtpExtType::csrc_audio_level,            RtpDirection::sendrecv},
                {RtpExtType::abs_send_time,               RtpDirection::sendrecv},
                {RtpExtType::transport_cc,                RtpDirection::sendrecv},
                //rtx�ش�rtpʱ������sdes_mid���͵�rtp ext,ʵ�ⷢ��Firefox�ڽ���rtxʱ���������sdes_mid��ext,�������޷�����
                //{RtpExtType::sdes_mid,RtpDirection::sendrecv},
                {RtpExtType::sdes_rtp_stream_id,          RtpDirection::sendrecv},
                {RtpExtType::sdes_repaired_rtp_stream_id, RtpDirection::sendrecv}
        };
        break;
    }
    case TrackVideo: {
        //�˴�����ƫ�õı����ʽ���ȼ�
        preferred_codec = toCodecArray(strPreferCodec);

        rtcp_fb = { SdpConst::kTWCCRtcpFb, SdpConst::kRembRtcpFb, "nack", "ccm fir", "nack pli" };
        extmap = {
                {RtpExtType::abs_send_time,               RtpDirection::sendrecv},
                {RtpExtType::transport_cc,                RtpDirection::sendrecv},
                //rtx�ش�rtpʱ������sdes_mid���͵�rtp ext,ʵ�ⷢ��Firefox�ڽ���rtxʱ���������sdes_mid��ext,�������޷�����
                //{RtpExtType::sdes_mid,RtpDirection::sendrecv},
                {RtpExtType::sdes_mid,RtpDirection::sendrecv},
                {RtpExtType::sdes_rtp_stream_id,          RtpDirection::sendrecv},
                {RtpExtType::sdes_repaired_rtp_stream_id, RtpDirection::sendrecv},
                //@Eric - ����abs_capture_time for B-Frame pts
                {RtpExtType::abs_capture_time,            RtpDirection::sendrecv },
                {RtpExtType::video_timing,                RtpDirection::sendrecv},
                {RtpExtType::color_space,                 RtpDirection::sendrecv},
                {RtpExtType::video_content_type,          RtpDirection::sendrecv},
                {RtpExtType::playout_delay,               RtpDirection::sendrecv},
                //�ֻ�����webrtc �������ת�Ƕȣ�rtcЭ������������ ����Э������������ת
                //{RtpExtType::video_orientation,           RtpDirection::sendrecv},
                {RtpExtType::toffset,                     RtpDirection::sendrecv},
                {RtpExtType::framemarking,                RtpDirection::sendrecv}
        };
        break;
    }
    case TrackApplication: {
        break;
    }
    default: break;
    }
}
void X2RtcConfigure::RtcTrackConfigure::clearAll()
{
    rtcp_mux = false;
    rtcp_rsize = false;
    group_bundle = false;
    support_rtx = false;
    support_red = false;
    support_ulpfec = false;
    ice_lite = false;
    ice_trickle = false;
    ice_renomination = false;

    rtcp_fb.clear();
    extmap.clear();
    preferred_codec.clear();
    candidate.clear();
}

void X2RtcConfigure::setDefaultSetting(string ice_ufrag, string ice_pwd, RtpDirection direction,
    const SdpAttrFingerprint& fingerprint) {
    if (str_video_codec_.length() > 0) {
        video.setDefaultSetting(TrackVideo, str_video_codec_);
    }
    else {
        video.setDefaultSetting(TrackVideo, "H264");
    }
    if (str_audio_codec_.length() > 0) {
        audio.setDefaultSetting(TrackAudio, str_audio_codec_);
    }
    else {
        audio.setDefaultSetting(TrackAudio, "opus");
    }
    
    application.setDefaultSetting(TrackApplication, "");

    video.ice_ufrag = audio.ice_ufrag = application.ice_ufrag = std::move(ice_ufrag);
    video.ice_pwd = audio.ice_pwd = application.ice_pwd = std::move(ice_pwd);
    video.direction = audio.direction = application.direction = direction;
    video.fingerprint = audio.fingerprint = application.fingerprint = fingerprint;

    if (gStrExternIp.length() > 0 && gExternPort > 0) {
        std::vector<std::string> extern_ips;
        if (gStrExternIp.length()) {
            extern_ips = split(gStrExternIp, ",");
        }
        const uint32_t delta = 10;
        uint32_t priority = 100 + delta * extern_ips.size();
        for (auto ip : extern_ips) {
            addCandidate(*makeIceCandidate(ip, gExternPort, priority, "udp"));
            priority -= delta;
        }
    }
}

void X2RtcConfigure::addCandidate(const SdpAttrCandidate& candidate, TrackType type) {
    switch (type) {
    case TrackAudio: {
        audio.candidate.emplace_back(candidate);
        break;
    }
    case TrackVideo: {
        video.candidate.emplace_back(candidate);
        break;
    }
    case TrackApplication: {
        application.candidate.emplace_back(candidate);
        break;
    }
    default: {
        if (audio.group_bundle) {
            audio.candidate.emplace_back(candidate);
        }
        if (video.group_bundle) {
            video.candidate.emplace_back(candidate);
        }
        if (application.group_bundle) {
            application.candidate.emplace_back(candidate);
        }
        break;
    }
    }
}

void X2RtcConfigure::enableTWCC(bool enable, TrackType type) {
    switch (type) {
    case TrackAudio: {
        audio.enableTWCC(enable);
        break;
    }
    case TrackVideo: {
        video.enableTWCC(enable);
        break;
    }
    default: {
        audio.enableTWCC(enable);
        video.enableTWCC(enable);
        break;
    }
    }
}

void X2RtcConfigure::enableREMB(bool enable, TrackType type) {
    switch (type) {
    case TrackAudio: {
        audio.enableREMB(enable);
        break;
    }
    case TrackVideo: {
        video.enableREMB(enable);
        break;
    }
    default: {
        audio.enableREMB(enable);
        video.enableREMB(enable);
        break;
    }
    }
}

void X2RtcConfigure::setAudioPreferCodec(const std::string& strCodec)
{
    str_audio_codec_ = strCodec;
}
void X2RtcConfigure::setVideoPreferCodec(const std::string& strCodec)
{
    if (strCodec.compare("H264") != 0 && strCodec.compare("H265") != 0) {
        return;
    }

    str_video_codec_ = strCodec;
}

void X2RtcConfigure::createOffer(const X2RtcSdp& offer) const
{
    for (auto& m : offer.media) {
        const RtcTrackConfigure* cfg_ptr = nullptr;

        switch (m.type) {
        case TrackAudio: cfg_ptr = &audio; break;
        case TrackVideo: cfg_ptr = &video; break;
        case TrackApplication: cfg_ptr = &application; break;
        default: return;
        }
        if (cfg_ptr != nullptr) {
            auto& configure = *cfg_ptr;

            RtcMedia* media = (RtcMedia*)&m;
            media->ice_ufrag = configure.ice_ufrag;
            media->ice_pwd = configure.ice_pwd;
            media->fingerprint = configure.fingerprint;
            media->ice_lite = configure.ice_lite;
            media->candidate = configure.candidate;
        }
    }
}
shared_ptr<X2RtcSdp> X2RtcConfigure::createAnswer(const X2RtcSdp& offer) const {
    shared_ptr<X2RtcSdp> ret = std::make_shared<X2RtcSdp>();
    ret->version = offer.version;
    ret->origin = offer.origin;
    ret->session_name = offer.session_name;
    ret->msid_semantic = offer.msid_semantic;

    for (auto& m : offer.media) {
        matchMedia(ret, m);
    }

    //��������Ƶ�˿ڸ���
    if (!offer.group.mids.empty()) {
        for (auto& m : ret->media) {
            ret->group.mids.emplace_back(m.mid);
        }
    }
    return ret;
}

static RtpDirection matchDirection(RtpDirection offer_direction, RtpDirection supported) {
    switch (offer_direction) {
    case RtpDirection::sendonly: {
        if (supported != RtpDirection::recvonly && supported != RtpDirection::sendrecv) {
            //���ǲ�֧�ֽ���
            return RtpDirection::inactive;
        }
        return RtpDirection::recvonly;
    }

    case RtpDirection::recvonly: {
        if (supported != RtpDirection::sendonly && supported != RtpDirection::sendrecv) {
            //���ǲ�֧�ַ���
            return RtpDirection::inactive;
        }
        return RtpDirection::sendonly;
    }

                               //�Է�֧�ַ��ͽ��գ���ô����������������������
    case RtpDirection::sendrecv: return  (supported == RtpDirection::invalid ? RtpDirection::inactive : supported);
    case RtpDirection::inactive: return RtpDirection::inactive;
    default: return RtpDirection::invalid;
    }
}

static DtlsRole mathDtlsRole(DtlsRole role) {
    switch (role) {
    case DtlsRole::actpass:
    case DtlsRole::active: return DtlsRole::passive;
    case DtlsRole::passive: return DtlsRole::active;
    default: CHECK(0, "invalid role:", getDtlsRoleString(role)); return DtlsRole::passive;
    }
}

#define XX(type, url) {url, RtpExtType::type},
static unordered_map<string/*ext*/, RtpExtType/*id*/> s_url_to_type = { RTP_EXT_MAP(XX) };
#undef XX

RtpExtType getExtType(const string& url) {
    auto it = s_url_to_type.find(url);
    if (it == s_url_to_type.end()) {
        throw std::invalid_argument(string("δʶ���rtp ext url����:") + url);
    }
    return it->second;
}

void X2RtcConfigure::matchMedia(const std::shared_ptr<X2RtcSdp>& ret, const RtcMedia& offer_media) const {
    bool check_profile = true;
    bool check_codec = true;
    const RtcTrackConfigure* cfg_ptr = nullptr;

    switch (offer_media.type) {
    case TrackAudio: cfg_ptr = &audio; break;
    case TrackVideo: cfg_ptr = &video; break;
    case TrackApplication: cfg_ptr = &application; break;
    default: return;
    }
    auto& configure = *cfg_ptr;

RETRY:

    if (offer_media.type == TrackApplication) {
        RtcMedia answer_media = offer_media;
        answer_media.role = mathDtlsRole(offer_media.role);
#ifdef ENABLE_SCTP
        answer_media.direction = matchDirection(offer_media.direction, configure.direction);
        answer_media.candidate = configure.candidate;
        answer_media.ice_ufrag = configure.ice_ufrag;
        answer_media.ice_pwd = configure.ice_pwd;
        answer_media.fingerprint = configure.fingerprint;
        answer_media.ice_lite = configure.ice_lite;
#else
        answer_media.direction = RtpDirection::inactive;
#endif
        ret->media.emplace_back(answer_media);
        return;
    }
    for (auto& codec : configure.preferred_codec) {
        if (offer_media.ice_lite && configure.ice_lite) {
            //WarnL << "answer sdp����Ϊice_liteģʽ����offer sdp�е�ice_liteģʽ��ͻ";
            continue;
        }
        const RtcCodecPlan* selected_plan = nullptr;
        for (auto& plan : offer_media.plan) {
            //�ȼ������ʽ�Ƿ�Ϊƫ��
            if (check_codec && getCodecId(plan.codec) != codec) {
                continue;
            }
            //����ƫ�õı����ʽ,Ȼ������
            if (check_profile && !onCheckCodecProfile(plan, codec)) {
                continue;
            }
            //�ҵ������codec
            selected_plan = &plan;
            break;
        }
        if (!selected_plan) {
            //offer�и�ý������е�codec����֧��
            continue;
        }
        RtcMedia answer_media;
        answer_media.type = offer_media.type;
        answer_media.mid = offer_media.mid;
        answer_media.proto = offer_media.proto;
        answer_media.port = offer_media.port;
        answer_media.addr = offer_media.addr;
        answer_media.bandwidth = offer_media.bandwidth;
        answer_media.rtcp_addr = offer_media.rtcp_addr;
        answer_media.rtcp_mux = offer_media.rtcp_mux && configure.rtcp_mux;
        answer_media.rtcp_rsize = offer_media.rtcp_rsize && configure.rtcp_rsize;
        answer_media.ice_trickle = offer_media.ice_trickle && configure.ice_trickle;
        answer_media.ice_renomination = offer_media.ice_renomination && configure.ice_renomination;
        answer_media.ice_ufrag = configure.ice_ufrag;
        answer_media.ice_pwd = configure.ice_pwd;
        answer_media.fingerprint = configure.fingerprint;
        answer_media.ice_lite = configure.ice_lite;
        answer_media.candidate = configure.candidate;
        // copy simulicast setting
        answer_media.rtp_rids = offer_media.rtp_rids;
        answer_media.rtp_ssrc_sim = offer_media.rtp_ssrc_sim;

        answer_media.role = mathDtlsRole(offer_media.role);

        //���codecƥ��ʧ�ܣ���ô���ø�track
        answer_media.direction = check_codec ? matchDirection(offer_media.direction, configure.direction)
            : RtpDirection::inactive;
        if (answer_media.direction == RtpDirection::invalid) {
            continue;
        }
        if (answer_media.direction == RtpDirection::sendrecv) {
            //������շ�˫����ô���ǿ���offer sdp��ssrc��ȷ��ssrcһ��
            answer_media.rtp_rtx_ssrc = offer_media.rtp_rtx_ssrc;
        }

        //���ý��plan
        answer_media.plan.emplace_back(*selected_plan);
        onSelectPlan(answer_media.plan.back(), codec);

        set<uint8_t> pt_selected = { selected_plan->pt };

        //���rtx,red,ulpfec plan
        if (configure.support_red || configure.support_rtx || configure.support_ulpfec) {
            for (auto& plan : offer_media.plan) {
                if (!strcasecmp(plan.codec.data(), "rtx")) {
                    if (configure.support_rtx && atoi(plan.getFmtp("apt").data()) == selected_plan->pt) {
                        answer_media.plan.emplace_back(plan);
                        pt_selected.emplace(plan.pt);
                    }
                    continue;
                }
                if (!strcasecmp(plan.codec.data(), "red")) {
                    if (configure.support_red) {
                        answer_media.plan.emplace_back(plan);
                        pt_selected.emplace(plan.pt);
                    }
                    continue;
                }
                if (!strcasecmp(plan.codec.data(), "ulpfec")) {
                    if (configure.support_ulpfec) {
                        answer_media.plan.emplace_back(plan);
                        pt_selected.emplace(plan.pt);
                    }
                    continue;
                }
            }
        }

        //�Է����ҷ���֧�ֵ���չ����ô���ǲ�֧��
        for (auto& ext : offer_media.extmap) {
            auto it = configure.extmap.find(getExtType(ext.ext));
            if (it != configure.extmap.end()) {
                auto new_dir = matchDirection(ext.direction, it->second);
                switch (new_dir) {
                case RtpDirection::invalid:
                case RtpDirection::inactive: continue;
                default: break;
                }
                answer_media.extmap.emplace_back(ext);
                answer_media.extmap.back().direction = new_dir;
            }
        }

        auto& rtcp_fb_ref = answer_media.plan[0].rtcp_fb;
        rtcp_fb_ref.clear();
        //�Է����ҷ���֧�ֵ�rtcpfb����ô���ǲ�֧��
        for (auto& fp : selected_plan->rtcp_fb) {
            if (configure.rtcp_fb.find(fp) != configure.rtcp_fb.end()) {
                //�Է���rtcp������֧��
                rtcp_fb_ref.emplace(fp);
            }
        }

#if 0
        //todo �˴�Ϊ�����Ч��plan��webrtc sdpͨ������plan pt˳��ѡ��ƥ���codec����ζ�ź����codec��ʵ����sdp�����������
        for (auto& plan : offer_media.plan) {
            if (pt_selected.find(plan.pt) == pt_selected.end()) {
                answer_media.plan.emplace_back(plan);
            }
        }
#endif
        ret->media.emplace_back(answer_media);
        return;
    }

    if (check_profile) {
        //��������ڼ��profile����ƥ��ʧ�ܣ���ô����һ�Σ��Ҳ����profile
        check_profile = false;
        goto RETRY;
    }

    if (check_codec) {
        //��������ڼ��codec����ƥ��ʧ�ܣ���ô����һ�Σ��Ҳ����codec
        check_codec = false;
        goto RETRY;
    }
}

void X2RtcConfigure::setPlayRtspInfo(const string& sdp) {
    X2RtcSdp session;
    video.direction = RtpDirection::inactive;
    audio.direction = RtpDirection::inactive;

    session.loadFrom(sdp);
    for (auto& m : session.media) {
        switch (m.type) {
        case TrackVideo: {
            video.direction = RtpDirection::sendonly;
            _rtsp_video_plan = std::make_shared<RtcCodecPlan>(m.plan[0]);
            video.preferred_codec.clear();
            video.preferred_codec.emplace_back(getCodecId(_rtsp_video_plan->codec));
            break;
        }
        case TrackAudio: {
            audio.direction = RtpDirection::sendonly;
            _rtsp_audio_plan = std::make_shared<RtcCodecPlan>(m.plan[0]);
            audio.preferred_codec.clear();
            audio.preferred_codec.emplace_back(getCodecId(_rtsp_audio_plan->codec));
            break;
        }
        default: break;
        }
    }
}

static const string kProfile{ "profile-level-id" };
static const string kMode{ "packetization-mode" };

bool X2RtcConfigure::onCheckCodecProfile(const RtcCodecPlan& plan, CodecId codec) const {
    if (_rtsp_audio_plan && codec == getCodecId(_rtsp_audio_plan->codec)) {
        if (plan.sample_rate != _rtsp_audio_plan->sample_rate || plan.channel != _rtsp_audio_plan->channel) {
            //��Ƶ�����ʺ�ͨ����������ͬ
            return false;
        }
        return true;
    }
    else {//@Eric - support aac/44100/2 or support aac/48000/2
        if (codec == CodecAAC_A) {
            if (plan.sample_rate == 48000 && plan.channel == 2) {
                return true;
            }
            return false;
        }
    }
    if (_rtsp_video_plan && codec == CodecH264 && getCodecId(_rtsp_video_plan->codec) == CodecH264) {
        //h264ʱ��profile-level-id
        if (strcasecmp(_rtsp_video_plan->fmtp[kProfile].data(), const_cast<RtcCodecPlan&>(plan).fmtp[kProfile].data())) {
            //profile-level-id ��ƥ��
            return false;
        }
        return true;
    }

    return true;
}

void X2RtcConfigure::onSelectPlan(RtcCodecPlan& plan, CodecId codec) const {
    if (_rtsp_video_plan && codec == CodecH264 && getCodecId(_rtsp_video_plan->codec) == CodecH264) {
        //h264ʱ������packetization-modΪһ��
        auto mode = _rtsp_video_plan->fmtp[kMode];
        plan.fmtp[kMode] = mode.empty() ? "0" : mode;
    }
}

bool canSendRtp(X2RtcSdp::Ptr sdp) {
    for (auto& m : sdp->media) {
        if (m.direction == RtpDirection::sendrecv || m.direction == RtpDirection::sendonly) {
            return true;
        }
    }
    return false;
}

void SdpCheckAnswer(X2RtcSdp::Ptr sdp)
{
    if (gStrExternIp.length() > 0 && gExternPort > 0) {
        // �޸�answer sdp��ip���˿���Ϣ
        std::vector<std::string> extern_ips;
        if (gStrExternIp.length()) {
            extern_ips = split(gStrExternIp, ",");
        }
        for (auto& m : sdp->media) {
            m.addr.reset();
            m.addr.address =  extern_ips[0];
            m.rtcp_addr.reset();
            m.rtcp_addr.address = m.addr.address;

            m.rtcp_addr.port = gExternPort;
            m.port = m.rtcp_addr.port;
            sdp->origin.address = m.addr.address;
        }
    }
   
    if (!canSendRtp(sdp)) {
        // �������Ƿ��͵�rtp��ssrc
        return;
    }

    for (auto& m : sdp->media) {
        if (m.type == TrackApplication) {
            continue;
        }
        if (!m.rtp_rtx_ssrc.empty()) {
            // �Ѿ�������ssrc
            continue;
        }
        // ���answer sdp��ssrc��Ϣ
        m.rtp_rtx_ssrc.emplace_back();
        auto& ssrc = m.rtp_rtx_ssrc.back();
        // ���͵�ssrc������㶨�壬��Ϊ�ڷ���rtpʱ���޸�Ϊ��ֵ
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


}	// namespace x2rtc