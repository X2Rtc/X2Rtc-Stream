#include "X2RtcLog.h"
#include "spdlog/spdlog.h"
#include <spdlog/sinks/rotating_file_sink.h>

int gLogLevel = INF;
std::shared_ptr<spdlog::logger> gLogger = NULL;

void OpenX2RtcLog(const char*strPath, int nLevel, int nMaxFileSize)
{
	if (gLogger == NULL) {
		gLogger = spdlog::rotating_logger_mt("[X2Rtc]", strPath, nMaxFileSize*1024*1024, 2);
		gLogger->set_level((spdlog::level::level_enum)nLevel);
	}
	
}
void CloseX2RtcLog()
{
	if (gLogger != NULL) {
		gLogger = NULL;

		spdlog::shutdown();
	}
}

bool ShouldLog(int nLevel)
{
	if (gLogger == NULL)
		return false;
	return gLogger->should_log((spdlog::level::level_enum)nLevel);
}
int X2RtcLog(int nLevel, const char*strMsg)
{
	if (!ShouldLog(nLevel))
		return -1;

	if (gLogger != NULL) {
		switch (nLevel)
		{
		case spdlog::level::level_enum::trace:
		{
			gLogger->trace(strMsg);
		}
		break;
		case spdlog::level::level_enum::debug:
		{
			gLogger->debug(strMsg);
		}
		break;
		case spdlog::level::level_enum::info:
		{
			gLogger->info(strMsg);
		}
		break;
		case spdlog::level::level_enum::warn:
		{
			gLogger->warn(strMsg);
		}
		break;
		case spdlog::level::level_enum::err:
		{
			gLogger->error(strMsg);
		}
		break;
		case spdlog::level::level_enum::critical:
		{
			gLogger->critical(strMsg);
		}
		break;
		default:
			return -1;
		};
		gLogger->flush();
	}
#ifdef WIN32
	if (gLogLevel <= INF) {
		printf("[X2Rtc] (%d): %s\r\n", nLevel, strMsg);
	}
#endif
	return 0;
}

int X2RtcPrintf(int nLevel, const char *fmt, ...)
{
	if (!ShouldLog(nLevel))
		return -1;

	char buffer[2048];
	va_list argptr;
	va_start(argptr, fmt);
	int ret = vsnprintf(buffer,2047, fmt, argptr);
	va_end(argptr);

	if (ret > 0) {
		X2RtcLog(nLevel, buffer);
	}
	return 0;
}