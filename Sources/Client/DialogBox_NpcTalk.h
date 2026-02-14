#pragma once
#include "IDialogBox.h"

class DialogBox_NpcTalk : public IDialogBox
{
public:
	DialogBox_NpcTalk(CGame* game);
	~DialogBox_NpcTalk() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;

private:
	int get_total_lines() const;
	void draw_buttons(short sX, short sY, short mouse_x, short mouse_y);
	void draw_text_content(short sX, short sY);
	void draw_scroll_bar(short sX, short sY, int total_lines);
	void handle_scroll_bar_drag(short sX, short sY, short mouse_x, short mouse_y, int total_lines, char lb);
};
