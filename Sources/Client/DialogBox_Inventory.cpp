#include "DialogBox_Inventory.h"
#include "CursorTarget.h"
#include "Game.h"
#include "InventoryManager.h"
#include "ItemNameFormatter.h"
#include "IInput.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Inventory::DialogBox_Inventory(CGame* game)
	: IDialogBox(DialogBoxId::Inventory, game)
{
	set_can_close_on_right_click(true);
	set_default_rect(380 , 210 , 225, 185);
}

// Helper: draw a single inventory item with proper coloring and state
void DialogBox_Inventory::draw_inventory_item(CItem* item, int itemIdx, int baseX, int baseY)
{
	CItem* cfg = m_game->get_item_config(item->m_id_num);
	if (cfg == nullptr) return;

	char item_color = item->m_item_color;
	bool disabled = m_game->m_is_item_disabled[itemIdx];
	bool is_weapon = (cfg->get_equip_pos() == EquipPos::LeftHand) ||
	                 (cfg->get_equip_pos() == EquipPos::RightHand) ||
	                 (cfg->get_equip_pos() == EquipPos::TwoHand);

	int drawX = baseX + ITEM_OFFSET_X + item->m_x;
	int drawY = baseY + ITEM_OFFSET_Y + item->m_y;
	auto sprite = m_game->m_sprite[ItemPackPivotPoint + cfg->m_sprite];
	uint32_t time = m_game->m_cur_time;

	// Select color arrays (weapons use different color set)
	const hb::shared::render::Color* colors = is_weapon ? GameColors::Weapons : GameColors::Items;
	// (wG/wB merged into hb::shared::render::Color array above)


	if (item_color == 0)
	{
		// No color tint
		if (disabled)
			sprite->draw(drawX, drawY, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::alpha_blend(0.25f));
		else
			sprite->draw(drawX, drawY, cfg->m_sprite_frame);
	}
	else
	{
		// Apply color tint
		int r = colors[item_color].r;
		int g = colors[item_color].g;
		int b = colors[item_color].b;

		if (disabled)
			sprite->draw(drawX, drawY, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tinted_alpha(r, g, b, 0.7f));
		else
			sprite->draw(drawX, drawY, cfg->m_sprite_frame, hb::shared::sprite::DrawParams::tint(r, g, b));
	}

	// Show item count for consumables and arrows
	if ((cfg->get_item_type() == ItemType::Consume) || (cfg->get_item_type() == ItemType::Arrow))
	{
		std::string countBuf;
		countBuf = m_game->format_comma_number(static_cast<uint32_t>(item->m_count));
		hb::shared::text::draw_text(GameFont::Default, baseX + COUNT_OFFSET_X + item->m_x, baseY + COUNT_OFFSET_Y + item->m_y, countBuf.c_str(), hb::shared::text::TextStyle::with_shadow(GameColors::UIDescription));
	}
}

void DialogBox_Inventory::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game->ensure_item_configs_loaded()) return;
	short sX = Info().m_x;
	short sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdInventory, sX, sY, 0);

	// draw all inventory items
	for (int i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		int itemIdx = m_game->m_item_order[i];
		if (itemIdx == -1) continue;

		if (m_game->m_item_list[itemIdx] == nullptr)
			continue;

		CItem* item = m_game->m_item_list[itemIdx].get();
		if (item == nullptr) continue;

		// Skip items that are selected (being dragged) or equipped
		bool selected = (CursorTarget::GetSelectedType() == SelectedObjectType::Item) &&
		                 (CursorTarget::get_selected_id() == itemIdx);
		bool equipped = m_game->m_is_item_equipped[itemIdx];

		if (!selected && !equipped)
		{
			draw_inventory_item(item, itemIdx, sX, sY);
		}
	}

	// Item Upgrade button hover
	if ((mouse_x >= sX + BTN_UPGRADE_X1) && (mouse_x <= sX + BTN_UPGRADE_X2) &&
	    (mouse_y >= sY + BTN_Y1) && (mouse_y <= sY + BTN_Y2))
	{
		draw_new_dialog_box(InterfaceNdInventory, sX + BTN_UPGRADE_X1, sY + BTN_Y1, 1);
	}

	// Manufacture button hover
	if ((mouse_x >= sX + BTN_MANUFACTURE_X1) && (mouse_x <= sX + BTN_MANUFACTURE_X2) &&
	    (mouse_y >= sY + BTN_Y1) && (mouse_y <= sY + BTN_Y2))
	{
		draw_new_dialog_box(InterfaceNdInventory, sX + BTN_MANUFACTURE_X1, sY + BTN_Y1, 2);
	}
}

bool DialogBox_Inventory::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// Item Upgrade button
	if ((mouse_x >= sX + BTN_UPGRADE_X1) && (mouse_x <= sX + BTN_UPGRADE_X2) &&
	    (mouse_y >= sY + BTN_Y1) && (mouse_y <= sY + BTN_Y2))
	{
		enable_dialog_box(DialogBoxId::ItemUpgrade, 5, 0, 0);
		play_sound_effect('E', 14, 5);
		return true;
	}

	// Manufacture button
	if ((mouse_x >= sX + BTN_MANUFACTURE_X1) && (mouse_x <= sX + BTN_MANUFACTURE_X2) &&
	    (mouse_y >= sY + BTN_Y1) && (mouse_y <= sY + BTN_Y2))
	{
		if (m_game->m_player->m_skill_mastery[13] == 0)
		{
			add_event_list(DLGBOXCLICK_INVENTORY1, 10);
			add_event_list(DLGBOXCLICK_INVENTORY2, 10);
		}
		else if (m_game->m_skill_using_status)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		else if (m_game->is_item_on_hand())
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		else
		{
			// Look for manufacturing hammer
			for (int i = 0; i < hb::shared::limits::MaxItems; i++)
			{
				CItem* item = m_game->m_item_list[i].get();
				if (item == nullptr) continue;
				CItem* cfg = m_game->get_item_config(item->m_id_num);
				if (cfg != nullptr &&
				    cfg->get_item_type() == ItemType::UseSkillEnableDialogBox &&
				    cfg->m_sprite_frame == 113 &&
				    item->m_cur_life_span > 0)
				{
					enable_dialog_box(DialogBoxId::Manufacture, 3, 0, 0);
					add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY12, 10);
					play_sound_effect('E', 14, 5);
					return true;
				}
			}
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY14, 10);
		}
		play_sound_effect('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_Inventory::on_double_click(short mouse_x, short mouse_y)
{
	if (m_game->m_item_using_status)
	{
		add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY1, 10);
		return true;
	}

	if (CursorTarget::GetSelectedType() != SelectedObjectType::Item) return false;
	int item_id = CursorTarget::get_selected_id();
	if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_item_list[item_id] == nullptr) return false;

	inventory_manager::get().set_item_order(0, item_id);

	CItem* cfg = m_game->get_item_config(m_game->m_item_list[item_id]->m_id_num);
	if (cfg == nullptr) return false;

	auto itemInfo = item_name_formatter::get().format(m_game->m_item_list[item_id].get());

	// Check if at repair shop
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::SaleMenu) &&
		!m_game->m_dialog_box_manager.is_enabled(DialogBoxId::SellOrRepair) &&
		m_game->m_dialog_box_manager.Info(DialogBoxId::GiveItem).m_v3 == 24)
	{
		if (cfg->get_equip_pos() != EquipPos::None)
		{
			send_command(MsgId::CommandCommon, CommonType::ReqRepairItem, 0, item_id,
				m_game->m_dialog_box_manager.Info(DialogBoxId::GiveItem).m_v3, 0,
				cfg->m_name,
				m_game->m_dialog_box_manager.Info(DialogBoxId::GiveItem).m_v4);
			return true;
		}
	}

	// Bank dialog - drop item there
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::Bank))
	{
		m_game->m_dialog_box_manager.get_dialog_box(DialogBoxId::Bank)->on_item_drop(mouse_x, mouse_y);
		return true;
	}
	// Sell list dialog
	else if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::SellList))
	{
		m_game->m_dialog_box_manager.get_dialog_box(DialogBoxId::SellList)->on_item_drop(mouse_x, mouse_y);
		return true;
	}
	// Item upgrade dialog
	else if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ItemUpgrade))
	{
		m_game->m_dialog_box_manager.get_dialog_box(DialogBoxId::ItemUpgrade)->on_item_drop(mouse_x, mouse_y);
		return true;
	}

	// Handle consumable/depletable items
	if (cfg->get_item_type() == ItemType::UseDeplete ||
		cfg->get_item_type() == ItemType::UsePerm ||
		cfg->get_item_type() == ItemType::Arrow ||
		cfg->get_item_type() == ItemType::Eat)
	{
		if (!inventory_manager::get().check_item_operation_enabled(item_id)) return true;

		// Check damage cooldown for scrolls
		if ((m_game->m_cur_time - m_game->m_damaged_time) < 10000)
		{
			if ((cfg->m_sprite == 6) &&
				(cfg->m_sprite_frame == 9 ||
				 cfg->m_sprite_frame == 89))
			{
				std::string G_cTxt;
				G_cTxt = std::format(BDLBBOX_DOUBLE_CLICK_INVENTORY3, itemInfo.name.c_str());
				add_event_list(G_cTxt.c_str(), 10);
				return true;
			}
		}

		send_command(MsgId::CommandCommon, CommonType::ReqUseItem, 0, item_id, 0, 0, 0);

		if (cfg->get_item_type() == ItemType::UseDeplete ||
			cfg->get_item_type() == ItemType::Eat)
		{
			m_game->m_is_item_disabled[item_id] = true;
			m_game->m_item_using_status = true;
		}
	}

	// Handle skill items (pointing mode)
	if (cfg->get_item_type() == ItemType::UseSkill)
	{
		if (m_game->is_item_on_hand())
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_game->m_skill_using_status)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		if (m_game->m_item_list[item_id]->m_cur_life_span == 0)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			m_game->m_is_get_pointing_mode = true;
			m_game->m_point_command_type = item_id;
			std::string txt;
			txt = std::format(BDLBBOX_DOUBLE_CLICK_INVENTORY7, itemInfo.name.c_str());
			add_event_list(txt.c_str(), 10);
		}
	}

	// Handle deplete-dest items (use on other items)
	if (cfg->get_item_type() == ItemType::UseDepleteDest)
	{
		if (m_game->is_item_on_hand())
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_game->m_skill_using_status)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY13, 10);
			return true;
		}
		if (m_game->m_item_list[item_id]->m_cur_life_span == 0)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			m_game->m_is_get_pointing_mode = true;
			m_game->m_point_command_type = item_id;
			std::string txt;
			txt = std::format(BDLBBOX_DOUBLE_CLICK_INVENTORY8, itemInfo.name.c_str());
			add_event_list(txt.c_str(), 10);
		}
	}

	// Handle skill items that enable dialog boxes (alchemy pot, anvil, crafting, slates)
	if (cfg->get_item_type() == ItemType::UseSkillEnableDialogBox)
	{
		if (m_game->is_item_on_hand())
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY4, 10);
			return true;
		}
		if (m_game->m_skill_using_status)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY5, 10);
			return true;
		}
		if (m_game->m_item_list[item_id]->m_cur_life_span == 0)
		{
			add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY6, 10);
		}
		else
		{
			switch (cfg->m_sprite_frame)
			{
			case 55: // Alchemy pot
				if (m_game->m_player->m_skill_mastery[12] == 0)
					add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY9, 10);
				else
				{
					enable_dialog_box(DialogBoxId::Manufacture, 1, 0, 0);
					add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY10, 10);
				}
				break;

			case 113: // Smith's Anvil
				if (m_game->m_player->m_skill_mastery[13] == 0)
					add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY11, 10);
				else
				{
					enable_dialog_box(DialogBoxId::Manufacture, 3, 0, 0);
					add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY12, 10);
				}
				break;

			case 0: // Crafting
				enable_dialog_box(DialogBoxId::Manufacture, 7, 0, 0);
				add_event_list(BDLBBOX_DOUBLE_CLICK_INVENTORY17, 10);
				break;

			case 151:
			case 152:
			case 153:
			case 154: // Slates
				enable_dialog_box(DialogBoxId::Slates, 1, 0, 0);
				break;
			}
		}
	}

	// If alchemy/manufacture/crafting dialog is open, drop item there
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::Manufacture))
	{
		char mode = m_game->m_dialog_box_manager.Info(DialogBoxId::Manufacture).m_mode;
		if (mode == 1 || mode == 4 || mode == 7)
			m_game->m_dialog_box_manager.get_dialog_box(DialogBoxId::Manufacture)->on_item_drop(mouse_x, mouse_y);
	}

	// Auto-equip equipment items
	if (cfg->get_item_type() == ItemType::Equip)
	{
		CursorTarget::set_selection(SelectedObjectType::Item, static_cast<short>(item_id), 0, 0);
		m_game->m_dialog_box_manager.get_dialog_box(DialogBoxId::CharacterInfo)->on_item_drop(mouse_x, mouse_y);
		CursorTarget::clear_selection();
	}

	return true;
}

PressResult DialogBox_Inventory::on_press(short mouse_x, short mouse_y)
{
	// Don't allow item selection if certain dialogs are open
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ItemDropExternal)) return PressResult::Normal;
	if (m_game->m_dialog_box_manager.is_enabled(DialogBoxId::ItemDropConfirm)) return PressResult::Normal;

	short sX = Info().m_x;
	short sY = Info().m_y;

	// Check items in reverse order (topmost first)
	for (int i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		int item_id = m_game->m_item_order[hb::shared::limits::MaxItems - 1 - i];
		if (item_id == -1) continue;

		CItem* item = m_game->m_item_list[item_id].get();
		if (item == nullptr) continue;

		CItem* cfg = m_game->get_item_config(item->m_id_num);
		if (cfg == nullptr) continue;

		// Skip disabled or equipped items
		if (m_game->m_is_item_disabled[item_id]) continue;
		if (m_game->m_is_item_equipped[item_id]) continue;

		// Calculate item bounds
		int spriteIdx = ItemPackPivotPoint + cfg->m_sprite;
		int itemDrawX = sX + ITEM_OFFSET_X + item->m_x;
		int itemDrawY = sY + ITEM_OFFSET_Y + item->m_y;

		m_game->m_sprite[spriteIdx]->CalculateBounds(itemDrawX, itemDrawY, cfg->m_sprite_frame);
		auto bounds = m_game->m_sprite[spriteIdx]->GetBoundRect();

		// Check if click is within item bounds
		if (mouse_x > bounds.left && mouse_x < bounds.right &&
			mouse_y > bounds.top && mouse_y < bounds.bottom)
		{
			// Pixel-perfect collision check
			if (m_game->m_sprite[spriteIdx]->CheckCollision(itemDrawX, itemDrawY, cfg->m_sprite_frame, mouse_x, mouse_y))
			{
				// Bring item to top of order
				inventory_manager::get().set_item_order(0, item_id);

				// Handle pointing mode (using items on other items)
				bool handled_pointing = false;
				if (m_game->m_is_get_pointing_mode &&
					m_game->m_point_command_type >= 0 &&
					m_game->m_point_command_type < 100 &&
					m_game->m_item_list[m_game->m_point_command_type] != nullptr &&
					m_game->m_point_command_type != item_id)
				{
					CItem* point_cfg = m_game->get_item_config(m_game->m_item_list[m_game->m_point_command_type]->m_id_num);
					if (point_cfg != nullptr &&
						point_cfg->get_item_type() == ItemType::UseDepleteDest)
					{
						m_game->point_command_handler(0, 0, item_id);
						m_game->m_is_get_pointing_mode = false;
						handled_pointing = true;
					}
				}
				if (!handled_pointing)
				{
					// Select the item for dragging
					CursorTarget::set_selection(SelectedObjectType::Item, item_id,
						mouse_x - itemDrawX, mouse_y - itemDrawY);
				}
				return PressResult::ItemSelected;
			}
		}
	}

	return PressResult::Normal;
}

bool DialogBox_Inventory::on_item_drop(short mouse_x, short mouse_y)
{
	if (m_game->m_player->m_Controller.get_command() < 0) return false;

	int selected_id = CursorTarget::get_selected_id();
	if (selected_id < 0 || selected_id >= hb::shared::limits::MaxItems) return false;
	if (m_game->m_item_list[selected_id] == nullptr) return false;

	// Can't move equipped items while using a skill
	if (m_game->m_skill_using_status && m_game->m_is_item_equipped[selected_id])
	{
		add_event_list(BITEMDROP_INVENTORY1, 10);
		return false;
	}
	if (m_game->m_is_item_disabled[selected_id]) return false;

	// Calculate new position in inventory grid
	short sX = Info().m_x;
	short sY = Info().m_y;
	short dX = mouse_x - sX - ITEM_OFFSET_X - CursorTarget::get_drag_dist_x();
	short dY = mouse_y - sY - ITEM_OFFSET_Y - CursorTarget::get_drag_dist_y();

	// clamp to valid inventory area
	if (dY < -10) dY = -10;
	if (dX < 0) dX = 0;
	if (dX > 170) dX = 170;
	if (dY > 95) dY = 95;

	m_game->m_item_list[selected_id]->m_x = dX;
	m_game->m_item_list[selected_id]->m_y = dY;

	// Shift+drop: move all items with the same name to this position
	if (hb::shared::input::is_shift_down())
	{
		for (int i = 0; i < hb::shared::limits::MaxItems; i++)
		{
			if (m_game->m_item_order[hb::shared::limits::MaxItems - 1 - i] != -1)
			{
				int item_id = m_game->m_item_order[hb::shared::limits::MaxItems - 1 - i];
				if (m_game->m_item_list[item_id] != nullptr &&
					m_game->m_item_list[item_id]->m_id_num == m_game->m_item_list[selected_id]->m_id_num)
				{
					m_game->m_item_list[item_id]->m_x = dX;
					m_game->m_item_list[item_id]->m_y = dY;
					send_command(MsgId::RequestSetItemPos, 0, item_id, dX, dY, 0, 0);
				}
			}
		}
	}
	else
	{
		send_command(MsgId::RequestSetItemPos, 0, selected_id, dX, dY, 0, 0);
	}

	// If item was equipped, unequip it
	if (m_game->m_is_item_equipped[selected_id])
	{
		CItem* cfg = m_game->get_item_config(m_game->m_item_list[selected_id]->m_id_num);
		if (cfg == nullptr) return false;

		std::string txt;
		auto itemInfo2 = item_name_formatter::get().format(m_game->m_item_list[selected_id].get());
		txt = std::format(ITEM_EQUIPMENT_RELEASED, itemInfo2.name.c_str());
		add_event_list(txt.c_str(), 10);

		{
			short id = m_game->m_item_list[selected_id]->m_id_num;
			if (id == hb::shared::item::ItemId::AngelicPandentSTR || id == hb::shared::item::ItemId::AngelicPandentDEX ||
				id == hb::shared::item::ItemId::AngelicPandentINT || id == hb::shared::item::ItemId::AngelicPandentMAG)
				m_game->play_game_sound('E', 53, 0);
			else
				m_game->play_game_sound('E', 29, 0);
		}

		// Remove Angelic Stats
		if (cfg->m_equip_pos >= 11 &&
			cfg->get_item_type() == ItemType::Equip)
		{
			if (m_game->m_item_list[selected_id]->m_id_num == hb::shared::item::ItemId::AngelicPandentSTR)
				m_game->m_player->m_angelic_str = 0;
			else if (m_game->m_item_list[selected_id]->m_id_num == hb::shared::item::ItemId::AngelicPandentDEX)
				m_game->m_player->m_angelic_dex = 0;
			else if (m_game->m_item_list[selected_id]->m_id_num == hb::shared::item::ItemId::AngelicPandentINT)
				m_game->m_player->m_angelic_int = 0;
			else if (m_game->m_item_list[selected_id]->m_id_num == hb::shared::item::ItemId::AngelicPandentMAG)
				m_game->m_player->m_angelic_mag = 0;
		}

		send_command(MsgId::CommandCommon, CommonType::ReleaseItem, 0, selected_id, 0, 0, 0);
		m_game->m_is_item_equipped[selected_id] = false;
		m_game->m_item_equipment_status[cfg->m_equip_pos] = -1;
	}

	return true;
}
