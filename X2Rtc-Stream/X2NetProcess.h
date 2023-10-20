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
#ifndef __X2_NET_PROCESS_H__
#define __X2_NET_PROCESS_H__
#include <list>
#include <map>
#include "X2NetConnection.h"

namespace x2rtc {
#define X2ProcessType_MAP(X2T) \
	X2T(X2Process_Rtmp, 1, "RTMP")			\
	X2T(X2Process_Srt, 2, "SRT")			\
	X2T(X2Process_Rtc, 3, "RTC")			\
	X2T(X2Process_Rtp, 4, "RTP")			\
	X2T(X2Process_Http, 5, "HTTP")			\
	X2T(X2Process_Talk, 10, "TALK")


	enum X2ProcessType
	{
		X2Process_Invalid = -1,
#define X2T(name,  value, str) name = value,
		X2ProcessType_MAP(X2T)
#undef X2T
		X2Process_Max,
	};

	const char* getX2ProcessTypeName(X2ProcessType eType);

class X2NetProcess
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2NetProcessNewConnection(X2NetType x2NetType, X2NetConnection* connection) = 0;
		virtual void OnX2NetProcessSendMessage(const char* strMsg) {};
	};


	X2NetProcess(void):cb_listener_(NULL) {
	};
	virtual ~X2NetProcess(void) {
		{
			MapX2NetConnection::iterator itmr = map_x2net_connection_.begin();
			while (itmr != map_x2net_connection_.end()) {
				delete itmr->second;
				itmr = map_x2net_connection_.erase(itmr);
			}
		}
		{
			JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
			ListX2NetConnection::iterator itlr = list_x2net_connection_for_delete_.begin();
			while (itlr != list_x2net_connection_for_delete_.end()) {
				delete (*itlr);
				itlr = list_x2net_connection_for_delete_.erase(itlr);
			}
		}
	};
public:
	virtual int Init(int nPort) = 0;
	virtual int DeInit() = 0;
	virtual int SetParameter(const char* strJsParams) { return -1; };

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

	void SetNodeId(const std::string& strNodeId) {
		str_node_id_ = strNodeId;
	}

	virtual int RunOnce() {
		JMutexAutoLock l(cs_list_x2net_connection_for_delete_);
		ListX2NetConnection::iterator itlr = list_x2net_connection_for_delete_.begin();
		while (itlr != list_x2net_connection_for_delete_.end()) {
			if ((*itlr)->CanDelete()) {
				delete (*itlr);
				itlr = list_x2net_connection_for_delete_.erase(itlr);
			}
			else {
				itlr++;
			}
		}

		return 0;
	};

protected:
	Listener* cb_listener_;

	std::string str_node_id_;

	typedef std::map<int32_t, X2NetConnection*>MapX2NetConnection;
	typedef std::list<X2NetConnection*> ListX2NetConnection;

	//* Active connection - Run on common work/self thread
	MapX2NetConnection map_x2net_connection_;

	JMutex cs_list_x2net_connection_for_delete_;
	//* Closed connection that waitting for delete - Run on common work thread
	ListX2NetConnection list_x2net_connection_for_delete_;
};

X2NetProcess* createX2NetProcess(X2ProcessType eType);

}	// namespace x2rtc
#endif	// __X2_NET_PROCESS_H__