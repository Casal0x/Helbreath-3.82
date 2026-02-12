#include "LogBackend.h"
#include "LogLevel.h"
#include <cstring>
#include <chrono>
#include <ctime>
#include <format>
#include <string>
#include <filesystem>

namespace hb::logger {

void log_backend::init(const config& cfg)
{
	std::lock_guard lock(m_mutex);
	strncpy(m_directory, cfg.directory, sizeof(m_directory) - 1);
	m_filenames = cfg.filenames;
	m_channel_count = cfg.channel_count;
	m_channel_namer = cfg.channel_namer;

	if (m_directory[0] != '\0')
		std::filesystem::create_directories(m_directory);

	for (int i = 0; i < m_channel_count && i < max_channels; ++i)
	{
		if (!m_filenames[i]) continue;
		std::string path = std::format("{}{}", m_directory, m_filenames[i]);
		m_files[i] = fopen(path.c_str(), "a");
	}
	m_initialized = true;
}

void log_backend::write(int channel, int level, const char* message)
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

	std::string timestamp = std::format("{:02d}:{:02d}:{:02d}.{:03d}",
		tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));

	std::string line;
	if (channel == 0 || !m_channel_namer)
		line = std::format("[{}] [{}] {}", timestamp, level_name(level), message);
	else
		line = std::format("[{}] [{}] [{}] {}", timestamp, level_name(level),
			m_channel_namer(channel), message);

	write_console(level, line.c_str());

	if (channel >= 0 && channel < m_channel_count && m_files[channel])
	{
		fprintf(m_files[channel], "%s\n", line.c_str());
		fflush(m_files[channel]);
	}

	if (channel != 0 && m_files[0])
	{
		fprintf(m_files[0], "%s\n", line.c_str());
		fflush(m_files[0]);
	}
}

void log_backend::write_console(int level, const char* formatted_line)
{
	printf("%s\n", formatted_line);
}

void log_backend::close()
{
	std::lock_guard lock(m_mutex);
	for (int i = 0; i < max_channels; ++i)
	{
		if (m_files[i])
		{
			fflush(m_files[i]);
			fclose(m_files[i]);
			m_files[i] = nullptr;
		}
	}
	m_initialized = false;
}

} // namespace hb::logger
