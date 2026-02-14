#pragma once
#include "IDialogBox.h"

class DialogBox_ItemUpgrade : public IDialogBox
{
public:
    DialogBox_ItemUpgrade(CGame* game);
    ~DialogBox_ItemUpgrade() override = default;

    void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
    bool on_click(short mouse_x, short mouse_y) override;
    bool on_item_drop(short mouse_x, short mouse_y) override;

private:
    // draw helpers for each mode
    void DrawMode1_GizonUpgrade(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode2_InProgress(int sX, int sY);
    void DrawMode3_Success(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode4_Failed(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode5_SelectUpgradeType(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode6_StoneUpgrade(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode7_ItemLost(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode8_MaxUpgrade(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode9_CannotUpgrade(int sX, int sY, int mouse_x, int mouse_y);
    void DrawMode10_NoPoints(int sX, int sY, int mouse_x, int mouse_y);

    // Shared drawing helper
    void draw_item_preview(int sX, int sY, int item_index);
    int calculate_upgrade_cost(int item_index);
};
