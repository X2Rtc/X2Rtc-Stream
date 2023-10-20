#ifndef MS_FUZZER_RTC_TREND_CALCULATOR_HPP
#define MS_FUZZER_RTC_TREND_CALCULATOR_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace TrendCalculator
		{
			void Fuzz(const uint8_t* data, size_t len);
		}
	} // namespace RTC
} // namespace Fuzzer

#endif
