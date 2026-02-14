#pragma once
#include "IDialogBox.h"

class DialogBox_Magic : public IDialogBox
{
public:
	DialogBox_Magic(CGame* game);
	~DialogBox_Magic() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};
