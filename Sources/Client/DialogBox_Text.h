#pragma once

#include "IDialogBox.h"

class DialogBox_Text : public IDialogBox
{
public:
	DialogBox_Text(CGame* game);
	~DialogBox_Text() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;

private:
	int get_total_lines() const;
};
