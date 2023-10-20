#ifndef __X2_UV_THREAD_H__
#define __X2_UV_THREAD_H__
#include "handles/X2Timer.hpp"
#include "X2SinglePort.h"
#include "Libuv.h"

#define TmpUvThreadImpl \
virtual void OnUvThreadStart();	\
virtual void OnUvThreadStop();	\
virtual void OnUvThreadTimer();

class X2UvThread : public X2Timer::Listener
{
public:
	X2UvThread(void);
	virtual ~X2UvThread(void);

	int StartThread(int nTimerMs);
	int StopThread();

	virtual void OnUvThreadStart() = 0;
	virtual void OnUvThreadStop() = 0;
	virtual void OnUvThreadTimer() = 0;

public:
	//* For UvTimer::Listener
	virtual void OnTimer(X2Timer* timer); 
	void WorkerFun();
	void Async(uv_async_t* handle);

public:
	void SetRtcSinglePort(int nPort);
	int GetRtcSinglePort();
	int GetRtcLocalPort();
	X2SinglePort* RtcSinglePortPtr();

private:
	X2SinglePort* single_port_rtc_{ NULL };
	int n_single_port_rtc_{ 0 };

private:
	uv_thread_t m_workThread{ NULL };
	uv_async_t m_async;
	uS::Loop* m_uv_loop_;

	int n_timer_ms_{ 0 };
	X2Timer* p_timer_{ NULL };

	bool m_isUserAskforclosed{ false };
};


#endif	// __X2_UV_THREAD_H__