#pragma once

#include <fmt/chrono.h>
#include <string>

namespace ed
{
	inline std::string TimePointToString(const std::chrono::system_clock::time_point& timePoint,
		bool utcOrLocal,
		bool insertTBetweenDateAndTime,
		bool addTimeZone)
	{
		const auto timeT = std::chrono::system_clock::to_time_t(timePoint);
		const auto timeTm = utcOrLocal ? fmt::gmtime(timeT) : fmt::localtime(timeT);

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
				timeAsString += fmt::format("{:%z}", timeTm);
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
