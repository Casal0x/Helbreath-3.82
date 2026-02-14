#pragma once
#include "IDialogBox.h"

class DialogBox_Shop : public IDialogBox
{
public:
    DialogBox_Shop(CGame* game);
    ~DialogBox_Shop() override = default;

    void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
    bool on_click(short mouse_x, short mouse_y) override;
    PressResult on_press(short mouse_x, short mouse_y) override;

private:
    void draw_item_list(short sX, short sY, short mouse_x, short mouse_y, short z, char lb);
    void draw_item_details(short sX, short sY, short mouse_x, short mouse_y, short z);
    void draw_weapon_stats(short sX, short sY, int item_index, bool& flag_red_shown);
    void draw_shield_stats(short sX, short sY, int item_index, bool& flag_red_shown);
    void draw_armor_stats(short sX, short sY, int item_index, bool& flag_stat_low, bool& flag_red_shown);
    void draw_level_requirement(short sX, short sY, int item_index, bool& flag_red_shown);
    void draw_quantity_selector(short sX, short sY, short mouse_x, short mouse_y, short z);
    int calculate_discounted_price(int item_index);

    bool on_click_item_list(short sX, short sY, short mouse_x, short mouse_y);
    bool on_click_item_details(short sX, short sY, short mouse_x, short mouse_y);
};
