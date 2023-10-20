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
#ifndef MS_X2RTC_HTTP_SERVER_HPP
#define MS_X2RTC_HTTP_SERVER_HPP

#include "X2common.hpp"
#include "X2NetTcpConnection.h"
#include "handles/X2TcpConnectionHandler.hpp"
#include "handles/X2TcpServerHandler.hpp"
#include "X2HttpConnection.h"
#include <string>

namespace x2rtc
{
	class X2HttpServer : public ::X2TcpServerHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			//@Eric - 20230612 - Can use self defined tcp connection.
			virtual void OnRtcTcpConnectionAccept(
				x2rtc::X2HttpServer* tcpServer, x2rtc::X2HttpConnection** connection) {};
			virtual void OnRtcTcpConnectionClosed(
			  x2rtc::X2HttpServer* tcpServer, x2rtc::X2HttpConnection* connection) = 0;
		};

	public:
		X2HttpServer(Listener* listener, x2rtc::X2HttpConnection::Listener* connListener);
		~X2HttpServer() override;

		bool Listen(std::string& ip, uint16_t port);

		/* Pure virtual methods inherited from ::TcpServerHandler. */
	public:
		void UserOnTcpConnectionAlloc() override;
		void UserOnTcpConnectionClosed(::X2TcpConnectionHandler* connection) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		x2rtc::X2HttpConnection::Listener* connListener{ nullptr };
		bool fixedPort{ false };
	};
} // namespace x2rtc

#endif
