#include "MagicCastingSystem.h"
#include "Game.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::shared::action;

MagicCastingSystem& MagicCastingSystem::Get()
{
	static MagicCastingSystem instance;
	return instance;
}

void MagicCastingSystem::SetGame(CGame* pGame)
{
	m_game = pGame;
}

int MagicCastingSystem::GetManaCost(int iMagicNo)
{
	if (!m_game) return 1;
	int i, iManaSave, iManaCost;
	iManaSave = 0;
	if (iMagicNo < 0 || iMagicNo >= 100) return 1;
	if (!m_game->m_pMagicCfgList[iMagicNo]) return 1;
	for (i = 0; i < hb::shared::limits::MaxItems; i++)
	{
		if (m_game->m_pItemList[i] == 0) continue;
		if (m_game->m_bIsItemEquipped[i] == true)
		{
			// Data-driven mana save calculation using ItemEffectType
			auto effectType = m_game->m_pItemList[i]->GetItemEffectType();
			switch (effectType)
			{
			case hb::shared::item::ItemEffectType::AttackManaSave:
				// Weapons with mana save: value stored in m_sItemEffectValue4
				iManaSave += m_game->m_pItemList[i]->m_sItemEffectValue4;
				break;

			case hb::shared::item::ItemEffectType::AddEffect:
				// AddEffect with sub-type ManaSave (necklaces, etc.)
				if (m_game->m_pItemList[i]->m_sItemEffectValue1 == hb::shared::item::ToInt(hb::shared::item::AddEffectType::ManaSave))
				{
					iManaSave += m_game->m_pItemList[i]->m_sItemEffectValue2;
				}
				break;

			default:
				break;
			}
		}
	}
	// Mana save max = 80%
	if (iManaSave > 80) iManaSave = 80;
	iManaCost = m_game->m_pMagicCfgList[iMagicNo]->m_sValue1;
	if (m_game->m_pPlayer->m_bIsSafeAttackMode) iManaCost = iManaCost * 140 / 100;
	if (iManaSave > 0)
	{
		double dV1 = static_cast<double>(iManaSave);
		double dV2 = (double)(dV1 / 100.0f);
		double dV3 = static_cast<double>(iManaCost);
		dV1 = dV2 * dV3;
		dV2 = dV3 - dV1;
		iManaCost = static_cast<int>(dV2);
	}
	if (iManaCost < 1) iManaCost = 1;
	return iManaCost;
}

void MagicCastingSystem::BeginCast(int iMagicNo)
{
	if (!m_game) return;
	if (!m_game->EnsureMagicConfigsLoaded()) return;
	if (iMagicNo < 0 || iMagicNo >= 100) return;
	if ((m_game->m_pPlayer->m_iMagicMastery[iMagicNo] == 0) || (m_game->m_pMagicCfgList[iMagicNo] == 0)) return;

	// Casting
	if (m_game->m_pPlayer->m_iHP <= 0) return;
	if (m_game->m_bIsGetPointingMode == true) return;
	if (GetManaCost(iMagicNo) > m_game->m_pPlayer->m_iMP) return;
	if (m_game->_bIsItemOnHand() == true)
	{
		m_game->AddEventList(DLGBOX_CLICK_MAGIC1, 10);
		return;
	}
	if (m_game->m_bSkillUsingStatus == true)
	{
		m_game->AddEventList(DLGBOX_CLICK_MAGIC2, 10);
		return;
	}
	if (!m_game->m_pPlayer->m_playerAppearance.bIsWalking) m_game->bSendCommand(MsgId::CommandCommon, CommonType::ToggleCombatMode, 0, 0, 0, 0, 0);
	m_game->m_pPlayer->m_Controller.SetCommand(Type::Magic);
	m_game->m_iCastingMagicType = iMagicNo;
	m_game->m_sMagicShortCut = iMagicNo;
	m_game->m_sRecentShortCut = iMagicNo + 100;
	m_game->m_iPointCommandType = iMagicNo + 100;
	m_game->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Magic);
}
