#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "any_string_array.h"

namespace chr = std::chrono;

namespace ed
{
	template<typename Test_, template<typename...> class Ref_>
		struct IsSpecialization : std::false_type {};
	template<template<typename...> class Ref_, typename... Args_>
		struct IsSpecialization<Ref_<Args_...>, Ref_> : std::true_type {};

	template <class Duration_ >
		using IsDuration = IsSpecialization<Duration_, std::chrono::duration>;

	template<class, class = void>
		struct IsClock : std::false_type {};
	template<class Clock_>
		struct IsClock<Clock_, std::void_t<typename Clock_::rep, typename Clock_::period, typename Clock_::duration,
			typename Clock_::time_point, decltype(Clock_::is_steady), decltype(Clock_::now())>> : std::true_type {};

	template<typename Clock_, class Duration = typename Clock_::duration>
	[[nodiscard]] time_t to_time_t(const std::chrono::time_point<Clock_, Duration>& time) noexcept {
		static_assert(IsClock<Clock_>::value, "The given type for Clock_ does not fulfill the requirements of a clock.");
		static_assert(IsDuration<Duration>::value, "The given type for Duration is not of std::chrono::duration.");
		static const long long offset = std::is_same_v< Clock_, std::chrono::system_clock> ? 0
			: (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::time_point().time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(typename Clock_::time_point().time_since_epoch()).count());
		return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count() + offset;
	}

	template<typename Clock_, class Duration_ = typename Clock_::duration>
	[[nodiscard]] std::chrono::time_point<Clock_, Duration_> from_time_t(time_t time) noexcept {
		static_assert(IsClock<Clock_>::value, "The given type for Clock_ does not fulfill the requirements of a clock.");
		static_assert(IsDuration<Duration_>::value, "The given type for Duration is not of std::chrono::duration.");
		static const long long offset = std::is_same_v< Clock_, std::chrono::system_clock> ? 0
			: (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::time_point().time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(Clock_::time_point().time_since_epoch()).count());
		return std::chrono::time_point<Clock_, Duration_>{ std::chrono::duration_cast<Duration_>(std::chrono::seconds{ time - offset }) };
	}

	template<typename Char_, typename Clock_, class Duration_ = typename Clock_::duration>
	std::basic_string<Char_> systemTimeToStringWithLocalTime(const std::chrono::time_point<Clock_, Duration_>& time, const std::basic_string<Char_>& betweenDateAndTime)
	{
		const time_t timeT = to_time_t(time);

		// ReSharper disable once CppUseStructuredBinding
		tm localTimeT{};
		if (localtime_r(&timeT, &localTimeT) != 0)
		{
			return std::basic_string<Char_>();
		}

		const auto microsecondsFraction = chr::duration_cast<chr::microseconds>(
			time.time_since_epoch()
			).count() % 1000000;

		std::basic_ostringstream<Char_> oss; oss
			<< std::setbase(10)	// setbase is "sticky"
	        << std::setfill(any_string_array<Char_>("0").data()[0]) // setfill is "sticky"
			<< std::setw(4) << localTimeT.tm_year + 1900 // setw is not sticky
			<< any_string_array<Char_>("-").data()
			<< std::setw(2) << localTimeT.tm_mon + 1
			<< any_string_array<Char_>("-").data()
			<< std::setw(2) << localTimeT.tm_mday
			<< betweenDateAndTime
			<< std::setw(2) << localTimeT.tm_hour
			<< any_string_array<Char_>(":").data()
			<< std::setw(2) << localTimeT.tm_min
			<< any_string_array<Char_>(":").data()
			<< std::setw(2) << localTimeT.tm_sec
			<< any_string_array<Char_>(".").data()
			<< std::setw(6)
			<< microsecondsFraction;

		return oss.str();
	}

	template<typename Clock_, class Duration_ = typename Clock_::duration>
	[[nodiscard]] std::string systemTimeAsStringWithLocalTime(const std::chrono::time_point<Clock_, Duration_>& time, const std::string& betweenDateAndTime = " ")
	{
		return systemTimeToStringWithLocalTime(time, betweenDateAndTime);
	}

	[[nodiscard]] inline std::string getLocalTimeAsString(const std::string& betweenDateAndTime = " ")
	{
		return systemTimeAsStringWithLocalTime(chr::system_clock::now(), betweenDateAndTime);
	}

	template<typename Clock_, class Duration_ = typename Clock_::duration>
	[[nodiscard]] std::wstring systemTimeAsWideStringWithLocalTime(const std::chrono::time_point<Clock_, Duration_>& time, const std::wstring& betweenDateAndTime = L" ")
	{
		return systemTimeToStringWithLocalTime(time, betweenDateAndTime);
	}

	[[nodiscard]] inline std::wstring getLocalTimeAsWideString(const std::wstring& betweenDateAndTime = L" ")
	{
		return systemTimeAsWideStringWithLocalTime(chr::system_clock::now(), betweenDateAndTime);
	}

}

