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
#ifndef __X2NET_HTTP_CONNECTION_H__
#define __X2NET_HTTP_CONNECTION_H__
#include "X2NetConnection.h"
#include "X2common.hpp"
#include "handles/X2TcpConnectionHandler.hpp"
#include <map>

namespace x2rtc
{
struct X2HttpReq
{
	std::string strUrl;
	std::map<std::string, std::string>mapHeaders;
	std::string strBody;

	std::string strLastHdr;
};

class X2HttpConnection : public ::X2TcpConnectionHandler
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual int OnHttpConnectionPacketProcess(
			x2rtc::X2HttpConnection* connection, const uint8_t* data, size_t len) {
			return len;
		};
	};

public:
	X2HttpConnection(Listener* listener, size_t bufferSize);
	~X2HttpConnection() override;

public:
	void SendResponse(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2);

	/* Pure virtual methods inherited from ::TcpConnectionHandler. */
public:
	void UserOnTcpConnectionRead() override;

private:
	// Passed by argument.
	Listener* listener{ nullptr };
	// Others.
	size_t frameStart{ 0u }; // Where the latest frame starts.
};
} // namespace x2rtc

#endif
