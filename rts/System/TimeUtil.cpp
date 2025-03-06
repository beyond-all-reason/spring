/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeUtil.h"

#include "System/StringUtil.h"
#include "System/Exceptions.h"

#include <string>
#include <functional>
#include <chrono>
#include <array>

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

	return fmt::sprintf("%04i-%02i-%02i_%02i-%02i-%02i-%03i",
		lt.tm_year + 1900,
		lt.tm_mon + 1,
		lt.tm_mday,
		lt.tm_hour,
		lt.tm_min,
		lt.tm_sec,
		static_cast<int>(ms)
	);
}

__time64_t CTimeUtil::NTFSTimeToTime64(uint32_t lDataTimePart, uint32_t hDataTimePart)
{
    struct ST {
        uint16_t wYear;
        uint16_t wMonth;
        uint16_t wDayOfWeek;
        uint16_t wDay;
        uint16_t wHour;
        uint16_t wMinute;
        uint16_t wSecond;
        uint16_t wMilliseconds;
    };

    ST systemTime = {};

    // Constants
    static constexpr uint64_t TICKS_PER_SECOND = 10000000ULL;
    static constexpr uint64_t TICKS_PER_MILLISECOND = 10000ULL;
    static constexpr uint64_t SECONDS_PER_DAY = 86400ULL;  // 24 * 60 * 60
    static constexpr uint64_t SECONDS_PER_HOUR = 3600ULL;
    static constexpr uint64_t SECONDS_PER_MINUTE = 60ULL;

    // Days in each month (non-leap year)
    static constexpr std::array<int, 12> DAYS_IN_MONTH = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Combine the 64-bit value
    uint64_t fileTimeValue = (static_cast<uint64_t>(hDataTimePart) << 32) | lDataTimePart;

    // Check if the value is too large (Windows API limitation)
    if (fileTimeValue >= 0x8000000000000000ULL) {
        return 0; // Return invalid value
    }

    // Convert to seconds and remaining ticks
    uint64_t seconds = fileTimeValue / TICKS_PER_SECOND;
    uint64_t remainingTicks = fileTimeValue % TICKS_PER_SECOND;

    // Calculate milliseconds
    systemTime.wMilliseconds = static_cast<uint16_t>(remainingTicks / TICKS_PER_MILLISECOND);

    // Calculate days and time
    uint64_t days = seconds / SECONDS_PER_DAY;
    uint64_t secondsOfDay = seconds % SECONDS_PER_DAY;

    // Set time components
    systemTime.wHour = static_cast<uint16_t>(secondsOfDay / SECONDS_PER_HOUR);
    secondsOfDay %= SECONDS_PER_HOUR;
    systemTime.wMinute = static_cast<uint16_t>(secondsOfDay / SECONDS_PER_MINUTE);
    systemTime.wSecond = static_cast<uint16_t>(secondsOfDay % SECONDS_PER_MINUTE);


    static auto IsLeapYear = [](int year) {
        return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
    };

    // Calculate date components
    int year = 1601;
    int month = 1;

    // Fast forward through complete years
    while (true) {
        uint64_t daysInYear = IsLeapYear(year) ? 366 : 365;
        if (days < daysInYear)
            break;
        days -= daysInYear;
        year++;
    }

    // Calculate month
    while (days >= DAYS_IN_MONTH[month - 1]) {
        // Adjust February for leap years
        if (month == 2 && IsLeapYear(year)) {
            if (days < 29)
                break;
            days -= 29;
        }
        else {
            days -= DAYS_IN_MONTH[month - 1];
        }
        month++;
    }

    // Set date components
    systemTime.wYear = static_cast<uint16_t>(year);
    systemTime.wMonth = static_cast<uint16_t>(month);
    systemTime.wDay = static_cast<uint16_t>(days + 1);  // 1-based day

    // Calculate day of week
    // Jan 1, 1601 was Monday (1)
    uint64_t totalDays = 0;
    for (int y = 1601; y < year; y++) {
        totalDays += IsLeapYear(y) ? 366 : 365;
    }
    for (int m = 1; m < month; m++) {
        if (m == 2 && IsLeapYear(year)) {
            totalDays += 29;
        }
        else {
            totalDays += DAYS_IN_MONTH[m - 1];
        }
    }
    totalDays += systemTime.wDay - 1;
    systemTime.wDayOfWeek = static_cast<uint16_t>((totalDays + 1) % 7);

    // Populate struct tm
    struct tm tm = { 0 };

    tm.tm_year = systemTime.wYear - 1900;
    tm.tm_mon = systemTime.wMonth - 1;
    tm.tm_mday = systemTime.wDay;
    tm.tm_hour = systemTime.wHour;
    tm.tm_min = systemTime.wMinute;
    tm.tm_sec = systemTime.wSecond;
    tm.tm_isdst = 0;

    return std::mktime(&tm);
}

__time64_t CTimeUtil::DosTimeToTime64(uint32_t dosTimeDate)
{
    // Extract date components
    int year = ((dosTimeDate >> 25) & 0x7F) + 1980;
    int month = (dosTimeDate >> 21) & 0x0F;
    int day = (dosTimeDate >> 16) & 0x1F;

    // Extract time components
    int hour = (dosTimeDate >> 11) & 0x1F;
    int minute = (dosTimeDate >> 5) & 0x3F;
    int second = (dosTimeDate & 0x1F) * 2; // DOS time has 2-second resolution

    // Populate struct tm
    struct tm tm = { 0 };

    tm.tm_year = year - 1900; // tm_year is years since 1900
    tm.tm_mon = month - 1;    // tm_mon is 0-based (0-11)
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = 0;

    return std::mktime(&tm);
}
