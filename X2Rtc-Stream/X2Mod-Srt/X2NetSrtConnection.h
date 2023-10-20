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
#ifndef __X2_NET_SRT_CONNECTION_H__ 
#define __X2_NET_SRT_CONNECTION_H__
#include "X2NetConnection.h"
#include <sstream>
#include "srt.h"
#include "X2RtcLog.h"

#define SET_SRT_OPT_STR(srtfd, optname, buf, size)                                  \
    if (srt_setsockflag(srtfd, optname, buf, size) == SRT_ERROR) {                  \
        std::stringstream ss;                                                       \
        ss << "srtfd=" << srtfd << ",set " << #optname                              \
           << " failed,err=" << srt_getlasterror_str();                             \
        X2RtcPrintf(ERR, "[srt] %s", ss.str().c_str());            \
        return -1; \
    } 

#define SET_SRT_OPT(srtfd, optname, val)                                            \
    if (srt_setsockflag(srtfd, optname, &val, sizeof(val)) == SRT_ERROR) {          \
        std::stringstream ss;                                                       \
        ss << "srtfd=" << srtfd << ",set " << #optname << "=" << val                \
           << " failed,err=" << srt_getlasterror_str();                             \
        X2RtcPrintf(ERR, "[srt] %s", ss.str().c_str());            \
        return -1; \
    } 

#define GET_SRT_OPT(srtfd, optname, val)                                        \
    do {                                                                        \
        int size = sizeof(val);                                                 \
        if (srt_getsockflag(srtfd, optname, &val, &size) == SRT_ERROR) {        \
            std::stringstream ss;                                               \
            ss << "srtfd=" << srtfd << ",get " << #optname                      \
               << " failed,err=" << srt_getlasterror_str();                     \
            X2RtcPrintf(ERR, "[srt] %s", ss.str().c_str());            \
            return -1; \
        }                                                                       \
    } while (0)

namespace x2rtc {

class X2NetSrtConnection : public X2NetConnection
{
public:
	X2NetSrtConnection(void);
	virtual ~X2NetSrtConnection(void);

	//* For X2NetConnection
	virtual bool CanDelete();

public:
	int SetSocket(int poll, SRTSOCKET sock);

	SRTSOCKET GetSocket();

	void SetClosed();

	int RunOnce();

    int DoRead();

private:
	bool b_sock_closed_;
    int			srt_poller_;
	SRTSOCKET   srt_socket_;
};

}	// namespace x2rtc
#endif	// __X2_NET_SRT_CONNECTION_H__