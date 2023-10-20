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
#ifndef __X2WHIP_TCP_CONNECTION_H__
#define __X2WHIP_TCP_CONNECTION_H__
#include "RTC/TcpConnection.hpp"
#include "common.hpp"
#include "handles/TcpConnectionHandler.hpp"

namespace x2rtc
{
	class X2WhipTcpConnection : public RTC::TcpConnection {
	public:
		X2WhipTcpConnection(RTC::TcpConnection::Listener* listener, size_t bufferSize);
		virtual ~X2WhipTcpConnection(void);

	public:
		void SendResponse(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2);

		/* Pure virtual methods inherited from ::TcpConnectionHandler. */
	public:
		void UserOnTcpConnectionRead() override;

	private:
		// Others.
		size_t frameStart{ 0u }; // Where the latest frame starts.

		RTC::TcpConnection::Listener* cb_listener_;
	};
	
} // namespace x2rtc

#endif	// __X2WHIP_TCP_CONNECTION_H__
