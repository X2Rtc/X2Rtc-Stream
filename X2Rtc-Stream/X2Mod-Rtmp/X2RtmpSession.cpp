#include "X2RtmpSession.h"
#include "Common/config.h"
#include "Common/Parser.h"
#include "Util/onceToken.h"
#include <memory>

using namespace std;
using namespace toolkit;

namespace mediakit {
    class X2RtmpRingDelegateHelper : public toolkit::RingDelegate<RtmpPacket::Ptr> {
    public:
        using onRtmp = std::function<void(RtmpPacket::Ptr in, bool is_key)>;

        ~X2RtmpRingDelegateHelper() override {}

        X2RtmpRingDelegateHelper(onRtmp on_rtp) {
            _on_rtmp = std::move(on_rtp);
        }

        void onWrite(RtmpPacket::Ptr in, bool is_key) override {
            _on_rtmp(std::move(in), is_key);
        }

    private:
        onRtmp _on_rtmp;
    };

    /////////////////////////////////////MediaInfo//////////////////////////////////////

    void MediaInfo::parse(const std::string& url_in) {
        full_url = url_in;
        auto url = url_in;
        auto pos = url.find("?");
        if (pos != string::npos) {
            param_strs = url.substr(pos + 1);
            url.erase(pos);
        }

        auto schema_pos = url.find("://");
        if (schema_pos != string::npos) {
            schema = url.substr(0, schema_pos);
        }
        else {
            schema_pos = -3;
        }
        auto split_vec = split(url.substr(schema_pos + 3), "/");
        if (split_vec.size() > 0) {
            splitUrl(split_vec[0], host, port);
            vhost = host;
            if (vhost == "localhost" || isIP(vhost.data())) {
                //如果访问的是localhost或ip，那么则为默认虚拟主机
                vhost = DEFAULT_VHOST;
            }
        }
        if (split_vec.size() > 1) {
            app = split_vec[1];
        }
        if (split_vec.size() > 2) {
            string stream_id;
            for (size_t i = 2; i < split_vec.size(); ++i) {
                stream_id.append(split_vec[i] + "/");
            }
            if (stream_id.back() == '/') {
                stream_id.pop_back();
            }
            stream = stream_id;
        }

        auto params = Parser::parseArgs(param_strs);
        if (params.find(VHOST_KEY) != params.end()) {
            vhost = params[VHOST_KEY];
        }

        GET_CONFIG(bool, enableVhost, General::kEnableVhost);
        if (!enableVhost || vhost.empty()) {
            //如果关闭虚拟主机或者虚拟主机为空，则设置虚拟主机为默认
            vhost = DEFAULT_VHOST;
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ProtocolOption::ProtocolOption() {
        GET_CONFIG(bool, s_modify_stamp, Protocol::kModifyStamp);
        GET_CONFIG(bool, s_enabel_audio, Protocol::kEnableAudio);
        GET_CONFIG(bool, s_add_mute_audio, Protocol::kAddMuteAudio);
        GET_CONFIG(uint32_t, s_continue_push_ms, Protocol::kContinuePushMS);

        GET_CONFIG(bool, s_enable_hls, Protocol::kEnableHls);
        GET_CONFIG(bool, s_enable_mp4, Protocol::kEnableMP4);
        GET_CONFIG(bool, s_enable_rtsp, Protocol::kEnableRtsp);
        GET_CONFIG(bool, s_enable_rtmp, Protocol::kEnableRtmp);
        GET_CONFIG(bool, s_enable_ts, Protocol::kEnableTS);
        GET_CONFIG(bool, s_enable_fmp4, Protocol::kEnableFMP4);

        GET_CONFIG(bool, s_hls_demand, Protocol::kHlsDemand);
        GET_CONFIG(bool, s_rtsp_demand, Protocol::kRtspDemand);
        GET_CONFIG(bool, s_rtmp_demand, Protocol::kRtmpDemand);
        GET_CONFIG(bool, s_ts_demand, Protocol::kTSDemand);
        GET_CONFIG(bool, s_fmp4_demand, Protocol::kFMP4Demand);

        GET_CONFIG(bool, s_mp4_as_player, Protocol::kMP4AsPlayer);
        GET_CONFIG(uint32_t, s_mp4_max_second, Protocol::kMP4MaxSecond);
        GET_CONFIG(string, s_mp4_save_path, Protocol::kMP4SavePath);

        GET_CONFIG(string, s_hls_save_path, Protocol::kHlsSavePath);

        modify_stamp = s_modify_stamp;
        enable_audio = s_enabel_audio;
        add_mute_audio = s_add_mute_audio;
        continue_push_ms = s_continue_push_ms;

        enable_hls = s_enable_hls;
        enable_mp4 = s_enable_mp4;
        enable_rtsp = s_enable_rtsp;
        enable_rtmp = s_enable_rtmp;
        enable_ts = s_enable_ts;
        enable_fmp4 = s_enable_fmp4;

        hls_demand = s_hls_demand;
        rtsp_demand = s_rtsp_demand;
        rtmp_demand = s_rtmp_demand;
        ts_demand = s_ts_demand;
        fmp4_demand = s_fmp4_demand;

        mp4_as_player = s_mp4_as_player;
        mp4_max_second = s_mp4_max_second;
        mp4_save_path = s_mp4_save_path;

        hls_save_path = s_hls_save_path;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    X2RtmpSession::X2RtmpSession(): cb_listener_(NULL){
        _push_src = false;
    }

    X2RtmpSession::~X2RtmpSession() {

    }

    void X2RtmpSession::DoTick() {
        GET_CONFIG(uint32_t, handshake_sec, Rtmp::kHandshakeSecond);
        GET_CONFIG(uint32_t, keep_alive_sec, Rtmp::kKeepAliveSecond);

        if (_ticker.createdTime() > handshake_sec * 1000) {
            if (!_push_src) {//!_ring_reader && 
                //shutdown(SockException(Err_timeout, "illegal connection"));
                SockException(Err_timeout, "illegal connection");
            }
        }
        if (_push_src) {
            // push
            if (_ticker.elapsedTime() > keep_alive_sec * 1000) {
                //shutdown(SockException(Err_timeout, "recv data from rtmp pusher timeout"));
                SockException(Err_timeout, "recv data from rtmp pusher timeout");
            }
        }
    }

    void X2RtmpSession::Close()
    {
        cb_listener_ = NULL;
        aac_rtmp_enc_ = NULL;
        h264_rtmp_enc_ = NULL;
        h265_rtmp_enc_ = NULL;
    }

    void X2RtmpSession::SendData(int codec, const void* data, size_t bytes, uint32_t timestamp)
    {
        
    }
    void X2RtmpSession::SendAacAudio(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
    {
        if (aac_rtmp_enc_ == NULL) {
            aac_rtmp_enc_ = std::make_shared< AACRtmpEncoder>(nullptr);
            aac_rtmp_enc_->setRtmpRing(_rtmp_ring);
        }
        
        if (aac_rtmp_enc_ != NULL) {
            auto frame = std::make_shared<FrameFromPtr>(CodecAAC, (char*)data, bytes, dts, pts, ADTS_HEADER_LEN);
            aac_rtmp_enc_->inputFrame(frame);
        }
    }
    void X2RtmpSession::SendH264Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
    {
        if (h264_rtmp_enc_ == NULL) {
            h264_rtmp_enc_ = std::make_shared< H264RtmpEncoder>(nullptr);
            h264_rtmp_enc_->setRtmpRing(_rtmp_ring);
        }

        if (h264_rtmp_enc_ != NULL) {
            splitH264((char*)data, bytes, prefixSize((char*)data, bytes), [&](const char* ptr, size_t len, size_t prefix) {
                auto frame = FrameImp::create<H264Frame>();
                frame->_dts = dts;
                frame->_pts = pts;
                frame->_buffer.assign((char*)ptr, len);
                frame->_prefix_size = prefix;
                h264_rtmp_enc_->inputFrame(frame);
            }); 
        }
    }
    void X2RtmpSession::SendH265Video(const void* data, size_t bytes, uint32_t dts, uint32_t pts)
    {
        if (h265_rtmp_enc_ == NULL) {
            h265_rtmp_enc_ = std::make_shared< H265RtmpEncoder>(nullptr);
            h265_rtmp_enc_->setRtmpRing(_rtmp_ring);
        }

        if (h265_rtmp_enc_ != NULL) {
            splitH264((char*)data, bytes, prefixSize((char*)data, bytes), [&](const char* ptr, size_t len, size_t prefix) {
                auto frame = FrameImp::create<H265Frame>();
                frame->_dts = dts;
                frame->_pts = pts;
                frame->_buffer.assign((char*)ptr, len);
                frame->_prefix_size = prefix;
                h265_rtmp_enc_->inputFrame(frame);
                });
        }
    }
    void X2RtmpSession::RecvData(const char* pData, int nLen) {
        _ticker.resetTime();
        _total_bytes += nLen;
        onParseRtmp(pData, nLen);
    }

    void X2RtmpSession::onCmd_connect(AMFDecoder& dec) {
        auto params = dec.load<AMFValue>();
        ///////////set chunk size////////////////
        sendChunkSize(60000);
        ////////////window Acknowledgement size/////
        sendAcknowledgementSize(5000000);
        ///////////set peerBandwidth////////////////
        sendPeerBandwidth(5000000);

        auto tc_url = params["tcUrl"].as_string();
        if (tc_url.empty()) {
            // defaultVhost:默认vhost
            tc_url = string(RTMP_SCHEMA) + "://" + DEFAULT_VHOST + "/" + _media_info.app;
        }
        else {
            auto pos = tc_url.rfind('?');
            if (pos != string::npos) {
                // tc_url 中可能包含?以及参数，参见issue: #692
                tc_url = tc_url.substr(0, pos);
            }
        }
        // 初步解析，只用于获取vhost信息
        _media_info.parse(tc_url);
        _media_info.schema = RTMP_SCHEMA;
        // 赋值rtmp app
        _media_info.app = params["app"].as_string();

        bool ok = true; //(app == APP_NAME);
        AMFValue version(AMF_OBJECT);
        version.set("fmsVer", "FMS/3,0,1,123");
        version.set("capabilities", 31.0);
        AMFValue status(AMF_OBJECT);
        status.set("level", ok ? "status" : "error");
        status.set("code", ok ? "NetConnection.Connect.Success" : "NetConnection.Connect.InvalidApp");
        status.set("description", ok ? "Connection succeeded." : "InvalidApp.");
        status.set("objectEncoding", params["objectEncoding"]);
        sendReply(ok ? "_result" : "_error", version, status);
        if (!ok) {
            throw std::runtime_error("Unsupported application: " + _media_info.app);
        }

        AMFEncoder invoke;
        invoke << "onBWDone" << 0.0 << nullptr;
        sendResponse(MSG_CMD, invoke.data());
    }

    void X2RtmpSession::onCmd_createStream(AMFDecoder& dec) {
        sendReply("_result", nullptr, double(STREAM_MEDIA));
    }

    void X2RtmpSession::onCmd_publish(AMFDecoder& dec) {
        std::shared_ptr<Ticker> ticker(new Ticker);
        weak_ptr<X2RtmpSession> weak_self = dynamic_pointer_cast<X2RtmpSession>(shared_from_this());
        std::shared_ptr<onceToken> token(new onceToken(nullptr, [ticker, weak_self]() {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                //DebugP(strong_self.get()) << "publish 回复时间:" << ticker->elapsedTime() << "ms";
            }
            }));
        dec.load<AMFValue>();/* NULL */
        // 赋值为rtmp stream id 信息
        _media_info.stream = getStreamId(dec.load<std::string>());
        // 再解析url，切割url为app/stream_id (不一定符合rtmp url切割规范)
        _media_info.parse(_media_info.getUrl());

        auto now_stream_index = _now_stream_index;
        auto on_res = [this, token, now_stream_index](const string& err, const ProtocolOption& option) {
            _now_stream_index = now_stream_index;
            if (!err.empty()) {
                sendStatus({ "level", "error",
                             "code", "NetStream.Publish.BadAuth",
                             "description", err,
                             "clientid", "0" });
                //shutdown(SockException(Err_shutdown, StrPrinter << "Unauthorized:" << err));
                SockException(Err_shutdown, StrPrinter << "Unauthorized:" << err);
                return;
            }

            assert(!_push_src);
            auto src = false;// MediaSource::find(RTMP_SCHEMA, _media_info.vhost, _media_info.app, _media_info.stream);
            auto push_failed = (bool)src;

            _continue_push_ms = option.continue_push_ms;
            sendStatus({ "level", "status",
                        "code", "NetStream.Publish.Start",
                        "description", "Started publishing stream.",
                        "clientid", "0" });

            setSocketFlags();
        };

        if (_media_info.app.empty() || _media_info.stream.empty()) {
            //不允许莫名其妙的推流url
            on_res("rtmp推流url非法", ProtocolOption());
            return;
        }

        auto flag = false;// NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastMediaPublish, MediaOriginType::rtmp_push, _media_info, invoker, static_cast<SockInfo&>(*this));
        if (!flag) {
            //该事件无人监听，默认鉴权成功
            on_res("", ProtocolOption());
        }

        _push_src = true;
        const char* stream = strstr(_media_info.full_url.c_str(), "/");
        if (stream != NULL) {
            stream += 1;
        }
        while (1) {
            const char*strFind = strstr(stream, "/");
            if (strFind != NULL) {
                strFind += 1;
                stream = strFind;
            }
            else {
                break;
            }
        }
        if (cb_listener_ != NULL) {
            cb_listener_->OnPublish(_media_info.app.c_str(), stream, "");
        }
    }

    void X2RtmpSession::onCmd_deleteStream(AMFDecoder& dec) {
        _push_src = false;
        //此时回复可能触发broken pipe事件，从而直接触发onError回调；所以需要先把_push_src置空，防止触发断流续推功能
        sendStatus({ "level", "status",
                     "code", "NetStream.Unpublish.Success",
                     "description", "Stop publishing." });
        if (cb_listener_ != NULL) {
            cb_listener_->OnClose();
        }

        throw std::runtime_error(StrPrinter << "Stop publishing" << endl);
    }

    void X2RtmpSession::sendStatus(const std::initializer_list<string>& key_value) {
        AMFValue status(AMF_OBJECT);
        int i = 0;
        string key;
        for (auto& val : key_value) {
            if (++i % 2 == 0) {
                status.set(key, val);
            }
            else {
                key = val;
            }
        }
        sendReply("onStatus", nullptr, status);
    }

    void X2RtmpSession::sendPlayResponse(const string& err, const RtmpMediaSource::Ptr& src) {
        bool auth_success = err.empty();
        bool ok = (auth_success);   //src.operator bool() && 
        if (ok) {
            //stream begin
            sendUserControl(CONTROL_STREAM_BEGIN, STREAM_MEDIA);
        }
        // onStatus(NetStream.Play.Reset)
        sendStatus({ "level", (ok ? "status" : "error"),
                     "code", (ok ? "NetStream.Play.Reset" : (auth_success ? "NetStream.Play.StreamNotFound" : "NetStream.Play.BadAuth")),
                     "description", (ok ? "Resetting and playing." : (auth_success ? "No such stream." : err.data())),
                     "details", _media_info.stream,
                     "clientid", "0" });

        if (!ok) {
            string err_msg = StrPrinter << (auth_success ? "no such stream:" : err.data()) << " " << _media_info.shortUrl();
            //shutdown(SockException(Err_shutdown, err_msg));
            SockException(Err_shutdown, err_msg);
            return;
        }

        // onStatus(NetStream.Play.Start)

        sendStatus({ "level", "status",
                     "code", "NetStream.Play.Start",
                     "description", "Started playing." ,
                     "details", _media_info.stream,
                     "clientid", "0" });

        // |RtmpSampleAccess(true, true)
        AMFEncoder invoke;
        invoke << "|RtmpSampleAccess" << true << true;
        sendResponse(MSG_DATA, invoke.data());

        //onStatus(NetStream.Data.Start)
        invoke.clear();
        AMFValue obj(AMF_OBJECT);
        obj.set("code", "NetStream.Data.Start");
        invoke << "onStatus" << obj;
        sendResponse(MSG_DATA, invoke.data());

        //onStatus(NetStream.Play.PublishNotify)
        sendStatus({ "level", "status",
                     "code", "NetStream.Play.PublishNotify",
                     "description", "Now published." ,
                     "details", _media_info.stream,
                     "clientid", "0" });

        {//Send MetaData
            invoke.clear();
            AMFValue metadata(AMF_OBJECT);
            metadata.set("major_brand", "isom");
            metadata.set("minor_version", "512");
            metadata.set("compatible_brands", "isomiso2avc1mp41");
            metadata.set("encoder", "Lavf58.76.100");
            metadata.set("server", "x2rtc/1.2.1");
            metadata.set("server_version", "1.2.1");
            invoke << "onMetaData" << metadata;
            sendResponse(MSG_DATA, invoke.data());
        }

        //提高服务器发送性能
        setSocketFlags();
    }

    void X2RtmpSession::doPlayResponse(const string& err, const std::function<void(bool)>& cb) {
        if (!err.empty()) {
            //鉴权失败，直接返回播放失败
            sendPlayResponse(err, nullptr);
            cb(false);
            return;
        }

        sendPlayResponse("", nullptr);
        cb(true);
    }

    void X2RtmpSession::doPlay(AMFDecoder& dec) {
        std::shared_ptr<Ticker> ticker(new Ticker);
        weak_ptr<X2RtmpSession> weak_self = dynamic_pointer_cast<X2RtmpSession>(shared_from_this());
        std::shared_ptr<onceToken> token(new onceToken(nullptr, [ticker, weak_self]() {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                //DebugP(strong_self.get()) << "play 回复时间:" << ticker->elapsedTime() << "ms";
            }
            }));
        auto now_stream_index = _now_stream_index;
        auto flag = false;// NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastMediaPlayed, _media_info, invoker, static_cast<SockInfo&>(*this));
        if (!flag) {
            // 该事件无人监听,默认不鉴权
            doPlayResponse("", [token](bool) {});
        }
        const char* stream = strstr(_media_info.full_url.c_str(), "/");
        if (stream != NULL) {
            stream += 1;
        }
        while (1) {
            const char* strFind = strstr(stream, "/");
            if (strFind != NULL) {
                strFind += 1;
                stream = strFind;
            }
            else {
                break;
            }
        }
        if (cb_listener_ != NULL) {
            cb_listener_->OnPlay(_media_info.app.c_str(), stream, 0, 1000, false);
        }

        _rtmp_ring = std::make_shared<RtmpRing::RingType>();
        _rtmp_ring->setDelegate(std::make_shared<mediakit::X2RtmpRingDelegateHelper>([this](mediakit::RtmpPacket::Ptr in, bool is_key) {
            onSendMedia(in);
        }));
       
    }

    void X2RtmpSession::onCmd_play2(AMFDecoder& dec) {
        doPlay(dec);
    }

    string X2RtmpSession::getStreamId(const string& str) {
        string stream_id;
        string params;
        auto pos = str.find('?');
        if (pos != string::npos) {
            //有url参数
            stream_id = str.substr(0, pos);
            //获取url参数
            params = str.substr(pos + 1);
        }
        else {
            //没有url参数
            stream_id = str;
        }

        pos = stream_id.find(":");
        if (pos != string::npos) {
            //vlc和ffplay在播放 rtmp://127.0.0.1/record/0.mp4时，
            //传过来的url会是rtmp://127.0.0.1/record/mp4:0,
            //我们在这里还原成0.mp4
            //实际使用时发现vlc，mpv等会传过来rtmp://127.0.0.1/record/mp4:0.mp4,这里做个判断
            auto ext = stream_id.substr(0, pos);
            stream_id = stream_id.substr(pos + 1);
            if (stream_id.find(ext) == string::npos) {
                stream_id = stream_id + "." + ext;
            }
        }

        if (params.empty()) {
            //没有url参数
            return stream_id;
        }

        //有url参数
        return stream_id + '?' + params;
    }

    void X2RtmpSession::onCmd_play(AMFDecoder& dec) {
        dec.load<AMFValue>();/* NULL */
        // 赋值为rtmp stream id 信息
        _media_info.stream = getStreamId(dec.load<std::string>());
        // 再解析url，切割url为app/stream_id (不一定符合rtmp url切割规范)
        _media_info.parse(_media_info.getUrl());
        doPlay(dec);
    }

    void X2RtmpSession::onCmd_pause(AMFDecoder& dec) {
        dec.load<AMFValue>();/* NULL */
        bool paused = dec.load<bool>();
        //TraceP(this) << paused;

        sendStatus({ "level", "status",
                     "code", (paused ? "NetStream.Pause.Notify" : "NetStream.Unpause.Notify"),
                     "description", (paused ? "Paused stream." : "Unpaused stream.") });

        //streamBegin
        sendUserControl(paused ? CONTROL_STREAM_EOF : CONTROL_STREAM_BEGIN, STREAM_MEDIA);
    }

    void X2RtmpSession::onCmd_playCtrl(AMFDecoder& dec) {
        dec.load<AMFValue>();
        auto ctrlObj = dec.load<AMFValue>();
        int ctrlType = ctrlObj["ctrlType"].as_integer();
        float speed = ctrlObj["speed"].as_number();

        sendStatus({ "level", "status",
                     "code", "NetStream.Speed.Notify",
                     "description", "Speeding" });

        //streamBegin
        sendUserControl(CONTROL_STREAM_EOF, STREAM_MEDIA);
    }

    void X2RtmpSession::setMetaData(AMFDecoder& dec) {
        std::string type = dec.load<std::string>();
        if (type != "onMetaData") {
            throw std::runtime_error("can only set metadata");
        }
        //_push_metadata = dec.load<AMFValue>();    //@Eric - crash when destory?
    }

    void X2RtmpSession::onProcessCmd(AMFDecoder& dec) {
        typedef void (X2RtmpSession::* cmd_function)(AMFDecoder& dec);
        static unordered_map<string, cmd_function> s_cmd_functions;
        static onceToken token([]() {
            s_cmd_functions.emplace("connect", &X2RtmpSession::onCmd_connect);
            s_cmd_functions.emplace("createStream", &X2RtmpSession::onCmd_createStream);
            s_cmd_functions.emplace("publish", &X2RtmpSession::onCmd_publish);
            s_cmd_functions.emplace("deleteStream", &X2RtmpSession::onCmd_deleteStream);
            s_cmd_functions.emplace("play", &X2RtmpSession::onCmd_play);
            s_cmd_functions.emplace("play2", &X2RtmpSession::onCmd_play2);
            s_cmd_functions.emplace("seek", &X2RtmpSession::onCmd_seek);
            s_cmd_functions.emplace("pause", &X2RtmpSession::onCmd_pause);
            s_cmd_functions.emplace("onPlayCtrl", &X2RtmpSession::onCmd_playCtrl);
            });

        std::string method = dec.load<std::string>();
        auto it = s_cmd_functions.find(method);
        if (it == s_cmd_functions.end()) {
            //		TraceP(this) << "can not support cmd:" << method;
            return;
        }
        _recv_req_id = dec.load<double>();
        auto fun = it->second;
        (this->*fun)(dec);
    }

    void X2RtmpSession::onRtmpChunk(RtmpPacket::Ptr packet) {
        auto& chunk_data = *packet;
        switch (chunk_data.type_id) {
        case MSG_CMD:
        case MSG_CMD3: {
            AMFDecoder dec(chunk_data.buffer, chunk_data.type_id == MSG_CMD3 ? 3 : 0);
            onProcessCmd(dec);
            break;
        }

        case MSG_DATA:
        case MSG_DATA3: {
            AMFDecoder dec(chunk_data.buffer, chunk_data.type_id == MSG_DATA3 ? 3 : 0);
            std::string type = dec.load<std::string>();
            if (type == "@setDataFrame") {
                setMetaData(dec);
            }
            else if (type == "onMetaData") {
                //兼容某些不规范的推流器
                //_push_metadata = dec.load<AMFValue>();    //@Eric - crash when destory?
            }
            else {
                //TraceP(this) << "unknown notify:" << type;
            }
            break;
        }

        case MSG_AUDIO:
        case MSG_VIDEO: {
            if (!_push_src) {
                //WarnL << "Not a rtmp push!";
                return;
            }

            if (!_set_meta_data) {
                _set_meta_data = true;
            }
            if (chunk_data.type_id == MSG_AUDIO) {
                if (cb_listener_ != NULL) {
                    //cb_listener_->OnRecvAudio(packet->data(), packet->size(), packet->time_stamp);
                    cb_listener_->OnRecvRtmpData(packet);
                }
            }
            else {
                if (cb_listener_ != NULL) {
                    //cb_listener_->OnRecvVideo(packet->isVideoKeyFrame(), packet->data(), packet->size(), packet->time_stamp);
                    cb_listener_->OnRecvRtmpData(packet);
                }
            }
            break;
        }

        default:
            //WarnP(this) << "unhandled message:" << (int)chunk_data.type_id << hexdump(chunk_data.buffer.data(), chunk_data.buffer.size());
            break;
        }
    }

    void X2RtmpSession::onCmd_seek(AMFDecoder& dec) {
        dec.load<AMFValue>();/* NULL */
        sendStatus({ "level", "status",
                     "code", "NetStream.Seek.Notify",
                     "description", "Seeking." });

        auto milliSeconds = (uint32_t)(dec.load<AMFValue>().as_number());
        //InfoP(this) << "rtmp seekTo(ms):" << milliSeconds;
    }

    void X2RtmpSession::onSendMedia(const RtmpPacket::Ptr& pkt) {
        sendRtmp(pkt->type_id, pkt->stream_index, pkt, pkt->time_stamp, pkt->chunk_id);
    }

    void X2RtmpSession::setSocketFlags() {
        GET_CONFIG(int, merge_write_ms, General::kMergeWriteMS);
        if (merge_write_ms > 0) {
            //推流模式下，关闭TCP_NODELAY会增加推流端的延时，但是服务器性能将提高
            //SockUtil::setNoDelay(getSock()->rawFD(), false);
            //播放模式下，开启MSG_MORE会增加延时，但是能提高发送性能
            //setSendFlags(SOCKET_DEFAULE_FLAGS | FLAG_MORE);
        }
    }

    void X2RtmpSession::dumpMetadata(const AMFValue& metadata) {
        if (metadata.type() != AMF_OBJECT && metadata.type() != AMF_ECMA_ARRAY) {
            //WarnL << "invalid metadata type:" << metadata.type();
            return;
        }
        _StrPrinter printer;
        metadata.object_for_each([&](const string& key, const AMFValue& val) {
            printer << "\r\n" << key << "\t:" << val.to_string();
            });
        //InfoL << _media_info.shortUrl() << (string)printer;
    }
} /* namespace mediakit */
