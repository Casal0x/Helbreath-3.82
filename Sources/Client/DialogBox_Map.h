#pragma once
#include "IDialogBox.h"

class DialogBox_Map : public IDialogBox
{
public:
	DialogBox_Map(CGame* game);
	~DialogBox_Map() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

	void on_enable(int type, int v1, int v2, char* string) override;
};
