#pragma once

#include "IDialogBox.h"
#include <string>

class DialogBox_Noticement : public IDialogBox
{
public:
	explicit DialogBox_Noticement(CGame* game);

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

	void set_shutdown_info(uint16_t seconds, const char* message);

private:
	std::string m_custom_message;
	uint16_t m_seconds_remaining = 0;
};
