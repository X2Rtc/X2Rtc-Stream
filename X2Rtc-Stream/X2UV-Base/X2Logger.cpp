#define MS_CLASS "X2Logger"
// #define MS_LOG_DEV_LEVEL 3

#include "X2Logger.hpp"
#include <uv.h>

/* Class variables. */

const uint64_t X2Logger::pid{ static_cast<uint64_t>(uv_os_getpid()) };
thread_local X2LogCallback* X2Logger::channel{ nullptr };
thread_local char X2Logger::buffer[X2Logger::bufferSize];

/* Class methods. */

void X2Logger::ClassInit(X2LogCallback* channel)
{
	X2Logger::channel = channel;

	MS_TRACE();
}
