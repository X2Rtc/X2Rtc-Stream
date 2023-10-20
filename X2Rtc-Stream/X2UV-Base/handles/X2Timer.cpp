#define MS_CLASS "X2Timer"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/X2Timer.hpp"
#include "X2Logger.hpp"
#include "X2MediaSoupErrors.hpp"

/* Static methods for UV callbacks. */

inline static void onTimer(uv_timer_t* handle)
{
	static_cast<X2Timer*>(handle->data)->OnUvTimer();
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

X2Timer::X2Timer(Listener* listener, uv_loop_t* uvLoop) : listener(listener)
{
	MS_TRACE();

	this->uvHandle       = new uv_timer_t;
	this->uvHandle->data = static_cast<void*>(this);

	const int err = uv_timer_init(uvLoop, this->uvHandle);

	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_timer_init() failed: %s", uv_strerror(err));
	}
}

X2Timer::~X2Timer()
{
	MS_TRACE();

	if (!this->closed)
	{
		Close();
	}
}

void X2Timer::Close()
{
	MS_TRACE();

	if (this->closed)
	{
		return;
	}

	this->closed = true;

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void X2Timer::Start(uint64_t timeout, uint64_t repeat)
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	this->timeout = timeout;
	this->repeat  = repeat;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		Stop();
	}

	const int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), timeout, repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

void X2Timer::Stop()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	const int err = uv_timer_stop(this->uvHandle);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_stop() failed: %s", uv_strerror(err));
	}
}

void X2Timer::Reset()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) == 0)
	{
		return;
	}

	if (this->repeat == 0u)
	{
		return;
	}

	const int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->repeat, this->repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

void X2Timer::Restart()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		Stop();
	}

	const int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->timeout, this->repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

inline void X2Timer::OnUvTimer()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->OnTimer(this);
}
