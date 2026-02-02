#include "DialogBox_Character.h"
#include "ConfigManager.h"
#include "CursorTarget.h"
#include "Game.h"
#include "lan_eng.h"
#include "SharedCalculations.h"

DialogBox_Character::DialogBox_Character(CGame* pGame)
	: IDialogBox(DialogBoxId::CharacterInfo, pGame)
{
	SetDefaultRect(30 + SCREENX(), 30 + SCREENY(), 270, 376);
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

// Helper: Render equipped item and check collision
char DialogBox_Character::DrawEquippedItem(int equipPos, int drawX, int drawY, short msX, short msY,
	const char* cEquipPoiStatus, bool useWeaponColors, int spriteOffset)
{
	int itemIdx = cEquipPoiStatus[equipPos];
	if (itemIdx == -1) return -1;

	short sSprH = m_pGame->m_pItemList[itemIdx]->m_sSprite;
	short sFrame = m_pGame->m_pItemList[itemIdx]->m_sSpriteFrame;
	char cItemColor = m_pGame->m_pItemList[itemIdx]->m_cItemColor;
	bool bDisabled = m_pGame->m_bIsItemDisabled[itemIdx];

	// Select color array based on item type (weapons use different colors)
	const GameColor* colors = useWeaponColors ? GameColors::Weapons : GameColors::Items;
	// (wG/wB merged into GameColor array above)
	

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

	if (pSprite->CheckCollision(drawX, drawY, sFrame, msX, msY))
		return (char)equipPos;

	return -1;
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

void DialogBox_Character::OnDraw(short msX, short msY, short msZ, char cLB)
{
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
	std::memset(cEquipPoiStatus, -1, sizeof(cEquipPoiStatus));
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		if (m_pGame->m_pItemList[i] != nullptr && m_pGame->m_bIsItemEquipped[i])
			cEquipPoiStatus[m_pGame->m_pItemList[i]->m_cEquipPos] = i;
	}

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
	int iR, iG, iB;
	short sSprH, sFrame;
	char cItemColor;

	// Base body
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 1);

	// Hair (if no helmet)
	if (cEquipPoiStatus[DEF_EQUIPPOS_HEAD] == -1)
	{
		m_pGame->_GetHairColorRGB(((m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
		m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18]->Draw(sX + 171, sY + 290, (m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x0F00) >> 8, SpriteLib::DrawParams::Tint(iR, iG, iB));
	}

	// Underwear
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19]->Draw(sX + 171, sY + 290, (m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x000F));

	// Equipment slots at character position (171, 290)
	char result;
	result = DrawEquippedItem(DEF_EQUIPPOS_BACK, sX + 41, sY + 137, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	// Weapons use weapon colors
	result = DrawEquippedItem(DEF_EQUIPPOS_LHAND, sX + 90, sY + 170, msX, msY, cEquipPoiStatus, true);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_RHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus, true);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_TWOHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus, true);
	if (result != -1) cCollison = result;

	// Accessories
	result = DrawEquippedItem(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	// HEAD at position (72, 135)
	result = DrawEquippedItem(DEF_EQUIPPOS_HEAD, sX + 72, sY + 135, msX, msY, cEquipPoiStatus, false);
	if (result != -1) cCollison = result;

	// Angel staff special case
	if (cEquipPoiStatus[DEF_EQUIPPOS_TWOHAND] != -1)
	{
		int itemIdx = cEquipPoiStatus[DEF_EQUIPPOS_TWOHAND];
		sSprH = m_pGame->m_pItemList[itemIdx]->m_sSprite;
		sFrame = m_pGame->m_pItemList[itemIdx]->m_sSpriteFrame;
		cItemColor = m_pGame->m_pItemList[itemIdx]->m_cItemColor;
		if (sSprH == 8) // Angel staff
		{
			if (!m_pGame->m_bIsItemDisabled[itemIdx])
				m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
			else
				m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, SpriteLib::DrawParams::Alpha(0.5f));
		}
	}
}

void DialogBox_Character::DrawFemaleCharacter(short sX, short sY, short msX, short msY,
	const char* cEquipPoiStatus, char& cCollison)
{
	int iR, iG, iB;
	short sSprH, sFrame;
	char cItemColor;
	int iSkirtDraw = 0;

	// Base body (female uses +40 offset from male sprites)
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40]->Draw(sX + 171, sY + 290, m_pGame->m_pPlayer->m_sPlayerType - 4);

	// Hair (if no helmet) - female hair is at +18+40 = +58
	if (cEquipPoiStatus[DEF_EQUIPPOS_HEAD] == -1)
	{
		m_pGame->_GetHairColorRGB(((m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
		m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18 + 40]->Draw(sX + 171, sY + 290, (m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x0F00) >> 8, SpriteLib::DrawParams::Tint(iR, iG, iB));
	}

	// Underwear - female underwear is at +19+40 = +59
	m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19 + 40]->Draw(sX + 171, sY + 290, (m_pGame->m_pPlayer->m_sPlayerAppr1 & 0x000F));

	// Check for skirt in pants slot (sprite 12, frame 0 = skirt)
	if (cEquipPoiStatus[DEF_EQUIPPOS_PANTS] != -1)
	{
		sSprH = m_pGame->m_pItemList[cEquipPoiStatus[DEF_EQUIPPOS_PANTS]]->m_sSprite;
		sFrame = m_pGame->m_pItemList[cEquipPoiStatus[DEF_EQUIPPOS_PANTS]]->m_sSpriteFrame;
		if ((sSprH == 12) && (sFrame == 0)) iSkirtDraw = 1;
	}

	// Back item - female position is different (45, 143)
	char result;
	result = DrawEquippedItem(DEF_EQUIPPOS_BACK, sX + 45, sY + 143, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// If wearing skirt, draw boots first (before pants)
	if ((cEquipPoiStatus[DEF_EQUIPPOS_BOOTS] != -1) && (iSkirtDraw == 1))
	{
		result = DrawEquippedItem(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
		if (result != -1) cCollison = result;
	}

	// Pants
	result = DrawEquippedItem(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// Arms
	result = DrawEquippedItem(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// If not wearing skirt, draw boots after arms
	if ((cEquipPoiStatus[DEF_EQUIPPOS_BOOTS] != -1) && (iSkirtDraw == 0))
	{
		result = DrawEquippedItem(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
		if (result != -1) cCollison = result;
	}

	// Body
	result = DrawEquippedItem(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// Fullbody
	result = DrawEquippedItem(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// Weapons (use weapon colors) - female positions are different
	result = DrawEquippedItem(DEF_EQUIPPOS_LHAND, sX + 84, sY + 175, msX, msY, cEquipPoiStatus, true, 40);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_RHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, true, 40);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_TWOHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, true, 40);
	if (result != -1) cCollison = result;

	// Accessories - female positions
	result = DrawEquippedItem(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	result = DrawEquippedItem(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// HEAD at position (72, 139) with female sprite offset (+40)
	result = DrawEquippedItem(DEF_EQUIPPOS_HEAD, sX + 72, sY + 139, msX, msY, cEquipPoiStatus, false, 40);
	if (result != -1) cCollison = result;

	// Angel staff special case
	if (cEquipPoiStatus[DEF_EQUIPPOS_TWOHAND] != -1)
	{
		int itemIdx = cEquipPoiStatus[DEF_EQUIPPOS_TWOHAND];
		sSprH = m_pGame->m_pItemList[itemIdx]->m_sSprite;
		sFrame = m_pGame->m_pItemList[itemIdx]->m_sSpriteFrame;
		cItemColor = m_pGame->m_pItemList[itemIdx]->m_cItemColor;
		if (sSprH == 8) // Angel staff
		{
			if (!m_pGame->m_bIsItemDisabled[itemIdx])
				m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame);
			else
				m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + 40]->Draw(sX + 45, sY + 143, sFrame, SpriteLib::DrawParams::Alpha(0.5f));
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

// Helper: Check if clicking on equipped item and return the item index
char DialogBox_Character::GetClickedEquipSlot(int equipPos, int drawX, int drawY, short msX, short msY,
	const char* cEquipPoiStatus, int spriteOffset)
{
	int itemIdx = cEquipPoiStatus[equipPos];
	if (itemIdx == -1) return -1;

	short sSprH = m_pGame->m_pItemList[itemIdx]->m_sSprite;
	short sFrame = m_pGame->m_pItemList[itemIdx]->m_sSpriteFrame;

	if (m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + spriteOffset]->CheckCollision(drawX, drawY, sFrame, msX, msY))
		return (char)itemIdx;

	return -1;
}

// Helper: Find the equipment item that was clicked (checks all slots)
char DialogBox_Character::FindClickedEquipItem(short msX, short msY, short sX, short sY, const char* cEquipPoiStatus)
{
	char cItemID = -1;

	// Male characters (types 1-3)
	if (m_pGame->m_pPlayer->m_sPlayerType >= 1 && m_pGame->m_pPlayer->m_sPlayerType <= 3)
	{
		// Check in reverse priority order (last checked = highest priority)
		if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BACK, sX + 41, sY + 137, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_LHAND, sX + 90, sY + 170, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_RHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_TWOHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_HEAD, sX + 72, sY + 135, msX, msY, cEquipPoiStatus)) != -1) {}
	}
	// Female characters (types 4-6)
	else if (m_pGame->m_pPlayer->m_sPlayerType >= 4 && m_pGame->m_pPlayer->m_sPlayerType <= 6)
	{
		if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BACK, sX + 45, sY + 143, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_LHAND, sX + 84, sY + 175, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_RHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_TWOHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus, 40)) != -1) {}
		else if ((cItemID = GetClickedEquipSlot(DEF_EQUIPPOS_HEAD, sX + 72, sY + 139, msX, msY, cEquipPoiStatus, 40)) != -1) {}
	}

	return cItemID;
}

bool DialogBox_Character::OnDoubleClick(short msX, short msY)
{
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal))
		return false;

	short sX = Info().sX;
	short sY = Info().sY;

	// Build equipment position status array
	char cEquipPoiStatus[DEF_MAXITEMEQUIPPOS];
	for (int i = 0; i < DEF_MAXITEMEQUIPPOS; i++) cEquipPoiStatus[i] = -1;
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		if (m_pGame->m_pItemList[i] != nullptr && m_pGame->m_bIsItemEquipped[i])
			cEquipPoiStatus[m_pGame->m_pItemList[i]->m_cEquipPos] = i;
	}

	// Find clicked item
	char cItemID = FindClickedEquipItem(msX, msY, sX, sY, cEquipPoiStatus);
	if (cItemID == -1 || m_pGame->m_pItemList[cItemID] == nullptr)
		return false;

	// Skip consumables, arrows, and stacked items
	if (m_pGame->m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_EAT ||
		m_pGame->m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_CONSUME ||
		m_pGame->m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_ARROW ||
		m_pGame->m_pItemList[cItemID]->m_dwCount > 1)
		return false;

	// Check if at repair shop
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SaleMenu) &&
		!m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) &&
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 == 24)
	{
		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_REPAIRITEM, 0, cItemID,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3, 0,
			m_pGame->m_pItemList[cItemID]->m_cName,
			m_pGame->m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4);
	}
	else
	{
		// Release (unequip) the item
		if (m_pGame->m_bIsItemEquipped[cItemID])
		{
			char cStr1[64], cStr2[64], cStr3[64];
			m_pGame->GetItemName(m_pGame->m_pItemList[cItemID].get(), cStr1, cStr2, cStr3);
			std::memset(m_pGame->G_cTxt, 0, sizeof(m_pGame->G_cTxt));
			wsprintf(m_pGame->G_cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
			AddEventList(m_pGame->G_cTxt, 10);

			{
				short sID = m_pGame->m_pItemList[cItemID]->m_sIDnum;
				if (sID == hb::item::ItemId::AngelicPandentSTR || sID == hb::item::ItemId::AngelicPandentDEX ||
					sID == hb::item::ItemId::AngelicPandentINT || sID == hb::item::ItemId::AngelicPandentMAG)
					m_pGame->PlaySound('E', 53, 0);
				else
					m_pGame->PlaySound('E', 29, 0);
			}

			// Remove Angelic Stats
			if (m_pGame->m_pItemList[cItemID]->m_cEquipPos >= 11 &&
				m_pGame->m_pItemList[cItemID]->m_cItemType == 1)
			{
				if (m_pGame->m_pItemList[cItemID]->m_sIDnum == hb::item::ItemId::AngelicPandentSTR)
					m_pGame->m_pPlayer->m_iAngelicStr = 0;
				else if (m_pGame->m_pItemList[cItemID]->m_sIDnum == hb::item::ItemId::AngelicPandentDEX)
					m_pGame->m_pPlayer->m_iAngelicDex = 0;
				else if (m_pGame->m_pItemList[cItemID]->m_sIDnum == hb::item::ItemId::AngelicPandentINT)
					m_pGame->m_pPlayer->m_iAngelicInt = 0;
				else if (m_pGame->m_pItemList[cItemID]->m_sIDnum == hb::item::ItemId::AngelicPandentMAG)
					m_pGame->m_pPlayer->m_iAngelicMag = 0;
			}

			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_RELEASEITEM, 0, cItemID, 0, 0, 0);
			m_pGame->m_bIsItemEquipped[cItemID] = false;
			m_pGame->m_sItemEquipmentStatus[m_pGame->m_pItemList[cItemID]->m_cEquipPos] = -1;
			CursorTarget::ClearSelection();
		}
	}

	return true;
}

// Helper: Check collision with an equipped item slot and select it if clicked
bool DialogBox_Character::CheckEquipSlotCollision(int equipPos, int drawX, int drawY, short msX, short msY,
	const char* cEquipPoiStatus, int spriteOffset)
{
	int itemIdx = cEquipPoiStatus[equipPos];
	if (itemIdx == -1) return false;

	short sSprH = m_pGame->m_pItemList[itemIdx]->m_sSprite;
	short sFrame = m_pGame->m_pItemList[itemIdx]->m_sSpriteFrame;

	if (m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + sSprH + spriteOffset]->CheckCollision(drawX, drawY, sFrame, msX, msY))
	{
		CursorTarget::SetSelection(SelectedObjectType::Item, m_pGame->m_sItemEquipmentStatus[equipPos], 0, 0);
		return true;
	}
	return false;
}

PressResult DialogBox_Character::OnPress(short msX, short msY)
{
	if (m_pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal))
		return PressResult::Normal;

	short sX = Info().sX;
	short sY = Info().sY;

	// Build equipment position status array
	char cEquipPoiStatus[DEF_MAXITEMEQUIPPOS];
	for (int i = 0; i < DEF_MAXITEMEQUIPPOS; i++) cEquipPoiStatus[i] = -1;
	for (int i = 0; i < DEF_MAXITEMS; i++)
	{
		if ((m_pGame->m_pItemList[i] != nullptr) && (m_pGame->m_bIsItemEquipped[i] == true))
			cEquipPoiStatus[m_pGame->m_pItemList[i]->m_cEquipPos] = i;
	}

	// Male characters (types 1-3)
	if ((m_pGame->m_pPlayer->m_sPlayerType >= 1) && (m_pGame->m_pPlayer->m_sPlayerType <= 3))
	{
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_HEAD, sX + 72, sY + 135, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_TWOHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_RHAND, sX + 57, sY + 186, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_LHAND, sX + 90, sY + 170, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BACK, sX + 41, sY + 137, msX, msY, cEquipPoiStatus)) return PressResult::ItemSelected;
	}
	// Female characters (types 4-6)
	else if ((m_pGame->m_pPlayer->m_sPlayerType >= 4) && (m_pGame->m_pPlayer->m_sPlayerType <= 6))
	{
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_HEAD, sX + 72, sY + 139, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_RFINGER, sX + 32, sY + 193, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_LFINGER, sX + 98, sY + 182, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_NECK, sX + 35, sY + 120, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_TWOHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_RHAND, sX + 60, sY + 191, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_LHAND, sX + 84, sY + 175, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_FULLBODY, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BOOTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_ARMS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_PANTS, sX + 171, sY + 290, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
		if (CheckEquipSlotCollision(DEF_EQUIPPOS_BACK, sX + 45, sY + 143, msX, msY, cEquipPoiStatus, 40)) return PressResult::ItemSelected;
	}

	return PressResult::Normal;
}

bool DialogBox_Character::OnItemDrop(short msX, short msY)
{
	m_pGame->ItemEquipHandler((char)CursorTarget::GetSelectedID());
	return true;
}
