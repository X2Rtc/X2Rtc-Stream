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
#ifndef __X2_RTP_GB_PROCESS_H__
#define __X2_RTP_GB_PROCESS_H__
#include "Decoder.h"
#include "ProcessInterface.h"
#include "Http/HttpRequestSplitter.h"
#include "Rtsp/RtpCodec.h"
#include "Common/MediaSource.h"

using namespace mediakit;

namespace x2rtc {
class RtpReceiverImp;
class X2RtpGBProcess : public ProcessInterface
{
public:
    using Ptr = std::shared_ptr<X2RtpGBProcess>;

    X2RtpGBProcess(const MediaInfo& media_info, MediaSinkInterface* sink);
    ~X2RtpGBProcess() override = default;

    /**
     * 输入rtp
     * @param data rtp数据指针
     * @param data_len rtp数据长度
     * @return 是否解析成功
     */
    bool inputRtp(bool, const char* data, size_t data_len) override;

    /**
     * 刷新输出所有缓存
     */
    void flush() override;

protected:
    void onRtpSorted(RtpPacket::Ptr rtp);

private:
    void onRtpDecode(const Frame::Ptr & frame);

private:
    MediaInfo _media_info;
    DecoderImp::Ptr _decoder;
    MediaSinkInterface* _interface;
    std::shared_ptr<FILE> _save_file_ps;
    std::unordered_map<uint8_t, RtpCodec::Ptr> _rtp_decoder;
    std::unordered_map<uint8_t, std::shared_ptr<RtpReceiverImp> > _rtp_receiver;

};

}	// namespace x2rtc
#endif	// __X2_RTP_GB_PROCESS_H__