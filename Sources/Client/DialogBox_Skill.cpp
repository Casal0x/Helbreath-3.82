#include "DialogBox_Skill.h"
#include "Game.h"
#include "IInput.h"
#include "Misc.h"
#include "lan_eng.h"

using namespace hb::shared::net;
DialogBox_Skill::DialogBox_Skill(CGame* pGame)
	: IDialogBox(DialogBoxId::Skill, pGame)
{
	SetDefaultRect(337 , 57 , 258, 339);
}

void DialogBox_Skill::OnDraw(short msX, short msY, short msZ, char cLB)
{
	if (!m_pGame->EnsureSkillConfigsLoaded()) return;
	short sX, sY;
	int i, iTotalLines, iPointerLoc;
	char cTemp[255], cTemp2[255];
	double d1, d2, d3;

	sX = Info().sX;
	sY = Info().sY;

	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 0); // Normal Dialog
	DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 1); // Skill Dialog Title Bar

	switch (Info().cMode) {
	case 0:
		for (i = 0; i < 17; i++)
			if ((i < hb::shared::limits::MaxSkillType) && (m_pGame->m_pSkillCfgList[i + Info().sView] != 0))
			{
				std::memset(cTemp, 0, sizeof(cTemp));
				std::snprintf(cTemp, sizeof(cTemp), "%s", m_pGame->m_pSkillCfgList[i + Info().sView]->m_cName);
				CMisc::ReplaceString(cTemp, '-', ' ');
				std::memset(cTemp2, 0, sizeof(cTemp2));
				std::snprintf(cTemp2, sizeof(cTemp2), "%3d%%", m_pGame->m_pSkillCfgList[i + Info().sView]->m_iLevel);
				if ((msX >= sX + 25) && (msX <= sX + 166) && (msY >= sY + 45 + i * 15) && (msY <= sY + 59 + i * 15))
				{
					if ((m_pGame->m_pSkillCfgList[i + Info().sView]->m_bIsUseable == true)
						&& (m_pGame->m_pSkillCfgList[i + Info().sView]->m_iLevel != 0))
					{
						PutString(sX + 30, sY + 45 + i * 15, cTemp, GameColors::UIWhite);
						PutString(sX + 183, sY + 45 + i * 15, cTemp2, GameColors::UIWhite);
					}
					else
					{
						PutString(sX + 30, sY + 45 + i * 15, cTemp, GameColors::UIBlack);
						PutString(sX + 183, sY + 45 + i * 15, cTemp2, GameColors::UIBlack);
					}
				}
				else
				{
					if ((m_pGame->m_pSkillCfgList[i + Info().sView]->m_bIsUseable == true)
						&& (m_pGame->m_pSkillCfgList[i + Info().sView]->m_iLevel != 0))
					{
						PutString(sX + 30, sY + 45 + i * 15, cTemp, GameColors::UIMagicBlue);
						PutString(sX + 183, sY + 45 + i * 15, cTemp2, GameColors::UIMagicBlue);
					}
					else
					{
						PutString(sX + 30, sY + 45 + i * 15, cTemp, GameColors::UIBlack);
						PutString(sX + 183, sY + 45 + i * 15, cTemp2, GameColors::UIBlack);
					}
				}

				if (m_pGame->m_iDownSkillIndex == (i + Info().sView))
					m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE]->Draw(sX + 215, sY + 47 + i * 15, 21, hb::shared::sprite::DrawParams::Tint(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
				else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE]->Draw(sX + 215, sY + 47 + i * 15, 20, hb::shared::sprite::DrawParams::Tint(1, 1, 1));
			}

		iTotalLines = 0;
		for (i = 0; i < hb::shared::limits::MaxSkillType; i++)
			if (m_pGame->m_pSkillCfgList[i] != 0) iTotalLines++;

		if (iTotalLines > 17)
		{
			d1 = (double)Info().sView;
			d2 = (double)(iTotalLines - 17);
			d3 = (274.0f * d1) / d2;
			iPointerLoc = (int)d3;
		}
		else iPointerLoc = 0;
		if (iTotalLines > 17)
		{
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 1);
			DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 242, sY + iPointerLoc + 35, 7);
		}

		if (cLB != 0 && iTotalLines > 17)
		{
			if ((m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Skill))
			{
				if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 30) && (msY <= sY + 320))
				{
					d1 = (double)(msY - (sY + 35));
					d2 = (double)(iTotalLines - 17);
					d3 = (d1 * d2) / 274.0f;
					iPointerLoc = (int)(d3 + 0.5);
					if (iPointerLoc > iTotalLines - 17) iPointerLoc = iTotalLines - 17;
					Info().sView = iPointerLoc;
				}
			}
		}
		else Info().bIsScrollSelected = false;
		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Skill && msZ != 0)
		{
			if (msZ > 0) Info().sView--;
			if (msZ < 0) Info().sView++;

		}
		if (Info().sView < 0) Info().sView = 0;
		if (iTotalLines > 17 && Info().sView > iTotalLines - 17) Info().sView = iTotalLines - 17;
		break;
	}
}

bool DialogBox_Skill::OnClick(short msX, short msY)
{
	int i;
	short sX, sY;
	sX = Info().sX;
	sY = Info().sY;
	switch (Info().cMode) {
	case -1:
		break;
	case 0:
		for (i = 0; i < 17; i++)
			if ((i < hb::shared::limits::MaxSkillType) && (m_pGame->m_pSkillCfgList[i + Info().sView] != 0))
			{
				if ((msX >= sX + 44) && (msX <= sX + 135 + 44) && (msY >= sY + 45 + i * 15) && (msY <= sY + 59 + i * 15))
				{
					if ((m_pGame->m_pSkillCfgList[i + Info().sView]->m_bIsUseable == true)
						&& (m_pGame->m_pSkillCfgList[i + Info().sView]->m_iLevel != 0))
					{
						if (m_pGame->m_bSkillUsingStatus == true)
						{
							AddEventList(DLGBOX_CLICK_SKILL1, 10); // "You are already using other skill."
							return true;
						}
						if ((m_pGame->m_pPlayer->m_Controller.IsCommandAvailable() == false) || (m_pGame->m_pPlayer->m_iHP <= 0))
						{
							AddEventList(DLGBOX_CLICK_SKILL2, 10); // "You can't use a skill while you are moving."
							return true;
						}
						if (m_pGame->m_bIsGetPointingMode == true)
						{
							return true;
						}
						switch (m_pGame->m_pSkillCfgList[i + Info().sView]->m_cUseMethod) {
						case 0:
						case 2:
							bSendCommand(MsgId::CommandCommon, CommonType::ReqUseSkill, 0, (i + Info().sView), 0, 0, 0);
							m_pGame->m_bSkillUsingStatus = true;
							DisableThisDialog();
							PlaySoundEffect('E', 14, 5);
							return true;
						}
					}
				}
				else if ((msX >= sX + 215) && (msX <= sX + 240) && (msY >= sY + 45 + i * 15) && (msY <= sY + 59 + i * 15))
				{
					if (Info().bFlag == false)
					{
						bSendCommand(MsgId::CommandCommon, CommonType::ReqSetDownSkillIndex, 0, i + Info().sView, 0, 0, 0);
						PlaySoundEffect('E', 14, 5);
						Info().bFlag = true;
						return true;
					}
				}
			}
		break;
	}
	return false;
}

PressResult DialogBox_Skill::OnPress(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Scroll bar region
	if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 40) && (msY <= sY + 320))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}

