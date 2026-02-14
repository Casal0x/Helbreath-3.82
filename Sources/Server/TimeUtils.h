#pragma once
#include <chrono>
#include <ctime>
#include <cstdio>

namespace hb::time {

struct local_time
{
	int year, month, day, hour, minute, second, day_of_week;

	static local_time now()
	{
		auto tp = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(tp);
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		return { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		         tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday };
	}
};

inline void format_timestamp(const local_time& lt, char* buf, size_t size)
{
	std::snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d",
		lt.year, lt.month, lt.day, lt.hour, lt.minute, lt.second);
}

} // namespace hb::time
