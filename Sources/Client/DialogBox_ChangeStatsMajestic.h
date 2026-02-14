#pragma once
#include "IDialogBox.h"

class DialogBox_ChangeStatsMajestic : public IDialogBox
{
public:
	DialogBox_ChangeStatsMajestic(CGame* game);
	~DialogBox_ChangeStatsMajestic() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void draw_stat_row(short sX, short sY, int y_offset, const char* label,
		int current_stat, int16_t pending_change, short mouse_x, short mouse_y,
		int arrow_y_offset, bool can_undo, bool can_reduce);
};
