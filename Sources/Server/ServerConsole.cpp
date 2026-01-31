#ifdef _WIN32
#include "ServerConsole.h"
#include <cstring>
#include <cstdio>

static ServerConsole s_console;

ServerConsole& GetServerConsole()
{
	return s_console;
}

ServerConsole::ServerConsole()
	: m_hOut(INVALID_HANDLE_VALUE)
	, m_hIn(INVALID_HANDLE_VALUE)
	, m_iInputLen(0)
	, m_iCursorPos(0)
	, m_sWidth(80)
	, m_sHeight(25)
	, m_bInit(false)
	, m_dwOrigMode(0)
{
	memset(m_szInput, 0, sizeof(m_szInput));
}

ServerConsole::~ServerConsole()
{
	if (m_bInit && m_hIn != INVALID_HANDLE_VALUE) {
		SetConsoleMode(m_hIn, m_dwOrigMode);
	}
}

bool ServerConsole::Init()
{
	m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hIn = GetStdHandle(STD_INPUT_HANDLE);

	if (m_hOut == INVALID_HANDLE_VALUE || m_hIn == INVALID_HANDLE_VALUE) {
		return false;
	}

	// Save original input mode and set non-blocking character mode
	if (!GetConsoleMode(m_hIn, &m_dwOrigMode)) {
		return false;
	}

	// Disable line input and echo so we get raw key events
	DWORD mode = ENABLE_WINDOW_INPUT;
	if (!SetConsoleMode(m_hIn, mode)) {
		return false;
	}

	// Get console dimensions
	CONSOLE_SCREEN_BUFFER_INFO info = {};
	if (GetConsoleScreenBufferInfo(m_hOut, &info)) {
		m_sWidth = info.srWindow.Right - info.srWindow.Left + 1;
		m_sHeight = info.srWindow.Bottom - info.srWindow.Top + 1;
	}

	m_bInit = true;
	DrawInputLine();
	return true;
}

SHORT ServerConsole::GetPromptRow()
{
	CONSOLE_SCREEN_BUFFER_INFO info = {};
	if (GetConsoleScreenBufferInfo(m_hOut, &info)) {
		return info.dwCursorPosition.Y;
	}
	return 0;
}

void ServerConsole::ClearInputLine()
{
	if (!m_bInit) return;

	SHORT row = GetPromptRow();
	COORD pos = { 0, row };
	DWORD written = 0;
	FillConsoleOutputCharacterA(m_hOut, ' ', m_sWidth, pos, &written);
	FillConsoleOutputAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, m_sWidth, pos, &written);
	SetConsoleCursorPosition(m_hOut, pos);
}

void ServerConsole::DrawInputLine()
{
	if (!m_bInit) return;

	SHORT row = GetPromptRow();
	COORD pos = { 0, row };
	DWORD written = 0;

	// Clear the line
	FillConsoleOutputCharacterA(m_hOut, ' ', m_sWidth, pos, &written);
	FillConsoleOutputAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, m_sWidth, pos, &written);

	// Write green "> " prompt
	SetConsoleCursorPosition(m_hOut, pos);
	SetConsoleTextAttribute(m_hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	const char* prompt = "> ";
	WriteConsoleA(m_hOut, prompt, 2, &written, nullptr);

	// Write input text in white
	if (m_iInputLen > 0) {
		SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		WriteConsoleA(m_hOut, m_szInput, m_iInputLen, &written, nullptr);
	}

	// Restore default color
	SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// Position cursor at the correct spot (prompt offset 2 + cursor position)
	COORD cursorPos = { (SHORT)(2 + m_iCursorPos), row };
	SetConsoleCursorPosition(m_hOut, cursorPos);
}

void ServerConsole::WriteLine(const char* text, WORD color)
{
	if (!m_bInit) {
		// Fallback if not initialized
		printf("%s\n", text);
		return;
	}

	DWORD written = 0;

	// Clear the prompt line
	ClearInputLine();

	// Write the log line with color at current position
	SetConsoleTextAttribute(m_hOut, color);
	WriteConsoleA(m_hOut, text, (DWORD)strlen(text), &written, nullptr);

	// Newline to scroll
	SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(m_hOut, "\n", 1, &written, nullptr);

	// Redraw prompt on the new bottom line
	DrawInputLine();
}

void ServerConsole::RedrawPrompt()
{
	if (!m_bInit) return;
	DrawInputLine();
}

bool ServerConsole::PollInput(char* pOutCmd, int maxLen)
{
	if (!m_bInit) return false;

	DWORD numEvents = 0;
	GetNumberOfConsoleInputEvents(m_hIn, &numEvents);
	if (numEvents == 0) return false;

	bool commandReady = false;

	// Process all pending events in one batch (non-blocking)
	INPUT_RECORD records[32];
	DWORD eventsRead = 0;
	while (numEvents > 0) {
		DWORD toRead = (numEvents > 32) ? 32 : numEvents;
		if (!ReadConsoleInput(m_hIn, records, toRead, &eventsRead)) {
			break;
		}
		numEvents -= eventsRead;

		for (DWORD i = 0; i < eventsRead; i++) {
			if (records[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
				m_sWidth = records[i].Event.WindowBufferSizeEvent.dwSize.X;
				m_sHeight = records[i].Event.WindowBufferSizeEvent.dwSize.Y;
				DrawInputLine();
				continue;
			}

			if (records[i].EventType != KEY_EVENT) continue;
			if (!records[i].Event.KeyEvent.bKeyDown) continue;

			KEY_EVENT_RECORD& key = records[i].Event.KeyEvent;
			WORD vk = key.wVirtualKeyCode;
			char ch = key.uChar.AsciiChar;

			if (vk == VK_RETURN) {
				// Enter pressed - copy buffer to output
				if (m_iInputLen > 0) {
					int copyLen = (m_iInputLen < maxLen - 1) ? m_iInputLen : (maxLen - 1);
					memcpy(pOutCmd, m_szInput, copyLen);
					pOutCmd[copyLen] = '\0';
					commandReady = true;
				}
				// Clear input buffer
				memset(m_szInput, 0, sizeof(m_szInput));
				m_iInputLen = 0;
				m_iCursorPos = 0;
				DrawInputLine();
				if (commandReady) return true;
				continue;
			}

			if (vk == VK_BACK) {
				if (m_iCursorPos > 0) {
					// Remove char before cursor
					memmove(&m_szInput[m_iCursorPos - 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
					m_iInputLen--;
					m_iCursorPos--;
					m_szInput[m_iInputLen] = '\0';
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_DELETE) {
				if (m_iCursorPos < m_iInputLen) {
					memmove(&m_szInput[m_iCursorPos], &m_szInput[m_iCursorPos + 1], m_iInputLen - m_iCursorPos - 1);
					m_iInputLen--;
					m_szInput[m_iInputLen] = '\0';
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_LEFT) {
				if (m_iCursorPos > 0) {
					m_iCursorPos--;
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_RIGHT) {
				if (m_iCursorPos < m_iInputLen) {
					m_iCursorPos++;
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_HOME) {
				m_iCursorPos = 0;
				DrawInputLine();
				continue;
			}

			if (vk == VK_END) {
				m_iCursorPos = m_iInputLen;
				DrawInputLine();
				continue;
			}

			// Printable characters
			if (ch >= 32 && ch < 127) {
				if (m_iInputLen < (int)(sizeof(m_szInput) - 1)) {
					// Insert at cursor position
					memmove(&m_szInput[m_iCursorPos + 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
					m_szInput[m_iCursorPos] = ch;
					m_iInputLen++;
					m_iCursorPos++;
					m_szInput[m_iInputLen] = '\0';
					DrawInputLine();
				}
				continue;
			}
		}
	}

	return false;
}

#endif
