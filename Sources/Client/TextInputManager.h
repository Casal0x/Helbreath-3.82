#pragma once

#include "NativeTypes.h"
#include <cstring>
#include <cstdint>

class TextInputManager
{
public:
	static TextInputManager& Get();

	void StartInput(int x, int y, unsigned char maxLen, char* buffer, bool isHidden = false);
	void EndInput();
	void ClearInput();
	void ShowInput(bool isHidden = false);
	void ReceiveString(char* dest);
	bool HandleChar(hb::shared::types::NativeWindowHandle hWnd, uint32_t msg, uintptr_t wparam, intptr_t lparam);

	bool IsActive() const { return m_is_active; }

private:
	TextInputManager() = default;
	~TextInputManager() = default;

	int GetCharKind(char* str, int index);

	char* m_buffer = nullptr;
	unsigned char m_max_len = 0;
	int m_input_x = 0;
	int m_input_y = 0;
	bool m_is_active = false;
	char m_edit[4]{};
	char m_display[128]{};
};
