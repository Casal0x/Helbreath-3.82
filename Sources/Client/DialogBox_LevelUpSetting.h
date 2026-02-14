#pragma once
#include "IDialogBox.h"

class DialogBox_LevelUpSetting : public IDialogBox
{
public:
	DialogBox_LevelUpSetting(CGame* game);
	~DialogBox_LevelUpSetting() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void draw_stat_row(short sX, short sY, int y_offset, const char* label,
	                 int current_stat, int pending_change, short mouse_x, short mouse_y,
	                 int arrow_y_offset, bool can_increase, bool can_decrease);

	bool handle_stat_click(short mouse_x, short mouse_y, short sX, short sY,
	                     int y_offset, int& current_stat, int16_t& pending_change);
};
