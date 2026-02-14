#include "DialogBox_Skill.h"
#include "Game.h"
#include "IInput.h"
#include "Misc.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Skill::DialogBox_Skill(CGame* game)
	: IDialogBox(DialogBoxId::Skill, game)
{
	set_default_rect(337 , 57 , 258, 339);
}

void DialogBox_Skill::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_skill_configs_loaded()) return;
	short sX, sY;
	int i, total_lines, pointer_loc;
	std::string temp2;

	char temp[255];
	double d1, d2, d3;

	sX = Info().m_x;
	sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0); // Normal Dialog
	draw_new_dialog_box(InterfaceNdText, sX, sY, 1); // Skill Dialog Title Bar

	switch (Info().m_mode) {
	case 0:
		for (i = 0; i < 17; i++)
			if (((i + Info().m_view) < hb::shared::limits::MaxSkillType) && (m_game->m_skill_cfg_list[i + Info().m_view] != 0))
			{
				std::snprintf(temp, sizeof(temp), "%s", m_game->m_skill_cfg_list[i + Info().m_view]->m_name.c_str());
				CMisc::replace_string(temp, '-', ' ');
				temp2 = std::format("{:3}%", m_game->m_skill_cfg_list[i + Info().m_view]->m_level);
				if ((mouse_x >= sX + 25) && (mouse_x <= sX + 166) && (mouse_y >= sY + 45 + i * 15) && (mouse_y <= sY + 59 + i * 15))
				{
					if ((m_game->m_skill_cfg_list[i + Info().m_view]->m_is_useable == true)
						&& (m_game->m_skill_cfg_list[i + Info().m_view]->m_level != 0))
					{
						put_string(sX + 30, sY + 45 + i * 15, temp, GameColors::UIWhite);
						put_string(sX + 183, sY + 45 + i * 15, temp2.c_str(), GameColors::UIWhite);
					}
					else
					{
						put_string(sX + 30, sY + 45 + i * 15, temp, GameColors::UIBlack);
						put_string(sX + 183, sY + 45 + i * 15, temp2.c_str(), GameColors::UIBlack);
					}
				}
				else
				{
					if ((m_game->m_skill_cfg_list[i + Info().m_view]->m_is_useable == true)
						&& (m_game->m_skill_cfg_list[i + Info().m_view]->m_level != 0))
					{
						put_string(sX + 30, sY + 45 + i * 15, temp, GameColors::UIMagicBlue);
						put_string(sX + 183, sY + 45 + i * 15, temp2.c_str(), GameColors::UIMagicBlue);
					}
					else
					{
						put_string(sX + 30, sY + 45 + i * 15, temp, GameColors::UIBlack);
						put_string(sX + 183, sY + 45 + i * 15, temp2.c_str(), GameColors::UIBlack);
					}
				}

				if (m_game->m_down_skill_index == (i + Info().m_view))
					m_game->m_sprite[InterfaceAddInterface]->draw(sX + 215, sY + 47 + i * 15, 21, hb::shared::sprite::DrawParams::tint(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
				else m_game->m_sprite[InterfaceAddInterface]->draw(sX + 215, sY + 47 + i * 15, 20, hb::shared::sprite::DrawParams::tint(1, 1, 1));
			}

		total_lines = 0;
		for (i = 0; i < hb::shared::limits::MaxSkillType; i++)
			if (m_game->m_skill_cfg_list[i] != 0) total_lines++;

		if (total_lines > 17)
		{
			d1 = static_cast<double>(Info().m_view);
			d2 = static_cast<double>(total_lines - 17);
			d3 = (274.0f * d1) / d2;
			pointer_loc = static_cast<int>(d3);
		}
		else pointer_loc = 0;
		if (total_lines > 17)
		{
			draw_new_dialog_box(InterfaceNdGame2, sX, sY, 1);
			draw_new_dialog_box(InterfaceNdGame2, sX + 242, sY + pointer_loc + 35, 7);
		}

		if (lb != 0 && total_lines > 17)
		{
			if ((m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Skill))
			{
				if ((mouse_x >= sX + 240) && (mouse_x <= sX + 260) && (mouse_y >= sY + 30) && (mouse_y <= sY + 320))
				{
					d1 = static_cast<double>(mouse_y - (sY + 35));
					d2 = static_cast<double>(total_lines - 17);
					d3 = (d1 * d2) / 274.0f;
					pointer_loc = static_cast<int>(d3 + 0.5);
					if (pointer_loc > total_lines - 17) pointer_loc = total_lines - 17;
					Info().m_view = pointer_loc;
				}
			}
		}
		else Info().m_is_scroll_selected = false;
		if (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::Skill && z != 0)
		{
			if (z > 0) Info().m_view--;
			if (z < 0) Info().m_view++;

		}
		if (Info().m_view < 0) Info().m_view = 0;
		if (total_lines > 17 && Info().m_view > total_lines - 17) Info().m_view = total_lines - 17;
		break;
	}
}

bool DialogBox_Skill::on_click(short mouse_x, short mouse_y)
{
	int i;
	short sX, sY;
	sX = Info().m_x;
	sY = Info().m_y;
	switch (Info().m_mode) {
	case -1:
		break;
	case 0:
		for (i = 0; i < 17; i++)
			if (((i + Info().m_view) < hb::shared::limits::MaxSkillType) && (m_game->m_skill_cfg_list[i + Info().m_view] != 0))
			{
				if ((mouse_x >= sX + 44) && (mouse_x <= sX + 135 + 44) && (mouse_y >= sY + 45 + i * 15) && (mouse_y <= sY + 59 + i * 15))
				{
					if ((m_game->m_skill_cfg_list[i + Info().m_view]->m_is_useable == true)
						&& (m_game->m_skill_cfg_list[i + Info().m_view]->m_level != 0))
					{
						if (m_game->m_skill_using_status == true)
						{
							add_event_list(DLGBOX_CLICK_SKILL1, 10); // "You are already using other skill."
							return true;
						}
						if ((m_game->m_player->m_Controller.is_command_available() == false) || (m_game->m_player->m_hp <= 0))
						{
							add_event_list(DLGBOX_CLICK_SKILL2, 10); // "You can't use a skill while you are moving."
							return true;
						}
						if (m_game->m_is_get_pointing_mode == true)
						{
							return true;
						}
						switch (m_game->m_skill_cfg_list[i + Info().m_view]->m_use_method) {
						case 0:
						case 2:
							send_command(MsgId::CommandCommon, CommonType::ReqUseSkill, 0, (i + Info().m_view), 0, 0, 0);
							m_game->m_skill_using_status = true;
							disable_this_dialog();
							play_sound_effect('E', 14, 5);
							return true;
						}
					}
				}
				else if ((mouse_x >= sX + 215) && (mouse_x <= sX + 240) && (mouse_y >= sY + 45 + i * 15) && (mouse_y <= sY + 59 + i * 15))
				{
					if (Info().m_flag == false)
					{
						send_command(MsgId::CommandCommon, CommonType::ReqSetDownSkillIndex, 0, i + Info().m_view, 0, 0, 0);
						play_sound_effect('E', 14, 5);
						Info().m_flag = true;
						return true;
					}
				}
			}
		break;
	}
	return false;
}

PressResult DialogBox_Skill::on_press(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// Scroll bar region
	if ((mouse_x >= sX + 240) && (mouse_x <= sX + 260) && (mouse_y >= sY + 40) && (mouse_y <= sY + 320))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}

