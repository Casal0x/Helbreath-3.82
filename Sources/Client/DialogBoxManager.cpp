#include "DialogBoxManager.h"
#include "IDialogBox.h"
#include "DialogBox_WarningMsg.h"
#include "DialogBox_Resurrect.h"
#include "DialogBox_Noticement.h"
#include "DialogBox_RepairAll.h"
#include "DialogBox_ConfirmExchange.h"
#include "DialogBox_Help.h"
#include "DialogBox_ItemDrop.h"
#include "DialogBox_LevelUpSetting.h"
#include "DialogBox_Character.h"
#include "DialogBox_Inventory.h"
#include "DialogBox_Skill.h"
#include "DialogBox_Magic.h"
#include "DialogBox_HudPanel.h"
#include "DialogBox_GuideMap.h"
#include "DialogBox_Fishing.h"
#include "DialogBox_CrusadeJob.h"
#include "DialogBox_ItemDropAmount.h"
#include "DialogBox_Map.h"
#include "DialogBox_NpcActionQuery.h"
#include "DialogBox_SysMenu.h"
#include "DialogBox_Text.h"
#include "DialogBox_MagicShop.h"
#include "DialogBox_NpcTalk.h"
#include "DialogBox_ChatHistory.h"
#include "DialogBox_CityHallMenu.h"
#include "DialogBox_Shop.h"
#include "DialogBox_ItemUpgrade.h"
#include "DialogBox_SellList.h"
#include "DialogBox_GuildMenu.h"
#include "DialogBox_GuildOperation.h"
#include "DialogBox_Bank.h"
#include "DialogBox_Exchange.h"
#include "DialogBox_Party.h"
#include "DialogBox_Quest.h"
#include "DialogBox_Commander.h"
#include "DialogBox_Constructor.h"
#include "DialogBox_Soldier.h"
#include "DialogBox_Slates.h"
#include "DialogBox_ChangeStatsMajestic.h"
#include "DialogBox_GuildHallMenu.h"
#include "DialogBox_SellOrRepair.h"
#include "DialogBox_Manufacture.h"
#include "Game.h"
#include "lan_eng.h"
#include "BuildItemManager.h"
#include "ShopManager.h"
#include "TextInputManager.h"
#include "CursorTarget.h"

using namespace hb::shared::net;

DialogBoxManager::DialogBoxManager(CGame* game)
	: m_game(game)
{
}
// Destructor is defaulted in header - unique_ptr handles cleanup automatically

void DialogBoxManager::initialize(CGame* game)
{
	m_game = game;
}

void DialogBoxManager::initialize_dialog_boxes()
{
	// Dialog boxes are registered here as they are migrated
	// Using std::make_unique for exception-safe, leak-free memory management
	register_dialog_box(std::make_unique<DialogBox_WarningMsg>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Resurrect>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Noticement>(m_game));
	register_dialog_box(std::make_unique<DialogBox_RepairAll>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ConfirmExchange>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Help>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ItemDrop>(m_game));
	register_dialog_box(std::make_unique<DialogBox_LevelUpSetting>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Character>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Inventory>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Skill>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Magic>(m_game));
	register_dialog_box(std::make_unique<DialogBox_HudPanel>(m_game));
	register_dialog_box(std::make_unique<DialogBox_GuideMap>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Fishing>(m_game));
	register_dialog_box(std::make_unique<DialogBox_CrusadeJob>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ItemDropAmount>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Map>(m_game));
	register_dialog_box(std::make_unique<DialogBox_NpcActionQuery>(m_game));
	register_dialog_box(std::make_unique<DialogBox_SysMenu>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Text>(m_game));
	register_dialog_box(std::make_unique<DialogBox_MagicShop>(m_game));
	register_dialog_box(std::make_unique<DialogBox_NpcTalk>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ChatHistory>(m_game));
	register_dialog_box(std::make_unique<DialogBox_CityHallMenu>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Shop>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ItemUpgrade>(m_game));
	register_dialog_box(std::make_unique<DialogBox_SellList>(m_game));
	register_dialog_box(std::make_unique<DialogBox_GuildMenu>(m_game));
	register_dialog_box(std::make_unique<DialogBox_GuildOperation>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Bank>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Exchange>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Party>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Quest>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Commander>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Constructor>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Soldier>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Slates>(m_game));
	register_dialog_box(std::make_unique<DialogBox_ChangeStatsMajestic>(m_game));
	register_dialog_box(std::make_unique<DialogBox_GuildHallMenu>(m_game));
	register_dialog_box(std::make_unique<DialogBox_SellOrRepair>(m_game));
	register_dialog_box(std::make_unique<DialogBox_Manufacture>(m_game));
}

void DialogBoxManager::register_dialog_box(std::unique_ptr<IDialogBox> dialog_box)
{
	if (!dialog_box) return;
	int id = static_cast<int>(dialog_box->get_id());
	if (id >= 0 && id < 61)
	{
		m_pDialogBoxes[id] = std::move(dialog_box);  // Transfer ownership
	}
}

IDialogBox* DialogBoxManager::get_dialog_box(DialogBoxId::Type id) const
{
	return get_dialog_box(static_cast<int>(id));
}

IDialogBox* DialogBoxManager::get_dialog_box(int box_id) const
{
	if (box_id >= 0 && box_id < 61)
	{
		return m_pDialogBoxes[box_id].get();  // Return raw pointer (no ownership transfer)
	}
	return nullptr;
}

void DialogBoxManager::init_defaults()
{
	//Character-Info Dialog(F5)
	m_info[DialogBoxId::CharacterInfo].m_x = 30 ;
	m_info[DialogBoxId::CharacterInfo].m_y = 30 ;
	m_info[DialogBoxId::CharacterInfo].m_size_x = 270;
	m_info[DialogBoxId::CharacterInfo].m_size_y = 376;

	//Inventory Dialog(F6)
	m_info[DialogBoxId::Inventory].m_x = 380 ;
	m_info[DialogBoxId::Inventory].m_y = 210 ;
	m_info[DialogBoxId::Inventory].m_size_x = 225;
	m_info[DialogBoxId::Inventory].m_size_y = 185;

	//Magic Circle Dialog(F7)
	m_info[DialogBoxId::Magic].m_x = 337 ;
	m_info[DialogBoxId::Magic].m_y = 57 ;
	m_info[DialogBoxId::Magic].m_size_x = 258;
	m_info[DialogBoxId::Magic].m_size_y = 328;

	// Item drop confirmation
	m_info[DialogBoxId::ItemDropConfirm].m_x = 0 ;
	m_info[DialogBoxId::ItemDropConfirm].m_y = 0 ;
	m_info[DialogBoxId::ItemDropConfirm].m_size_x = 270;
	m_info[DialogBoxId::ItemDropConfirm].m_size_y = 105;

	// ** This is a battle area **
	m_info[DialogBoxId::WarningBattleArea].m_x = 0 ;
	m_info[DialogBoxId::WarningBattleArea].m_y = 0 ;
	m_info[DialogBoxId::WarningBattleArea].m_size_x = 310;
	m_info[DialogBoxId::WarningBattleArea].m_size_y = 170;

	//Guild Menu Dialog
	m_info[DialogBoxId::GuildMenu].m_x = 337 ;
	m_info[DialogBoxId::GuildMenu].m_y = 57 ;
	m_info[DialogBoxId::GuildMenu].m_size_x = 258;
	m_info[DialogBoxId::GuildMenu].m_size_y = 339;

	//Guild Operation Dialog
	m_info[DialogBoxId::GuildOperation].m_x = 337 ;
	m_info[DialogBoxId::GuildOperation].m_y = 57 ;
	m_info[DialogBoxId::GuildOperation].m_size_x = 295;
	m_info[DialogBoxId::GuildOperation].m_size_y = 346;

	//Guide Map Dialog
	m_info[DialogBoxId::GuideMap].m_x = LOGICAL_MAX_X() - 128;
	m_info[DialogBoxId::GuideMap].m_y = 0;
	m_info[DialogBoxId::GuideMap].m_size_x = 128;
	m_info[DialogBoxId::GuideMap].m_size_y = 128;

	//Chatting History Dialog(F9)
	m_info[DialogBoxId::ChatHistory].m_x = 135 ;
	m_info[DialogBoxId::ChatHistory].m_y = 273  ;
	m_info[DialogBoxId::ChatHistory].m_size_x = 364;
	m_info[DialogBoxId::ChatHistory].m_size_y = 162;

	//Sale Menu Dialog
	m_info[DialogBoxId::SaleMenu].m_x = 70 ;
	m_info[DialogBoxId::SaleMenu].m_y = 50 ;
	m_info[DialogBoxId::SaleMenu].m_size_x = 258;
	m_info[DialogBoxId::SaleMenu].m_size_y = 339;

	//Level-Up Setting Dialog
	m_info[DialogBoxId::LevelUpSetting].m_x = 0 ;
	m_info[DialogBoxId::LevelUpSetting].m_y = 0 ;
	m_info[DialogBoxId::LevelUpSetting].m_size_x = 258;
	m_info[DialogBoxId::LevelUpSetting].m_size_y = 339;

	//City Hall Menu Dialog
	m_info[DialogBoxId::CityHallMenu].m_x = 337 ;
	m_info[DialogBoxId::CityHallMenu].m_y = 57 ;
	m_info[DialogBoxId::CityHallMenu].m_size_x = 258;
	m_info[DialogBoxId::CityHallMenu].m_size_y = 339;

	//Bank Dialog
	m_info[DialogBoxId::Bank].m_x = 60 ; //337
	m_info[DialogBoxId::Bank].m_y = 50 ;
	m_info[DialogBoxId::Bank].m_size_x = 258;
	m_info[DialogBoxId::Bank].m_size_y = 339;
	m_info[DialogBoxId::Bank].m_v1 = 13;

	//Skill Menu(F8)
	m_info[DialogBoxId::Skill].m_x = 337 ;
	m_info[DialogBoxId::Skill].m_y = 57 ;
	m_info[DialogBoxId::Skill].m_size_x = 258;
	m_info[DialogBoxId::Skill].m_size_y = 339;

	//Magic Shop Menu
	m_info[DialogBoxId::MagicShop].m_x = 30 ;
	m_info[DialogBoxId::MagicShop].m_y = 30 ;
	m_info[DialogBoxId::MagicShop].m_size_x = 304;
	m_info[DialogBoxId::MagicShop].m_size_y = 328;

	//Dialog items drop external screen
	m_info[DialogBoxId::ItemDropExternal].m_x = 0 ;
	m_info[DialogBoxId::ItemDropExternal].m_y = 0 ;
	m_info[DialogBoxId::ItemDropExternal].m_size_x = 215;
	m_info[DialogBoxId::ItemDropExternal].m_size_y = 87;

	//Text Dialog
	m_info[DialogBoxId::Text].m_x = 20 ;
	m_info[DialogBoxId::Text].m_y = 65 ;
	m_info[DialogBoxId::Text].m_size_x = 258; // 238
	m_info[DialogBoxId::Text].m_size_y = 339; // 274

	//System Menu Dialog(F12)
	m_info[DialogBoxId::SystemMenu].m_x = 337 ;
	m_info[DialogBoxId::SystemMenu].m_y = 107 ;
	m_info[DialogBoxId::SystemMenu].m_size_x = 258;
	m_info[DialogBoxId::SystemMenu].m_size_y = 268;

	//NpcActionQuery Dialog
	m_info[DialogBoxId::NpcActionQuery].m_x = 237 ;
	m_info[DialogBoxId::NpcActionQuery].m_y = 57 ;
	m_info[DialogBoxId::NpcActionQuery].m_size_x = 252;
	m_info[DialogBoxId::NpcActionQuery].m_size_y = 87;

	//NpcTalk Dialog
	m_info[DialogBoxId::NpcTalk].m_x = 337 ;
	m_info[DialogBoxId::NpcTalk].m_y = 57 ;
	m_info[DialogBoxId::NpcTalk].m_size_x = 258;
	m_info[DialogBoxId::NpcTalk].m_size_y = 339;

	//Map
	m_info[DialogBoxId::Map].m_x = 336 ;
	m_info[DialogBoxId::Map].m_y = 88 ;
	m_info[DialogBoxId::Map].m_size_x = 270;
	m_info[DialogBoxId::Map].m_size_y = 346;

	//ItemSellorRepair Dialog
	m_info[DialogBoxId::SellOrRepair].m_x = 337 ;
	m_info[DialogBoxId::SellOrRepair].m_y = 57 ;
	m_info[DialogBoxId::SellOrRepair].m_size_x = 258;
	m_info[DialogBoxId::SellOrRepair].m_size_y = 339;

	//Fishing Dialog
	m_info[DialogBoxId::Fishing].m_x = 193 ;
	m_info[DialogBoxId::Fishing].m_y = 241 ;
	m_info[DialogBoxId::Fishing].m_size_x = 263;
	m_info[DialogBoxId::Fishing].m_size_y = 100;

	//Noticement Dialog
	m_info[DialogBoxId::Noticement].m_x = 162 ;
	m_info[DialogBoxId::Noticement].m_y = 40 ;
	m_info[DialogBoxId::Noticement].m_size_x = 315;
	m_info[DialogBoxId::Noticement].m_size_y = 171;

	//Manufacture Dialog
	m_info[DialogBoxId::Manufacture].m_x = 100 ;
	m_info[DialogBoxId::Manufacture].m_y = 60 ;
	m_info[DialogBoxId::Manufacture].m_size_x = 258;
	m_info[DialogBoxId::Manufacture].m_size_y = 339;

	//Exchange Dialog
	m_info[DialogBoxId::Exchange].m_x = 100 ;
	m_info[DialogBoxId::Exchange].m_y = 30 ;
	m_info[DialogBoxId::Exchange].m_size_x = 520;
	m_info[DialogBoxId::Exchange].m_size_y = 357;

	//Quest Dialog
	m_info[DialogBoxId::Quest].m_x = 0 ;
	m_info[DialogBoxId::Quest].m_y = 0 ;
	m_info[DialogBoxId::Quest].m_size_x = 258;
	m_info[DialogBoxId::Quest].m_size_y = 339;

	//HUD Panel (combined gauge + icon panel)
	m_info[DialogBoxId::HudPanel].m_x = 0;
	m_info[DialogBoxId::HudPanel].m_y = LOGICAL_HEIGHT() - ICON_PANEL_HEIGHT();
	m_info[DialogBoxId::HudPanel].m_size_x = ICON_PANEL_WIDTH();
	m_info[DialogBoxId::HudPanel].m_size_y = ICON_PANEL_HEIGHT();

	//Sell List Dialog
	m_info[DialogBoxId::SellList].m_x = 170 ;
	m_info[DialogBoxId::SellList].m_y = 70 ;
	m_info[DialogBoxId::SellList].m_size_x = 258;
	m_info[DialogBoxId::SellList].m_size_y = 339;

	//Party Dialog
	m_info[DialogBoxId::Party].m_x = 0 ;
	m_info[DialogBoxId::Party].m_y = 0 ;
	m_info[DialogBoxId::Party].m_size_x = 258;
	m_info[DialogBoxId::Party].m_size_y = 339;

	//Crusade Job Dialog
	m_info[DialogBoxId::CrusadeJob].m_x = 360 ;
	m_info[DialogBoxId::CrusadeJob].m_y = 65 ;
	m_info[DialogBoxId::CrusadeJob].m_size_x = 258;
	m_info[DialogBoxId::CrusadeJob].m_size_y = 339;

	//Item Upgrade Dialog
	m_info[DialogBoxId::ItemUpgrade].m_x = 60 ;
	m_info[DialogBoxId::ItemUpgrade].m_y = 50 ;
	m_info[DialogBoxId::ItemUpgrade].m_size_x = 258;
	m_info[DialogBoxId::ItemUpgrade].m_size_y = 339;

	//Help Menu Dialog(F1)
	m_info[DialogBoxId::Help].m_x = 358 ;
	m_info[DialogBoxId::Help].m_y = 65 ;
	m_info[DialogBoxId::Help].m_size_x = 258;
	m_info[DialogBoxId::Help].m_size_y = 339;

	//Crusade Commander Dialog
	m_info[DialogBoxId::CrusadeCommander].m_x = 20 ;
	m_info[DialogBoxId::CrusadeCommander].m_y = 20 ;
	m_info[DialogBoxId::CrusadeCommander].m_size_x = 310;
	m_info[DialogBoxId::CrusadeCommander].m_size_y = 386;

	//Crusade Constructor Dialog
	m_info[DialogBoxId::CrusadeConstructor].m_x = 20 ;
	m_info[DialogBoxId::CrusadeConstructor].m_y = 20 ;
	m_info[DialogBoxId::CrusadeConstructor].m_size_x = 310;
	m_info[DialogBoxId::CrusadeConstructor].m_size_y = 386;

	//Crusade Soldier Dialog
	m_info[DialogBoxId::CrusadeSoldier].m_x = 20 ;
	m_info[DialogBoxId::CrusadeSoldier].m_y = 20 ;
	m_info[DialogBoxId::CrusadeSoldier].m_size_x = 310;
	m_info[DialogBoxId::CrusadeSoldier].m_size_y = 386;

	// Give item ???
	m_info[DialogBoxId::GiveItem].m_x = 0 ;
	m_info[DialogBoxId::GiveItem].m_y = 0 ;
	m_info[DialogBoxId::GiveItem].m_size_x = 291;
	m_info[DialogBoxId::GiveItem].m_size_y = 413;

	// 3.51 Slates Dialog - Diuuude
	m_info[DialogBoxId::Slates].m_x = 100 ;
	m_info[DialogBoxId::Slates].m_y = 60 ;
	m_info[DialogBoxId::Slates].m_size_x = 258;
	m_info[DialogBoxId::Slates].m_size_y = 339;

	// Snoopy: Item exchange confirmation
	m_info[DialogBoxId::ConfirmExchange].m_x = 285 ;
	m_info[DialogBoxId::ConfirmExchange].m_y = 200 ;
	m_info[DialogBoxId::ConfirmExchange].m_size_x = 270;
	m_info[DialogBoxId::ConfirmExchange].m_size_y = 105;

	// MJ Stats Change DialogBox - Diuuude
	m_info[DialogBoxId::ChangeStatsMajestic].m_x = 0 ;
	m_info[DialogBoxId::ChangeStatsMajestic].m_y = 0 ;
	m_info[DialogBoxId::ChangeStatsMajestic].m_size_x = 258;
	m_info[DialogBoxId::ChangeStatsMajestic].m_size_y = 339;

	// Snoopy: Resurection
	m_info[DialogBoxId::Resurrect].m_x = 185 ;
	m_info[DialogBoxId::Resurrect].m_y = 100 ;
	m_info[DialogBoxId::Resurrect].m_size_x = 270;
	m_info[DialogBoxId::Resurrect].m_size_y = 105;

	//Guild Hall Menu Dialog
	m_info[DialogBoxId::GuildHallMenu].m_x = 337 ;
	m_info[DialogBoxId::GuildHallMenu].m_y = 57 ;
	m_info[DialogBoxId::GuildHallMenu].m_size_x = 258;
	m_info[DialogBoxId::GuildHallMenu].m_size_y = 339;

	//50Cent - Repair All
	m_info[DialogBoxId::RepairAll].m_x = 337 ;
	m_info[DialogBoxId::RepairAll].m_y = 57 ;
	m_info[DialogBoxId::RepairAll].m_size_x = 258;
	m_info[DialogBoxId::RepairAll].m_size_y = 339;

	// Dialogs that cannot be closed with right-click
	m_info[DialogBoxId::Inventory].m_can_close_on_right_click = false;
	m_info[DialogBoxId::Magic].m_can_close_on_right_click = false;
	m_info[DialogBoxId::GuildMenu].m_can_close_on_right_click = false;
	m_info[DialogBoxId::LevelUpSetting].m_can_close_on_right_click = false;
	m_info[DialogBoxId::ItemUpgrade].m_can_close_on_right_click = false;
	m_info[DialogBoxId::Party].m_can_close_on_right_click = false;
	m_info[DialogBoxId::Slates].m_can_close_on_right_click = false;
	m_info[DialogBoxId::CrusadeCommander].m_can_close_on_right_click = false;
}

void DialogBoxManager::update_dialog_boxs()
{
	// update all enabled dialogs (order doesn't matter for update)
	for (int i = 0; i < 61; i++)
	{
		if (m_enabled[i] && m_pDialogBoxes[i])
		{
			m_pDialogBoxes[i]->on_update();  // unique_ptr supports -> operator
		}
	}
}

void DialogBoxManager::draw_dialog_boxs(short mouse_x, short mouse_y, short z, char lb)
{
	if (!m_game) return;
	// For now, delegate to CGame which handles the full draw loop
	// Individual dialogs will be migrated incrementally
	m_game->draw_dialog_boxs(mouse_x, mouse_y, z, lb);
}

void DialogBoxManager::enable_dialog_box(int box_id, int type, int v1, int v2, char* string)
{
	int i;
	short sX, sY;

	switch (box_id) {
	case DialogBoxId::RepairAll: //50Cent - Repair all
		Info(DialogBoxId::RepairAll).m_mode = type;
		break;
	case DialogBoxId::SaleMenu:
		if (is_enabled(DialogBoxId::SaleMenu) == false)
		{
			switch (type) {
			case 0:
				break;
			default:
				// Check if shop items are already loaded (called from ResponseShopContentsHandler)
				if (shop_manager::get().has_items()) {
					// Items already loaded - just show the dialog
					Info(DialogBoxId::SaleMenu).m_v1 = type;
					Info(DialogBoxId::SaleMenu).m_mode = 0;
					Info(DialogBoxId::SaleMenu).m_view = 0;
					Info(DialogBoxId::SaleMenu).m_flag = true;
					Info(DialogBoxId::SaleMenu).m_v3 = 1;
				} else {
					// Request contents from server - dialog will be shown when response arrives
					shop_manager::get().set_pending_shop_type(type);
					shop_manager::get().request_shop_menu(type);
				}
				break;
			}
		}
		break;

	case DialogBoxId::LevelUpSetting: // levelup diag
		if (is_enabled(DialogBoxId::LevelUpSetting) == false)
		{
			Info(DialogBoxId::LevelUpSetting).m_x = Info(DialogBoxId::CharacterInfo).m_x + 20;
			Info(DialogBoxId::LevelUpSetting).m_y = Info(DialogBoxId::CharacterInfo).m_y + 20;
			Info(DialogBoxId::LevelUpSetting).m_v1 = m_game->m_player->m_lu_point;
		}
		break;

	case DialogBoxId::Magic: // Magic Dialog
		break;

	case DialogBoxId::ItemDropConfirm:
		if (is_enabled(DialogBoxId::ItemDropConfirm) == false) {
			Info(DialogBoxId::ItemDropConfirm).m_view = type;
		}
		break;

	case DialogBoxId::WarningBattleArea:
		if (is_enabled(DialogBoxId::WarningBattleArea) == false) {
			Info(DialogBoxId::WarningBattleArea).m_view = type;
		}
		break;

	case DialogBoxId::GuildMenu:
		if (Info(DialogBoxId::GuildMenu).m_mode == 1) {
			sX = Info(DialogBoxId::GuildMenu).m_x;
			sY = Info(DialogBoxId::GuildMenu).m_y;
			text_input_manager::get().end_input();
			text_input_manager::get().start_input(sX + 75, sY + 140, 21, m_game->m_player->m_guild_name);
		}
		break;

	case DialogBoxId::ItemDropExternal: // demande quantit�
		if (is_enabled(DialogBoxId::ItemDropExternal) == false)
		{
			Info(box_id).m_mode = 1;
			Info(DialogBoxId::ItemDropExternal).m_view = type;
			text_input_manager::get().end_input();
			m_game->m_amount_string = std::to_string(v1);
			sX = Info(DialogBoxId::ItemDropExternal).m_x;
			sY = Info(DialogBoxId::ItemDropExternal).m_y;
			text_input_manager::get().start_input(sX + 40, sY + 57, CGame::AmountStringMaxLen, m_game->m_amount_string);
		}
		else
		{
			if (Info(DialogBoxId::ItemDropExternal).m_mode == 1)
			{
				sX = Info(DialogBoxId::ItemDropExternal).m_x;
				sY = Info(DialogBoxId::ItemDropExternal).m_y;
				text_input_manager::get().end_input();
				text_input_manager::get().start_input(sX + 40, sY + 57, CGame::AmountStringMaxLen, m_game->m_amount_string);
			}
		}
		break;

	case DialogBoxId::Text:
		if (is_enabled(DialogBoxId::Text) == false)
		{
			switch (type) {
			case 0:
				Info(DialogBoxId::Text).m_mode = 0;
				Info(DialogBoxId::Text).m_view = 0;
				break;
			default:
				m_game->load_text_dlg_contents(type);
				Info(DialogBoxId::Text).m_mode = 0;
				Info(DialogBoxId::Text).m_view = 0;
				break;
			}
		}
		break;

	case DialogBoxId::SystemMenu:
		break;

	case DialogBoxId::NpcActionQuery: // Talk to npc or unicorn
		{ int idx = Info(DialogBoxId::NpcActionQuery).m_v1;
		if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		if (is_enabled(DialogBoxId::NpcActionQuery) == false)
		{
			Info(DialogBoxId::SaleMenu).m_v1 = Info(DialogBoxId::SaleMenu).m_v2 = Info(DialogBoxId::SaleMenu).m_v3 =
				Info(DialogBoxId::SaleMenu).m_v4 = Info(DialogBoxId::SaleMenu).m_v5 = Info(DialogBoxId::SaleMenu).m_v6 = 0;
			Info(DialogBoxId::NpcActionQuery).m_mode = type;
			Info(DialogBoxId::NpcActionQuery).m_view = 0;
			Info(DialogBoxId::NpcActionQuery).m_v1 = v1;
			Info(DialogBoxId::NpcActionQuery).m_v2 = v2;
		}
		break;

	case DialogBoxId::NpcTalk:
		if (is_enabled(DialogBoxId::NpcTalk) == false)
		{
			Info(DialogBoxId::NpcTalk).m_mode = type;
			Info(DialogBoxId::NpcTalk).m_view = 0;
			Info(DialogBoxId::NpcTalk).m_v1 = m_game->load_text_dlg_contents2(v1 + 20);
			Info(DialogBoxId::NpcTalk).m_v2 = v1 + 20;
		}
		break;

	case DialogBoxId::Map:
		if (is_enabled(DialogBoxId::Map) == false) {
			Info(DialogBoxId::Map).m_v1 = v1;
			Info(DialogBoxId::Map).m_v2 = v2;

			Info(DialogBoxId::Map).m_size_x = 290;
			Info(DialogBoxId::Map).m_size_y = 290;
		}
		break;

	case DialogBoxId::SellOrRepair:
		if (is_enabled(DialogBoxId::SellOrRepair) == false) {
			Info(DialogBoxId::SellOrRepair).m_mode = type;
			Info(DialogBoxId::SellOrRepair).m_v1 = v1;		// ItemID
			Info(DialogBoxId::SellOrRepair).m_v2 = v2;
			if (type == 2)
			{
				Info(DialogBoxId::SellOrRepair).m_x = Info(DialogBoxId::SaleMenu).m_x;
				Info(DialogBoxId::SellOrRepair).m_y = Info(DialogBoxId::SaleMenu).m_y;
			}
		}
		break;

	case DialogBoxId::Skill:
		break;

	case DialogBoxId::Fishing:
		if (is_enabled(DialogBoxId::Fishing) == false)
		{
			Info(DialogBoxId::Fishing).m_mode = type;
			Info(DialogBoxId::Fishing).m_v1 = v1;
			Info(DialogBoxId::Fishing).m_v2 = v2;
			m_game->m_skill_using_status = true;
		}
		break;

	case DialogBoxId::Noticement:
		if (is_enabled(DialogBoxId::Noticement) == false) {
			Info(DialogBoxId::Noticement).m_mode = type;
			Info(DialogBoxId::Noticement).m_v1 = v1;
			Info(DialogBoxId::Noticement).m_v2 = v2;
		}
		break;

	case DialogBoxId::Manufacture:
		switch (type) {
		case DialogBoxId::CharacterInfo:
		case DialogBoxId::Inventory: //
			if (is_enabled(DialogBoxId::Manufacture) == false)
			{
				Info(DialogBoxId::Manufacture).m_mode = type;
				Info(DialogBoxId::Manufacture).m_v1 = -1;
				Info(DialogBoxId::Manufacture).m_v2 = -1;
				Info(DialogBoxId::Manufacture).m_v3 = -1;
				Info(DialogBoxId::Manufacture).m_v4 = -1;
				Info(DialogBoxId::Manufacture).m_v5 = -1;
				Info(DialogBoxId::Manufacture).m_v6 = -1;
				Info(DialogBoxId::Manufacture).m_str[0] = 0;
				m_game->m_skill_using_status = true;
				Info(DialogBoxId::Manufacture).m_size_x = 195;
				Info(DialogBoxId::Manufacture).m_size_y = 215;
				disable_dialog_box(DialogBoxId::ItemDropExternal);
				disable_dialog_box(DialogBoxId::NpcActionQuery);
				disable_dialog_box(DialogBoxId::SellOrRepair);
			}
			break;

		case DialogBoxId::Magic:	//
			if (is_enabled(DialogBoxId::Manufacture) == false)
			{
				Info(DialogBoxId::Manufacture).m_view = 0;
				Info(DialogBoxId::Manufacture).m_mode = type;
				Info(DialogBoxId::Manufacture).m_v1 = -1;
				Info(DialogBoxId::Manufacture).m_v2 = -1;
				Info(DialogBoxId::Manufacture).m_v3 = -1;
				Info(DialogBoxId::Manufacture).m_v4 = -1;
				Info(DialogBoxId::Manufacture).m_v5 = -1;
				Info(DialogBoxId::Manufacture).m_v6 = -1;
				Info(DialogBoxId::Manufacture).m_str[0] = 0;
				Info(DialogBoxId::Manufacture).m_str[1] = 0;
				Info(DialogBoxId::Manufacture).m_str[4] = 0;
				m_game->m_skill_using_status = true;
				build_item_manager::get().update_available_recipes();
				Info(DialogBoxId::Manufacture).m_size_x = 270;
				Info(DialogBoxId::Manufacture).m_size_y = 381;
				disable_dialog_box(DialogBoxId::ItemDropExternal);
				disable_dialog_box(DialogBoxId::NpcActionQuery);
				disable_dialog_box(DialogBoxId::SellOrRepair);
			}
			break;

		case DialogBoxId::WarningBattleArea:
			if (is_enabled(DialogBoxId::Manufacture) == false)
			{
				Info(DialogBoxId::Manufacture).m_mode = type;
				Info(DialogBoxId::Manufacture).m_str[2] = v1;
				Info(DialogBoxId::Manufacture).m_str[3] = v2;
				Info(DialogBoxId::Manufacture).m_size_x = 270;
				Info(DialogBoxId::Manufacture).m_size_y = 381;
				m_game->m_skill_using_status = true;
				build_item_manager::get().update_available_recipes();
				disable_dialog_box(DialogBoxId::ItemDropExternal);
				disable_dialog_box(DialogBoxId::NpcActionQuery);
				disable_dialog_box(DialogBoxId::SellOrRepair);
			}
			break;
			// Crafting
		case DialogBoxId::GuildMenu:
		case DialogBoxId::GuildOperation:
			if (is_enabled(DialogBoxId::Manufacture) == false)
			{
				Info(DialogBoxId::Manufacture).m_mode = type;
				Info(DialogBoxId::Manufacture).m_v1 = -1;
				Info(DialogBoxId::Manufacture).m_v2 = -1;
				Info(DialogBoxId::Manufacture).m_v3 = -1;
				Info(DialogBoxId::Manufacture).m_v4 = -1;
				Info(DialogBoxId::Manufacture).m_v5 = -1;
				Info(DialogBoxId::Manufacture).m_v6 = -1;
				Info(DialogBoxId::Manufacture).m_str[0] = 0;
				Info(DialogBoxId::Manufacture).m_str[1] = 0;
				m_game->m_skill_using_status = true;
				Info(DialogBoxId::Manufacture).m_size_x = 195;
				Info(DialogBoxId::Manufacture).m_size_y = 215;
				disable_dialog_box(DialogBoxId::ItemDropExternal);
				disable_dialog_box(DialogBoxId::NpcActionQuery);
				disable_dialog_box(DialogBoxId::SellOrRepair);
			}
			break;
		}
		break;

	case DialogBoxId::Exchange: // Snoopy: 7 mar 06 (multitrade) case rewriten
		if (is_enabled(DialogBoxId::Exchange) == false)
		{
			Info(DialogBoxId::Exchange).m_mode = type;
			for (i = 0; i < 8; i++)
			{
				m_game->m_dialog_box_exchange_info[i].v1 = -1;
				m_game->m_dialog_box_exchange_info[i].v2 = -1;
				m_game->m_dialog_box_exchange_info[i].v3 = -1;
				m_game->m_dialog_box_exchange_info[i].v4 = -1;
				m_game->m_dialog_box_exchange_info[i].v5 = -1;
				m_game->m_dialog_box_exchange_info[i].v6 = -1;
				m_game->m_dialog_box_exchange_info[i].v7 = -1;
				m_game->m_dialog_box_exchange_info[i].dw_v1 = 0;
			}
			disable_dialog_box(DialogBoxId::ItemDropExternal);
			disable_dialog_box(DialogBoxId::NpcActionQuery);
			disable_dialog_box(DialogBoxId::SellOrRepair);
			disable_dialog_box(DialogBoxId::Manufacture);
		}
		break;

	case DialogBoxId::ConfirmExchange: // Snoopy: 7 mar 06 (MultiTrade) Confirmation dialog
		break;

	case DialogBoxId::Quest:
		if (is_enabled(DialogBoxId::Quest) == false) {
			Info(DialogBoxId::Quest).m_mode = type;
			Info(DialogBoxId::Quest).m_x = Info(DialogBoxId::CharacterInfo).m_x + 20;
			Info(DialogBoxId::Quest).m_y = Info(DialogBoxId::CharacterInfo).m_y + 20;
		}
		break;

	case DialogBoxId::Party:
		if (is_enabled(DialogBoxId::Party) == false) {
			Info(DialogBoxId::Party).m_mode = type;
			Info(DialogBoxId::Party).m_x = Info(DialogBoxId::CharacterInfo).m_x + 20;
			Info(DialogBoxId::Party).m_y = Info(DialogBoxId::CharacterInfo).m_y + 20;
		}
		break;

	case DialogBoxId::CrusadeJob:
		if ((m_game->m_player->m_hp <= 0) || (m_game->m_player->m_citizen == false)) return;
		if (is_enabled(DialogBoxId::CrusadeJob) == false)
		{
			Info(DialogBoxId::CrusadeJob).m_mode = type;
			Info(DialogBoxId::CrusadeJob).m_x = 360 ;
			Info(DialogBoxId::CrusadeJob).m_y = 65 ;
			Info(DialogBoxId::CrusadeJob).m_v1 = v1;
		}
		break;

	case DialogBoxId::ItemUpgrade:
		if (is_enabled(DialogBoxId::ItemUpgrade) == false)
		{
			Info(DialogBoxId::ItemUpgrade).m_mode = type;
			Info(DialogBoxId::ItemUpgrade).m_v1 = -1;
			Info(DialogBoxId::ItemUpgrade).m_dw_v1 = 0;
		}
		else
		{
			int so_x, so_m;
			so_x = so_m = 0;
			for (i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_game->m_item_list[i] != 0)
				{
					if ((m_game->m_item_list[i]->m_sprite == 6) && (m_game->m_item_list[i]->m_sprite_frame == 128)) so_x++;
					if ((m_game->m_item_list[i]->m_sprite == 6) && (m_game->m_item_list[i]->m_sprite_frame == 129)) so_m++;
				}
			if ((so_x > 0) || (so_m > 0))
			{
				Info(DialogBoxId::ItemUpgrade).m_mode = 6; // Stone upgrade
				Info(DialogBoxId::ItemUpgrade).m_v2 = so_x;
				Info(DialogBoxId::ItemUpgrade).m_v3 = so_m;
				Info(DialogBoxId::ItemUpgrade).m_v1 = -1;
				Info(DialogBoxId::ItemUpgrade).m_dw_v1 = 0;
			}
			else if (m_game->m_gizon_item_upgrade_left > 0)
			{
				Info(DialogBoxId::ItemUpgrade).m_mode = 1;
				Info(DialogBoxId::ItemUpgrade).m_v2 = -1;
				Info(DialogBoxId::ItemUpgrade).m_v3 = -1;
				Info(DialogBoxId::ItemUpgrade).m_v1 = -1;
				Info(DialogBoxId::ItemUpgrade).m_dw_v1 = 0;
			}
			else
			{
				m_game->add_event_list(DRAW_DIALOGBOX_ITEMUPGRADE30, 10); // "Stone of Xelima or Merien is not present."
				return;
			}
		}
		break;

	case DialogBoxId::MagicShop:
		if (is_enabled(box_id) == false) {
			if (m_game->m_player->m_skill_mastery[4] == 0) {
				disable_dialog_box(DialogBoxId::MagicShop);
				enable_dialog_box(DialogBoxId::NpcTalk, 0, 480, 0);
				return;
			}
			else {
				Info(box_id).m_mode = 0;
				Info(box_id).m_view = 0;
			}
		}
		break;

	case DialogBoxId::Bank:
		text_input_manager::get().end_input();
		if (is_enabled(box_id) == false) {
			Info(box_id).m_mode = 0;
			Info(box_id).m_view = 0;
			enable_dialog_box(DialogBoxId::Inventory, 0, 0, 0);
		}
		break;

	case DialogBoxId::Slates: // Slates
		if (is_enabled(DialogBoxId::Slates) == false) {
			Info(DialogBoxId::Slates).m_view = 0;
			Info(DialogBoxId::Slates).m_mode = type;
			Info(DialogBoxId::Slates).m_v1 = -1;
			Info(DialogBoxId::Slates).m_v2 = -1;
			Info(DialogBoxId::Slates).m_v3 = -1;
			Info(DialogBoxId::Slates).m_v4 = -1;
			Info(DialogBoxId::Slates).m_v5 = -1;
			Info(DialogBoxId::Slates).m_v6 = -1;
			Info(DialogBoxId::Slates).m_str[0] = 0;
			Info(DialogBoxId::Slates).m_str[1] = 0;
			Info(DialogBoxId::Slates).m_str[4] = 0;

			Info(DialogBoxId::Slates).m_size_x = 180;
			Info(DialogBoxId::Slates).m_size_y = 183;

			disable_dialog_box(DialogBoxId::ItemDropExternal);
			disable_dialog_box(DialogBoxId::NpcActionQuery);
			disable_dialog_box(DialogBoxId::SellOrRepair);
			disable_dialog_box(DialogBoxId::Manufacture);
		}
		break;
	case DialogBoxId::ChangeStatsMajestic:
		if (is_enabled(DialogBoxId::ChangeStatsMajestic) == false) {
			Info(DialogBoxId::ChangeStatsMajestic).m_x = Info(DialogBoxId::LevelUpSetting).m_x + 10;
			Info(DialogBoxId::ChangeStatsMajestic).m_y = Info(DialogBoxId::LevelUpSetting).m_y + 10;
			Info(DialogBoxId::ChangeStatsMajestic).m_mode = 0;
			Info(DialogBoxId::ChangeStatsMajestic).m_view = 0;
			m_game->m_player->m_lu_str = m_game->m_player->m_lu_vit = m_game->m_player->m_lu_dex = 0;
			m_game->m_player->m_lu_int = m_game->m_player->m_lu_mag = m_game->m_player->m_lu_char = 0;
			m_game->m_skill_using_status = false;
		}
		break;
	case DialogBoxId::Resurrect: // Snoopy: Resurection
		if (is_enabled(DialogBoxId::Resurrect) == false)
		{
			Info(DialogBoxId::Resurrect).m_x = 185;
			Info(DialogBoxId::Resurrect).m_y = 100;
			Info(DialogBoxId::Resurrect).m_mode = 0;
			Info(DialogBoxId::Resurrect).m_view = 0;
			m_game->m_skill_using_status = false;
		}
		break;

	default:
		text_input_manager::get().end_input();
		if (is_enabled(box_id) == false) {
			Info(box_id).m_mode = 0;
			Info(box_id).m_view = 0;
		}
		break;
	}
	// Bounds-check dialog positions, but skip the HudPanel which has a fixed position at the bottom
	if (box_id != DialogBoxId::HudPanel)
	{
		if (is_enabled(box_id) == false)
		{
			// clamp dialog positions to ensure they stay visible on screen
			int maxX = LOGICAL_WIDTH() - 20;
			int maxY = LOGICAL_HEIGHT() - ICON_PANEL_HEIGHT() - 20;
			if (Info(box_id).m_y > maxY) Info(box_id).m_y = maxY;
			if (Info(box_id).m_x > maxX) Info(box_id).m_x = maxX;
			if ((Info(box_id).m_x + Info(box_id).m_size_x) < 10) Info(box_id).m_x += 20;
			if ((Info(box_id).m_y + Info(box_id).m_size_y) < 10) Info(box_id).m_y += 20;
		}
	}
	set_enabled(box_id, true);
	if (string != 0) std::snprintf(Info(box_id).m_str, sizeof(Info(box_id).m_str), "%s", string);
	for (i = 0; i < 61; i++)
		if (order_at(i) == box_id) set_order_at(i, 0);
	for (i = 1; i < 61; i++)
		if ((order_at(i - 1) == 0) && (order_at(i) != 0)) {
			set_order_at(i - 1, order_at(i));
			set_order_at(i, 0);
		}
	for (i = 0; i < 61; i++)
		if (order_at(i) == 0) {
			set_order_at(i, static_cast<uint8_t>(box_id));
			return;
		}
}


void DialogBoxManager::enable_dialog_box(DialogBoxId::Type id, int type, int v1, int v2, char* string)
{
	enable_dialog_box(static_cast<int>(id), type, v1, v2, string);
}

void DialogBoxManager::disable_dialog_box(int box_id)
{
	int i;

	switch (box_id) {
	case DialogBoxId::ItemDropConfirm:
		{ int idx = Info(DialogBoxId::ItemDropConfirm).m_view;
		if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		break;

	case DialogBoxId::WarningBattleArea:
		{ int idx = Info(DialogBoxId::WarningBattleArea).m_view;
		if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		break;

	case DialogBoxId::GuildMenu:
		if (Info(DialogBoxId::GuildMenu).m_mode == 1)
			text_input_manager::get().end_input();
		Info(DialogBoxId::GuildMenu).m_mode = 0;
		break;

	case DialogBoxId::SaleMenu:
		shop_manager::get().clear_items();
		Info(DialogBoxId::GiveItem).m_v3 = 0;
		Info(DialogBoxId::GiveItem).m_v4 = 0; // v1.4
		Info(DialogBoxId::GiveItem).m_v5 = 0;
		Info(DialogBoxId::GiveItem).m_v6 = 0;
		break;

	case DialogBoxId::Bank:
		if (Info(DialogBoxId::Bank).m_mode < 0) return;
		break;

	case DialogBoxId::ItemDropExternal:
		if (Info(DialogBoxId::ItemDropExternal).m_mode == 1) {
			text_input_manager::get().end_input();
			{ int idx = Info(DialogBoxId::ItemDropExternal).m_view;
			if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		}
		break;

	case DialogBoxId::NpcActionQuery: // v1.4
		{ int idx = Info(DialogBoxId::NpcActionQuery).m_v1;
		if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		break;

	case DialogBoxId::NpcTalk:
		if (Info(DialogBoxId::NpcTalk).m_v2 == 500)
		{
			m_game->send_command(MsgId::CommandCommon, CommonType::GetMagicAbility, 0, 0, 0, 0, 0);
		}
		break;

	case DialogBoxId::Fishing:
		m_game->m_skill_using_status = false;
		break;

	case DialogBoxId::Manufacture:
		{ auto& mfg = Info(DialogBoxId::Manufacture);
		auto clearItem = [&](int idx) { if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; };
		clearItem(mfg.m_v1); clearItem(mfg.m_v2); clearItem(mfg.m_v3);
		clearItem(mfg.m_v4); clearItem(mfg.m_v5); clearItem(mfg.m_v6); }
		m_game->m_skill_using_status = false;
		break;

	case DialogBoxId::Exchange: //Snoopy: 7 mar 06 (multiTrade) case rewriten
		for (i = 0; i < 8; i++)
		{
			// Re-enable item before clearing the slot
			int item_id = m_game->m_dialog_box_exchange_info[i].item_id;
			if (item_id >= 0 && item_id < hb::shared::limits::MaxItems && m_game->m_is_item_disabled[item_id])
				m_game->m_is_item_disabled[item_id] = false;

			m_game->m_dialog_box_exchange_info[i].v1 = -1;
			m_game->m_dialog_box_exchange_info[i].v2 = -1;
			m_game->m_dialog_box_exchange_info[i].v3 = -1;
			m_game->m_dialog_box_exchange_info[i].v4 = -1;
			m_game->m_dialog_box_exchange_info[i].v5 = -1;
			m_game->m_dialog_box_exchange_info[i].v6 = -1;
			m_game->m_dialog_box_exchange_info[i].v7 = -1;
			m_game->m_dialog_box_exchange_info[i].item_id = -1;
			m_game->m_dialog_box_exchange_info[i].dw_v1 = 0;
		}
		break;

	case DialogBoxId::SellList:
		for (i = 0; i < game_limits::max_sell_list; i++)
		{
			{ int idx = m_game->m_sell_item_list[i].index;
			if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
			m_game->m_sell_item_list[i].index = -1;
			m_game->m_sell_item_list[i].amount = 0;
		}
		break;

	case DialogBoxId::ItemUpgrade:
		{ int idx = Info(DialogBoxId::ItemUpgrade).m_v1;
		if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false; }
		break;

	case DialogBoxId::Slates:
	{
		auto& info = Info(DialogBoxId::Slates);
		auto clearSlot = [&](int idx) {
			if (idx >= 0 && idx < hb::shared::limits::MaxItems) m_game->m_is_item_disabled[idx] = false;
		};
		clearSlot(info.m_v1);
		clearSlot(info.m_v2);
		clearSlot(info.m_v3);
		clearSlot(info.m_v4);
	}

		std::memset(Info(DialogBoxId::Slates).m_str, 0, sizeof(Info(DialogBoxId::Slates).m_str));
		std::memset(Info(DialogBoxId::Slates).m_str3, 0, sizeof(Info(DialogBoxId::Slates).m_str3));
		std::memset(Info(DialogBoxId::Slates).m_str4, 0, sizeof(Info(DialogBoxId::Slates).m_str4));
		Info(DialogBoxId::Slates).m_v1 = -1;
		Info(DialogBoxId::Slates).m_v2 = -1;
		Info(DialogBoxId::Slates).m_v3 = -1;
		Info(DialogBoxId::Slates).m_v4 = -1;
		Info(DialogBoxId::Slates).m_v5 = -1;
		Info(DialogBoxId::Slates).m_v6 = -1;
		Info(DialogBoxId::Slates).m_v9 = -1;
		Info(DialogBoxId::Slates).m_v10 = -1;
		Info(DialogBoxId::Slates).m_v11 = -1;
		Info(DialogBoxId::Slates).m_v12 = -1;
		Info(DialogBoxId::Slates).m_v13 = -1;
		Info(DialogBoxId::Slates).m_v14 = -1;
		Info(DialogBoxId::Slates).m_dw_v1 = 0;
		Info(DialogBoxId::Slates).m_dw_v2 = 0;
		break;

	case DialogBoxId::ChangeStatsMajestic:
		m_game->m_player->m_lu_str = 0;
		m_game->m_player->m_lu_vit = 0;
		m_game->m_player->m_lu_dex = 0;
		m_game->m_player->m_lu_int = 0;
		m_game->m_player->m_lu_mag = 0;
		m_game->m_player->m_lu_char = 0;
		break;

	}

	// Call on_disable for migrated dialogs
	if (auto* dlg = get_dialog_box(box_id))
		dlg->on_disable();

	set_enabled(box_id, false);
	for (i = 0; i < 61; i++)
		if (order_at(i) == box_id)
			set_order_at(i, 0);

	for (i = 1; i < 61; i++)
		if ((order_at(i - 1) == 0) && (order_at(i) != 0))
		{
			set_order_at(i - 1, order_at(i));
			set_order_at(i, 0);
		}
}


void DialogBoxManager::disable_dialog_box(DialogBoxId::Type id)
{
	disable_dialog_box(static_cast<int>(id));
}

void DialogBoxManager::toggle_dialog_box(DialogBoxId::Type id, int type, int v1, int v2, char* string)
{
	if (is_enabled(id))
	{
		disable_dialog_box(id);
	}
	else
	{
		enable_dialog_box(id, type, v1, v2, string);
	}
}

int DialogBoxManager::get_top_dialog_box_index() const
{
	if (!m_game) return 0;
	return m_game->get_top_dialog_box_index();
}

void DialogBoxManager::draw_all(short mouse_x, short mouse_y, short z, char lb)
{
	draw_dialog_boxs(mouse_x, mouse_y, z, lb);
}

bool DialogBoxManager::handle_click(short mouse_x, short mouse_y)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		int dlg_id = m_order[60 - i];
		if (dlg_id == 0) continue;

		auto& info = m_info[dlg_id];
		if (mouse_x > info.m_x && mouse_x < info.m_x + info.m_size_x &&
			mouse_y > info.m_y && mouse_y < info.m_y + info.m_size_y)
		{
			if (auto* dlg = m_pDialogBoxes[dlg_id].get())
			{
				dlg->on_click(mouse_x, mouse_y);
			}
			return true;
		}
	}
	return false;
}

bool DialogBoxManager::handle_double_click(short mouse_x, short mouse_y)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		int dlg_id = m_order[60 - i];
		if (dlg_id == 0) continue;

		auto& info = m_info[dlg_id];
		if (mouse_x > info.m_x && mouse_x < info.m_x + info.m_size_x &&
			mouse_y > info.m_y && mouse_y < info.m_y + info.m_size_y)
		{
			if (auto* dlg = m_pDialogBoxes[dlg_id].get())
			{
				dlg->on_double_click(mouse_x, mouse_y);
			}
			return true; // Consumed even if dialog didn't handle it
		}
	}
	return false;
}

PressResult DialogBoxManager::handle_press(int dlg_id, short mouse_x, short mouse_y)
{
	if (dlg_id < 0 || dlg_id >= 61) return PressResult::Normal;

	if (auto* dlg = m_pDialogBoxes[dlg_id].get())
	{
		return dlg->on_press(mouse_x, mouse_y);
	}
	return PressResult::Normal;
}

bool DialogBoxManager::handle_item_drop(int dlg_id, short mouse_x, short mouse_y)
{
	if (dlg_id < 0 || dlg_id >= 61) return false;

	if (auto* dlg = m_pDialogBoxes[dlg_id].get())
	{
		return dlg->on_item_drop(mouse_x, mouse_y);
	}
	return false;
}

bool DialogBoxManager::handle_dragging_item_release(short mouse_x, short mouse_y)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		int dlg_id = m_order[60 - i];
		if (dlg_id == 0) continue;

		auto& info = m_info[dlg_id];
		if (mouse_x > info.m_x && mouse_x < info.m_x + info.m_size_x &&
			mouse_y > info.m_y && mouse_y < info.m_y + info.m_size_y)
		{
			// Bring dialog to front
			enable_dialog_box(dlg_id, 0, 0, 0);

			// Route to dialog's item drop handler
			handle_item_drop(dlg_id, mouse_x, mouse_y);

			return true; // Consumed by this dialog
		}
	}
	return false; // Not consumed - should go to external screen
}

void DialogBoxManager::enable(DialogBoxId::Type id, int type, int v1, int v2, char* string)
{
	enable_dialog_box(id, type, v1, v2, string);
}

void DialogBoxManager::disable(DialogBoxId::Type id)
{
	disable_dialog_box(id);
}

void DialogBoxManager::toggle(DialogBoxId::Type id, int type, int v1, int v2, char* string)
{
	toggle_dialog_box(id, type, v1, v2, string);
}

int DialogBoxManager::get_top_id() const
{
	return get_top_dialog_box_index();
}

bool DialogBoxManager::is_enabled(DialogBoxId::Type id) const
{
	return is_enabled(static_cast<int>(id));
}

bool DialogBoxManager::is_enabled(int box_id) const
{
	if (box_id < 0 || box_id >= 61) return false;
	return m_enabled[box_id];
}

void DialogBoxManager::set_enabled(DialogBoxId::Type id, bool enabled)
{
	set_enabled(static_cast<int>(id), enabled);
}

void DialogBoxManager::set_enabled(int box_id, bool enabled)
{
	if (box_id < 0 || box_id >= 61) return;
	m_enabled[box_id] = enabled;
}

DialogBoxInfo& DialogBoxManager::Info(DialogBoxId::Type id)
{
	return m_info[static_cast<int>(id)];
}

const DialogBoxInfo& DialogBoxManager::Info(DialogBoxId::Type id) const
{
	return m_info[static_cast<int>(id)];
}

DialogBoxInfo& DialogBoxManager::Info(int box_id)
{
	static DialogBoxInfo s_dummy{};
	if (box_id < 0 || box_id >= 61) return s_dummy;
	return m_info[box_id];
}

const DialogBoxInfo& DialogBoxManager::Info(int box_id) const
{
	static const DialogBoxInfo s_dummy{};
	if (box_id < 0 || box_id >= 61) return s_dummy;
	return m_info[box_id];
}

uint8_t DialogBoxManager::order_at(int index) const
{
	if (index < 0 || index >= 61) return 0;
	return m_order[index];
}

void DialogBoxManager::set_order_at(int index, uint8_t value)
{
	if (index < 0 || index >= 61) return;
	m_order[index] = value;
}

int DialogBoxManager::handle_mouse_down(short mouse_x, short mouse_y)
{
	// Find topmost dialog under mouse (iterate in reverse z-order)
	for (int i = 0; i < 61; i++)
	{
		int dlg_id = m_order[60 - i];
		if (dlg_id == 0) continue;

		auto& info = m_info[dlg_id];
		if (mouse_x >= info.m_x && mouse_x <= info.m_x + info.m_size_x &&
			mouse_y >= info.m_y && mouse_y <= info.m_y + info.m_size_y)
		{
			// Bring dialog to front
			enable_dialog_box(dlg_id, 0, 0, 0);

			// Set up drag tracking
			CursorTarget::set_prev_position(mouse_x, mouse_y);
			short dragDistX = mouse_x - info.m_x;
			short dragDistY = mouse_y - info.m_y;

			// Let the dialog handle the press
			PressResult result = handle_press(dlg_id, mouse_x, mouse_y);

			if (result == PressResult::ScrollClaimed)
			{
				// Scroll/slider region claimed - prevent dragging
				info.m_is_scroll_selected = true;
				return -1;
			}
			else if (result == PressResult::Normal)
			{
				// Normal click - set up for dialog dragging
				CursorTarget::set_selection(SelectedObjectType::DialogBox, dlg_id, dragDistX, dragDistY);
			}
			// ItemSelected means item was selected, on_press already set up CursorTarget

			return 1;
		}
	}
	return 0;
}

bool DialogBoxManager::handle_right_click(short mouse_x, short mouse_y, uint32_t time)
{
	// Debounce - prevent closing too quickly
	if ((time - m_dwDialogCloseTime) < 300) return false;

	// Find topmost dialog under mouse
	for (int i = 0; i < 61; i++)
	{
		int dlg_id = m_order[60 - i];
		if (dlg_id == 0) continue;

		auto& info = m_info[dlg_id];
		if (mouse_x > info.m_x && mouse_x < info.m_x + info.m_size_x &&
			mouse_y > info.m_y && mouse_y < info.m_y + info.m_size_y)
		{
			// Check if this dialog can be closed on right-click
			bool can_close = info.m_can_close_on_right_click;

			// Special mode-dependent cases
			switch (dlg_id)
			{
			case DialogBoxId::SellOrRepair:     // 23
				// Can only close if mode < 3
				can_close = (info.m_mode < 3);
				break;

			case DialogBoxId::Exchange:         // 32
				// Cannot close during exchange modes 1 or 3
				if (info.m_mode == 1 || info.m_mode == 3) can_close = false;
				break;
			}

			if (can_close)
			{
				disable_dialog_box(dlg_id);
			}

			m_dwDialogCloseTime = time;
			return true;
		}
	}
	return false;
}

