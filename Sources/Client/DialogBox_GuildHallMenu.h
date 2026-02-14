#pragma once
#include "IDialogBox.h"

class DialogBox_GuildHallMenu : public IDialogBox
{
public:
	DialogBox_GuildHallMenu(CGame* game);
	~DialogBox_GuildHallMenu() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};
