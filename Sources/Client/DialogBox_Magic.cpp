#include "DialogBox_Magic.h"
#include "ConfigManager.h"
#include "Game.h"
#include "MagicCastingSystem.h"
#include "GameFonts.h"
#include "IInput.h"
#include "Misc.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include "WeatherManager.h"
#include <format>
#include <string>

using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Magic::DialogBox_Magic(CGame* pGame)
	: IDialogBox(DialogBoxId::Magic, pGame)
{
	SetCanCloseOnRightClick(true);
	SetDefaultRect(337 , 57 , 258, 328);
}

void DialogBox_Magic::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureMagicConfigsLoaded()) return;
	short sX, sY;
	int iCPivot, i, iYloc, iMagicCircle, iManaCost;
	const bool dialogTrans = ConfigManager::Get().IsDialogTransparencyEnabled();
	std::string cMana;

	char cTxt[120];
	double dV1, dV2, dV3, dV4;
	int iResult;
	short sLevelMagic;
	uint32_t dwTime = m_pGame->m_dwCurTime;

	sX = Info().sX;
	sY = Info().sY;

	DrawNewDialogBox(InterfaceNdGame1, sX, sY, 1, false, dialogTrans);
	DrawNewDialogBox(InterfaceNdText, sX, sY, 7, false, dialogTrans);

	// Handle scroll wheel input
	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Magic && msZ != 0)
	{
		if (msZ > 0) Info().sView--;
		if (msZ < 0) Info().sView++;
	}
	if (Info().sView < 0) Info().sView = 9;
	if (Info().sView > 9) Info().sView = 0;

	// Circle title
	static constexpr const char* kMagicCircleNames[] = {
		DRAW_DIALOGBOX_MAGIC1, DRAW_DIALOGBOX_MAGIC2, DRAW_DIALOGBOX_MAGIC3,
		DRAW_DIALOGBOX_MAGIC4, DRAW_DIALOGBOX_MAGIC5, DRAW_DIALOGBOX_MAGIC6,
		DRAW_DIALOGBOX_MAGIC7, DRAW_DIALOGBOX_MAGIC8, DRAW_DIALOGBOX_MAGIC9,
		DRAW_DIALOGBOX_MAGIC10
	};
	const char* pCircleName = kMagicCircleNames[Info().sView];
	PutAlignedString(sX + 3, sX + 256, sY + 50, pCircleName);
	PutAlignedString(sX + 4, sX + 257, sY + 50, pCircleName);

	iCPivot = Info().sView * 10;
	iYloc = 0;
	for (i = 0; i < 9; i++)
	{
		if ((m_pGame->m_pPlayer->m_iMagicMastery[iCPivot + i] != 0) && (m_pGame->m_pMagicCfgList[iCPivot + i] != 0))
		{
			std::snprintf(cTxt, sizeof(cTxt), "%s", m_pGame->m_pMagicCfgList[iCPivot + i]->m_cName.c_str());
			CMisc::ReplaceString(cTxt, '-', ' ');

			iManaCost = MagicCastingSystem::Get().GetManaCost(iCPivot + i);

			if (iManaCost > m_pGame->m_pPlayer->m_iMP)
			{
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 30, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicPurple));
				cMana = std::format("{:3}", iManaCost);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 206, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicPurple));
			}
			else if ((msX >= sX + 30) && (msX <= sX + 240) && (msY >= sY + 70 + iYloc) && (msY <= sY + 70 + 14 + iYloc))
			{
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 30, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UINearWhite));
				cMana = std::format("{:3}", iManaCost);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 206, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UINearWhite));
			}
			else
			{
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 30, sY + 70 + iYloc, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
				cMana = std::format("{:3}", iManaCost);
				hb::shared::text::DrawText(GameFont::Bitmap1, sX + 206, sY + 70 + iYloc, cMana.c_str(), hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
			}

			iYloc += 18;
		}
	}

	// No spells message
	if (iYloc == 0)
	{
		PutAlignedString(sX + 3, sX + 256, sY + 100, DRAW_DIALOGBOX_MAGIC11);
		PutAlignedString(sX + 3, sX + 256, sY + 115, DRAW_DIALOGBOX_MAGIC12);
		PutAlignedString(sX + 3, sX + 256, sY + 130, DRAW_DIALOGBOX_MAGIC13);
		PutAlignedString(sX + 3, sX + 256, sY + 145, DRAW_DIALOGBOX_MAGIC14);
		PutAlignedString(sX + 3, sX + 256, sY + 160, DRAW_DIALOGBOX_MAGIC15);
	}

	// Circle selector bar
	m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 30, sY + 250, 19);

	// Circle selector highlight
	switch (Info().sView) {
	case 0: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 30, sY + 250, 20); break;
	case 1: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 43, sY + 250, 21); break;
	case 2: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 61, sY + 250, 22); break;
	case 3: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 86, sY + 250, 23); break;
	case 4: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 106, sY + 250, 24); break;
	case 5: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 121, sY + 250, 25); break;
	case 6: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 142, sY + 250, 26); break;
	case 7: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 169, sY + 250, 27); break;
	case 8: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 202, sY + 250, 28); break;
	case 9: m_pGame->m_pSprite[InterfaceSprFonts]->Draw(sX + 222, sY + 250, 29); break;
	}

	// Calculate magic probability
	iMagicCircle = Info().sView + 1;

	static int _tmp_iMCProb[] = { 0, 300, 250, 200, 150, 100, 80, 70, 60, 50, 40 };
	static int _tmp_iMLevelPenalty[] = { 0,   5,   5,   8,   8,  10, 14, 28, 32, 36, 40 };

	if (m_pGame->m_pPlayer->m_iSkillMastery[4] == 0)
		dV1 = 1.0f;
	else
		dV1 = static_cast<double>(m_pGame->m_pPlayer->m_iSkillMastery[4]);

	dV2 = static_cast<double>(dV1 / 100.0f);
	dV3 = static_cast<double>(_tmp_iMCProb[iMagicCircle]);
	dV1 = dV2 * dV3;
	iResult = static_cast<int>(dV1);

	if ((m_pGame->m_pPlayer->m_iInt + m_pGame->m_pPlayer->m_iAngelicInt) > 50)
		iResult += ((m_pGame->m_pPlayer->m_iInt + m_pGame->m_pPlayer->m_iAngelicInt) - 50) / 2;

	sLevelMagic = (m_pGame->m_pPlayer->m_iLevel / 10);
	if (iMagicCircle != sLevelMagic)
	{
		if (iMagicCircle > sLevelMagic)
		{
			dV1 = static_cast<double>(m_pGame->m_pPlayer->m_iLevel - sLevelMagic * 10);
			dV2 = static_cast<double>(abs(iMagicCircle - sLevelMagic)) * _tmp_iMLevelPenalty[iMagicCircle];
			dV3 = static_cast<double>(abs(iMagicCircle - sLevelMagic)) * 10;
			dV4 = (dV1 / dV3) * dV2;
			iResult -= abs(abs(iMagicCircle - sLevelMagic) * _tmp_iMLevelPenalty[iMagicCircle] - static_cast<int>(dV4));
		}
		else
		{
			iResult += 5 * abs(iMagicCircle - sLevelMagic);
		}
	}

	// Weather modifier
	switch (WeatherManager::Get().GetWeatherStatus())
	{
	case 0: break;
	case 1: iResult = iResult - (iResult / 24); break;
	case 2: iResult = iResult - (iResult / 12); break;
	case 3: iResult = iResult - (iResult / 5);  break;
	}

	// Equipment magic bonus
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if (m_pGame->m_pItemList[i] == 0) continue;
		if (m_pGame->m_bIsItemEquipped[i] == true)
		{
			if (((m_pGame->m_pItemList[i]->m_dwAttribute & 0x00F00000) >> 20) == 10)
			{
				dV1 = static_cast<double>(iResult);
				dV2 = static_cast<double>(((m_pGame->m_pItemList[i]->m_dwAttribute & 0x000F0000) >> 16) * 3);
				dV3 = dV1 + dV2;
				iResult = static_cast<int>(dV3);
				break;
			}
		}
	}

	if (iResult > 100) iResult = 100;
	if (m_pGame->m_pPlayer->m_iSP < 1) iResult = iResult * 9 / 10;
	if (iResult < 1) iResult = 1;

	// Display magic probability
	auto cTxt_str = std::format(DRAW_DIALOGBOX_MAGIC16, iResult);
	PutAlignedString(sX, sX + 256, sY + 267, cTxt_str.c_str());
	PutAlignedString(sX + 1, sX + 257, sY + 267, cTxt_str.c_str());

	// Alchemy button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 285) && (msY <= sY + 285 + ui_layout::btn_size_y))
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + 285, 49, false, dialogTrans);
	else
		DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + 285, 48, false, dialogTrans);
}

bool DialogBox_Magic::OnClick(short msX, short msY)
{
	int i, iCPivot, iYloc;
	short sX, sY;

	sX = Info().sX;
	sY = Info().sY;
	iCPivot = Info().sView * 10;
	iYloc = 0;
	for (i = 0; i < 9; i++)
	{
		if ((m_pGame->m_pPlayer->m_iMagicMastery[iCPivot + i] != 0) && (m_pGame->m_pMagicCfgList[iCPivot + i] != 0))
		{
			if ((msX >= sX + 30) && (msX <= sX + 240) && (msY >= sY + 70 + iYloc) && (msY <= sY + 70 + 18 + iYloc))
			{
				MagicCastingSystem::Get().BeginCast(iCPivot + i);
				PlaySoundEffect('E', 14, 5);
				return true;
			}
			iYloc += 18;
		}
	}
	if ((msX >= sX + 16) && (msX <= sX + 38) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 0;
	if ((msX >= sX + 39) && (msX <= sX + 56) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 1;
	if ((msX >= sX + 57) && (msX <= sX + 81) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 2;
	if ((msX >= sX + 82) && (msX <= sX + 101) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 3;
	if ((msX >= sX + 102) && (msX <= sX + 116) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 4;
	if ((msX >= sX + 117) && (msX <= sX + 137) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 5;
	if ((msX >= sX + 138) && (msX <= sX + 165) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 6;
	if ((msX >= sX + 166) && (msX <= sX + 197) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 7;
	if ((msX >= sX + 198) && (msX <= sX + 217) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 8;
	if ((msX >= sX + 218) && (msX <= sX + 239) && (msY >= sY + 240) && (msY <= sY + 268))
		Info().sView = 9;

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + 285) && (msY <= sY + 285 + ui_layout::btn_size_y))
	{
		if (m_pGame->m_pPlayer->m_iSkillMastery[12] == 0) AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY16, 10);
		else
		{
			for (i = 0; i < hb::shared::limits::MaxItems; i++)
			{
				if (m_pGame->m_pItemList[i] == 0) continue;
				CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[i]->m_sIDnum);
				if (pCfg && (pCfg->GetItemType() == ItemType::UseSkillEnableDialogBox) &&
					(pCfg->m_sSpriteFrame == 55))
				{
					EnableDialogBox(DialogBoxId::Manufacture, 1, 0, 0, 0);
					AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY10, 10);
					PlaySoundEffect('E', 14, 5);
					return true;
				}
			}
			AddEventList(BDLBBOX_DOUBLE_CLICK_INVENTORY15, 10);
		}
		PlaySoundEffect('E', 14, 5);
	}
	return false;
}
