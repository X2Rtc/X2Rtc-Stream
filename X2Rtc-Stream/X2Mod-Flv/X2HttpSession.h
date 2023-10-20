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
#ifndef __X2_HTTP_HTTPSESSION_H__
#define __X2_HTTP_HTTPSESSION_H__

#include <functional>
#include "X2FlvMuxer.h"
#include "Common/Parser.h"
#include "Http/HttpRequestSplitter.h"
#include "Http/WebSocketSplitter.h"
#include "X2HttpBody.h"

using namespace mediakit;

namespace x2rtc {

class X2HttpSession: public std::enable_shared_from_this<X2HttpSession>, public FlvMuxer,
                   public HttpRequestSplitter,
                   public WebSocketSplitter {
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;

    public:
        virtual void OnPlay(const char* app, const char* stream, const char*args) = 0;
        virtual void OnClose() = 0;
        virtual void OnSendData(const char* pData, int nLen) = 0;
        virtual void OnSendData(const char* pHdr, int nLen1, const char* pData, int nLen) = 0;
    };

    void SetListener(Listener* listener) {
        cb_listener_ = listener;
    };

protected:
    Listener* cb_listener_{nullptr};
public:
    using Ptr = std::shared_ptr<X2HttpSession>;
    using KeyValue = StrCaseMap;
    friend class AsyncSender;
    /**
     * @param errMsg 如果为空，则代表鉴权通过，否则为错误提示
     * @param accessPath 运行或禁止访问的根目录
     * @param cookieLifeSecond 鉴权cookie有效期
     **/
    using HttpAccessPathInvoker = std::function<void(const std::string &errMsg,const std::string &accessPath, int cookieLifeSecond)>;

    X2HttpSession();
    ~X2HttpSession() override;

    void RecvData(const char* pData, int nLen);
    void Close();
    static std::string urlDecode(const std::string &str);

protected:
    //FlvMuxer override
    void onWrite(const toolkit::Buffer::Ptr &data, bool flush) override ;
    void onDetach() override;
    std::shared_ptr<FlvMuxer> getSharedPtr() override;

    //HttpRequestSplitter override
    ssize_t onRecvHeader(const char *data,size_t len) override;
    void onRecvContent(const char *data,size_t len) override;

    /**
     * 重载之用于处理不定长度的content
     * 这个函数可用于处理大文件上传、http-flv推流
     * @param header http请求头
     * @param data content分片数据
     * @param len content分片数据大小
     * @param totalSize content总大小,如果为0则是不限长度content
     * @param recvedSize 已收数据大小
     */
    virtual void onRecvUnlimitedContent(const Parser &header,
                                        const char *data,
                                        size_t len,
                                        size_t totalSize,
                                        size_t recvedSize){
        //shutdown(toolkit::SockException(toolkit::Err_shutdown,"http post content is too huge,default closed"));
        toolkit::SockException(toolkit::Err_shutdown, "http post content is too huge,default closed");
    }

    /**
     * websocket客户端连接上事件
     * @param header http头
     * @return true代表允许websocket连接，否则拒绝
     */
    virtual bool onWebSocketConnect(const Parser &header){
        //WarnP(this) << "http server do not support websocket default";
        return false;
    }

    //WebSocketSplitter override
    /**
     * 发送数据进行websocket协议打包后回调
     * @param buffer websocket协议数据
     */
    void onWebSocketEncodeData(toolkit::Buffer::Ptr buffer) override;

    /**
     * 接收到完整的一个webSocket数据包后回调
     * @param header 数据包包头
     */
    void onWebSocketDecodeComplete(const WebSocketHeader &header_in) override;

private:
    void Handle_Req_GET(ssize_t &content_len);
    void Handle_Req_GET_l(ssize_t &content_len, bool sendBody);
    void Handle_Req_POST(ssize_t &content_len);
    void Handle_Req_HEAD(ssize_t &content_len);
    void Handle_Req_OPTIONS(ssize_t &content_len);

    bool checkLiveStream(const std::string &schema, const std::string  &url_suffix, const std::function<void(int nCode)> &cb);

    bool checkLiveStreamFlv(const std::function<void()> &cb = nullptr);
    bool checkLiveStreamTS(const std::function<void()> &cb = nullptr);
    bool checkLiveStreamFMP4(const std::function<void()> &fmp4_list = nullptr);

    bool checkWebSocket();
    bool emitHttpEvent(bool doInvoke);
    void urlDecode(Parser &parser);
    void sendNotFound(bool bClose);
    void sendResponse(int code, bool bClose, const char *pcContentType = nullptr,
                      const X2HttpSession::KeyValue &header = X2HttpSession::KeyValue(),
                      const X2HttpBody::Ptr &body = nullptr, bool no_content_length = false);

    //设置socket标志
    void setSocketFlags();

protected:
    MediaInfo _mediaInfo;

private:
    bool _is_live_stream = false;
    bool _live_over_websocket = false;
    //消耗的总流量
    uint64_t _total_bytes_usage = 0;
    std::string _origin;
    Parser _parser;
    toolkit::Ticker _ticker;
    //处理content数据的callback
    std::function<bool (const char *data,size_t len) > _contentCallBack;
};


} /* namespace mediakit */

#endif /* __X2_HTTP_HTTPSESSION_H__ */
