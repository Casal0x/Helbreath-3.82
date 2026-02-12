#include "TextInputManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "GameTimer.h"

TextInputManager& TextInputManager::Get()
{
	static TextInputManager instance;
	return instance;
}

void TextInputManager::StartInput(int x, int y, unsigned char maxLen, std::string& buffer, bool isHidden)
{
	m_is_active = true;
	m_input_x = x;
	m_input_y = y;
	m_buffer = &buffer;
	m_max_len = maxLen;
	m_is_hidden = isHidden;
}

void TextInputManager::EndInput()
{
	m_is_active = false;
}

void TextInputManager::ClearInput()
{
	if (m_buffer) m_buffer->clear();
}

void TextInputManager::ShowInput()
{
	if (m_buffer == nullptr) return;

	std::string display = *m_buffer;

	if (m_is_hidden)
	{
		for (auto& ch : display)
			if (ch != 0) ch = '*';
	}

	if ((GameClock::GetTimeMS() % 400) < 210) display += '_';

	hb::shared::text::DrawText(GameFont::Default, m_input_x, m_input_y, display.c_str(), hb::shared::text::TextStyle::WithShadow(GameColors::InputNormal));
}

bool TextInputManager::HandleChar(hb::shared::types::NativeWindowHandle hWnd, uint32_t msg, uintptr_t wparam, intptr_t lparam)
{
	if (m_buffer == nullptr) return false;

#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif

	switch (msg) {
	case WM_CHAR:
		if (wparam == 8)
		{
			if (!m_buffer->empty())
			{
				int len = static_cast<int>(m_buffer->size());
				switch (GetCharKind(*m_buffer, len - 1)) {
				case 1:
					m_buffer->pop_back();
					break;
				case 2:
				case 3:
					if (m_buffer->size() >= 2)
					{
						m_buffer->pop_back();
						m_buffer->pop_back();
					}
					break;
				}
			}
		}
		else if ((wparam != 9) && (wparam != 13) && (wparam != 27))
		{
			if (m_buffer->size() >= static_cast<size_t>(m_max_len) - 1) return false;
			*m_buffer += static_cast<char>(wparam & 0xff);
		}
		return true;
	}
	return false;
}

int TextInputManager::GetCharKind(const std::string& str, int index)
{
	int kind = 1;
	int i = 0;
	for (int pos = 0; pos <= index && pos < static_cast<int>(str.size()); pos++)
	{
		if (kind == 2) kind = 3;
		else
		{
			if (static_cast<unsigned char>(str[pos]) < 128) kind = 1;
			else kind = 2;
		}
	}
	return kind;
}
