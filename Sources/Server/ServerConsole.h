#pragma once

#ifdef _WIN32
	#include <windows.h>
#else
	#include <termios.h>
#endif

namespace console_color
{
	constexpr int normal  = 0;
	constexpr int error   = 1; // bright red
	constexpr int warning = 2; // bright yellow
	constexpr int success = 3; // bright green
	constexpr int bright  = 4; // bright white
}

class ServerConsole
{
public:
	ServerConsole();
	~ServerConsole();
	bool init();
	void write_line(const char* text, int color = console_color::normal);
	bool poll_input(char* out_cmd, int maxLen);
	void redraw_prompt();

private:
	void clear_input_line();
	void draw_input_line();

	char m_input[256];
	int m_input_len;
	int m_cursor_pos;
	bool m_init;

#ifdef _WIN32
	HANDLE m_out;
	HANDLE m_in;
	DWORD m_orig_mode;
#else
	struct termios m_orig_termios;
#endif
};

ServerConsole& GetServerConsole();
