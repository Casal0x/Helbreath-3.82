#include "TextInputManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "GameTimer.h"
#include <cstdio>

TextInputManager& TextInputManager::Get()
{
	static TextInputManager instance;
	return instance;
}

void TextInputManager::StartInput(int x, int y, unsigned char maxLen, char* buffer, bool isHidden)
{
	m_is_active = true;
	m_input_x = x;
	m_input_y = y;
	m_buffer = buffer;
	std::memset(m_edit, 0, sizeof(m_edit));
	m_max_len = maxLen;
}

void TextInputManager::EndInput()
{
	m_is_active = false;
	size_t len = strlen(m_edit);
	if (len > 0)
	{
		m_edit[len] = 0;
		std::snprintf(m_buffer + strlen(m_buffer), m_max_len - strlen(m_buffer), "%s", m_edit);
		std::memset(m_edit, 0, sizeof(m_edit));
	}
}

void TextInputManager::ClearInput()
{
	if (m_buffer != 0) std::memset(m_buffer, 0, sizeof(m_buffer));
	std::memset(m_edit, 0, sizeof(m_edit));
}

void TextInputManager::ShowInput(bool isHidden)
{
	if (m_buffer == nullptr) return;

	std::memset(m_display, 0, sizeof(m_display));

	std::snprintf(m_display, sizeof(m_display), "%s", m_buffer);
	if ((m_edit[0] != 0) && (strlen(m_buffer) + strlen(m_edit) + 1 <= m_max_len))
	{
		std::snprintf(m_display + strlen(m_buffer), sizeof(m_display) - strlen(m_buffer), "%s", m_edit);
	}

	if (isHidden == true)
	{
		for (unsigned char i = 0; i < strlen(m_display); i++)
			if (m_display[i] != 0) m_display[i] = '*';
	}

	if ((GameClock::GetTimeMS() % 400) < 210) m_display[strlen(m_display)] = '_';

	hb::shared::text::DrawText(GameFont::Default, m_input_x, m_input_y, m_display, hb::shared::text::TextStyle::WithShadow(GameColors::InputNormal));
}

void TextInputManager::ReceiveString(char* dest)
{
	std::snprintf(dest, m_max_len, "%s", m_buffer);
}

bool TextInputManager::HandleChar(hb::shared::types::NativeWindowHandle hWnd, uint32_t msg, uintptr_t wparam, intptr_t lparam)
{
	size_t len;
	if (m_buffer == 0) return false;

#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif

	switch (msg) {
	case WM_CHAR:
		if (wparam == 8)
		{
			if (strlen(m_buffer) > 0)
			{
				len = strlen(m_buffer);
				switch (GetCharKind(m_buffer, static_cast<int>(len) - 1)) {
				case 1:
					m_buffer[len - 1] = 0;
					break;
				case 2:
				case 3:
					m_buffer[len - 2] = 0;
					m_buffer[len - 1] = 0;
					break;
				}
				std::memset(m_edit, 0, sizeof(m_edit));
			}
		}
		else if ((wparam != 9) && (wparam != 13) && (wparam != 27))
		{
			len = strlen(m_buffer);
			if (len >= static_cast<size_t>(m_max_len) - 1) return false;
			m_buffer[len] = wparam & 0xff;
			m_buffer[len + 1] = 0;
		}
		return true;
	}
	return false;
}

int TextInputManager::GetCharKind(char* str, int index)
{
	int kind = 1;
	do
	{
		if (kind == 2) kind = 3;
		else
		{
			if ((unsigned char)*str < 128) kind = 1;
			else kind = 2;
		}
		str++;
		index--;
	} while (index >= 0);
	return kind;
}
