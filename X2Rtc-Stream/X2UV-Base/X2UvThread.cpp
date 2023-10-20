#include "X2UvThread.h"
#include "X2Log.hpp"
#include "X2Logger.hpp"
#include "X2Utils.hpp"


#define MS_CLASS "X2UvThread-inst"

static void StaticWorkerFun(void* arg) {
	MS_lOGF();
	static_cast<X2UvThread*>(arg)->WorkerFun();
}
static void StaticAsync(uv_async_t* handle) {
	MS_lOGF();
	X2UvThread* theclass = (X2UvThread*)handle->data;

	theclass->Async(handle);
}

X2UvThread::X2UvThread(void)
	: m_uv_loop_(NULL)
{
	
}
X2UvThread::~X2UvThread(void)
{
	
}

int X2UvThread::StartThread(int nTimerMs)
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
int X2UvThread::StopThread()
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

void X2UvThread::OnTimer(X2Timer* timer)
{
	OnUvThreadTimer();

	if (!m_isUserAskforclosed) {
		p_timer_->Restart();
	}
}

void X2UvThread::WorkerFun()
{
	{//Start
		// Initialize libuv stuff (we need it for the Channel).
		if (m_uv_loop_ == NULL) {
			m_uv_loop_ = uS::Loop::createLoop(false);
		}
		OnUvThreadStart();
		X2Utils::Crypto::ClassInit();
		uv_async_init(m_uv_loop_, &m_async, StaticAsync);
		m_async.data = this;
		if (n_timer_ms_ > 0) {
			p_timer_ = new X2Timer(this, m_uv_loop_);
			p_timer_->Start(n_timer_ms_, 0);
		}
	}

	if (n_single_port_rtc_ != 0) {
		std::string strIp = "0.0.0.0";
		single_port_rtc_ = new X2SinglePort(strIp, n_single_port_rtc_);
	}

	{//Run
		MS_DEBUG_DEV("starting libuv loop");
		m_uv_loop_->run();
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
		if (m_uv_loop_ != NULL) {
			m_uv_loop_->destroy();
			m_uv_loop_ = NULL;
		}

		X2Utils::Crypto::ClassDestroy();
	}

}

void X2UvThread::Async(uv_async_t* handle)
{
	MS_lOGF();

	if (m_isUserAskforclosed == true) {
		uv_stop(handle->loop);
		return;
	}
}

void X2UvThread::SetRtcSinglePort(int nPort)
{
	n_single_port_rtc_ = nPort;
}
int X2UvThread::GetRtcSinglePort()
{
	return n_single_port_rtc_;
}
int X2UvThread::GetRtcLocalPort()
{
	if (single_port_rtc_ == NULL)
		return 0;
	return single_port_rtc_->LocalMapPort();
}
X2SinglePort* X2UvThread::RtcSinglePortPtr()
{
	return single_port_rtc_;
}
