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
#ifndef __X2NET_TCP_CLIENT_H__
#define __X2NET_TCP_CLIENT_H__
#include <stdint.h>
#include "X2NetConnection.h"

namespace x2rtc {

class X2NetTcpClient 
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2NetTcpClientDataReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData) = 0;
		virtual void OnX2NetTcpClientConnected() = 0;
		virtual void OnX2NetTcpClientClosed() = 0;
		virtual void OnX2NetTcpClientFailed() = 0;
	};

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

protected:
	Listener* cb_listener_{ nullptr };

public:
	X2NetTcpClient(void) {};
	virtual ~X2NetTcpClient(void) {};

	virtual int Connect(const std::string& strIp, int nPort) = 0;
	virtual int ReConnect() = 0;  
	virtual int DisConnect() = 0;
	virtual int SendData(x2rtc::scoped_refptr<X2NetDataRef> x2NetData) = 0;
	virtual int RunOnce() = 0;
};

X2NetTcpClient* createX2NetTcpClient();

}	// namespace x2rtc
#endif	// __X2NET_TCP_CLIENT_H__