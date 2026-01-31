#pragma once

#ifdef _WIN32
#include <windows.h>

class ServerConsole
{
public:
	ServerConsole();
	~ServerConsole();
	bool Init();
	void WriteLine(const char* text, WORD color);
	bool PollInput(char* pOutCmd, int maxLen);
	void RedrawPrompt();

private:
	void ClearInputLine();
	void DrawInputLine();
	SHORT GetPromptRow();

	HANDLE m_hOut;
	HANDLE m_hIn;
	char m_szInput[256];
	int m_iInputLen;
	int m_iCursorPos;
	SHORT m_sWidth;
	SHORT m_sHeight;
	bool m_bInit;
	DWORD m_dwOrigMode;
};

ServerConsole& GetServerConsole();

#endif
