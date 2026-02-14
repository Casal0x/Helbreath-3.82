#pragma once
#include "IDialogBox.h"

class DialogBox_ChatHistory : public IDialogBox
{
public:
	DialogBox_ChatHistory(CGame* game);
	~DialogBox_ChatHistory() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;

private:
	void draw_scroll_bar(short sX, short sY);
	void draw_chat_messages(short sX, short sY);
	void handle_scroll_input(short sX, short sY, short mouse_x, short mouse_y, short z, char lb);
};
