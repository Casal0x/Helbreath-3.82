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
#include "CursorTarget.h"

DialogBoxManager::DialogBoxManager(CGame* game)
	: m_game(game)
{
}
// Destructor is defaulted in header - unique_ptr handles cleanup automatically

void DialogBoxManager::Initialize(CGame* game)
{
	m_game = game;
}

void DialogBoxManager::InitializeDialogBoxes()
{
	// Dialog boxes are registered here as they are migrated
	// Using std::make_unique for exception-safe, leak-free memory management
	RegisterDialogBox(std::make_unique<DialogBox_WarningMsg>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Resurrect>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Noticement>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_RepairAll>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ConfirmExchange>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Help>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ItemDrop>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_LevelUpSetting>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Character>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Inventory>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Skill>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Magic>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_HudPanel>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_GuideMap>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Fishing>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_CrusadeJob>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ItemDropAmount>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Map>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_NpcActionQuery>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_SysMenu>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Text>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_MagicShop>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_NpcTalk>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ChatHistory>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_CityHallMenu>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Shop>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ItemUpgrade>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_SellList>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_GuildMenu>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_GuildOperation>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Bank>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Exchange>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Party>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Quest>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Commander>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Constructor>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Soldier>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Slates>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_ChangeStatsMajestic>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_GuildHallMenu>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_SellOrRepair>(m_game));
	RegisterDialogBox(std::make_unique<DialogBox_Manufacture>(m_game));
}

void DialogBoxManager::RegisterDialogBox(std::unique_ptr<IDialogBox> pDialogBox)
{
	if (!pDialogBox) return;
	int id = static_cast<int>(pDialogBox->GetId());
	if (id >= 0 && id < 61)
	{
		m_pDialogBoxes[id] = std::move(pDialogBox);  // Transfer ownership
	}
}

IDialogBox* DialogBoxManager::GetDialogBox(DialogBoxId::Type id) const
{
	return GetDialogBox(static_cast<int>(id));
}

IDialogBox* DialogBoxManager::GetDialogBox(int iBoxID) const
{
	if (iBoxID >= 0 && iBoxID < 61)
	{
		return m_pDialogBoxes[iBoxID].get();  // Return raw pointer (no ownership transfer)
	}
	return nullptr;
}

void DialogBoxManager::InitDefaults()
{
	//Character-Info Dialog(F5)
	m_info[DialogBoxId::CharacterInfo].sX = 30 ;
	m_info[DialogBoxId::CharacterInfo].sY = 30 ;
	m_info[DialogBoxId::CharacterInfo].sSizeX = 270;
	m_info[DialogBoxId::CharacterInfo].sSizeY = 376;

	//Inventory Dialog(F6)
	m_info[DialogBoxId::Inventory].sX = 380 ;
	m_info[DialogBoxId::Inventory].sY = 210 ;
	m_info[DialogBoxId::Inventory].sSizeX = 225;
	m_info[DialogBoxId::Inventory].sSizeY = 185;

	//Magic Circle Dialog(F7)
	m_info[DialogBoxId::Magic].sX = 337 ;
	m_info[DialogBoxId::Magic].sY = 57 ;
	m_info[DialogBoxId::Magic].sSizeX = 258;//280;
	m_info[DialogBoxId::Magic].sSizeY = 328;//346;

	// Item drop confirmation
	m_info[DialogBoxId::ItemDropConfirm].sX = 0 ;
	m_info[DialogBoxId::ItemDropConfirm].sY = 0 ;
	m_info[DialogBoxId::ItemDropConfirm].sSizeX = 270;
	m_info[DialogBoxId::ItemDropConfirm].sSizeY = 105;

	// ** This is a battle area **
	m_info[DialogBoxId::WarningBattleArea].sX = 0 ;
	m_info[DialogBoxId::WarningBattleArea].sY = 0 ;
	m_info[DialogBoxId::WarningBattleArea].sSizeX = 310;
	m_info[DialogBoxId::WarningBattleArea].sSizeY = 170;

	//Guild Menu Dialog
	m_info[DialogBoxId::GuildMenu].sX = 337 ;
	m_info[DialogBoxId::GuildMenu].sY = 57 ;
	m_info[DialogBoxId::GuildMenu].sSizeX = 258;
	m_info[DialogBoxId::GuildMenu].sSizeY = 339;

	//Guild Operation Dialog
	m_info[DialogBoxId::GuildOperation].sX = 337 ;
	m_info[DialogBoxId::GuildOperation].sY = 57 ;
	m_info[DialogBoxId::GuildOperation].sSizeX = 295;
	m_info[DialogBoxId::GuildOperation].sSizeY = 346;

	//Guide Map Dialog
	m_info[DialogBoxId::GuideMap].sX = LOGICAL_MAX_X() - 128;
	m_info[DialogBoxId::GuideMap].sY = 0;
	m_info[DialogBoxId::GuideMap].sSizeX = 128;
	m_info[DialogBoxId::GuideMap].sSizeY = 128;

	//Chatting History Dialog(F9)
	m_info[DialogBoxId::ChatHistory].sX = 135 ;
	m_info[DialogBoxId::ChatHistory].sY = 273  ;
	m_info[DialogBoxId::ChatHistory].sSizeX = 364;
	m_info[DialogBoxId::ChatHistory].sSizeY = 162;

	//Sale Menu Dialog
	m_info[DialogBoxId::SaleMenu].sX = 70 ;
	m_info[DialogBoxId::SaleMenu].sY = 50 ;
	m_info[DialogBoxId::SaleMenu].sSizeX = 258;
	m_info[DialogBoxId::SaleMenu].sSizeY = 339;

	//Level-Up Setting Dialog
	m_info[DialogBoxId::LevelUpSetting].sX = 0 ;
	m_info[DialogBoxId::LevelUpSetting].sY = 0 ;
	m_info[DialogBoxId::LevelUpSetting].sSizeX = 258;
	m_info[DialogBoxId::LevelUpSetting].sSizeY = 339;

	//City Hall Menu Dialog
	m_info[DialogBoxId::CityHallMenu].sX = 337 ;
	m_info[DialogBoxId::CityHallMenu].sY = 57 ;
	m_info[DialogBoxId::CityHallMenu].sSizeX = 258;
	m_info[DialogBoxId::CityHallMenu].sSizeY = 339;

	//Bank Dialog
	m_info[DialogBoxId::Bank].sX = 60 ; //337
	m_info[DialogBoxId::Bank].sY = 50 ;
	m_info[DialogBoxId::Bank].sSizeX = 258;
	m_info[DialogBoxId::Bank].sSizeY = 339;
	m_info[DialogBoxId::Bank].sV1 = 13;

	//Skill Menu(F8)
	m_info[DialogBoxId::Skill].sX = 337 ;
	m_info[DialogBoxId::Skill].sY = 57 ;
	m_info[DialogBoxId::Skill].sSizeX = 258;
	m_info[DialogBoxId::Skill].sSizeY = 339;

	//Magic Shop Menu
	m_info[DialogBoxId::MagicShop].sX = 30 ;
	m_info[DialogBoxId::MagicShop].sY = 30 ;
	m_info[DialogBoxId::MagicShop].sSizeX = 304;
	m_info[DialogBoxId::MagicShop].sSizeY = 328;

	//Dialog items drop external screen
	m_info[DialogBoxId::ItemDropExternal].sX = 0 ;
	m_info[DialogBoxId::ItemDropExternal].sY = 0 ;
	m_info[DialogBoxId::ItemDropExternal].sSizeX = 215;
	m_info[DialogBoxId::ItemDropExternal].sSizeY = 87;

	//Text Dialog
	m_info[DialogBoxId::Text].sX = 20 ;
	m_info[DialogBoxId::Text].sY = 65 ;
	m_info[DialogBoxId::Text].sSizeX = 258; // 238
	m_info[DialogBoxId::Text].sSizeY = 339; // 274

	//System Menu Dialog(F12)
	m_info[DialogBoxId::SystemMenu].sX = 337 ;
	m_info[DialogBoxId::SystemMenu].sY = 107 ;
	m_info[DialogBoxId::SystemMenu].sSizeX = 258;//270; //v2.18
	m_info[DialogBoxId::SystemMenu].sSizeY = 268;//346;

	//NpcActionQuery Dialog
	m_info[DialogBoxId::NpcActionQuery].sX = 237 ;
	m_info[DialogBoxId::NpcActionQuery].sY = 57 ;
	m_info[DialogBoxId::NpcActionQuery].sSizeX = 252;
	m_info[DialogBoxId::NpcActionQuery].sSizeY = 87;

	//NpcTalk Dialog
	m_info[DialogBoxId::NpcTalk].sX = 337 ;
	m_info[DialogBoxId::NpcTalk].sY = 57 ;
	m_info[DialogBoxId::NpcTalk].sSizeX = 258;
	m_info[DialogBoxId::NpcTalk].sSizeY = 339;

	//Map
	m_info[DialogBoxId::Map].sX = 336 ;
	m_info[DialogBoxId::Map].sY = 88 ;
	m_info[DialogBoxId::Map].sSizeX = 270;
	m_info[DialogBoxId::Map].sSizeY = 346;

	//ItemSellorRepair Dialog
	m_info[DialogBoxId::SellOrRepair].sX = 337 ;
	m_info[DialogBoxId::SellOrRepair].sY = 57 ;
	m_info[DialogBoxId::SellOrRepair].sSizeX = 258;
	m_info[DialogBoxId::SellOrRepair].sSizeY = 339;

	//Fishing Dialog
	m_info[DialogBoxId::Fishing].sX = 193 ;
	m_info[DialogBoxId::Fishing].sY = 241 ;
	m_info[DialogBoxId::Fishing].sSizeX = 263;
	m_info[DialogBoxId::Fishing].sSizeY = 100;

	//Noticement Dialog
	m_info[DialogBoxId::Noticement].sX = 162 ;
	m_info[DialogBoxId::Noticement].sY = 40 ;
	m_info[DialogBoxId::Noticement].sSizeX = 315;
	m_info[DialogBoxId::Noticement].sSizeY = 171;

	//Manufacture Dialog
	m_info[DialogBoxId::Manufacture].sX = 100 ;
	m_info[DialogBoxId::Manufacture].sY = 60 ;
	m_info[DialogBoxId::Manufacture].sSizeX = 258;
	m_info[DialogBoxId::Manufacture].sSizeY = 339;

	//Exchange Dialog
	m_info[DialogBoxId::Exchange].sX = 100 ;
	m_info[DialogBoxId::Exchange].sY = 30 ;
	m_info[DialogBoxId::Exchange].sSizeX = 520;
	m_info[DialogBoxId::Exchange].sSizeY = 357;

	//Quest Dialog
	m_info[DialogBoxId::Quest].sX = 0 ;
	m_info[DialogBoxId::Quest].sY = 0 ;
	m_info[DialogBoxId::Quest].sSizeX = 258;
	m_info[DialogBoxId::Quest].sSizeY = 339;

	//HUD Panel (combined gauge + icon panel)
	m_info[DialogBoxId::HudPanel].sX = 0;
	m_info[DialogBoxId::HudPanel].sY = LOGICAL_HEIGHT() - ICON_PANEL_HEIGHT();
	m_info[DialogBoxId::HudPanel].sSizeX = ICON_PANEL_WIDTH();
	m_info[DialogBoxId::HudPanel].sSizeY = ICON_PANEL_HEIGHT();

	//Sell List Dialog
	m_info[DialogBoxId::SellList].sX = 170 ;
	m_info[DialogBoxId::SellList].sY = 70 ;
	m_info[DialogBoxId::SellList].sSizeX = 258;
	m_info[DialogBoxId::SellList].sSizeY = 339;

	//Party Dialog
	m_info[DialogBoxId::Party].sX = 0 ;
	m_info[DialogBoxId::Party].sY = 0 ;
	m_info[DialogBoxId::Party].sSizeX = 258;
	m_info[DialogBoxId::Party].sSizeY = 339;

	//Crusade Job Dialog
	m_info[DialogBoxId::CrusadeJob].sX = 360 ;
	m_info[DialogBoxId::CrusadeJob].sY = 65 ;
	m_info[DialogBoxId::CrusadeJob].sSizeX = 258;
	m_info[DialogBoxId::CrusadeJob].sSizeY = 339;

	//Item Upgrade Dialog
	m_info[DialogBoxId::ItemUpgrade].sX = 60 ;
	m_info[DialogBoxId::ItemUpgrade].sY = 50 ;
	m_info[DialogBoxId::ItemUpgrade].sSizeX = 258;
	m_info[DialogBoxId::ItemUpgrade].sSizeY = 339;

	//Help Menu Dialog(F1)
	m_info[DialogBoxId::Help].sX = 358 ;
	m_info[DialogBoxId::Help].sY = 65 ;
	m_info[DialogBoxId::Help].sSizeX = 258;
	m_info[DialogBoxId::Help].sSizeY = 339;

	//Crusade Commander Dialog
	m_info[DialogBoxId::CrusadeCommander].sX = 20 ;
	m_info[DialogBoxId::CrusadeCommander].sY = 20 ;
	m_info[DialogBoxId::CrusadeCommander].sSizeX = 310;
	m_info[DialogBoxId::CrusadeCommander].sSizeY = 386;

	//Crusade Constructor Dialog
	m_info[DialogBoxId::CrusadeConstructor].sX = 20 ;
	m_info[DialogBoxId::CrusadeConstructor].sY = 20 ;
	m_info[DialogBoxId::CrusadeConstructor].sSizeX = 310;
	m_info[DialogBoxId::CrusadeConstructor].sSizeY = 386;

	//Crusade Soldier Dialog
	m_info[DialogBoxId::CrusadeSoldier].sX = 20 ;
	m_info[DialogBoxId::CrusadeSoldier].sY = 20 ;
	m_info[DialogBoxId::CrusadeSoldier].sSizeX = 310;
	m_info[DialogBoxId::CrusadeSoldier].sSizeY = 386;

	// Give item ???
	m_info[DialogBoxId::GiveItem].sX = 0 ;
	m_info[DialogBoxId::GiveItem].sY = 0 ;
	m_info[DialogBoxId::GiveItem].sSizeX = 291;
	m_info[DialogBoxId::GiveItem].sSizeY = 413;

	// 3.51 Slates Dialog - Diuuude
	m_info[DialogBoxId::Slates].sX = 100 ;
	m_info[DialogBoxId::Slates].sY = 60 ;
	m_info[DialogBoxId::Slates].sSizeX = 258;
	m_info[DialogBoxId::Slates].sSizeY = 339;

	// Snoopy: Item exchange confirmation
	m_info[DialogBoxId::ConfirmExchange].sX = 285 ;
	m_info[DialogBoxId::ConfirmExchange].sY = 200 ;
	m_info[DialogBoxId::ConfirmExchange].sSizeX = 270;
	m_info[DialogBoxId::ConfirmExchange].sSizeY = 105;

	// MJ Stats Change DialogBox - Diuuude
	m_info[DialogBoxId::ChangeStatsMajestic].sX = 0 ;
	m_info[DialogBoxId::ChangeStatsMajestic].sY = 0 ;
	m_info[DialogBoxId::ChangeStatsMajestic].sSizeX = 258;
	m_info[DialogBoxId::ChangeStatsMajestic].sSizeY = 339;

	// Snoopy: Resurection
	m_info[DialogBoxId::Resurrect].sX = 185 ;
	m_info[DialogBoxId::Resurrect].sY = 100 ;
	m_info[DialogBoxId::Resurrect].sSizeX = 270;
	m_info[DialogBoxId::Resurrect].sSizeY = 105;

	//Guild Hall Menu Dialog
	m_info[DialogBoxId::GuildHallMenu].sX = 337 ;
	m_info[DialogBoxId::GuildHallMenu].sY = 57 ;
	m_info[DialogBoxId::GuildHallMenu].sSizeX = 258;
	m_info[DialogBoxId::GuildHallMenu].sSizeY = 339;

	//50Cent - Repair All
	m_info[DialogBoxId::RepairAll].sX = 337 ;
	m_info[DialogBoxId::RepairAll].sY = 57 ;
	m_info[DialogBoxId::RepairAll].sSizeX = 258;
	m_info[DialogBoxId::RepairAll].sSizeY = 339;

	// Dialogs that cannot be closed with right-click
	m_info[DialogBoxId::Inventory].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::Magic].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::GuildMenu].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::LevelUpSetting].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::ItemUpgrade].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::Party].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::Slates].bCanCloseOnRightClick = false;
	m_info[DialogBoxId::CrusadeCommander].bCanCloseOnRightClick = false;
}

void DialogBoxManager::UpdateDialogBoxs()
{
	// Update all enabled dialogs (order doesn't matter for update)
	for (int i = 0; i < 61; i++)
	{
		if (m_enabled[i] && m_pDialogBoxes[i])
		{
			m_pDialogBoxes[i]->OnUpdate();  // unique_ptr supports -> operator
		}
	}
}

void DialogBoxManager::DrawDialogBoxs(short msX, short msY, short msZ, char cLB)
{
	if (!m_game) return;
	// For now, delegate to CGame which handles the full draw loop
	// Individual dialogs will be migrated incrementally
	m_game->DrawDialogBoxs(msX, msY, msZ, cLB);
}

void DialogBoxManager::EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char* pString)
{
	if (!m_game) return;
	m_game->EnableDialogBox(iBoxID, cType, sV1, sV2, pString);
}

void DialogBoxManager::EnableDialogBox(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString)
{
	EnableDialogBox(static_cast<int>(id), cType, sV1, sV2, pString);
}

void DialogBoxManager::DisableDialogBox(int iBoxID)
{
	if (!m_game) return;
	m_game->DisableDialogBox(iBoxID);
}

void DialogBoxManager::DisableDialogBox(DialogBoxId::Type id)
{
	DisableDialogBox(static_cast<int>(id));
}

void DialogBoxManager::ToggleDialogBox(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString)
{
	if (IsEnabled(id))
	{
		DisableDialogBox(id);
	}
	else
	{
		EnableDialogBox(id, cType, sV1, sV2, pString);
	}
}

int DialogBoxManager::iGetTopDialogBoxIndex() const
{
	if (!m_game) return 0;
	return m_game->iGetTopDialogBoxIndex();
}

void DialogBoxManager::DrawAll(short msX, short msY, short msZ, char cLB)
{
	DrawDialogBoxs(msX, msY, msZ, cLB);
}

bool DialogBoxManager::HandleClick(short msX, short msY)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		char cDlgID = m_order[60 - i];
		if (cDlgID == 0) continue;

		auto& info = m_info[cDlgID];
		if (msX > info.sX && msX < info.sX + info.sSizeX &&
			msY > info.sY && msY < info.sY + info.sSizeY)
		{
			if (auto* pDlg = m_pDialogBoxes[cDlgID].get())
			{
				pDlg->OnClick(msX, msY);
			}
			return true;
		}
	}
	return false;
}

bool DialogBoxManager::HandleDoubleClick(short msX, short msY)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		char cDlgID = m_order[60 - i];
		if (cDlgID == 0) continue;

		auto& info = m_info[cDlgID];
		if (msX > info.sX && msX < info.sX + info.sSizeX &&
			msY > info.sY && msY < info.sY + info.sSizeY)
		{
			if (auto* pDlg = m_pDialogBoxes[cDlgID].get())
			{
				pDlg->OnDoubleClick(msX, msY);
			}
			return true; // Consumed even if dialog didn't handle it
		}
	}
	return false;
}

PressResult DialogBoxManager::HandlePress(int iDlgID, short msX, short msY)
{
	if (iDlgID < 0 || iDlgID >= 61) return PressResult::Normal;

	if (auto* pDlg = m_pDialogBoxes[iDlgID].get())
	{
		return pDlg->OnPress(msX, msY);
	}
	return PressResult::Normal;
}

bool DialogBoxManager::HandleItemDrop(int iDlgID, short msX, short msY)
{
	if (iDlgID < 0 || iDlgID >= 61) return false;

	if (auto* pDlg = m_pDialogBoxes[iDlgID].get())
	{
		return pDlg->OnItemDrop(msX, msY);
	}
	return false;
}

bool DialogBoxManager::HandleDraggingItemRelease(short msX, short msY)
{
	// Iterate through dialogs in reverse z-order (topmost first)
	for (int i = 0; i < 61; i++)
	{
		char cDlgID = m_order[60 - i];
		if (cDlgID == 0) continue;

		auto& info = m_info[cDlgID];
		if (msX > info.sX && msX < info.sX + info.sSizeX &&
			msY > info.sY && msY < info.sY + info.sSizeY)
		{
			// Bring dialog to front
			EnableDialogBox(cDlgID, 0, 0, 0);

			// Route to dialog's item drop handler
			HandleItemDrop(cDlgID, msX, msY);

			return true; // Consumed by this dialog
		}
	}
	return false; // Not consumed - should go to external screen
}

void DialogBoxManager::Enable(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString)
{
	EnableDialogBox(id, cType, sV1, sV2, pString);
}

void DialogBoxManager::Disable(DialogBoxId::Type id)
{
	DisableDialogBox(id);
}

void DialogBoxManager::Toggle(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString)
{
	ToggleDialogBox(id, cType, sV1, sV2, pString);
}

int DialogBoxManager::GetTopId() const
{
	return iGetTopDialogBoxIndex();
}

bool DialogBoxManager::IsEnabled(DialogBoxId::Type id) const
{
	return IsEnabled(static_cast<int>(id));
}

bool DialogBoxManager::IsEnabled(int iBoxID) const
{
	return m_enabled[iBoxID];
}

void DialogBoxManager::SetEnabled(DialogBoxId::Type id, bool enabled)
{
	SetEnabled(static_cast<int>(id), enabled);
}

void DialogBoxManager::SetEnabled(int iBoxID, bool enabled)
{
	m_enabled[iBoxID] = enabled;
}

DialogBoxInfo& DialogBoxManager::Info(DialogBoxId::Type id)
{
	return m_info[static_cast<int>(id)];
}

const DialogBoxInfo& DialogBoxManager::Info(DialogBoxId::Type id) const
{
	return m_info[static_cast<int>(id)];
}

DialogBoxInfo& DialogBoxManager::Info(int iBoxID)
{
	return m_info[iBoxID];
}

const DialogBoxInfo& DialogBoxManager::Info(int iBoxID) const
{
	return m_info[iBoxID];
}

char DialogBoxManager::OrderAt(int index) const
{
	return m_order[index];
}

void DialogBoxManager::SetOrderAt(int index, char value)
{
	m_order[index] = value;
}

int DialogBoxManager::HandleMouseDown(short msX, short msY)
{
	// Find topmost dialog under mouse (iterate in reverse z-order)
	for (int i = 0; i < 61; i++)
	{
		char cDlgID = m_order[60 - i];
		if (cDlgID == 0) continue;

		auto& info = m_info[cDlgID];
		if (msX >= info.sX && msX <= info.sX + info.sSizeX &&
			msY >= info.sY && msY <= info.sY + info.sSizeY)
		{
			// Bring dialog to front
			EnableDialogBox(cDlgID, 0, 0, 0);

			// Set up drag tracking
			CursorTarget::SetPrevPosition(msX, msY);
			short dragDistX = msX - info.sX;
			short dragDistY = msY - info.sY;

			// Let the dialog handle the press
			PressResult result = HandlePress(cDlgID, msX, msY);

			if (result == PressResult::ScrollClaimed)
			{
				// Scroll/slider region claimed - prevent dragging
				info.bIsScrollSelected = true;
				return -1;
			}
			else if (result == PressResult::Normal)
			{
				// Normal click - set up for dialog dragging
				CursorTarget::SetSelection(SelectedObjectType::DialogBox, cDlgID, dragDistX, dragDistY);
			}
			// ItemSelected means item was selected, OnPress already set up CursorTarget

			return 1;
		}
	}
	return 0;
}

bool DialogBoxManager::HandleRightClick(short msX, short msY, uint32_t dwTime)
{
	// Debounce - prevent closing too quickly
	if ((dwTime - m_dwDialogCloseTime) < 300) return false;

	// Find topmost dialog under mouse
	for (int i = 0; i < 61; i++)
	{
		char cDlgID = m_order[60 - i];
		if (cDlgID == 0) continue;

		auto& info = m_info[cDlgID];
		if (msX > info.sX && msX < info.sX + info.sSizeX &&
			msY > info.sY && msY < info.sY + info.sSizeY)
		{
			// Check if this dialog can be closed on right-click
			bool bCanClose = info.bCanCloseOnRightClick;

			// Special mode-dependent cases
			switch (cDlgID)
			{
			case DialogBoxId::SellOrRepair:     // 23
				// Can only close if cMode < 3
				bCanClose = (info.cMode < 3);
				break;

			case DialogBoxId::Exchange:         // 32
				// Cannot close during exchange modes 1 or 3
				if (info.cMode == 1 || info.cMode == 3) bCanClose = false;
				break;
			}

			if (bCanClose)
			{
				DisableDialogBox(cDlgID);
			}

			m_dwDialogCloseTime = dwTime;
			return true;
		}
	}
	return false;
}

