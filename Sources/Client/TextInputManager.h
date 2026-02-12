#pragma once

#include "NativeTypes.h"
#include <string>
#include <cstdint>

class TextInputManager
{
public:
	static TextInputManager& Get();

	void StartInput(int x, int y, unsigned char maxLen, std::string& buffer, bool isHidden = false);
	void EndInput();
	void ClearInput();
	void ShowInput();
	std::string GetInputString() const { return m_buffer ? *m_buffer : std::string(); }
	bool HandleChar(hb::shared::types::NativeWindowHandle hWnd, uint32_t msg, uintptr_t wparam, intptr_t lparam);

	bool IsActive() const { return m_is_active; }

private:
	TextInputManager() = default;
	~TextInputManager() = default;

	int GetCharKind(const std::string& str, int index);

	std::string* m_buffer = nullptr;
	unsigned char m_max_len = 0;
	int m_input_x = 0;
	int m_input_y = 0;
	bool m_is_active = false;
	bool m_is_hidden = false;
};
