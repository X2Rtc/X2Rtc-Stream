#include <stdio.h>
#include <sys/stat.h>
#include <algorithm>
#include "Common/config.h"
#include "X2HttpSession.h"
#include "X2HttpBody.h"
#include "Common/strCoding.h"
#include "Http/HttpConst.h"
#include "Util/base64.h"
#include "Util/SHA1.h"

using namespace std;
using namespace toolkit;

namespace x2rtc {

X2HttpSession::X2HttpSession() {
    GET_CONFIG(uint32_t,keep_alive_sec,Http::kKeepAliveSecond);
    //pSock->setSendTimeOutSecond(keep_alive_sec);
}

X2HttpSession::~X2HttpSession() = default;

void X2HttpSession::Handle_Req_HEAD(ssize_t &content_len){
    //暂时全部返回200 OK，因为HTTP GET存在按需生成流的操作，所以不能按照HTTP GET的流程返回
    //如果直接返回404，那么又会导致按需生成流的逻辑失效，所以HTTP HEAD在静态文件或者已存在资源时才有效
    //对于按需生成流的直播场景并不适用
    sendResponse(200, false);
}

void X2HttpSession::Handle_Req_OPTIONS(ssize_t &content_len) {
    KeyValue header;
    header.emplace("Allow", "GET, POST, HEAD, OPTIONS");
    GET_CONFIG(bool, allow_cross_domains, Http::kAllowCrossDomains);
    if (allow_cross_domains) {
        header.emplace("Access-Control-Allow-Origin", "*");
        header.emplace("Access-Control-Allow-Headers", "*");
        header.emplace("Access-Control-Allow-Methods", "GET, POST, HEAD, OPTIONS");
    }
    header.emplace("Access-Control-Allow-Credentials", "true");
    header.emplace("Access-Control-Request-Methods", "GET, POST, OPTIONS");
    header.emplace("Access-Control-Request-Headers", "Accept,Accept-Language,Content-Language,Content-Type");
    sendResponse(200, true, nullptr, header);
}

ssize_t X2HttpSession::onRecvHeader(const char *header,size_t len) {
    typedef void (X2HttpSession::*HttpCMDHandle)(ssize_t &);
    static unordered_map<string, HttpCMDHandle> s_func_map;
    static onceToken token([]() {
        s_func_map.emplace("GET",&X2HttpSession::Handle_Req_GET);
        s_func_map.emplace("DELETE",&X2HttpSession::Handle_Req_GET);
        s_func_map.emplace("POST",&X2HttpSession::Handle_Req_POST);
        s_func_map.emplace("HEAD",&X2HttpSession::Handle_Req_HEAD);
        s_func_map.emplace("OPTIONS",&X2HttpSession::Handle_Req_OPTIONS);
    }, nullptr);

    _parser.parse(header, len);
    CHECK(_parser.url()[0] == '/');

    urlDecode(_parser);
    string cmd = _parser.method();
    auto it = s_func_map.find(cmd);
    if (it == s_func_map.end()) {
        //WarnP(this) << "不支持该命令:" << cmd;
        sendResponse(405, true);
        return 0;
    }

    //跨域
    _origin = _parser["Origin"];

    //默认后面数据不是content而是header
    ssize_t content_len = 0;
    (this->*(it->second))(content_len);

    //清空解析器节省内存
    _parser.clear();
    //返回content长度
    return content_len;
}

void X2HttpSession::onRecvContent(const char *data,size_t len) {
    if(_contentCallBack){
        if(!_contentCallBack(data,len)){
            _contentCallBack = nullptr;
        }
    }
}

void X2HttpSession::RecvData(const char* pData, int nLen){
    _ticker.resetTime();
    input(pData, nLen);
}

void X2HttpSession::Close() {
    if (_is_live_stream) {
        //flv/ts播放器
        uint64_t duration = _ticker.createdTime() / 1000;
        /*WarnP(this) << "FLV/TS/FMP4播放器("
                    << _mediaInfo.shortUrl()
                    << ")断开:" << err
                    << ",耗时(s):" << duration;*/

        GET_CONFIG(uint32_t, iFlowThreshold, General::kFlowThreshold);
        if (_total_bytes_usage >= iFlowThreshold * 1024) {
            //NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastFlowReport, _mediaInfo, _total_bytes_usage,
             //                                  duration, true, static_cast<SockInfo &>(*this));
        }
        return;
    }
}

bool X2HttpSession::checkWebSocket(){
    auto Sec_WebSocket_Key = _parser["Sec-WebSocket-Key"];
    if (Sec_WebSocket_Key.empty()) {
        return false;
    }
    auto Sec_WebSocket_Accept = encodeBase64(SHA1::encode_bin(Sec_WebSocket_Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));

    KeyValue headerOut;
    headerOut["Upgrade"] = "websocket";
    headerOut["Connection"] = "Upgrade";
    headerOut["Sec-WebSocket-Accept"] = Sec_WebSocket_Accept;
    if (!_parser["Sec-WebSocket-Protocol"].empty()) {
        headerOut["Sec-WebSocket-Protocol"] = _parser["Sec-WebSocket-Protocol"];
    }

    auto res_cb = [this, headerOut]() {
        _live_over_websocket = true;
        sendResponse(101, false, nullptr, headerOut, nullptr, true);
    };
    
    auto res_cb_flv = [this, headerOut]() mutable {
        _live_over_websocket = true;
        headerOut.emplace("Cache-Control", "no-store");
        sendResponse(101, false, nullptr, headerOut, nullptr, true);
    };

    //判断是否为websocket-flv
    if (checkLiveStreamFlv(res_cb_flv)) {
        //这里是websocket-flv直播请求
        return true;
    }

    //判断是否为websocket-ts
    if (checkLiveStreamTS(res_cb)) {
        //这里是websocket-ts直播请求
        return true;
    }

    //判断是否为websocket-fmp4
    if (checkLiveStreamFMP4(res_cb)) {
        //这里是websocket-fmp4直播请求
        return true;
    }

    //这是普通的websocket连接
    if (!onWebSocketConnect(_parser)) {
        sendResponse(501, true, nullptr, headerOut);
        return true;
    }
    sendResponse(101, false, nullptr, headerOut, nullptr, true);
    return true;
}

bool X2HttpSession::checkLiveStream(const string &schema, const string  &url_suffix, const function<void(int nCode)> &cb){
    std::string url = _parser.url();
    auto it = _parser.getUrlArgs().find("schema");
    if (it != _parser.getUrlArgs().end()) {
        if (strcasecmp(it->second.c_str(), schema.c_str())) {
            // unsupported schema
            return false;
        }
    } else {
        auto prefix_size = url_suffix.size();
        if (url.size() < prefix_size || strcasecmp(url.data() + (url.size() - prefix_size), url_suffix.data())) {
            //未找到后缀
            return false;
        }
        // url去除特殊后缀
        url.resize(url.size() - prefix_size);
    }

    //带参数的url
    if (!_parser.params().empty()) {
        url += "?";
        url += _parser.params();
    }

    //解析带上协议+参数完整的url
    _mediaInfo.parse(schema + "://" + _parser["Host"] + url);

    if (_mediaInfo.app.empty() || _mediaInfo.stream.empty()) {
        //url不合法
        return false;
    }

    bool close_flag = !strcasecmp(_parser["Connection"].data(), "close");
    //触发回调
    cb(200);
    if (cb_listener_ != NULL) {
        cb_listener_->OnPlay(_mediaInfo.app.c_str(), _mediaInfo.stream.c_str(), _mediaInfo.param_strs.c_str());
    }
    return true;
}

//http-fmp4 链接格式:http://vhost-url:port/app/streamid.x2rtc.mp4?key1=value1&key2=value2
bool X2HttpSession::checkLiveStreamFMP4(const function<void()> &cb){
    return checkLiveStream(FMP4_SCHEMA, ".x2rtc.mp4", [this, cb](int code) {
        if (!cb) {
            //找到源，发送http头，负载后续发送
            sendResponse(code, false, getHttpContentType(".mp4").data(), KeyValue(), nullptr, true);
        } else {
            //自定义发送http头
            cb();
        }
    });
}

//http-ts 链接格式:http://vhost-url:port/app/streamid.x2rtc.ts?key1=value1&key2=value2
bool X2HttpSession::checkLiveStreamTS(const function<void()> &cb){
    return checkLiveStream(TS_SCHEMA, ".x2rtc.ts", [this, cb](int code) {
        if (!cb) {
            //找到源，发送http头，负载后续发送
            sendResponse(code, false, getHttpContentType(".ts").data(), KeyValue(), nullptr, true);
        } else {
            //自定义发送http头
            cb();
        }
    });
}

//http-flv 链接格式:http://vhost-url:port/app/streamid.x2rtc.flv?key1=value1&key2=value2
bool X2HttpSession::checkLiveStreamFlv(const function<void()> &cb){
    auto start_pts = atoll(_parser.getUrlArgs()["starPts"].data());
    return checkLiveStream(RTMP_SCHEMA, ".x2rtc.flv", [this, cb, start_pts](int code) {
        if (!cb) {
            //找到源，发送http头，负载后续发送
            KeyValue headerOut;
            headerOut["Cache-Control"] = "no-store";
            sendResponse(code, false, getHttpContentType(".flv").data(), headerOut, nullptr, true);
        } else {
            //自定义发送http头
            cb();
        }
    });
}

void X2HttpSession::Handle_Req_GET(ssize_t &content_len) {
    Handle_Req_GET_l(content_len, true);
}

void X2HttpSession::Handle_Req_GET_l(ssize_t &content_len, bool sendBody) {
    //先看看是否为WebSocket请求
    if (checkWebSocket()) {
        content_len = -1;
        _contentCallBack = [this](const char *data, size_t len) {
            WebSocketSplitter::decode((uint8_t *) data, len);
            //_contentCallBack是可持续的，后面还要处理后续数据
            return true;
        };
        return;
    }

    if (checkLiveStreamFlv()) {
        //拦截http-flv播放器
        return;
    }

    if (checkLiveStreamTS()) {
        //拦截http-ts播放器
        return;
    }

    if (checkLiveStreamFMP4()) {
        //拦截http-fmp4播放器
        return;
    }

    bool bClose = !strcasecmp(_parser["Connection"].data(),"close");
    //未找到该流
    sendNotFound(bClose);
}

static string dateStr() {
    char buf[64];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "%a, %b %d %Y %H:%M:%S GMT", gmtime(&tt));
    return buf;
}

static const string kDate = "Date";
static const string kServer = "Server";
static const string kConnection = "Connection";
static const string kKeepAlive = "Keep-Alive";
static const string kContentType = "Content-Type";
static const string kContentLength = "Content-Length";
static const string kAccessControlAllowOrigin = "Access-Control-Allow-Origin";
static const string kAccessControlAllowCredentials = "Access-Control-Allow-Credentials";

void X2HttpSession::sendResponse(int code,
                               bool bClose,
                               const char *pcContentType,
                               const X2HttpSession::KeyValue &header,
                               const X2HttpBody::Ptr &body,
                               bool no_content_length ){
    GET_CONFIG(string,charSet,Http::kCharSet);
    GET_CONFIG(uint32_t,keepAliveSec,Http::kKeepAliveSecond);

    //body默认为空
    int64_t size = 0;
    if (body && body->remainSize()) {
        //有body，获取body大小
        size = body->remainSize();
    }

    if (no_content_length) {
        // http-flv直播是Keep-Alive类型
        bClose = false;
    } else if ((size_t)size >= SIZE_MAX || size < 0) {
        //不固定长度的body，那么发送完body后应该关闭socket，以便浏览器做下载完毕的判断
        bClose = true;
    }

    X2HttpSession::KeyValue &headerOut = const_cast<X2HttpSession::KeyValue &>(header);
    headerOut.emplace(kDate, dateStr());
    headerOut.emplace(kServer, "X2Rtc-Stream");
    headerOut.emplace("Access-Control-Allow-Origin", "*");
    headerOut.emplace(kConnection, bClose ? "close" : "keep-alive");
    if (!bClose) {
        string keepAliveString = "timeout=";
        keepAliveString += to_string(keepAliveSec);
        keepAliveString += ", max=100";
        headerOut.emplace(kKeepAlive, std::move(keepAliveString));
    }

    if (!_origin.empty()) {
        //设置跨域
        headerOut.emplace(kAccessControlAllowOrigin, _origin);
        headerOut.emplace(kAccessControlAllowCredentials, "true");
    }

    if (!no_content_length && size >= 0 && (size_t)size < SIZE_MAX) {
        //文件长度为固定值,且不是http-flv强制设置Content-Length
        headerOut[kContentLength] = to_string(size);
    }

    if (size && !pcContentType) {
        //有body时，设置缺省类型
        pcContentType = "text/plain";
    }

    if ((size || no_content_length) && pcContentType) {
        //有body时，设置文件类型
        string strContentType = pcContentType;
        strContentType += "; charset=";
        strContentType += charSet;
        headerOut.emplace(kContentType, std::move(strContentType));
    }

    //发送http头
    string str;
    str.reserve(256);
    str += "HTTP/1.1 ";
    str += to_string(code);
    str += ' ';
    str += getHttpStatusMessage(code);
    str += "\r\n";
    for (auto &pr : header) {
        str += pr.first;
        str += ": ";
        str += pr.second;
        str += "\r\n";
    }
    str += "\r\n";
    //SockSender::send(std::move(str)); 
    if (cb_listener_ != NULL) {
        cb_listener_->OnSendData(str.c_str(), str.length());
    }
    _ticker.resetTime();

    if (!size) {
        //没有body
        if (bClose) {
            SockException(Err_shutdown, StrPrinter << "close connection after send http header completed with status code:" << code);
            //shutdown(SockException(Err_shutdown,StrPrinter << "close connection after send http header completed with status code:" << code));
        }
        return;
    }

    size_t remainSize = body->remainSize();
    toolkit::Buffer::Ptr buffer = body->readData(remainSize);
    if (buffer != NULL) {
        if (cb_listener_ != NULL) {
            cb_listener_->OnSendData(buffer->data(), buffer->size());
        }
    }
    
}

string X2HttpSession::urlDecode(const string &str){
    auto ret = strCoding::UrlDecode(str);
#ifdef _WIN32
    GET_CONFIG(string,charSet,Http::kCharSet);
    bool isGb2312 = !strcasecmp(charSet.data(), "gb2312");
    if (isGb2312) {
        ret = strCoding::UTF8ToGB2312(ret);
    }
#endif // _WIN32
    return ret;
}

void X2HttpSession::urlDecode(Parser &parser){
    parser.setUrl(urlDecode(parser.url()));
    for(auto &pr : _parser.getUrlArgs()){
        const_cast<string &>(pr.second) = urlDecode(pr.second);
    }
}

bool X2HttpSession::emitHttpEvent(bool doInvoke){
    bool bClose = !strcasecmp(_parser["Connection"].data(),"close");
#if 0
    /////////////////////异步回复Invoker///////////////////////////////
    weak_ptr<X2HttpSession> weak_self = static_pointer_cast<X2HttpSession>(shared_from_this());
    HttpResponseInvoker invoker = [weak_self,bClose](int code, const KeyValue &headerOut, const HttpBody::Ptr &body){
        auto strong_self = weak_self.lock();
        if(!strong_self) {
            return;
        }
        strong_self->async([weak_self, bClose, code, headerOut, body]() {
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                //本对象已经销毁
                return;
            }
            strong_self->sendResponse(code, bClose, nullptr, headerOut, body);
        });
    };
    ///////////////////广播HTTP事件///////////////////////////
    bool consumed = false;//该事件是否被消费
    NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastHttpRequest,_parser,invoker,consumed,static_cast<SockInfo &>(*this));
    if(!consumed && doInvoke){
        //该事件无人消费，所以返回404
        invoker(404,KeyValue(), HttpBody::Ptr());
    }
    return consumed;
#endif
    return false;
}

void X2HttpSession::Handle_Req_POST(ssize_t &content_len) {
    GET_CONFIG(size_t,maxReqSize,Http::kMaxReqSize);

    ssize_t totalContentLen = _parser["Content-Length"].empty() ? -1 : atoll(_parser["Content-Length"].data());

    if(totalContentLen == 0){
        //content为空
        //emitHttpEvent内部会选择是否关闭连接
        emitHttpEvent(true);
        return;
    }

    if(totalContentLen > 0 && (size_t)totalContentLen < maxReqSize ){
        //返回固定长度的content
        content_len = totalContentLen;
        auto parserCopy = _parser;
        _contentCallBack = [this,parserCopy](const char *data,size_t len){
            //恢复http头
            _parser = parserCopy;
            //设置content
            _parser.setContent(string(data,len));
            //触发http事件，emitHttpEvent内部会选择是否关闭连接
            emitHttpEvent(true);
            //清空数据,节省内存
            _parser.clear();
            //content已经接收完毕
            return false;
        };
    }else{
        //返回不固定长度的content或者超过长度限制的content
        content_len = -1;
        auto parserCopy = _parser;
        std::shared_ptr<size_t> recvedContentLen = std::make_shared<size_t>(0);
        bool bClose = !strcasecmp(_parser["Connection"].data(),"close");

        _contentCallBack = [this,parserCopy,totalContentLen,recvedContentLen,bClose](const char *data,size_t len){
            *(recvedContentLen) += len;
            if (totalContentLen < 0) {
                //不固定长度的content,源源不断接收数据
                onRecvUnlimitedContent(parserCopy, data, len, SIZE_MAX, *(recvedContentLen));
                return true;
            }

            //长度超过限制的content
            onRecvUnlimitedContent(parserCopy,data,len,totalContentLen,*(recvedContentLen));

            if(*(recvedContentLen) < (size_t)totalContentLen){
                //数据还没接收完毕
                //_contentCallBack是可持续的，后面还要处理后续content数据
                return true;
            }

            //数据接收完毕
            if(!bClose){
                //keep-alive类型连接
                //content接收完毕，后续都是http header
                setContentLen(0);
                //content已经接收完毕
                return false;
            }

            //连接类型是close类型，收完content就关闭连接
            SockException(Err_shutdown, "recv http content completed");
            //shutdown(SockException(Err_shutdown,"recv http content completed"));
            //content已经接收完毕
            return false ;
        };
    }
    //有后续content数据要处理,暂时不关闭连接
}

void X2HttpSession::sendNotFound(bool bClose) {
    GET_CONFIG(string, notFound, Http::kNotFound);
    sendResponse(404, bClose, "text/html", KeyValue(), std::make_shared<X2HttpStringBody>(notFound));
}

void X2HttpSession::setSocketFlags(){
    GET_CONFIG(int, mergeWriteMS, General::kMergeWriteMS);
    if(mergeWriteMS > 0) {
        //推流模式下，关闭TCP_NODELAY会增加推流端的延时，但是服务器性能将提高
        //SockUtil::setNoDelay(getSock()->rawFD(), false);
        //播放模式下，开启MSG_MORE会增加延时，但是能提高发送性能
        //setSendFlags(SOCKET_DEFAULE_FLAGS | FLAG_MORE);
    }
}

void X2HttpSession::onWrite(const Buffer::Ptr &buffer, bool flush) {
    if(flush){
        //需要flush那么一次刷新缓存
        //X2HttpSession::setSendFlushFlag(true);
    }

    _ticker.resetTime();
    if (!_live_over_websocket) {
        _total_bytes_usage += buffer->size();
        //send(std::move(buffer));
        if (cb_listener_ != NULL) {
            cb_listener_->OnSendData(buffer->data(), buffer->size());
        }
    } else {
        WebSocketHeader header;
        header._fin = true;
        header._reserved = 0;
        header._opcode = WebSocketHeader::BINARY;
        header._mask_flag = false;
        WebSocketSplitter::encode(header, buffer);
    }

    if (flush) {
        //本次刷新缓存后，下次不用刷新缓存
        //X2HttpSession::setSendFlushFlag(false);
    }
}

void X2HttpSession::onWebSocketEncodeData(Buffer::Ptr buffer){
    _total_bytes_usage += buffer->size();
    //send(std::move(buffer));
    if (cb_listener_ != NULL) {
        cb_listener_->OnSendData(buffer->data(), buffer->size());
    }
}

void X2HttpSession::onWebSocketDecodeComplete(const WebSocketHeader &header_in){
    WebSocketHeader& header = const_cast<WebSocketHeader&>(header_in);
    header._mask_flag = false;

    switch (header._opcode) {
        case WebSocketHeader::CLOSE: {
            encode(header, nullptr);
            if (cb_listener_ != NULL) {
                cb_listener_->OnClose();
            }
            //shutdown(SockException(Err_shutdown, "recv close request from client"));
            SockException(Err_shutdown, "recv close request from client");
            break;
        }

        default : break;
    }
}

void X2HttpSession::onDetach() {
    if (cb_listener_ != NULL) {
        cb_listener_->OnClose();
    }
    //shutdown(SockException(Err_shutdown,"rtmp ring buffer detached"));
    SockException(Err_shutdown, "rtmp ring buffer detached");
}

std::shared_ptr<FlvMuxer> X2HttpSession::getSharedPtr(){
    return dynamic_pointer_cast<FlvMuxer>(shared_from_this());
}

} /* namespace mediakit */
