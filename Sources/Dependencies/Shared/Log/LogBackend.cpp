#include "LogBackend.h"
#include "LogLevel.h"
#include <chrono>
#include <ctime>
#include <format>
#include <iostream>
#include <string>

namespace hb::logger {

void log_backend::init(const config& cfg)
{
	std::lock_guard lock(m_mutex);
	m_directory = cfg.directory;
	m_filenames = cfg.filenames;
	m_channel_count = cfg.channel_count;
	m_channel_namer = cfg.channel_namer;

	if (!m_directory.empty())
		std::filesystem::create_directories(m_directory);

	for (int i = 0; i < m_channel_count && i < max_channels; ++i)
	{
		if (m_filenames[i].empty()) continue;
		auto path = m_directory / m_filenames[i];
		m_files[i].open(path, std::ios::app);
	}
	m_initialized = true;
}

void log_backend::write(int channel, int level, std::string_view message)
{
	std::lock_guard lock(m_mutex);
	if (!m_initialized) return;

	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;
	struct tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &time_t_now);
#else
	localtime_r(&time_t_now, &tm_buf);
#endif

	auto timestamp = std::format("{:02d}:{:02d}:{:02d}.{:03d}",
		tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));

	std::string line;
	if (channel == 0 || !m_channel_namer)
		line = std::format("[{}] [{}] {}", timestamp, level_name(level), message);
	else
		line = std::format("[{}] [{}] [{}] {}", timestamp, level_name(level),
			m_channel_namer(channel), message);

	write_console(level, line);

	if (channel >= 0 && channel < m_channel_count && m_files[channel].is_open())
	{
		m_files[channel] << line << '\n';
		m_files[channel].flush();
	}

	if (channel != 0 && m_files[0].is_open())
	{
		m_files[0] << line << '\n';
		m_files[0].flush();
	}
}

void log_backend::write_console(int level, std::string_view formatted_line)
{
	std::cout << formatted_line << '\n';
}

void log_backend::close()
{
	std::lock_guard lock(m_mutex);
	for (auto& file : m_files)
	{
		if (file.is_open())
		{
			file.flush();
			file.close();
		}
	}
	m_initialized = false;
}

} // namespace hb::logger
