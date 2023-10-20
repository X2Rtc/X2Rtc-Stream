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

#ifndef __X2_RTC_TIME_SYNC_H__
#define __X2_RTC_TIME_SYNC_H__
#include <set>
#include <cstdint>

#define MAX_DELTA_STAMP (3 * 1000)
#define STAMP_LOOP_DELTA (60 * 1000)
#define MAX_CTS 500
#define ABS(x) ((x) > 0 ? (x) : (-x))


class DeltaStamp {
public:
    DeltaStamp() {};
    ~DeltaStamp() {};

    /**
     * 计算时间戳增量
     * @param stamp 绝对时间戳
     * @return 时间戳增量
     */
    int64_t deltaStamp(int64_t stamp) {
        if (!_last_stamp) {
            // 第一次计算时间戳增量,时间戳增量为0
            if (stamp) {
                _last_stamp = stamp;
            }
            return 0;
        }

        int64_t ret = stamp - _last_stamp;
        if (ret >= 0) {
            // 时间戳增量为正，返回之
            _last_stamp = stamp;
            // 在直播情况下，时间戳增量不得大于MAX_DELTA_STAMP，否则强制相对时间戳加1
            return ret < MAX_DELTA_STAMP ? ret : 1;
        }

        // 时间戳增量为负，说明时间戳回环了或回退了
        _last_stamp = stamp;

        // 如果时间戳回退不多，那么返回负值，否则返回加1
        return -ret < MAX_CTS ? ret : 1;
    }
    int64_t relativeStamp(int64_t stamp) {
        _relative_stamp += deltaStamp(stamp);
        return _relative_stamp;
    }
    int64_t relativeStamp() {
        return _relative_stamp;
    }

private:
    int64_t _last_stamp = 0;
    int64_t _relative_stamp = 0;
};

//该类解决时间戳回环、回退问题
//计算相对时间戳或者产生平滑时间戳
class Stamp : public DeltaStamp {
public:
    Stamp() {};
    ~Stamp() {};

    /**
     * 求取相对时间戳,同时实现了音视频同步、限制dts回退等功能
     * @param dts 输入dts，如果为0则根据系统时间戳生成
     * @param pts 输入pts，如果为0则等于dts
     * @param dts_out 输出dts
     * @param pts_out 输出pts
     * @param modifyStamp 是否用系统时间戳覆盖
     */
    void revise(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        revise_l(dts, pts, dts_out, pts_out, modifyStamp);
        if (_playback) {
            // 回放允许时间戳回退
            return;
        }

        if (dts_out < _last_dts_out) {
            // WarnL << "dts回退:" << dts_out << " < " << _last_dts_out;
            dts_out = _last_dts_out;
            pts_out = _last_pts_out;
            return;
        }
        _last_dts_out = dts_out;
        _last_pts_out = pts_out;
    }

    /**
     * 再设置相对时间戳，用于seek用
     * @param relativeStamp 相对时间戳
     */
    void setRelativeStamp(int64_t relativeStamp) {
        _relative_stamp = relativeStamp;
    }

    /**
     * 获取当前相对时间戳
     * @return
     */
    int64_t getRelativeStamp() const {
        return _relative_stamp;
    }

    /**
     * 设置是否为回放模式，回放模式运行时间戳回退
     * @param playback 是否为回放模式
     */
    void setPlayBack(bool playback = true) {
        _playback = playback;
    }


    /**
     * 音视频同步用，音频应该同步于视频(只修改音频时间戳)
     * 因为音频时间戳修改后不影响播放速度
     */
    void syncTo(Stamp& other) {
        _sync_master = &other;
    }

private:
    //主要实现音视频时间戳同步功能
    void revise_l(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        revise_l2(dts, pts, dts_out, pts_out, modifyStamp);
        if (!_sync_master || modifyStamp || _playback) {
            // 自动生成时间戳或回放或同步完毕
            return;
        }

        if (_sync_master && _sync_master->_last_dts_in) {
            // 音视频dts当前时间差
            int64_t dts_diff = _last_dts_in - _sync_master->_last_dts_in;
            if (ABS(dts_diff) < 5000) {
                // 如果绝对时间戳小于5秒，那么说明他们的起始时间戳是一致的，那么强制同步
                _relative_stamp = _sync_master->_relative_stamp + dts_diff;
            }
            // 下次不用再强制同步
            _sync_master = nullptr;
        }
    }


    //主要实现获取相对时间戳功能
    void revise_l2(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        if (!pts) {
            // 没有播放时间戳,使其赋值为解码时间戳
            pts = dts;
        }

        if (_playback) {
            // 这是点播
            dts_out = dts;
            pts_out = pts;
            _relative_stamp = dts_out;
            _last_dts_in = dts;
            return;
        }

        // pts和dts的差值
        int64_t pts_dts_diff = pts - dts;

        if (_last_dts_in != dts) {
            // 时间戳发生变更
            if (modifyStamp) {
                // 内部自己生产时间戳
                _relative_stamp = _ticker.elapsedTime();
            }
            else {
                _relative_stamp += deltaStamp(dts);
            }
            _last_dts_in = dts;
        }
        dts_out = _relative_stamp;

        //////////////以下是播放时间戳的计算//////////////////
        if (ABS(pts_dts_diff) > MAX_CTS) {
            // 如果差值太大，则认为由于回环导致时间戳错乱了
            pts_dts_diff = 0;
        }

        pts_out = dts_out + pts_dts_diff;
    }

private:
    int64_t _relative_stamp = 0;
    int64_t _last_dts_in = 0;
    int64_t _last_dts_out = 0;
    int64_t _last_pts_out = 0;
    toolkit::SmoothTicker _ticker;
    bool _playback = false;
    Stamp* _sync_master = nullptr;
};


#endif	// __X2_RTC_TIME_SYNC_H__
