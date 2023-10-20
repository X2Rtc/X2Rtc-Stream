#ifndef __UV_THREAD_H__
#define __UV_THREAD_H__
#include "handles/Timer.hpp"
#include "SinglePort.h"

#define TmpUvThreadImpl \
virtual void OnUvThreadStart();	\
virtual void OnUvThreadStop();	\
virtual void OnUvThreadTimer();

class UvThread : public Timer::Listener
{
public:
	UvThread(void);
	virtual ~UvThread(void);

	int StartThread(int nTimerMs);
	int StopThread();

	virtual void OnUvThreadStart() = 0;
	virtual void OnUvThreadStop() = 0;
	virtual void OnUvThreadTimer() = 0;

public:
	//* For UvTimer::Listener
	virtual void OnTimer(Timer* timer); 
	void WorkerFun();
	void Async(uv_async_t* handle);

public:
	void SetRtcSinglePort(int nPort);
	int GetRtcSinglePort();
	int GetRtcLocalPort();
	SinglePort* RtcSinglePortPtr();

private:
	SinglePort* single_port_rtc_{ NULL };
	int n_single_port_rtc_{ 0 };

private:
	uv_thread_t m_workThread{ NULL };
	uv_async_t m_async;

	int n_timer_ms_{ 0 };
	Timer* p_timer_{ NULL };

	bool m_isUserAskforclosed{ false };
};


#endif	// __UV_THREAD_H__