#pragma once

#include "NativeTypes.h"
#include <string>
#include <cstdint>

class text_input_manager
{
public:
	static text_input_manager& get();

	void start_input(int x, int y, unsigned char maxLen, std::string& buffer, bool hidden = false);
	void end_input();
	void clear_input();
	void show_input();
	std::string get_input_string() const { return m_buffer ? *m_buffer : std::string(); }
	bool handle_char(hb::shared::types::NativeWindowHandle hWnd, uint32_t msg, uintptr_t wparam, intptr_t lparam);

	bool is_active() const { return m_is_active; }

private:
	text_input_manager() = default;
	~text_input_manager() = default;

	int get_char_kind(const std::string& str, int index);

	std::string* m_buffer = nullptr;
	unsigned char m_max_len = 0;
	int m_input_x = 0;
	int m_input_y = 0;
	bool m_is_active = false;
	bool m_is_hidden = false;
};
