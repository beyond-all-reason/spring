/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeUtil.h"

#include "System/StringUtil.h"
#include "System/Exceptions.h"

#include <string>
#include <functional>
#include <chrono>

#include "lib/fmt/printf.h"

std::string CTimeUtil::GetCurrentTimeStr(bool utc)
{
	//https://stackoverflow.com/a/35157784/9819318

	auto now = std::chrono::system_clock::now();
	// get number of milliseconds for the current second
	// (remainder after division into seconds)
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

	// convert to std::time_t in order to convert to std::tm (broken time)
	auto timer = std::chrono::system_clock::to_time_t(now);

	// convert to broken time
	static decltype(std::gmtime)* ConvertFunc[] = { &std::localtime, &std::gmtime };
	std::tm lt = *ConvertFunc[utc](&timer);

	return fmt::sprintf("%04i%02i%02i_%02i%02i%02i_%03i",
		lt.tm_year + 1900,
		lt.tm_mon + 1,
		lt.tm_mday,
		lt.tm_hour,
		lt.tm_min,
		lt.tm_sec,
		static_cast<int>(ms)
	);
}
