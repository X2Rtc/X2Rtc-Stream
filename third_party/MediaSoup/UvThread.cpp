#include "UvThread.h"
#include "DepLibUV.hpp"
#include "Log.hpp"
#include "Logger.hpp"

#define MS_CLASS "UvThread-inst"

static void StaticWorkerFun(void* arg) {
	MS_lOGF();
	static_cast<UvThread*>(arg)->WorkerFun();
}
static void StaticAsync(uv_async_t* handle) {
	MS_lOGF();
	UvThread* theclass = (UvThread*)handle->data;

	theclass->Async(handle);
}

UvThread::UvThread(void)
{

}
UvThread::~UvThread(void)
{

}

int UvThread::StartThread(int nTimerMs)
{
	if (m_workThread == NULL)
	{
		n_timer_ms_ = nTimerMs;
		// run loop
		int ret = uv_thread_create(&m_workThread, StaticWorkerFun, this);
		if (0 != ret) {
			StopThread();
			return 1;
		}
	}


	return 0;
}
int UvThread::StopThread()
{
	{
		m_isUserAskforclosed = true;

		if (m_workThread != NULL) {
			// notify quit
			if (m_isUserAskforclosed) {
				uv_async_send(&m_async);
			}
			MS_lOGI("wait work thread quit");
			uv_thread_join(&m_workThread);
			m_workThread = NULL;
		}
	}

	return 0;
}

void UvThread::OnTimer(Timer* timer)
{
	OnUvThreadTimer();

	if (!m_isUserAskforclosed) {
		p_timer_->Restart();
	}
}

void UvThread::WorkerFun()
{
	{//Start
		// Initialize libuv stuff (we need it for the Channel).
		DepLibUV::ClassInit();
		OnUvThreadStart();
		uv_async_init(DepLibUV::GetLoop(), &m_async, StaticAsync);
		m_async.data = this;
		if (n_timer_ms_ > 0) {
			p_timer_ = new Timer(this);
			p_timer_->Start(n_timer_ms_, 0);
		}
	}

	if (n_single_port_rtc_ != 0) {
		std::string strIp = "0.0.0.0";
		single_port_rtc_ = new SinglePort(strIp, n_single_port_rtc_);
	}

	{//Run
		MS_DEBUG_DEV("starting libuv loop");
		DepLibUV::RunLoop();
		MS_DEBUG_DEV("libuv loop ended");
	}

	if (single_port_rtc_ != NULL) {
		delete single_port_rtc_;
		single_port_rtc_ = NULL;
	}

	{//Stop
		if (p_timer_ != NULL) {
			p_timer_->Stop();
			delete p_timer_;
			p_timer_ = NULL;
		}

		//StopThread();
		DepLibUV::ClassDestroy();
	}

}

void UvThread::Async(uv_async_t* handle)
{
	MS_lOGF();

	if (m_isUserAskforclosed == true) {
		uv_stop(handle->loop);
		return;
	}
}

void UvThread::SetRtcSinglePort(int nPort)
{
	n_single_port_rtc_ = nPort;
}
int UvThread::GetRtcSinglePort()
{
	return n_single_port_rtc_;
}
int UvThread::GetRtcLocalPort()
{
	if (single_port_rtc_ == NULL)
		return 0;
	return single_port_rtc_->LocalMapPort();
}
SinglePort* UvThread::RtcSinglePortPtr()
{
	return single_port_rtc_;
}
