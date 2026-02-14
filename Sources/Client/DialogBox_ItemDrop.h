#pragma once
#include "IDialogBox.h"

class DialogBox_ItemDrop : public IDialogBox
{
public:
	DialogBox_ItemDrop(CGame* game);
	~DialogBox_ItemDrop() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};
