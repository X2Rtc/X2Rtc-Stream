#ifndef X2_MS_LOG_LEVEL_HPP
#define X2_MS_LOG_LEVEL_HPP

#include "X2common.hpp"

enum class LogLevel : uint8_t
{
	LOG_DEBUG = 3,
	LOG_WARN  = 2,
	LOG_ERROR = 1,
	LOG_NONE  = 0
};

#endif	// X2_MS_LOG_LEVEL_HPP
