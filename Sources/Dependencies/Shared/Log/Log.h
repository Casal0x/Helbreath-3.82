#pragma once

#include "LogLevel.h"
#include <format>
#include <string>

namespace hb::logger {

namespace detail {
	void write(int channel, int level, const char* message);
}

void initialize(const char* log_directory);
void shutdown();

template<auto C = 0, typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
	std::string msg = std::format(fmt, std::forward<Args>(args)...);
	detail::write(static_cast<int>(C), level::error, msg.c_str());
}

template<auto C = 0, typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args)
{
	std::string msg = std::format(fmt, std::forward<Args>(args)...);
	detail::write(static_cast<int>(C), level::warn, msg.c_str());
}

template<auto C = 0, typename... Args>
void log(std::format_string<Args...> fmt, Args&&... args)
{
	std::string msg = std::format(fmt, std::forward<Args>(args)...);
	detail::write(static_cast<int>(C), level::log, msg.c_str());
}

template<auto C = 0, typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args)
{
	std::string msg = std::format(fmt, std::forward<Args>(args)...);
	detail::write(static_cast<int>(C), level::debug, msg.c_str());
}

} // namespace hb::logger
