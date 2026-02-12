#include "DialogBox_Character.h"
#include "ConfigManager.h"
#include "CursorTarget.h"
#include "Game.h"
#include "InventoryManager.h"
#include "ItemNameFormatter.h"
#include "lan_eng.h"
#include "SharedCalculations.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;

using hb::shared::item::EquipPos;
using namespace hb::client::sprite_id;

// Draw order: first entry drawn first (bottom layer), last entry drawn last (top layer).
// Collision checks iterate in reverse so topmost-drawn item has highest click priority.
static constexpr EquipSlotLayout MaleEquipSlots[] = {
	{ EquipPos::Back,        41,  137, false },
	{ EquipPos::Pants,      171,  290, false },
	{ EquipPos::Arms,       171,  290, false },
	{ EquipPos::Leggings,   171,  290, false },
	{ EquipPos::Body,       171,  290, false },
	{ EquipPos::FullBody,   171,  290, false },
	{ EquipPos::LeftHand,    90,  170, true  },
	{ EquipPos::RightHand,   57,  186, true  },
	{ EquipPos::TwoHand,     57,  186, true  },
	{ EquipPos::Neck,        35,  120, false },
	{ EquipPos::RightFinger, 32,  193, false },
	{ EquipPos::LeftFinger,  98,  182, false },
	{ EquipPos::Head,        72,  135, false },
};

static constexpr EquipSlotLayout FemaleEquipSlots[] = {
	{ EquipPos::Back,        45,  143, false },
	{ EquipPos::Pants,      171,  290, false },
	{ EquipPos::Arms,       171,  290, false },
	{ EquipPos::Leggings,   171,  290, false },
	{ EquipPos::Body,       171,  290, false },
	{ EquipPos::FullBody,   171,  290, false },
	{ EquipPos::LeftHand,    84,  175, true  },
	{ EquipPos::RightHand,   60,  191, true  },
	{ EquipPos::TwoHand,     60,  191, true  },
	{ EquipPos::Neck,        35,  120, false },
	{ EquipPos::RightFinger, 32,  193, false },
	{ EquipPos::LeftFinger,  98,  182, false },
	{ EquipPos::Head,        72,  139, false },
};

DialogBox_Character::DialogBox_Character(CGame* pGame)
	: IDialogBox(DialogBoxId::CharacterInfo, pGame)
{
	SetDefaultRect(30 , 30 , 270, 376);
}

// Helper: Display stat with optional angelic bonus (blue if boosted)
void DialogBox_Character::DrawStat(int x1, int x2, int y, int baseStat, int angelicBonus)
{
	if (angelicBonus == 0)
	{
		auto buf = std::format("{}", baseStat);
		PutAlignedString(x1, x2, y, buf.c_str(), GameColors::UILabel);
	}
	else
	{
		auto buf = std::format("{}", baseStat + angelicBonus);
		PutAlignedString(x1, x2, y, buf.c_str(), GameColors::UIModifiedStat);
	}
}

// Find the topmost equipped slot colliding with the mouse, using the given table.
// Returns the EquipPos of the topmost hit, or EquipPos::None if nothing collides.
static EquipPos FindHoverSlot(CGame* pGame, const EquipSlotLayout* slots, int slotCount,
	short sX, short sY, short msX, short msY, const char* cEquipPoiStatus, int spriteOffset)
{
	for (int i = slotCount - 1; i >= 0; i--)
	{
		int ep = static_cast<int>(slots[i].equipPos);
		int itemIdx = cEquipPoiStatus[ep];
		if (itemIdx == -1) continue;

		CItem* pCfg = pGame->GetItemConfig(pGame->m_pItemList[itemIdx]->m_sIDnum);
		if (pCfg == nullptr) continue;

		if (pGame->m_pSprite[ItemEquipPivotPoint + pCfg->m_sSprite + spriteOffset]->CheckCollision(
			sX + slots[i].offsetX, sY + slots[i].offsetY, pCfg->m_sSpriteFrame, msX, msY))
		{
			return slots[i].equipPos;
		}
	}
	return EquipPos::None;
}

// Helper: Render equipped item with optional hover highlight
void DialogBox_Character::DrawEquippedItem(hb::shared::item::EquipPos equipPos, int drawX, int drawY,
	const char* cEquipPoiStatus, bool useWeaponColors, bool bHighlight, int spriteOffset)
{
	int itemIdx = cEquipPoiStatus[static_cast<int>(equipPos)];
	if (itemIdx == -1) return;

	CItem* pItem = m_pGame->m_pItemList[itemIdx].get();
	CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
	if (pCfg == nullptr) return;

	short sSprH = pCfg->m_sSprite;
	short sFrame = pCfg->m_sSpriteFrame;
	char cItemColor = pItem->m_cItemColor;
	bool bDisabled = m_pGame->m_bIsItemDisabled[itemIdx];

	// Select color array based on item type (weapons use different colors)
	const hb::shared::render::Color* colors = useWeaponColors ? GameColors::Weapons : GameColors::Items;

	auto pSprite = m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + spriteOffset];

	if (!bDisabled)
	{
		if (cItemColor == 0)
			pSprite->Draw(drawX, drawY, sFrame);
		else
			pSprite->Draw(drawX, drawY, sFrame, hb::shared::sprite::DrawParams::Tint(colors[cItemColor].r, colors[cItemColor].g, colors[cItemColor].b));
	}
	else
	{
		if (cItemColor == 0)
			pSprite->Draw(drawX, drawY, sFrame, hb::shared::sprite::DrawParams::Alpha(0.25f));
		else
			pSprite->Draw(drawX, drawY, sFrame, hb::shared::sprite::DrawParams::TintedAlpha(colors[cItemColor].r, colors[cItemColor].g, colors[cItemColor].b, 0.7f));
	}

	if (bHighlight)
		pSprite->Draw(drawX, drawY, sFrame, hb::shared::sprite::DrawParams::Additive(0.35f));
}

// Helper: Draw hover button
void DialogBox_Character::DrawHoverButton(int sX, int sY, int btnX, int btnY,
	short msX, short msY, int hoverFrame, int normalFrame)
{
	bool bHover = (msX >= sX + btnX) && (msX <= sX + btnX + ui_layout::btn_size_x) &&
	              (msY >= sY + btnY) && (msY <= sY + btnY + ui_layout::btn_size_y);
	const bool dialogTrans = ConfigManager::Get().IsDialogTransparencyEnabled();
	DrawNewDialogBox(InterfaceNdButton, sX + btnX, sY + btnY,
		bHover ? hoverFrame : normalFrame, false, dialogTrans);
}

void DialogBox_Character::BuildEquipStatusArray(char (&cEquipPoiStatus)[DEF_MAXITEMEQUIPPOS]) const
{
	std::memset(cEquipPoiStatus, -1, sizeof(cEquipPoiStatus));
	for (int i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if (m_pGame->m_pItemList[i] != nullptr && m_pGame->m_bIsItemEquipped[i])
		{
			CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[i]->m_sIDnum);
			if (pCfg != nullptr)
				cEquipPoiStatus[pCfg->m_cEquipPos] = i;
		}
	}
}

char DialogBox_Character::FindEquipItemAtPoint(short msX, short msY, short sX, short sY,
	const char* cEquipPoiStatus) const
{
	const EquipSlotLayout* slots = nullptr;
	int slotCount = 0;
	int spriteOffset = 0;

	if (m_pGame->m_pPlayer->m_sPlayerType >= 1 && m_pGame->m_pPlayer->m_sPlayerType <= 3)
	{
		slots = MaleEquipSlots;
		slotCount = static_cast<int>(std::size(MaleEquipSlots));
	}
	else if (m_pGame->m_pPlayer->m_sPlayerType >= 4 && m_pGame->m_pPlayer->m_sPlayerType <= 6)
	{
		slots = FemaleEquipSlots;
		slotCount = static_cast<int>(std::size(FemaleEquipSlots));
		spriteOffset = 40;
	}

	// Iterate in reverse: topmost drawn item gets highest click priority
	for (int i = slotCount - 1; i >= 0; i--)
	{
		int ep = static_cast<int>(slots[i].equipPos);
		int itemIdx = cEquipPoiStatus[ep];
		if (itemIdx == -1) continue;

		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[itemIdx]->m_sIDnum);
		if (pCfg == nullptr) continue;

		short sSprH = pCfg->m_sSprite;
		short sFrame = pCfg->m_sSpriteFrame;

		if (m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + spriteOffset]->CheckCollision(
			sX + slots[i].offsetX, sY + slots[i].offsetY, sFrame, msX, msY))
		{
			return static_cast<char>(itemIdx);
		}
	}

	return -1;
}

void DialogBox_Character::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureItemConfigsLoaded()) return;
	short sX = Info().sX;
	short sY = Info().sY;
	char cCollison = -1;
	const bool dialogTrans = ConfigManager::Get().IsDialogTransparencyEnabled();

	DrawNewDialogBox(InterfaceNdText, sX, sY, 0, false, dialogTrans);

	// Player name and PK/contribution
	std::string cTxt2;
	std::string infoBuf = m_pGame->m_pPlayer->m_cPlayerName + " : ";

	if (m_pGame->m_pPlayer->m_iPKCount > 0) {
		cTxt2 = std::format(DRAW_DIALOGBOX_CHARACTER1, m_pGame->m_pPlayer->m_iPKCount);
		infoBuf += cTxt2;
	}
	cTxt2 = std::format(DRAW_DIALOGBOX_CHARACTER2, m_pGame->m_pPlayer->m_iContribution);
	infoBuf += cTxt2;
	PutAlignedString(sX + 24, sX + 252, sY + 52, infoBuf.c_str(), GameColors::UIDarkRed);

	// Citizenship / Guild status
	std::string statusBuf;
	if (!m_pGame->m_pPlayer->m_bCitizen)
	{
		statusBuf = DRAW_DIALOGBOX_CHARACTER7;
	}
	else
	{
		statusBuf = m_pGame->m_pPlayer->m_bHunter
			? (m_pGame->m_pPlayer->m_bAresden ? DEF_MSG_ARECIVIL : DEF_MSG_ELVCIVIL)
			: (m_pGame->m_pPlayer->m_bAresden ? DEF_MSG_ARESOLDIER : DEF_MSG_ELVSOLDIER);

		if (m_pGame->m_pPlayer->m_iGuildRank >= 0)
		{
			statusBuf += "(";
			statusBuf += m_pGame->m_pPlayer->m_cGuildName;
			statusBuf += m_pGame->m_pPlayer->m_iGuildRank == 0 ? DEF_MSG_GUILDMASTER1 : DEF_MSG_GUILDSMAN1;
		}
	}
	PutAlignedString(sX, sX + 275, sY + 69, statusBuf.c_str(), GameColors::UILabel);

	// Level, Exp, Next Exp
	std::string statBuf;
	statBuf = std::format("{}", m_pGame->m_pPlayer->m_iLevel);
	PutAlignedString(sX + 180, sX + 250, sY + 106, statBuf.c_str(), GameColors::UILabel);

	statBuf = m_pGame->FormatCommaNumber(m_pGame->m_pPlayer->m_iExp);
	PutAlignedString(sX + 180, sX + 250, sY + 125, statBuf.c_str(), GameColors::UILabel);

	statBuf = m_pGame->FormatCommaNumber(m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel + 1));
	PutAlignedString(sX + 180, sX + 250, sY + 142, statBuf.c_str(), GameColors::UILabel);

	// Calculate max stats
	int iMaxHP = hb::shared::calc::CalculateMaxHP(m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_iLevel, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr);
	int iMaxMP = hb::shared::calc::CalculateMaxMP(m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_iAngelicMag, m_pGame->m_pPlayer->m_iLevel, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_iAngelicInt);
	int iMaxSP = hb::shared::calc::CalculateMaxSP(m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr, m_pGame->m_pPlayer->m_iLevel);
	int iMaxLoad = hb::shared::calc::CalculateMaxLoad(m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr, m_pGame->m_pPlayer->m_iLevel);

	// HP, MP, SP
	std::string valueBuf;
	valueBuf = std::format("{}/{}", m_pGame->m_pPlayer->m_iHP, iMaxHP);
	PutAlignedString(sX + 180, sX + 250, sY + 173, valueBuf.c_str(), GameColors::UILabel);

	valueBuf = std::format("{}/{}", m_pGame->m_pPlayer->m_iMP, iMaxMP);
	PutAlignedString(sX + 180, sX + 250, sY + 191, valueBuf.c_str(), GameColors::UILabel);

	valueBuf = std::format("{}/{}", m_pGame->m_pPlayer->m_iSP, iMaxSP);
	PutAlignedString(sX + 180, sX + 250, sY + 208, valueBuf.c_str(), GameColors::UILabel);

	// Max Load
	int iTotalWeight = InventoryManager::Get().CalcTotalWeight();
	valueBuf = std::format("{}/{}", (iTotalWeight / 100), iMaxLoad);
	PutAlignedString(sX + 180, sX + 250, sY + 240, valueBuf.c_str(), GameColors::UILabel);

	// Enemy Kills
	valueBuf = std::format("{}", m_pGame->m_pPlayer->m_iEnemyKillCount);
	PutAlignedString(sX + 180, sX + 250, sY + 257, valueBuf.c_str(), GameColors::UILabel);

	// Stats with angelic bonuses
	DrawStat(sX + 48, sX + 82, sY + 285, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr);   // Str
	DrawStat(sX + 48, sX + 82, sY + 302, m_pGame->m_pPlayer->m_iDex, m_pGame->m_pPlayer->m_iAngelicDex);   // Dex
	DrawStat(sX + 135, sX + 167, sY + 285, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_iAngelicInt); // Int
	DrawStat(sX + 135, sX + 167, sY + 302, m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_iAngelicMag); // Mag

	// Vit and Chr (no angelic bonus)
	std::string vitChrBuf;
	vitChrBuf = std::format("{}", m_pGame->m_pPlayer->m_iVit);
	PutAlignedString(sX + 218, sX + 251, sY + 285, vitChrBuf.c_str(), GameColors::UILabel);
	vitChrBuf = std::format("{}", m_pGame->m_pPlayer->m_iCharisma);
	PutAlignedString(sX + 218, sX + 251, sY + 302, vitChrBuf.c_str(), GameColors::UILabel);

	// Build equipment status array
	char cEquipPoiStatus[DEF_MAXITEMEQUIPPOS];
	BuildEquipStatusArray(cEquipPoiStatus);

	// Draw character model based on gender
	if (m_pGame->m_pPlayer->m_sPlayerType >= 1 && m_pGame->m_pPlayer->m_sPlayerType <= 3)
	{
		DrawMaleCharacter(sX, sY, msX, msY, cEquipPoiStatus, cCollison);
	}
	else if (m_pGame->m_pPlayer->m_sPlayerType >= 4 && m_pGame->m_pPlayer->m_sPlayerType <= 6)
	{
		DrawFemaleCharacter(sX, sY, msX, msY, cEquipPoiStatus, cCollison);
	}

	// Draw buttons (Quest, Party, LevelUp)
	DrawHoverButton(sX, sY, 15, 340, msX, msY, 5, 4);   // Quest
	DrawHoverButton(sX, sY, 98, 340, msX, msY, 45, 44); // Party
	DrawHoverButton(sX, sY, 180, 340, msX, msY, 11, 10); // LevelUp
}

void DialogBox_Character::DrawMaleCharacter(short sX, short sY, short msX, short msY,
	const char* cEquipPoiStatus, char& cCollison)
{
	// Base body
	m_pGame->m_pSprite[ItemEquipPivotPoint + 0]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 1);

	// Hair (if no helmet)
	if (cEquipPoiStatus[ToInt(EquipPos::Head)] == -1)
	{
		const auto& hc = GameColors::Hair[m_pGame->m_pPlayer->m_playerAppearance.iHairColor];
		m_pGame->m_pSprite[ItemEquipPivotPoint + 18]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iHairStyle, hb::shared::sprite::DrawParams::Tint(hc.r, hc.g, hc.b));
	}

	// Underwear
	m_pGame->m_pSprite[ItemEquipPivotPoint + 19]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iUnderwearType);

	// Find topmost hovered slot (reverse scan) before drawing
	EquipPos hoverSlot = FindHoverSlot(m_pGame, MaleEquipSlots, static_cast<int>(std::size(MaleEquipSlots)),
		sX, sY, msX, msY, cEquipPoiStatus, 0);
	if (hoverSlot != EquipPos::None)
		cCollison = static_cast<char>(hoverSlot);

	// Equipment slots (draw order from table)
	for (const auto& slot : MaleEquipSlots)
	{
		DrawEquippedItem(slot.equipPos, sX + slot.offsetX, sY + slot.offsetY,
			cEquipPoiStatus, slot.useWeaponColors, slot.equipPos == hoverSlot);
	}

	// Angel staff special case
	if (cEquipPoiStatus[ToInt(EquipPos::TwoHand)] != -1)
	{
		int itemIdx = cEquipPoiStatus[ToInt(EquipPos::TwoHand)];
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[itemIdx]->m_sIDnum);
		if (pCfg != nullptr)
		{
			short sSprH = pCfg->m_sSprite;
			short sFrame = pCfg->m_sSpriteFrame;
			if (sSprH == 8) // Angel staff
			{
				if (!m_pGame->m_bIsItemDisabled[itemIdx])
					m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
				else
					m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
			}
		}
	}
}

void DialogBox_Character::DrawFemaleCharacter(short sX, short sY, short msX, short msY,
	const char* cEquipPoiStatus, char& cCollison)
{
	// Base body (female uses +40 offset from male sprites)
	m_pGame->m_pSprite[ItemEquipPivotPoint + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 4);

	// Hair (if no helmet) - female hair is at +18+40 = +58
	if (cEquipPoiStatus[ToInt(EquipPos::Head)] == -1)
	{
		const auto& hc = GameColors::Hair[m_pGame->m_pPlayer->m_playerAppearance.iHairColor];
		m_pGame->m_pSprite[ItemEquipPivotPoint + 18 + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iHairStyle, hb::shared::sprite::DrawParams::Tint(hc.r, hc.g, hc.b));
	}

	// Underwear - female underwear is at +19+40 = +59
	m_pGame->m_pSprite[ItemEquipPivotPoint + 19 + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iUnderwearType);

	// Check for skirt in pants slot (sprite 12, frame 0 = skirt)
	bool bSkirt = false;
	if (cEquipPoiStatus[ToInt(EquipPos::Pants)] != -1)
	{
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cEquipPoiStatus[ToInt(EquipPos::Pants)]]->m_sIDnum);
		if (pCfg != nullptr && pCfg->m_sSprite == 12 && pCfg->m_sSpriteFrame == 0)
			bSkirt = true;
	}

	// Find topmost hovered slot (reverse scan) before drawing
	EquipPos hoverSlot = FindHoverSlot(m_pGame, FemaleEquipSlots, static_cast<int>(std::size(FemaleEquipSlots)),
		sX, sY, msX, msY, cEquipPoiStatus, 40);
	if (hoverSlot != EquipPos::None)
		cCollison = static_cast<char>(hoverSlot);

	// If wearing skirt, pre-draw boots under the skirt
	if (bSkirt)
		DrawEquippedItem(EquipPos::Leggings, sX + 171, sY + 290, cEquipPoiStatus, false, hoverSlot == EquipPos::Leggings, 40);

	// Equipment slots (draw order from table)
	for (const auto& slot : FemaleEquipSlots)
	{
		if (bSkirt && slot.equipPos == EquipPos::Leggings) continue; // already drawn
		DrawEquippedItem(slot.equipPos, sX + slot.offsetX, sY + slot.offsetY,
			cEquipPoiStatus, slot.useWeaponColors, slot.equipPos == hoverSlot, 40);
	}

	// Angel staff special case
	if (cEquipPoiStatus[ToInt(EquipPos::TwoHand)] != -1)
	{
		int itemIdx = cEquipPoiStatus[ToInt(EquipPos::TwoHand)];
		CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[itemIdx]->m_sIDnum);
		if (pCfg != nullptr)
		{
			short sSprH = pCfg->m_sSprite;
			short sFrame = pCfg->m_sSpriteFrame;
			if (sSprH == 8) // Angel staff
			{
				if (!m_pGame->m_bIsItemDisabled[itemIdx])
					m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
				else
					m_pGame->m_pSprite[ItemEquipPivotPoint + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
			}
		}
	}
}

bool DialogBox_Character::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Quest button
	if ((msX >= sX + 15) && (msX <= sX + 15 + ui_layout::btn_size_x) && (msY >= sY + 340) && (msY <= sY + 340 + ui_layout::btn_size_y)) {
		EnableDialogBox(DialogBoxId::Quest, 1, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}
	// Party button
	if ((msX >= sX + 98) && (msX <= sX + 98 + ui_layout::btn_size_x) && (msY >= sY + 340) && (msY <= sY + 340 + ui_layout::btn_size_y)) {
		EnableDialogBox(DialogBoxId::Party, 0, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}
	// LevelUp button
	if ((msX >= sX + 180) && (msX <= sX + 180 + ui_layout::btn_size_x) && (msY >= sY + 340) && (msY <= sY + 340 + ui_layout::btn_size_y)) {
		EnableDialogBox(DialogBoxId::LevelUpSetting, 0, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}

	return false;
}

bool DialogBox_Character::OnDoubleClick(short msX, short msY)
{
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal))
		return false;

	short sX = Info().sX;
	short sY = Info().sY;

	// Build equipment position status array
	char cEquipPoiStatus[DEF_MAXITEMEQUIPPOS];
	BuildEquipStatusArray(cEquipPoiStatus);

	// Find clicked item
	char cItemID = FindEquipItemAtPoint(msX, msY, sX, sY, cEquipPoiStatus);
	if (cItemID == -1 || m_pGame->m_pItemList[cItemID] == nullptr)
		return false;

	CItem* pItem = m_pGame->m_pItemList[cItemID].get();
	CItem* pCfg = m_pGame->GetItemConfig(pItem->m_sIDnum);
	if (pCfg == nullptr)
		return false;

	// Skip consumables, arrows, and stacked items
	if (pCfg->GetItemType() == ItemType::Eat ||
		pCfg->GetItemType() == ItemType::Consume ||
		pCfg->GetItemType() == ItemType::Arrow ||
		pItem->m_dwCount > 1)
		return false;

	// Check if at repair shop
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SaleMenu) &&
		!m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) &&
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 == 24)
	{
		bSendCommand(MsgId::CommandCommon, CommonType::ReqRepairItem, 0, cItemID,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3, 0,
			pCfg->m_cName,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
	}
	else
	{
		// Release (unequip) the item
		if (m_pGame->m_bIsItemEquipped[cItemID])
		{
			std::string G_cTxt;
			auto itemInfo = ItemNameFormatter::Get().Format(pItem);
			G_cTxt = std::format(ITEM_EQUIPMENT_RELEASED, itemInfo.name.c_str());
			AddEventList(G_cTxt.c_str(), 10);

			{
				short sID = pItem->m_sIDnum;
				if (sID == hb::shared::item::ItemId::AngelicPandentSTR || sID == hb::shared::item::ItemId::AngelicPandentDEX ||
					sID == hb::shared::item::ItemId::AngelicPandentINT || sID == hb::shared::item::ItemId::AngelicPandentMAG)
					m_pGame->PlayGameSound('E', 53, 0);
				else
					m_pGame->PlayGameSound('E', 29, 0);
			}

			// Remove Angelic Stats
			if (pCfg->m_cEquipPos >= 11 &&
				pCfg->GetItemType() == ItemType::Equip)
			{
				if (pItem->m_sIDnum == hb::shared::item::ItemId::AngelicPandentSTR)
					m_pGame->m_pPlayer->m_iAngelicStr = 0;
				else if (pItem->m_sIDnum == hb::shared::item::ItemId::AngelicPandentDEX)
					m_pGame->m_pPlayer->m_iAngelicDex = 0;
				else if (pItem->m_sIDnum == hb::shared::item::ItemId::AngelicPandentINT)
					m_pGame->m_pPlayer->m_iAngelicInt = 0;
				else if (pItem->m_sIDnum == hb::shared::item::ItemId::AngelicPandentMAG)
					m_pGame->m_pPlayer->m_iAngelicMag = 0;
			}

			bSendCommand(MsgId::CommandCommon, CommonType::ReleaseItem, 0, cItemID, 0, 0, 0);
			m_pGame->m_bIsItemEquipped[cItemID] = false;
			m_pGame->m_sItemEquipmentStatus[pCfg->m_cEquipPos] = -1;
			CursorTarget::ClearSelection();
		}
	}

	return true;
}

PressResult DialogBox_Character::OnPress(short msX, short msY)
{
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal))
		return PressResult::Normal;

	short sX = Info().sX;
	short sY = Info().sY;

	char cEquipPoiStatus[DEF_MAXITEMEQUIPPOS];
	BuildEquipStatusArray(cEquipPoiStatus);

	char itemIdx = FindEquipItemAtPoint(msX, msY, sX, sY, cEquipPoiStatus);
	if (itemIdx != -1)
	{
		CursorTarget::SetSelection(SelectedObjectType::Item, static_cast<short>(itemIdx), 0, 0);
		return PressResult::ItemSelected;
	}

	return PressResult::Normal;
}

bool DialogBox_Character::OnItemDrop(short msX, short msY)
{
	InventoryManager::Get().EquipItem(static_cast<char>(CursorTarget::GetSelectedID()));
	return true;
}
