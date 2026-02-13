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
	bool Init();
	void WriteLine(const char* text, int color = console_color::normal);
	bool PollInput(char* pOutCmd, int maxLen);
	void RedrawPrompt();

private:
	void ClearInputLine();
	void DrawInputLine();

	char m_szInput[256];
	int m_iInputLen;
	int m_iCursorPos;
	bool m_bInit;

#ifdef _WIN32
	HANDLE m_hOut;
	HANDLE m_hIn;
	DWORD m_dwOrigMode;
#else
	struct termios m_origTermios;
#endif
};

ServerConsole& GetServerConsole();
