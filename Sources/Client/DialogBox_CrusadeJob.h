#pragma once
#include "IDialogBox.h"

class DialogBox_CrusadeJob : public IDialogBox
{
public:
	DialogBox_CrusadeJob(CGame* game);
	~DialogBox_CrusadeJob() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void draw_mode_select_job(short sX, short sY, short mouse_x, short mouse_y);
	void draw_mode_confirm(short sX, short sY, short mouse_x, short mouse_y);
};
