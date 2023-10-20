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
#ifndef __X2_RTC_THREAD_POOL_H__
#define __X2_RTC_THREAD_POOL_H__
#include <stdint.h>
#include <list>
#include <map>
#include <vector>
#include "jthread.h"
#include "jmutexautolock.h"
#include "XUtil.h"
#include "X2RtcRef.h"
#include "RefCountedObject.h"
#include "X2RtcCheck.h"

namespace x2rtc {

typedef std::map<std::string, std::string>MapStr;
typedef std::map<std::string, int>MapInt;
struct FuncEvent
{
	FuncEvent(void) :bWaitRun(false), nRetVal(0) {
		nRetVal = true;
		nRetVal = -10001;
	}
	std::string	strFuncName;
	MapStr	mapStr;
	MapInt	mapInt;

	bool bWaitRun;
	int32_t nRetVal;
};
TemplateX2RtcRef(FuncEvent);


class X2RtcTick
{
public:
	X2RtcTick(void) {};
	virtual ~X2RtcTick(void) {};

	virtual void OnX2RtcTick() = 0;
	virtual void OnFuncEvent(scoped_refptr<FuncEventRef>funcEvent) {
		// Not implement
		funcEvent->bWaitRun = false;
	};

	void CheckFuncEvent() {
		scoped_refptr<FuncEventRef>funcEvent;
		{
			JMutexAutoLock l(cs_list_func_event_);
			if (list_func_event_.size() > 0) {
				funcEvent = list_func_event_.front();
				list_func_event_.pop_front();
			}
		}
		if (funcEvent != NULL) {
			OnFuncEvent(funcEvent);
		}
	}

	void RunFuncEvent(scoped_refptr<FuncEventRef>funcEvent) {
		{
			JMutexAutoLock l(cs_list_func_event_);
			list_func_event_.push_back(funcEvent);
		}
		while (funcEvent->bWaitRun) {
			XSleep(1);
		}
	}

private:
	JMutex cs_list_func_event_;
	std::list<scoped_refptr<FuncEventRef>> list_func_event_;
};

typedef std::map<void*, X2RtcTick*> MapX2RtcTick;

class X2RtcThread : public JThread
{
public:
	X2RtcThread(void)
		: b_running_(false)
		, b_wait_stop_(false)
#ifdef _WIN32
		, n_sleep_time_(1)
#else
		, n_sleep_time_(10)
#endif
	{
		
	};
	virtual ~X2RtcThread(void)
	{
		StopThread(false);
	};

	void StartThread() {
		b_running_ = true;
		//JThread::SetName("XThreadMgrThread", this);
		JThread::Start();
	}

	void StopThread(bool bWaitStop = true) {
		if (b_running_) {
			b_running_ = false;
			b_wait_stop_ = bWaitStop;
			while (b_wait_stop_) {
				XSleep(1);
			}
			JThread::Kill();
		}
	}

	//* For X2RtcThread
	virtual void OnRtcThreadStart() {};
	virtual void OnRtcThreadStop() {};
	virtual void OnRtcThreadTick() {};

public:
	virtual void RegisteX2RtcTick(void* ptr, X2RtcTick* x2RtcTick)
	{
		JMutexAutoLock l(cs_rtc_tick_);
		if (map_x2rtc_tick_.find(ptr) != map_x2rtc_tick_.end()) {
			map_x2rtc_tick_.erase(ptr);
		}

		map_x2rtc_tick_[ptr] = x2RtcTick;
	}
	virtual bool UnRegisteX2RtcTick(void* ptr)
	{
		JMutexAutoLock l(cs_rtc_tick_);
		if (map_x2rtc_tick_.find(ptr) != map_x2rtc_tick_.end()) {
			map_x2rtc_tick_.erase(ptr);
			return true;
		}
		return false;
	}

	virtual void* Thread()
	{
		JThread::ThreadStarted();

		OnRtcThreadStart();

		while (b_running_) {

			{
				JMutexAutoLock l(cs_rtc_tick_);
				MapX2RtcTick::iterator iter = map_x2rtc_tick_.begin();
				while (iter != map_x2rtc_tick_.end()) {
					iter->second->OnX2RtcTick();
					iter->second->CheckFuncEvent();
					iter++;
				}
			}

			OnRtcThreadTick();

			XSleep(n_sleep_time_);
		}

		OnRtcThreadStop();
		b_wait_stop_ = false;
		return NULL;
	}
private:
	bool			b_running_;
	bool			b_wait_stop_;
	int32_t			n_sleep_time_;

protected:
	JMutex cs_rtc_tick_;
	MapX2RtcTick		map_x2rtc_tick_;
};
TemplateX2RtcRef(X2RtcThread);

class X2RtcThreadPool
{// Not thread safe, should use all function in main thread
public:
	X2RtcThreadPool(void): b_inited_(false), n_thread_num_(0), n_thread_cur_(0), n_next_tick_time_(0){};
	virtual ~X2RtcThreadPool(void) {};

	static X2RtcThreadPool& Inst() {
		static X2RtcThreadPool gMgr;
		return gMgr;
	}
	
	int Init(int nThreadNum) {
		if (b_inited_) {
			return -1;
		}
		if (nThreadNum <= 0 || nThreadNum > 128) {
			return -2;
		}
		b_inited_ = true;
		n_next_tick_time_ = XGetUtcTimestamp();
		n_thread_num_ = nThreadNum;
		for (int i = 0; i < n_thread_num_; i++) {
			scoped_refptr<X2RtcThreadRef> rtcThread = new x2rtc::RefCountedObject<X2RtcThreadRef>();
			rtcThread->StartThread();
			vec_x2rtc_threads_.push_back(rtcThread);
		}
		return 0;
	};

	int DeInit() {
		if (!b_inited_) {
			return -1;
		}
		b_inited_ = false;
		n_next_tick_time_ = 0;
		vec_x2rtc_threads_.clear();
		return 0;
	};

	void RunOnce() {
		if(n_next_tick_time_!= 0 && n_next_tick_time_<= XGetUtcTimestamp())
		{
			n_next_tick_time_ = XGetUtcTimestamp() + 10;

			JMutexAutoLock l(cs_rtc_tick_);
			MapX2RtcTick::iterator iter = map_x2rtc_tick_.begin();
			while (iter != map_x2rtc_tick_.end()) {
				iter->second->OnX2RtcTick();
				iter++;
			}
		}
	}

	void RegisteMainX2RtcTick(void* ptr, X2RtcTick* x2RtcTick)
	{
		JMutexAutoLock l(cs_rtc_tick_);
		if (map_x2rtc_tick_.find(ptr) != map_x2rtc_tick_.end()) {
			map_x2rtc_tick_.erase(ptr);
		}

		map_x2rtc_tick_[ptr] = x2RtcTick;
	}
	bool UnRegisteMainX2RtcTick(void* ptr)
	{
		JMutexAutoLock l(cs_rtc_tick_);
		if (map_x2rtc_tick_.find(ptr) != map_x2rtc_tick_.end()) {
			map_x2rtc_tick_.erase(ptr);
			return true;
		}
		return false;
	}

	void RegisteX2RtcTick(void* ptr, X2RtcTick* x2RtcTick) {
		//Must be used before Init:success
		X2RTC_CHECK(b_inited_);
		uint32_t nCurPos = n_thread_cur_++;
		nCurPos = nCurPos % n_thread_num_;
		vec_x2rtc_threads_[nCurPos]->RegisteX2RtcTick(ptr, x2RtcTick);
	}
	void UnRegisteX2RtcTick(void* ptr) {
		//Must be used before Init:success
		X2RTC_CHECK(b_inited_);
		for (int i = 0; i < n_thread_num_; i++) {
			if (vec_x2rtc_threads_[i]->UnRegisteX2RtcTick(ptr)) {
				break;
			}
		}
	}

private:
	bool b_inited_;
	uint32_t n_thread_num_;
	uint32_t n_thread_cur_;
	int64_t	n_next_tick_time_;

	std::vector<scoped_refptr<X2RtcThreadRef>> vec_x2rtc_threads_;

private:
	JMutex cs_rtc_tick_;
	MapX2RtcTick		map_x2rtc_tick_;
};


}	// namespace x2rtc
#endif //__X2_RTC_THREAD_POOL_H__