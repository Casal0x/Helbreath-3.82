#include "DialogBox_Character.h"
#include "ConfigManager.h"
#include "CursorTarget.h"
#include "Game.h"
#include "lan_eng.h"
#include "SharedCalculations.h"

using namespace hb::item;

using hb::item::EquipPos;

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
	char buf[16];
	if (angelicBonus == 0)
	{
		snprintf(buf, sizeof(buf), "%d", baseStat);
		PutAlignedString(x1, x2, y, buf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%d", baseStat + angelicBonus);
		PutAlignedString(x1, x2, y, buf, GameColors::UIModifiedStat.r, GameColors::UIModifiedStat.g, GameColors::UIModifiedStat.b);
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

		if (pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + pCfg->m_sSprite + spriteOffset]->CheckCollision(
			sX + slots[i].offsetX, sY + slots[i].offsetY, pCfg->m_sSpriteFrame, msX, msY))
		{
			return slots[i].equipPos;
		}
	}
	return EquipPos::None;
}

// Helper: Render equipped item with optional hover highlight
void DialogBox_Character::DrawEquippedItem(hb::item::EquipPos equipPos, int drawX, int drawY,
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
	const GameColor* colors = useWeaponColors ? GameColors::Weapons : GameColors::Items;

	auto pSprite = m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + spriteOffset];

	if (!bDisabled)
	{
		if (cItemColor == 0)
			pSprite->Draw(drawX, drawY, sFrame);
		else
			pSprite->Draw(drawX, drawY, sFrame, SpriteLib::DrawParams::Tint(colors[cItemColor].r - GameColors::Base.r, colors[cItemColor].g - GameColors::Base.g, colors[cItemColor].b - GameColors::Base.b));
	}
	else
	{
		if (cItemColor == 0)
			pSprite->Draw(drawX, drawY, sFrame, SpriteLib::DrawParams::Alpha(0.25f));
		else
			pSprite->Draw(drawX, drawY, sFrame, SpriteLib::DrawParams::TintedAlpha(colors[cItemColor].r - GameColors::Base.r, colors[cItemColor].g - GameColors::Base.g, colors[cItemColor].b - GameColors::Base.b, 0.7f));
	}

	if (bHighlight)
		pSprite->Draw(drawX, drawY, sFrame, SpriteLib::DrawParams::Additive(0.35f));
}

// Helper: Draw hover button
void DialogBox_Character::DrawHoverButton(int sX, int sY, int btnX, int btnY,
	short msX, short msY, int hoverFrame, int normalFrame)
{
	bool bHover = (msX >= sX + btnX) && (msX <= sX + btnX + DEF_BTNSZX) &&
	              (msY >= sY + btnY) && (msY <= sY + btnY + DEF_BTNSZY);
	const bool dialogTrans = ConfigManager::Get().IsDialogTransparencyEnabled();
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + btnX, sY + btnY,
		bHover ? hoverFrame : normalFrame, false, dialogTrans);
}

void DialogBox_Character::BuildEquipStatusArray(char (&cEquipPoiStatus)[DEF_MAXITEMEQUIPPOS]) const
{
	std::memset(cEquipPoiStatus, -1, sizeof(cEquipPoiStatus));
	for (int i = 0; i < DEF_MAXITEMS; i++)
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

		if (m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + spriteOffset]->CheckCollision(
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

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 0, false, dialogTrans);

	// Player name and PK/contribution
	char infoBuf[128];
	char cTxt2[64];
	snprintf(infoBuf, sizeof(infoBuf), "%s : ", m_pGame->m_pPlayer->m_cPlayerName);

	if (m_pGame->m_pPlayer->m_iPKCount > 0) {
		snprintf(cTxt2, sizeof(cTxt2), DRAW_DIALOGBOX_CHARACTER1, m_pGame->m_pPlayer->m_iPKCount);
		strncat(infoBuf, cTxt2, sizeof(infoBuf) - strlen(infoBuf) - 1);
	}
	snprintf(cTxt2, sizeof(cTxt2), DRAW_DIALOGBOX_CHARACTER2, m_pGame->m_pPlayer->m_iContribution);
	strncat(infoBuf, cTxt2, sizeof(infoBuf) - strlen(infoBuf) - 1);
	PutAlignedString(sX + 24, sX + 252, sY + 52, infoBuf, GameColors::UIDarkRed.r, GameColors::UIDarkRed.g, GameColors::UIDarkRed.b);

	// Citizenship / Guild status
	char statusBuf[128] = {};
	if (!m_pGame->m_pPlayer->m_bCitizen)
	{
		strncpy(statusBuf, DRAW_DIALOGBOX_CHARACTER7, sizeof(statusBuf) - 1);
	}
	else
	{
		strncpy(statusBuf, m_pGame->m_pPlayer->m_bHunter
			? (m_pGame->m_pPlayer->m_bAresden ? DEF_MSG_ARECIVIL : DEF_MSG_ELVCIVIL)
			: (m_pGame->m_pPlayer->m_bAresden ? DEF_MSG_ARESOLDIER : DEF_MSG_ELVSOLDIER), sizeof(statusBuf) - 1);

		if (m_pGame->m_pPlayer->m_iGuildRank >= 0)
		{
			strncat(statusBuf, "(", sizeof(statusBuf) - strlen(statusBuf) - 1);
			strncat(statusBuf, m_pGame->m_pPlayer->m_cGuildName, sizeof(statusBuf) - strlen(statusBuf) - 1);
			strncat(statusBuf, m_pGame->m_pPlayer->m_iGuildRank == 0 ? DEF_MSG_GUILDMASTER1 : DEF_MSG_GUILDSMAN1, sizeof(statusBuf) - strlen(statusBuf) - 1);
		}
	}
	PutAlignedString(sX, sX + 275, sY + 69, statusBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	// Level, Exp, Next Exp
	char statBuf[32];
	snprintf(statBuf, sizeof(statBuf), "%d", m_pGame->m_pPlayer->m_iLevel);
	PutAlignedString(sX + 180, sX + 250, sY + 106, statBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	m_pGame->FormatCommaNumber(m_pGame->m_pPlayer->m_iExp, statBuf, sizeof(statBuf));
	PutAlignedString(sX + 180, sX + 250, sY + 125, statBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	m_pGame->FormatCommaNumber(m_pGame->iGetLevelExp(m_pGame->m_pPlayer->m_iLevel + 1), statBuf, sizeof(statBuf));
	PutAlignedString(sX + 180, sX + 250, sY + 142, statBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	// Calculate max stats
	int iMaxHP = CalculateMaxHP(m_pGame->m_pPlayer->m_iVit, m_pGame->m_pPlayer->m_iLevel, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr);
	int iMaxMP = CalculateMaxMP(m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_iAngelicMag, m_pGame->m_pPlayer->m_iLevel, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_iAngelicInt);
	int iMaxSP = CalculateMaxSP(m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr, m_pGame->m_pPlayer->m_iLevel);
	int iMaxLoad = CalculateMaxLoad(m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr, m_pGame->m_pPlayer->m_iLevel);

	// HP, MP, SP
	char valueBuf[32];
	snprintf(valueBuf, sizeof(valueBuf), "%d/%d", m_pGame->m_pPlayer->m_iHP, iMaxHP);
	PutAlignedString(sX + 180, sX + 250, sY + 173, valueBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	snprintf(valueBuf, sizeof(valueBuf), "%d/%d", m_pGame->m_pPlayer->m_iMP, iMaxMP);
	PutAlignedString(sX + 180, sX + 250, sY + 191, valueBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	snprintf(valueBuf, sizeof(valueBuf), "%d/%d", m_pGame->m_pPlayer->m_iSP, iMaxSP);
	PutAlignedString(sX + 180, sX + 250, sY + 208, valueBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	// Max Load
	int iTotalWeight = m_pGame->_iCalcTotalWeight();
	snprintf(valueBuf, sizeof(valueBuf), "%d/%d", (iTotalWeight / 100), iMaxLoad);
	PutAlignedString(sX + 180, sX + 250, sY + 240, valueBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	// Enemy Kills
	snprintf(valueBuf, sizeof(valueBuf), "%d", m_pGame->m_pPlayer->m_iEnemyKillCount);
	PutAlignedString(sX + 180, sX + 250, sY + 257, valueBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

	// Stats with angelic bonuses
	DrawStat(sX + 48, sX + 82, sY + 285, m_pGame->m_pPlayer->m_iStr, m_pGame->m_pPlayer->m_iAngelicStr);   // Str
	DrawStat(sX + 48, sX + 82, sY + 302, m_pGame->m_pPlayer->m_iDex, m_pGame->m_pPlayer->m_iAngelicDex);   // Dex
	DrawStat(sX + 135, sX + 167, sY + 285, m_pGame->m_pPlayer->m_iInt, m_pGame->m_pPlayer->m_iAngelicInt); // Int
	DrawStat(sX + 135, sX + 167, sY + 302, m_pGame->m_pPlayer->m_iMag, m_pGame->m_pPlayer->m_iAngelicMag); // Mag

	// Vit and Chr (no angelic bonus)
	char vitChrBuf[16];
	snprintf(vitChrBuf, sizeof(vitChrBuf), "%d", m_pGame->m_pPlayer->m_iVit);
	PutAlignedString(sX + 218, sX + 251, sY + 285, vitChrBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);
	snprintf(vitChrBuf, sizeof(vitChrBuf), "%d", m_pGame->m_pPlayer->m_iCharisma);
	PutAlignedString(sX + 218, sX + 251, sY + 302, vitChrBuf, GameColors::UILabel.r, GameColors::UILabel.g, GameColors::UILabel.b);

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
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 1);

	// Hair (if no helmet)
	if (cEquipPoiStatus[ToInt(EquipPos::Head)] == -1)
	{
		int iR, iG, iB;
		m_pGame->_GetHairColorRGB(m_pGame->m_pPlayer->m_playerAppearance.iHairColor, &iR, &iG, &iB);
		m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iHairStyle, SpriteLib::DrawParams::Tint(iR, iG, iB));
	}

	// Underwear
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iUnderwearType);

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
					m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
				else
					m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
		}
	}
}

void DialogBox_Character::DrawFemaleCharacter(short sX, short sY, short msX, short msY,
	const char* cEquipPoiStatus, char& cCollison)
{
	// Base body (female uses +40 offset from male sprites)
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 4);

	// Hair (if no helmet) - female hair is at +18+40 = +58
	if (cEquipPoiStatus[ToInt(EquipPos::Head)] == -1)
	{
		int iR, iG, iB;
		m_pGame->_GetHairColorRGB(m_pGame->m_pPlayer->m_playerAppearance.iHairColor, &iR, &iG, &iB);
		m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18 + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iHairStyle, SpriteLib::DrawParams::Tint(iR, iG, iB));
	}

	// Underwear - female underwear is at +19+40 = +59
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19 + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_playerAppearance.iUnderwearType);

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
					m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
				else
					m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
		}
	}
}

bool DialogBox_Character::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Quest button
	if ((msX >= sX + 15) && (msX <= sX + 15 + DEF_BTNSZX) && (msY >= sY + 340) && (msY <= sY + 340 + DEF_BTNSZY)) {
		EnableDialogBox(DialogBoxId::Quest, 1, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}
	// Party button
	if ((msX >= sX + 98) && (msX <= sX + 98 + DEF_BTNSZX) && (msY >= sY + 340) && (msY <= sY + 340 + DEF_BTNSZY)) {
		EnableDialogBox(DialogBoxId::Party, 0, 0, 0);
		DisableThisDialog();
		PlaySoundEffect('E', 14, 5);
		return true;
	}
	// LevelUp button
	if ((msX >= sX + 180) && (msX <= sX + 180 + DEF_BTNSZX) && (msY >= sY + 340) && (msY <= sY + 340 + DEF_BTNSZY)) {
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
		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_REPAIRITEM, 0, cItemID,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3, 0,
			pCfg->m_cName,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
	}
	else
	{
		// Release (unequip) the item
		if (m_pGame->m_bIsItemEquipped[cItemID])
		{
			char cStr1[64], cStr2[64], cStr3[64];
			m_pGame->GetItemName(pItem, cStr1, cStr2, cStr3);
			std::memset(m_pGame->G_cTxt, 0, sizeof(m_pGame->G_cTxt));
			wsprintf(m_pGame->G_cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
			AddEventList(m_pGame->G_cTxt, 10);

			{
				short sID = pItem->m_sIDnum;
				if (sID == hb::item::ItemId::AngelicPandentSTR || sID == hb::item::ItemId::AngelicPandentDEX ||
					sID == hb::item::ItemId::AngelicPandentINT || sID == hb::item::ItemId::AngelicPandentMAG)
					m_pGame->PlayGameSound('E', 53, 0);
				else
					m_pGame->PlayGameSound('E', 29, 0);
			}

			// Remove Angelic Stats
			if (pCfg->m_cEquipPos >= 11 &&
				pCfg->GetItemType() == ItemType::Equip)
			{
				if (pItem->m_sIDnum == hb::item::ItemId::AngelicPandentSTR)
					m_pGame->m_pPlayer->m_iAngelicStr = 0;
				else if (pItem->m_sIDnum == hb::item::ItemId::AngelicPandentDEX)
					m_pGame->m_pPlayer->m_iAngelicDex = 0;
				else if (pItem->m_sIDnum == hb::item::ItemId::AngelicPandentINT)
					m_pGame->m_pPlayer->m_iAngelicInt = 0;
				else if (pItem->m_sIDnum == hb::item::ItemId::AngelicPandentMAG)
					m_pGame->m_pPlayer->m_iAngelicMag = 0;
			}

			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_RELEASEITEM, 0, cItemID, 0, 0, 0);
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
	m_pGame->ItemEquipHandler((char)CursorTarget::GetSelectedID());
	return true;
}
