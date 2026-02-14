#pragma once
#include "IDialogBox.h"

class CItem;

class DialogBox_Inventory : public IDialogBox
{
public:
	DialogBox_Inventory(CGame* game);
	~DialogBox_Inventory() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_double_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;

private:
	// Layout constants
	static constexpr int ITEM_OFFSET_X = 32;
	static constexpr int ITEM_OFFSET_Y = 44;
	static constexpr int COUNT_OFFSET_X = 39;  // 29 + 10
	static constexpr int COUNT_OFFSET_Y = 51;  // 41 + 10

	// Button regions
	static constexpr int BTN_UPGRADE_X1 = 23;
	static constexpr int BTN_UPGRADE_X2 = 76;
	static constexpr int BTN_MANUFACTURE_X1 = 140;
	static constexpr int BTN_MANUFACTURE_X2 = 212;
	static constexpr int BTN_Y1 = 172;
	static constexpr int BTN_Y2 = 184;

	// Helper: draw a single inventory item with proper coloring and state
	void draw_inventory_item(CItem* item, int itemIdx, int baseX, int baseY);
};
