#pragma once
#include "IDialogBox.h"

class DialogBox_Commander : public IDialogBox
{
public:
	DialogBox_Commander(CGame* game);
	~DialogBox_Commander() override = default;

	void on_update() override;
	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};
