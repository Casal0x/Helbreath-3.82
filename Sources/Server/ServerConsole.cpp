#include "ServerConsole.h"
#include <cstring>
#include <cstdio>

#ifndef _WIN32
	#include <unistd.h>
	#include <sys/select.h>
#endif

static ServerConsole s_console;

ServerConsole& GetServerConsole()
{
	return s_console;
}

// --- ANSI color table ---

static const char* GetAnsiColor(int color)
{
	switch (color)
	{
	case console_color::error:   return "\033[1;31m";
	case console_color::warning: return "\033[1;33m";
	case console_color::success: return "\033[1;32m";
	case console_color::bright:  return "\033[1;37m";
	default:                     return "";
	}
}

// --- Constructor / Destructor ---

ServerConsole::ServerConsole()
	: m_iInputLen(0)
	, m_iCursorPos(0)
	, m_bInit(false)
#ifdef _WIN32
	, m_hOut(INVALID_HANDLE_VALUE)
	, m_hIn(INVALID_HANDLE_VALUE)
	, m_dwOrigMode(0)
#else
	, m_origTermios{}
#endif
{
	std::memset(m_szInput, 0, sizeof(m_szInput));
}

ServerConsole::~ServerConsole()
{
	if (!m_bInit) return;
#ifdef _WIN32
	SetConsoleMode(m_hIn, m_dwOrigMode);
#else
	tcsetattr(STDIN_FILENO, TCSANOW, &m_origTermios);
#endif
}

// --- Init (platform-split) ---

bool ServerConsole::Init()
{
#ifdef _WIN32
	m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (m_hOut == INVALID_HANDLE_VALUE || m_hIn == INVALID_HANDLE_VALUE)
		return false;

	// Save original input mode and set raw key-event mode
	if (!GetConsoleMode(m_hIn, &m_dwOrigMode))
		return false;
	if (!SetConsoleMode(m_hIn, ENABLE_WINDOW_INPUT))
		return false;

	// Enable ANSI escape sequence processing on output
	DWORD outMode = 0;
	if (GetConsoleMode(m_hOut, &outMode))
		SetConsoleMode(m_hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
	// Set terminal to raw mode (no echo, no canonical buffering)
	if (tcgetattr(STDIN_FILENO, &m_origTermios) < 0)
		return false;
	struct termios raw = m_origTermios;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0)
		return false;
#endif

	m_bInit = true;
	return true;
}

// --- Output (ANSI, shared across platforms) ---

void ServerConsole::ClearInputLine()
{
	if (!m_bInit) return;
	std::fputs("\r\033[K", stdout);
	std::fflush(stdout);
}

void ServerConsole::DrawInputLine()
{
	if (!m_bInit) return;
	// Clear line, draw green "> " prompt, input text in bright white, reset
	std::fputs("\r\033[K\033[1;32m> \033[1;37m", stdout);
	if (m_iInputLen > 0)
		std::fwrite(m_szInput, 1, m_iInputLen, stdout);
	std::fputs("\033[0m", stdout);
	// Move cursor to correct column (1-based): prompt ">" + space = 2 chars + cursor offset
	std::fprintf(stdout, "\033[%dG", 3 + m_iCursorPos);
	std::fflush(stdout);
}

void ServerConsole::WriteLine(const char* text, int color)
{
	if (!m_bInit) {
		std::printf("%s\n", text);
		return;
	}

	ClearInputLine();

	const char* ansi = GetAnsiColor(color);
	if (ansi[0] != '\0')
		std::fputs(ansi, stdout);
	std::fputs(text, stdout);
	if (ansi[0] != '\0')
		std::fputs("\033[0m", stdout);
	std::fputc('\n', stdout);

	DrawInputLine();
}

void ServerConsole::RedrawPrompt()
{
	if (!m_bInit) return;
	DrawInputLine();
}

// =========================================================================
// Input polling — platform-split
// =========================================================================

#ifdef _WIN32

bool ServerConsole::PollInput(char* pOutCmd, int maxLen)
{
	if (!m_bInit) return false;

	DWORD numEvents = 0;
	GetNumberOfConsoleInputEvents(m_hIn, &numEvents);
	if (numEvents == 0) return false;

	INPUT_RECORD records[32];
	DWORD eventsRead = 0;

	while (numEvents > 0) {
		DWORD toRead = (numEvents > 32) ? 32 : numEvents;
		if (!ReadConsoleInput(m_hIn, records, toRead, &eventsRead))
			break;
		numEvents -= eventsRead;

		for (DWORD i = 0; i < eventsRead; i++) {
			if (records[i].EventType != KEY_EVENT) continue;
			if (!records[i].Event.KeyEvent.bKeyDown) continue;

			KEY_EVENT_RECORD& key = records[i].Event.KeyEvent;
			WORD vk = key.wVirtualKeyCode;
			char ch = key.uChar.AsciiChar;

			if (vk == VK_RETURN) {
				if (m_iInputLen > 0) {
					int copyLen = (m_iInputLen < maxLen - 1) ? m_iInputLen : (maxLen - 1);
					std::memcpy(pOutCmd, m_szInput, copyLen);
					pOutCmd[copyLen] = '\0';
					std::memset(m_szInput, 0, sizeof(m_szInput));
					m_iInputLen = 0;
					m_iCursorPos = 0;
					DrawInputLine();
					return true;
				}
				std::memset(m_szInput, 0, sizeof(m_szInput));
				m_iInputLen = 0;
				m_iCursorPos = 0;
				DrawInputLine();
				continue;
			}

			if (vk == VK_BACK) {
				if (m_iCursorPos > 0) {
					std::memmove(&m_szInput[m_iCursorPos - 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
					m_iInputLen--;
					m_iCursorPos--;
					m_szInput[m_iInputLen] = '\0';
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_DELETE) {
				if (m_iCursorPos < m_iInputLen) {
					std::memmove(&m_szInput[m_iCursorPos], &m_szInput[m_iCursorPos + 1], m_iInputLen - m_iCursorPos - 1);
					m_iInputLen--;
					m_szInput[m_iInputLen] = '\0';
					DrawInputLine();
				}
				continue;
			}

			if (vk == VK_LEFT)  { if (m_iCursorPos > 0) { m_iCursorPos--; DrawInputLine(); } continue; }
			if (vk == VK_RIGHT) { if (m_iCursorPos < m_iInputLen) { m_iCursorPos++; DrawInputLine(); } continue; }
			if (vk == VK_HOME)  { m_iCursorPos = 0; DrawInputLine(); continue; }
			if (vk == VK_END)   { m_iCursorPos = m_iInputLen; DrawInputLine(); continue; }

			if (ch >= 32 && ch < 127 && m_iInputLen < (int)(sizeof(m_szInput) - 1)) {
				std::memmove(&m_szInput[m_iCursorPos + 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
				m_szInput[m_iCursorPos] = ch;
				m_iInputLen++;
				m_iCursorPos++;
				m_szInput[m_iInputLen] = '\0';
				DrawInputLine();
				continue;
			}
		}
	}

	return false;
}

#else // POSIX

bool ServerConsole::PollInput(char* pOutCmd, int maxLen)
{
	if (!m_bInit) return false;

	fd_set fds;
	struct timeval tv;

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		tv = {0, 0};
		if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
			break;

		char ch = 0;
		if (read(STDIN_FILENO, &ch, 1) != 1)
			break;

		// Escape sequences (arrow keys, delete, home, end)
		if (ch == 27) {
			FD_ZERO(&fds);
			FD_SET(STDIN_FILENO, &fds);
			tv = {0, 50000}; // 50ms timeout for rest of escape sequence
			if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
				char seq1 = 0;
				if (read(STDIN_FILENO, &seq1, 1) == 1 && seq1 == '[') {
					char seq2 = 0;
					if (read(STDIN_FILENO, &seq2, 1) == 1) {
						switch (seq2) {
						case 'D': if (m_iCursorPos > 0) { m_iCursorPos--; DrawInputLine(); } break;
						case 'C': if (m_iCursorPos < m_iInputLen) { m_iCursorPos++; DrawInputLine(); } break;
						case 'H': m_iCursorPos = 0; DrawInputLine(); break;
						case 'F': m_iCursorPos = m_iInputLen; DrawInputLine(); break;
						case '3': // Delete key: ESC[3~
						{
							char tilde = 0;
							read(STDIN_FILENO, &tilde, 1);
							if (m_iCursorPos < m_iInputLen) {
								std::memmove(&m_szInput[m_iCursorPos], &m_szInput[m_iCursorPos + 1], m_iInputLen - m_iCursorPos - 1);
								m_iInputLen--;
								m_szInput[m_iInputLen] = '\0';
								DrawInputLine();
							}
						}
						break;
						}
					}
				}
			}
			continue;
		}

		// Enter
		if (ch == '\n' || ch == '\r') {
			if (m_iInputLen > 0) {
				int copyLen = (m_iInputLen < maxLen - 1) ? m_iInputLen : (maxLen - 1);
				std::memcpy(pOutCmd, m_szInput, copyLen);
				pOutCmd[copyLen] = '\0';
				std::memset(m_szInput, 0, sizeof(m_szInput));
				m_iInputLen = 0;
				m_iCursorPos = 0;
				DrawInputLine();
				return true;
			}
			std::memset(m_szInput, 0, sizeof(m_szInput));
			m_iInputLen = 0;
			m_iCursorPos = 0;
			DrawInputLine();
			continue;
		}

		// Backspace (127 on most terminals, 8 on some)
		if (ch == 127 || ch == 8) {
			if (m_iCursorPos > 0) {
				std::memmove(&m_szInput[m_iCursorPos - 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
				m_iInputLen--;
				m_iCursorPos--;
				m_szInput[m_iInputLen] = '\0';
				DrawInputLine();
			}
			continue;
		}

		// Printable characters
		if (ch >= 32 && ch < 127 && m_iInputLen < (int)(sizeof(m_szInput) - 1)) {
			std::memmove(&m_szInput[m_iCursorPos + 1], &m_szInput[m_iCursorPos], m_iInputLen - m_iCursorPos);
			m_szInput[m_iCursorPos] = ch;
			m_iInputLen++;
			m_iCursorPos++;
			m_szInput[m_iInputLen] = '\0';
			DrawInputLine();
		}
	}

	return false;
}

#endif
