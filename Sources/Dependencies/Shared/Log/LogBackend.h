#pragma once

#include <cstdio>
#include <mutex>

namespace hb::logger {

class log_backend
{
public:
	static constexpr int max_channels = 32;

	struct config
	{
		const char* directory = "";
		const char* const* filenames = nullptr;
		int channel_count = 0;
		const char* (*channel_namer)(int) = nullptr;
	};

	void init(const config& cfg);
	void write(int channel, int level, const char* message);
	void close();

protected:
	virtual void write_console(int level, const char* formatted_line);

private:
	FILE* m_files[max_channels] = {};
	std::mutex m_mutex;
	char m_directory[512] = {};
	const char* const* m_filenames = nullptr;
	const char* (*m_channel_namer)(int) = nullptr;
	int m_channel_count = 0;
	bool m_initialized = false;
};

} // namespace hb::logger
