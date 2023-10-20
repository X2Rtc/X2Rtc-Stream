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
     * ����ʱ�������
     * @param stamp ����ʱ���
     * @return ʱ�������
     */
    int64_t deltaStamp(int64_t stamp) {
        if (!_last_stamp) {
            // ��һ�μ���ʱ�������,ʱ�������Ϊ0
            if (stamp) {
                _last_stamp = stamp;
            }
            return 0;
        }

        int64_t ret = stamp - _last_stamp;
        if (ret >= 0) {
            // ʱ�������Ϊ��������֮
            _last_stamp = stamp;
            // ��ֱ������£�ʱ����������ô���MAX_DELTA_STAMP������ǿ�����ʱ�����1
            return ret < MAX_DELTA_STAMP ? ret : 1;
        }

        // ʱ�������Ϊ����˵��ʱ����ػ��˻������
        _last_stamp = stamp;

        // ���ʱ������˲��࣬��ô���ظ�ֵ�����򷵻ؼ�1
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

//������ʱ����ػ�����������
//�������ʱ������߲���ƽ��ʱ���
class Stamp : public DeltaStamp {
public:
    Stamp() {};
    ~Stamp() {};

    /**
     * ��ȡ���ʱ���,ͬʱʵ��������Ƶͬ��������dts���˵ȹ���
     * @param dts ����dts�����Ϊ0�����ϵͳʱ�������
     * @param pts ����pts�����Ϊ0�����dts
     * @param dts_out ���dts
     * @param pts_out ���pts
     * @param modifyStamp �Ƿ���ϵͳʱ�������
     */
    void revise(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        revise_l(dts, pts, dts_out, pts_out, modifyStamp);
        if (_playback) {
            // �ط�����ʱ�������
            return;
        }

        if (dts_out < _last_dts_out) {
            // WarnL << "dts����:" << dts_out << " < " << _last_dts_out;
            dts_out = _last_dts_out;
            pts_out = _last_pts_out;
            return;
        }
        _last_dts_out = dts_out;
        _last_pts_out = pts_out;
    }

    /**
     * ���������ʱ���������seek��
     * @param relativeStamp ���ʱ���
     */
    void setRelativeStamp(int64_t relativeStamp) {
        _relative_stamp = relativeStamp;
    }

    /**
     * ��ȡ��ǰ���ʱ���
     * @return
     */
    int64_t getRelativeStamp() const {
        return _relative_stamp;
    }

    /**
     * �����Ƿ�Ϊ�ط�ģʽ���ط�ģʽ����ʱ�������
     * @param playback �Ƿ�Ϊ�ط�ģʽ
     */
    void setPlayBack(bool playback = true) {
        _playback = playback;
    }


    /**
     * ����Ƶͬ���ã���ƵӦ��ͬ������Ƶ(ֻ�޸���Ƶʱ���)
     * ��Ϊ��Ƶʱ����޸ĺ�Ӱ�첥���ٶ�
     */
    void syncTo(Stamp& other) {
        _sync_master = &other;
    }

private:
    //��Ҫʵ������Ƶʱ���ͬ������
    void revise_l(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        revise_l2(dts, pts, dts_out, pts_out, modifyStamp);
        if (!_sync_master || modifyStamp || _playback) {
            // �Զ�����ʱ�����طŻ�ͬ�����
            return;
        }

        if (_sync_master && _sync_master->_last_dts_in) {
            // ����Ƶdts��ǰʱ���
            int64_t dts_diff = _last_dts_in - _sync_master->_last_dts_in;
            if (ABS(dts_diff) < 5000) {
                // �������ʱ���С��5�룬��ô˵�����ǵ���ʼʱ�����һ�µģ���ôǿ��ͬ��
                _relative_stamp = _sync_master->_relative_stamp + dts_diff;
            }
            // �´β�����ǿ��ͬ��
            _sync_master = nullptr;
        }
    }


    //��Ҫʵ�ֻ�ȡ���ʱ�������
    void revise_l2(int64_t dts, int64_t pts, int64_t& dts_out, int64_t& pts_out, bool modifyStamp = false) {
        if (!pts) {
            // û�в���ʱ���,ʹ�丳ֵΪ����ʱ���
            pts = dts;
        }

        if (_playback) {
            // ���ǵ㲥
            dts_out = dts;
            pts_out = pts;
            _relative_stamp = dts_out;
            _last_dts_in = dts;
            return;
        }

        // pts��dts�Ĳ�ֵ
        int64_t pts_dts_diff = pts - dts;

        if (_last_dts_in != dts) {
            // ʱ����������
            if (modifyStamp) {
                // �ڲ��Լ�����ʱ���
                _relative_stamp = _ticker.elapsedTime();
            }
            else {
                _relative_stamp += deltaStamp(dts);
            }
            _last_dts_in = dts;
        }
        dts_out = _relative_stamp;

        //////////////�����ǲ���ʱ����ļ���//////////////////
        if (ABS(pts_dts_diff) > MAX_CTS) {
            // �����ֵ̫������Ϊ���ڻػ�����ʱ���������
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
