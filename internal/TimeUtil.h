#pragma once

#include <fmt/chrono.h>
#include <ctime>
#include <string>

namespace ed
{
    inline std::string LocalUtcOffsetString(const std::chrono::system_clock::time_point& timePoint)
    {
        using namespace std::chrono;

        const zoned_time zt{ current_zone(), timePoint };
        const auto offset = zt.get_info().offset;
        auto totalMinutes = duration_cast<minutes>(offset).count();

        const char sign = totalMinutes < 0 ? '-' : '+';
        if (totalMinutes < 0)
        {
            totalMinutes = -totalMinutes;
        }

        const auto hours = totalMinutes / 60;
        const auto minutesPart = totalMinutes % 60;

        return fmt::format("{}{:02}:{:02}", sign, hours, minutesPart);
    }

    inline std::tm ToTm(std::time_t timeT, bool utcOrLocal)
    {
        std::tm tm{};
#if defined(_WIN32)
        if (utcOrLocal)
        {
            gmtime_s(&tm, &timeT);
        }
        else
        {
            localtime_s(&tm, &timeT);
        }
#else
        if (utcOrLocal)
        {
            gmtime_r(&timeT, &tm);
        }
        else
        {
            localtime_r(&timeT, &tm);
        }
#endif
        return tm;
    }

    inline std::string TimePointToString(const std::chrono::system_clock::time_point& timePoint,
        bool utcOrLocal,
        bool insertTBetweenDateAndTime,
        bool addTimeZone)
    {
        const auto timeT = std::chrono::system_clock::to_time_t(timePoint);
        const auto timeTm = ToTm(timeT, utcOrLocal);

        // Microseconds
        const auto sinceEpoch = timePoint.time_since_epoch();
        const auto microSec = std::chrono::duration_cast<std::chrono::microseconds>(sinceEpoch).count() % 1000000;

        std::string timeAsString;
        if (insertTBetweenDateAndTime)
        {
            timeAsString = fmt::format("{:%Y-%m-%dT%H:%M:%S}.{:06d}", timeTm, microSec);
        }
        else
        {
            timeAsString = fmt::format("{:%Y-%m-%d %H:%M:%S}.{:06d}", timeTm, microSec);
        }

        if (addTimeZone)
        {
            if (utcOrLocal)
            {
                timeAsString += "Z";
            }
            else
            {
                timeAsString += LocalUtcOffsetString(timePoint);
            }
        }

        return timeAsString;
    }

    inline std::string TimePointToStringAsUtc(const std::chrono::system_clock::time_point& timePoint,
        bool insertTBetweenDateAndTime,
        bool addTimeZone)
    {
        return TimePointToString(timePoint, true, insertTBetweenDateAndTime, addTimeZone);
    }

    inline std::string TimePointToStringAsLocal(const std::chrono::system_clock::time_point& timePoint,
        bool insertTBetweenDateAndTime,
        bool addTimeZone)
    {
        return TimePointToString(timePoint, false, insertTBetweenDateAndTime, addTimeZone);
    }
}
