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
#ifndef __X2NET_TCP_CONNECTION_H__
#define __X2NET_TCP_CONNECTION_H__
#include "X2NetConnection.h"
#include "X2common.hpp"
#include "handles/X2TcpConnectionHandler.hpp"
#include "Libuv.h"

namespace x2rtc
{
	class X2TcpDataHanlder : public ::X2TcpConnectionHandler {
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnX2TcpDataHanlderPacketReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData) = 0;
			virtual void OnX2TcpDataHanlderClosed() = 0;
		};
	public:
		X2TcpDataHanlder(size_t bufferSize);
		virtual ~X2TcpDataHanlder(void);

		void SetUserData(void* userData) {
			user_data_ = userData;
		}
		void* GetUserData() {
			return user_data_;
		}

		void SetListener(Listener* listener);
		void SetClosed();

	public:
		/* Pure virtual methods inherited from ::TcpConnectionHandler. */
		virtual void UserOnTcpConnectionRead();

	private:
		// Others.
		size_t frameStart{ 0u }; // Where the latest frame starts.

		Listener* cb_listener_;
		void* user_data_;
	};
	
	class X2NetTcpConnection : public X2NetConnection, public X2TcpDataHanlder::Listener
	{
	public:
		X2NetTcpConnection();
		~X2NetTcpConnection() override;

		void SetX2TcpDataHanlder(X2TcpDataHanlder* handler);

		//* For X2NetConnection
		virtual bool CanDelete();

		//* For X2TcpDataHanlder::Listener
		virtual void OnX2TcpDataHanlderPacketReceived(x2rtc::scoped_refptr<X2NetDataRef> x2NetData);
		virtual void OnX2TcpDataHanlderClosed();

		int RunOnce();

	private:
		bool b_sock_closed_;
		uS::Timer* uv_timer_;
		X2TcpDataHanlder* x2tcp_data_handler_;
	};
} // namespace x2rtc

#endif	// __X2NET_TCP_CONNECTION_H__
