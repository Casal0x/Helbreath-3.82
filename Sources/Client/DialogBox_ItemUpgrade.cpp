#include "DialogBox_ItemUpgrade.h"
#include "CursorTarget.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_ItemUpgrade::DialogBox_ItemUpgrade(CGame* game)
	: IDialogBox(DialogBoxId::ItemUpgrade, game)
{
	set_default_rect(60 , 50 , 258, 339);
}

void DialogBox_ItemUpgrade::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
    if (!m_game->ensure_item_configs_loaded()) return;
    int sX = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_x;
    int sY = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_y;

    m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);
    m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 5); // Item Upgrade Text

    switch (m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode) {
    case 1:
        DrawMode1_GizonUpgrade(sX, sY, mouse_x, mouse_y);
        break;
    case 2:
        DrawMode2_InProgress(sX, sY);
        break;
    case 3:
        DrawMode3_Success(sX, sY, mouse_x, mouse_y);
        break;
    case 4:
        DrawMode4_Failed(sX, sY, mouse_x, mouse_y);
        break;
    case 5:
        DrawMode5_SelectUpgradeType(sX, sY, mouse_x, mouse_y);
        break;
    case 6:
        DrawMode6_StoneUpgrade(sX, sY, mouse_x, mouse_y);
        break;
    case 7:
        DrawMode7_ItemLost(sX, sY, mouse_x, mouse_y);
        break;
    case 8:
        DrawMode8_MaxUpgrade(sX, sY, mouse_x, mouse_y);
        break;
    case 9:
        DrawMode9_CannotUpgrade(sX, sY, mouse_x, mouse_y);
        break;
    case 10:
        DrawMode10_NoPoints(sX, sY, mouse_x, mouse_y);
        break;
    }
}

int DialogBox_ItemUpgrade::calculate_upgrade_cost(int item_index)
{
    int value = (m_game->m_item_list[item_index]->m_attribute & 0xF0000000) >> 28;
    value = value * (value + 6) / 8 + 2;

    // Special handling for Angelic Pendants
    CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_index]->m_id_num);
    if (cfg && (cfg->m_equip_pos >= 11)
        && (cfg->get_item_type() == ItemType::Equip))
    {
        short id = m_game->m_item_list[item_index]->m_id_num;
        if (id == hb::shared::item::ItemId::AngelicPandentSTR || id == hb::shared::item::ItemId::AngelicPandentDEX ||
            id == hb::shared::item::ItemId::AngelicPandentINT || id == hb::shared::item::ItemId::AngelicPandentMAG)
        {
            value = (m_game->m_item_list[item_index]->m_attribute & 0xF0000000) >> 28;
            switch (value) {
            case 0: value = 10; break;
            case 1: value = 11; break;
            case 2: value = 13; break;
            case 3: value = 16; break;
            case 4: value = 20; break;
            case 5: value = 25; break;
            case 6: value = 31; break;
            case 7: value = 38; break;
            case 8: value = 46; break;
            case 9: value = 55; break;
            }
        }
    }
    return value;
}

void DialogBox_ItemUpgrade::draw_item_preview(int sX, int sY, int item_index)
{
    char item_color = m_game->m_item_list[item_index]->m_item_color;
    CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_index]->m_id_num);
    if (!cfg) return;

    if (item_color == 0)
    {
        m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame);
    }
    else if ((cfg->get_equip_pos() == EquipPos::LeftHand)
        || (cfg->get_equip_pos() == EquipPos::RightHand)
        || (cfg->get_equip_pos() == EquipPos::TwoHand))
    {
        m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Weapons[item_color].r, GameColors::Weapons[item_color].g, GameColors::Weapons[item_color].b));
    }
    else
    {
        m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Items[item_color].r, GameColors::Items[item_color].g, GameColors::Items[item_color].b));
    }

    auto itemInfo = item_name_formatter::get().format(m_game->m_item_list[item_index].get());
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, itemInfo.name.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, itemInfo.effect.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, itemInfo.extra.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
}

void DialogBox_ItemUpgrade::DrawMode1_GizonUpgrade(int sX, int sY, int mouse_x, int mouse_y)
{
    uint32_t time = m_game->m_cur_time;
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;
    std::string txt;

    m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE1, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE2, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE3, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    txt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE11, m_game->m_gizon_item_upgrade_left);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, txt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (item_index != -1)
    {
        m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
        int value = calculate_upgrade_cost(item_index);

        txt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE12, value);
        if (m_game->m_gizon_item_upgrade_left < value)
            hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, txt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        else
            hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, txt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

        draw_item_preview(sX, sY, item_index);

        if (m_game->m_gizon_item_upgrade_left < value)
            m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
        else
        {
            if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
                m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 47);
            else
                m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
        }
    }
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    // Cancel button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode2_InProgress(int sX, int sY)
{
    uint32_t time = m_game->m_cur_time;
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;

    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE5, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE6, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (item_index != -1)
    {
        m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
        char item_color = m_game->m_item_list[item_index]->m_item_color;
        CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_index]->m_id_num);

        if (cfg)
        {
            if (item_color == 0)
            {
                m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame);
            }
            else if ((cfg->get_equip_pos() == EquipPos::LeftHand)
                || (cfg->get_equip_pos() == EquipPos::RightHand)
                || (cfg->get_equip_pos() == EquipPos::TwoHand))
            {
                m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Weapons[item_color].r, GameColors::Weapons[item_color].g, GameColors::Weapons[item_color].b));
            }
            else
            {
                m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(GameColors::Items[item_color].r, GameColors::Items[item_color].g, GameColors::Items[item_color].b));
            }

            // Flickering effect
            if ((rand() % 5) == 0)
                m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite]->draw(sX + 134, sY + 182, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.25f));
        }

        auto itemInfo2 = item_name_formatter::get().format(m_game->m_item_list[item_index].get());
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.name.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.effect.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.extra.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    // Send upgrade command after 4 seconds
    if (((time - m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_dw_v1) / 1000 > 4)
        && (m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_dw_v1 != 0))
    {
        m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_dw_v1 = 0;
        m_game->send_command(MsgId::CommandCommon, CommonType::UpgradeItem, 0, item_index, 0, 0, 0);
    }
}

void DialogBox_ItemUpgrade::DrawMode3_Success(int sX, int sY, int mouse_x, int mouse_y)
{
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;

    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE7, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE8, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (item_index != -1)
    {
        m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
        draw_item_preview(sX, sY, item_index);
    }

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode4_Failed(int sX, int sY, int mouse_x, int mouse_y)
{
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;

    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE9, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // Check if item was destroyed
    if ((item_index != -1) && (m_game->m_item_list[item_index] == 0))
    {
        m_game->play_game_sound('E', 24, 0, 0);
        m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode = 7;
        return;
    }

    if (item_index != -1)
    {
        m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
        draw_item_preview(sX, sY, item_index);
    }

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode5_SelectUpgradeType(int sX, int sY, int mouse_x, int mouse_y)
{
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE13, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // Normal item upgrade option
    if ((mouse_x > sX + 24) && (mouse_x < sX + 248) && (mouse_y > sY + 100) && (mouse_y < sY + 115))
    {
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, hb::shared::text::TextStyle::from_color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE16, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE17, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE18, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE19, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 210, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE20, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE21, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 255, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE26, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 270, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE27, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, hb::shared::text::TextStyle::from_color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

    // Majestic item upgrade option
    if ((mouse_x > sX + 24) && (mouse_x < sX + 248) && (mouse_y > sY + 120) && (mouse_y < sY + 135))
    {
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, hb::shared::text::TextStyle::from_color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE22, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE23, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE24, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE25, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE28, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 240, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE29, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, hb::shared::text::TextStyle::from_color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

    // Cancel button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode6_StoneUpgrade(int sX, int sY, int mouse_x, int mouse_y)
{
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;
    int so_x = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v2;
    int so_m = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v3;
    std::string txt;

    m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE31, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE32, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE33, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (so_x == 0)
    {
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE41, hb::shared::text::TextStyle::from_color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
    }
    else
    {
        txt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE34, so_x);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, txt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    if (so_m == 0)
    {
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE42, hb::shared::text::TextStyle::from_color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
    }
    else
    {
        txt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE35, so_m);
        hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, txt.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    if (item_index != -1)
    {
        m_game->draw_new_dialog_box(InterfaceNdGame3, sX, sY, 3);
        draw_item_preview(sX, sY, item_index);

        if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
            m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 47);
        else
            m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
    }

    // Cancel button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode7_ItemLost(int sX, int sY, int mouse_x, int mouse_y)
{
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE36, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 145, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE37, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode8_MaxUpgrade(int sX, int sY, int mouse_x, int mouse_y)
{
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE38, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode9_CannotUpgrade(int sX, int sY, int mouse_x, int mouse_y)
{
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE39, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode10_NoPoints(int sX, int sY, int mouse_x, int mouse_y)
{
    hb::shared::text::draw_text_aligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE40, hb::shared::text::TextStyle::from_color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

bool DialogBox_ItemUpgrade::on_click(short mouse_x, short mouse_y)
{
    short sX = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_x;
    short sY = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_y;
    int item_index = m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v1;

    switch (m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode) {
    case 1: // Gizon upgrade
        if ((item_index != -1) && (mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x)
            && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            int value = calculate_upgrade_cost(item_index);
            if (m_game->m_gizon_item_upgrade_left < value) break;

            m_game->play_game_sound('E', 14, 5);
            m_game->play_game_sound('E', 44, 0);
            m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode = 2;
            m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_dw_v1 = m_game->m_cur_time;
            return true;
        }
        if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_game->play_game_sound('E', 14, 5);
            m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 3:  // Success
    case 4:  // Failed
    case 7:  // Item lost
    case 8:  // Max upgrade
    case 9:  // Cannot upgrade
    case 10: // No points
    case 12: // Need stone
        if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_game->play_game_sound('E', 14, 5);
            m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 5: // Main menu - select upgrade type
        // Normal item upgrade (Stone)
        if ((mouse_x > sX + 24) && (mouse_x < sX + 248) && (mouse_y > sY + 100) && (mouse_y < sY + 115))
        {
            m_game->play_game_sound('E', 14, 5);
            int so_x = 0, so_m = 0;
            for (int i = 0; i < hb::shared::limits::MaxItems; i++)
                if (m_game->m_item_list[i] != 0)
                {
                    CItem* cfg = m_game->get_item_config(m_game->m_item_list[i]->m_id_num);
                    if (!cfg) continue;
                    if ((cfg->m_sprite == 6) && (cfg->m_sprite_frame == 128)) so_x++;
                    if ((cfg->m_sprite == 6) && (cfg->m_sprite_frame == 129)) so_m++;
                }

            if ((so_x > 0) || (so_m > 0))
            {
                m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode = 6;
                m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v2 = so_x;
                m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_v3 = so_m;
            }
            else
            {
                m_game->add_event_list(DRAW_DIALOGBOX_ITEMUPGRADE30, 10);
                m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            }
            return true;
        }
        // Majestic item upgrade (Gizon)
        if ((mouse_x > sX + 24) && (mouse_x < sX + 248) && (mouse_y > sY + 120) && (mouse_y < sY + 135))
        {
            m_game->play_game_sound('E', 14, 5);
            if (m_game->m_gizon_item_upgrade_left > 0)
            {
                m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode = 1;
            }
            else
            {
                m_game->add_event_list(DRAW_DIALOGBOX_ITEMUPGRADE40, 10);
                m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            }
            return true;
        }
        // Cancel
        if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_game->play_game_sound('E', 14, 5);
            m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 6: // Stone upgrade
        if ((item_index != -1) && (mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x)
            && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_game->play_game_sound('E', 14, 5);
            m_game->play_game_sound('E', 44, 0);
            m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_mode = 2;
            m_game->m_dialog_box_manager.Info(DialogBoxId::ItemUpgrade).m_dw_v1 = m_game->m_cur_time;
            return true;
        }
        if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_game->play_game_sound('E', 14, 5);
            m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;
    }

    return false;
}

bool DialogBox_ItemUpgrade::on_item_drop(short mouse_x, short mouse_y)
{
	int item_id = CursorTarget::get_selected_id();
	if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_is_item_disabled[item_id]) return false;
	if (m_game->m_player->m_Controller.get_command() < 0) return false;
	CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
	if (!cfg || cfg->get_equip_pos() == EquipPos::None) return false;

	switch (Info().m_mode) {
	case 1:
		if (Info().m_v1 >= 0 && Info().m_v1 < hb::shared::limits::MaxItems)
			m_game->m_is_item_disabled[Info().m_v1] = false;
		Info().m_v1 = item_id;
		m_game->m_is_item_disabled[item_id] = true;
		m_game->play_game_sound('E', 29, 0);
		break;

	case 6:
		if (Info().m_v1 >= 0 && Info().m_v1 < hb::shared::limits::MaxItems)
			m_game->m_is_item_disabled[Info().m_v1] = false;
		Info().m_v1 = item_id;
		m_game->m_is_item_disabled[item_id] = true;
		m_game->play_game_sound('E', 29, 0);
		break;
	}

	return true;
}
