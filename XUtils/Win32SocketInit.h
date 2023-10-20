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
#ifndef WIN32_SOCKET_INIT_H_
#define WIN32_SOCKET_INIT_H_

#ifdef _WIN32
#include <Windows.h>
#endif

namespace x2rtc {

class WinsockInitializer {
 public:
  WinsockInitializer():err_(0){
#ifdef _WIN32
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1, 0);
    err_ = WSAStartup(wVersionRequested, &wsaData);
#endif
  }
  ~WinsockInitializer() {
#ifdef _WIN32
    if (!err_)
      WSACleanup();
#endif
  }
  int error() { return err_; }

 private:
  int err_;
};

}  // namespace rtc

#endif  // RTC_BASE_WIN32_SOCKET_INIT_H_
