#pragma once

#include <cstdint>

struct DialogBoxInfo
{
	int m_v1, m_v2, m_v3, m_v4, m_v5, m_v6, m_v7, m_v8, m_v9, m_v10, m_v11, m_v12, m_v13, m_v14;
	uint32_t m_dw_v1, m_dw_v2, m_dw_t1;
	bool m_flag;
	short m_x, m_y;
	short m_size_x, m_size_y;
	short m_view;
	char m_str[32], m_str2[32], m_str3[32], m_str4[32];
	char m_mode;
	bool m_is_scroll_selected;
	bool m_can_close_on_right_click = true;  // Whether right-click can close this dialog
};
