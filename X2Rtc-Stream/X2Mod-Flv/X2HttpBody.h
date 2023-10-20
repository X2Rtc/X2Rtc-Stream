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
#ifndef __X2_HTTP_BODY_H__
#define __X2_HTTP_BODY_H__

#include <stdlib.h>
#include <memory>
#include "Network/Buffer.h"
#include "Util/ResourcePool.h"
#include "Util/logger.h"
#include "Util/mini.h"
#include "Common/Parser.h"
#include "Common/strCoding.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b) )
#endif //MIN

using namespace mediakit;

namespace x2rtc {

/**
 * http content部分基类定义
 */
class X2HttpBody : public std::enable_shared_from_this<X2HttpBody>{
public:
    using Ptr = std::shared_ptr<X2HttpBody>;
    X2HttpBody() = default;

    virtual ~X2HttpBody() = default;

    /**
     * 剩余数据大小，如果返回-1, 那么就不设置content-length
     */
    virtual int64_t remainSize() { return 0;};

    /**
     * 读取一定字节数，返回大小可能小于size
     * @param size 请求大小
     * @return 字节对象,如果读完了，那么请返回nullptr
     */
    virtual toolkit::Buffer::Ptr readData(size_t size) { return nullptr;};

    /**
     * 异步请求读取一定字节数，返回大小可能小于size
     * @param size 请求大小
     * @param cb 回调函数
     */
    virtual void readDataAsync(size_t size,const std::function<void(const toolkit::Buffer::Ptr &buf)> &cb){
        //由于unix和linux是通过mmap的方式读取文件，所以把读文件操作放在后台线程并不能提高性能
        //反而会由于频繁的线程切换导致性能降低以及延时增加，所以我们默认同步获取文件内容
        //(其实并没有读，拷贝文件数据时在内核态完成文件读)
        cb(readData(size));
    }

    /**
     * 使用sendfile优化文件发送
     * @param fd socket fd
     * @return 0成功，其他为错误代码
     */
    virtual int sendFile(int fd) {
        return -1;
    }
};

/**
 * std::string类型的content
 */
class X2HttpStringBody : public X2HttpBody{
public:
    using Ptr = std::shared_ptr<X2HttpStringBody>;
    X2HttpStringBody(std::string str);
    ~X2HttpStringBody() override = default;

    int64_t remainSize() override;
    toolkit::Buffer::Ptr readData(size_t size) override ;

private:
    size_t _offset = 0;
    mutable std::string _str;
};

/**
 * Buffer类型的content
 */
class X2HttpBufferBody : public X2HttpBody{
public:
    using Ptr = std::shared_ptr<X2HttpBufferBody>;
    X2HttpBufferBody(toolkit::Buffer::Ptr buffer);
    ~X2HttpBufferBody() override = default;

    int64_t remainSize() override;
    toolkit::Buffer::Ptr readData(size_t size) override;

private:
    toolkit::Buffer::Ptr _buffer;
};

/**
 * 文件类型的content
 */
class X2HttpFileBody : public X2HttpBody {
public:
    using Ptr = std::shared_ptr<X2HttpFileBody>;

    /**
     * 构造函数
     * @param file_path 文件路径
     * @param use_mmap 是否使用mmap方式访问文件
     */
    X2HttpFileBody(const std::string &file_path, bool use_mmap = true);
    ~X2HttpFileBody() override = default;

    /**
     * 设置读取范围
     * @param offset 相对文件头的偏移量
     * @param max_size 最大读取字节数
     */
    void setRange(uint64_t offset, uint64_t max_size);

    int64_t remainSize() override;
    toolkit::Buffer::Ptr readData(size_t size) override;
    int sendFile(int fd) override;

private:
    int64_t _read_to = 0;
    uint64_t _file_offset = 0;
    std::shared_ptr<FILE> _fp;
    std::shared_ptr<char> _map_addr;
    toolkit::ResourcePool<toolkit::BufferRaw> _pool;
};

class X2HttpArgs : public std::map<std::string, toolkit::variant, StrCaseCompare> {
public:
    X2HttpArgs() = default;
    ~X2HttpArgs() = default;

    std::string make() const {
        std::string ret;
        for (auto& pr : *this) {
            ret.append(pr.first);
            ret.append("=");
            ret.append(strCoding::UrlEncode(pr.second));
            ret.append("&");
        }
        if (ret.size()) {
            ret.pop_back();
        }
        return ret;
    }
};

/**
 * http MultiForm 方式提交的http content
 */
class X2HttpMultiFormBody : public X2HttpBody {
public:
    using Ptr = std::shared_ptr<X2HttpMultiFormBody>;

    /**
     * 构造函数
     * @param args http提交参数列表
     * @param filePath 文件路径
     * @param boundary boundary字符串
     */
    X2HttpMultiFormBody(const X2HttpArgs &args,const std::string &filePath,const std::string &boundary = "0xKhTmLbOuNdArY");
    virtual ~X2HttpMultiFormBody() = default;
    int64_t remainSize() override ;
    toolkit::Buffer::Ptr readData(size_t size) override;

public:
    static std::string multiFormBodyPrefix(const X2HttpArgs &args,const std::string &boundary,const std::string &fileName);
    static std::string multiFormBodySuffix(const std::string &boundary);
    static std::string multiFormContentType(const std::string &boundary);

private:
    uint64_t _offset = 0;
    int64_t _totalSize;
    std::string _bodyPrefix;
    std::string _bodySuffix;
    X2HttpFileBody::Ptr _fileBody;
};

}//namespace mediakit

#endif //__X2_HTTP_BODY_H__
