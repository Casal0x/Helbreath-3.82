#include "LootManager.h"
#include "Game.h"
#include "ItemManager.h"
#include "CombatManager.h"
#include "Packet/SharedPackets.h"
#include "Item.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;

extern char G_cTxt[512];
extern void PutLogList(char* cStr);
extern void PutLogFileList(char* cStr);


void LootManager::ApplyPKpenalty(short sAttackerH, short sVictumH)
{
	uint32_t iV1, iV2;

	if (m_pGame->m_pClientList[sAttackerH] == 0) return;
	if (m_pGame->m_pClientList[sVictumH] == 0) return;
	if ((m_pGame->m_pClientList[sAttackerH]->m_bIsSafeAttackMode) && (m_pGame->m_pClientList[sAttackerH]->m_iPKCount == 0)) return;
	if ((strcmp(m_pGame->m_pClientList[sVictumH]->m_cLocation, "aresden") != 0) && (strcmp(m_pGame->m_pClientList[sVictumH]->m_cLocation, "elvine") != 0) && (strcmp(m_pGame->m_pClientList[sVictumH]->m_cLocation, "elvhunter") != 0) && (strcmp(m_pGame->m_pClientList[sVictumH]->m_cLocation, "arehunter") != 0)) {
		return;
	}

	// PK Count
	m_pGame->m_pClientList[sAttackerH]->m_iPKCount++;

	m_pGame->m_pCombatManager->_bPKLog(PkLog::ByPk, sAttackerH, sVictumH, 0);

	iV1 = m_pGame->iDice((m_pGame->m_pClientList[sVictumH]->m_iLevel / 2) + 1, 50);
	iV2 = m_pGame->iDice((m_pGame->m_pClientList[sAttackerH]->m_iLevel / 2) + 1, 50);

	m_pGame->m_pClientList[sAttackerH]->m_iExp -= iV1;
	m_pGame->m_pClientList[sAttackerH]->m_iExp -= iV2;
	if (m_pGame->m_pClientList[sAttackerH]->m_iExp < 0) m_pGame->m_pClientList[sAttackerH]->m_iExp = 0;

	m_pGame->SendNotifyMsg(0, sAttackerH, Notify::PkPenalty, 0, 0, 0, 0);

	m_pGame->SendEventToNearClient_TypeA(sAttackerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);

	// std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) PK-penalty: (%s)  (%d) (%d) ", m_pGame->m_pClientList[sAttackerH]->m_cCharName, iV1+iV2, m_pGame->m_pClientList[sAttackerH]->m_iExp);
	//PutLogFileList(G_cTxt);

	m_pGame->m_stCityStatus[m_pGame->m_pClientList[sAttackerH]->m_cSide].iCrimes++;

	m_pGame->m_pClientList[sAttackerH]->m_iRating -= 10;
	if (m_pGame->m_pClientList[sAttackerH]->m_iRating > 10000)  m_pGame->m_pClientList[sAttackerH]->m_iRating = 10000;
	if (m_pGame->m_pClientList[sAttackerH]->m_iRating < -10000) m_pGame->m_pClientList[sAttackerH]->m_iRating = -10000;


	if (strcmp(m_pGame->m_pClientList[sAttackerH]->m_cLocation, "aresden") == 0) {
		if ((strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "arebrk11") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "arebrk12") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "arebrk21") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "arebrk22") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "aresden") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "huntzone2") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "areuni") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "arefarm") == 0)) {

			// PK .   5
			std::memset(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName, 0, sizeof(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName));
			strcpy(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName, "arejail");
			m_pGame->m_pClientList[sAttackerH]->m_iLockedMapTime = 60 * 3;
			m_pGame->RequestTeleportHandler(sAttackerH, "2   ", "arejail", -1, -1);
			return;
		}
	}

	if (strcmp(m_pGame->m_pClientList[sAttackerH]->m_cLocation, "elvine") == 0) {
		if ((strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvbrk11") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvbrk12") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvbrk21") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvbrk22") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvine") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "huntzone1") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvuni") == 0) ||
			(strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cName, "elvfarm") == 0)) {

			// PK .   5
			std::memset(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName, 0, sizeof(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName));
			strcpy(m_pGame->m_pClientList[sAttackerH]->m_cLockedMapName, "elvjail");
			m_pGame->m_pClientList[sAttackerH]->m_iLockedMapTime = 60 * 3;
			m_pGame->RequestTeleportHandler(sAttackerH, "2   ", "elvjail", -1, -1);
			return;
		}
	}
}


// 05/17/2004 - Hypnotoad - register pk log
void LootManager::PK_KillRewardHandler(short sAttackerH, short sVictumH)
{
	if (m_pGame->m_pClientList[sAttackerH] == 0) return;
	if (m_pGame->m_pClientList[sVictumH] == 0)   return;

	m_pGame->m_pCombatManager->_bPKLog(PkLog::ByPlayer, sAttackerH, sVictumH, 0);

	if (m_pGame->m_pClientList[sAttackerH]->m_iPKCount != 0) {
		// PK   PK   .

	}
	else {
		m_pGame->m_pClientList[sAttackerH]->m_iRewardGold += m_pGame->iGetExpLevel(m_pGame->m_pClientList[sVictumH]->m_iExp) * 3;


		if (m_pGame->m_pClientList[sAttackerH]->m_iRewardGold > MaxRewardGold)
			m_pGame->m_pClientList[sAttackerH]->m_iRewardGold = MaxRewardGold;
		if (m_pGame->m_pClientList[sAttackerH]->m_iRewardGold < 0)
			m_pGame->m_pClientList[sAttackerH]->m_iRewardGold = 0;

		m_pGame->SendNotifyMsg(0, sAttackerH, Notify::PkCaptured, m_pGame->m_pClientList[sVictumH]->m_iPKCount, m_pGame->m_pClientList[sVictumH]->m_iLevel, 0, m_pGame->m_pClientList[sVictumH]->m_cCharName);
	}
}

void LootManager::EnemyKillRewardHandler(int iAttackerH, int iClientH)
{
	// enemy-kill-mode = 1 | 0
	// if m_bEnemyKillMode is true than death match mode

	int iEK_Level;
	uint32_t iRewardExp;

	if (m_pGame->m_pClientList[iAttackerH] == 0) return;
	if (m_pGame->m_pClientList[iClientH] == 0)   return;

	m_pGame->m_pCombatManager->_bPKLog(PkLog::ByEnemy, iAttackerH, iClientH, 0);

	iEK_Level = 30;
	if (m_pGame->m_pClientList[iAttackerH]->m_iLevel >= 80) iEK_Level = 80;
	if (m_pGame->m_pClientList[iAttackerH]->m_iLevel >= m_pGame->m_iMaxLevel) {
		if (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp) >= iEK_Level) {
			if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cMapName, 10) != 0)
				&& (m_pGame->m_bEnemyKillMode == false)) {
				m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
			}

			if (m_pGame->m_bEnemyKillMode) {
				m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
			}
		}
		m_pGame->m_pClientList[iAttackerH]->m_iRewardGold += m_pGame->iDice(1, (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)));
		if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold > MaxRewardGold)
			m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = MaxRewardGold;
		if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold < 0)
			m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = 0;

		m_pGame->SendNotifyMsg(0, iAttackerH, Notify::EnemyKillReward, iClientH, 0, 0, 0);
		return;
	}

	if (m_pGame->m_pClientList[iAttackerH]->m_iPKCount != 0) {
	}
	else {
		if (m_pGame->m_pClientList[iClientH]->m_iGuildRank == -1) {
			iRewardExp = (m_pGame->iDice(3, (3 * m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp))) + m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)) / 3;

			if (m_pGame->m_bIsCrusadeMode) {
				m_pGame->m_pClientList[iAttackerH]->m_iExp += (iRewardExp / 3) * 4;
				m_pGame->m_pClientList[iAttackerH]->m_iWarContribution += (iRewardExp - (iRewardExp / 3)) * 12;

				if (m_pGame->m_pClientList[iAttackerH]->m_iWarContribution > m_pGame->m_iMaxWarContribution)
					m_pGame->m_pClientList[iAttackerH]->m_iWarContribution = m_pGame->m_iMaxWarContribution;

				m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint += m_pGame->m_pClientList[iClientH]->m_iLevel / 2;

				if (m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint > m_pGame->m_iMaxConstructionPoints)
					m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint = m_pGame->m_iMaxConstructionPoints;

				//testcode
				std::snprintf(G_cTxt, sizeof(G_cTxt), "Enemy Player Killed by Player! Construction: +%d WarContribution +%d", m_pGame->m_pClientList[iClientH]->m_iLevel / 2, (iRewardExp - (iRewardExp / 3)) * 6);
				PutLogList(G_cTxt);

				m_pGame->SendNotifyMsg(0, iAttackerH, Notify::ConstructionPoint, m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint, m_pGame->m_pClientList[iAttackerH]->m_iWarContribution, 0, 0);

				if (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp) >= iEK_Level) {
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cMapName, 10) != 0) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}
					if (m_pGame->m_bEnemyKillMode) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}
				}
				m_pGame->m_pClientList[iAttackerH]->m_iRewardGold += m_pGame->iDice(1, (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)));
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold > MaxRewardGold)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = MaxRewardGold;
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold < 0)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = 0;
			}
			else {
				m_pGame->m_pClientList[iAttackerH]->m_iExp += iRewardExp;
				if (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp) >= iEK_Level) {
					if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cMapName, 10) != 0)
						&& (m_pGame->m_bEnemyKillMode == false)) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}

					if (m_pGame->m_bEnemyKillMode) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}
				}
				m_pGame->m_pClientList[iAttackerH]->m_iRewardGold += m_pGame->iDice(1, (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)));
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold > MaxRewardGold)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = MaxRewardGold;
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold < 0)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = 0;
			}
		}
		else {
			iRewardExp = (m_pGame->iDice(3, (3 * m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp))) + m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)) / 3;

			if (m_pGame->m_bIsCrusadeMode) {
				m_pGame->m_pClientList[iAttackerH]->m_iExp += (iRewardExp / 3) * 4;
				m_pGame->m_pClientList[iAttackerH]->m_iWarContribution += (iRewardExp - (iRewardExp / 3)) * 12;

				if (m_pGame->m_pClientList[iAttackerH]->m_iWarContribution > m_pGame->m_iMaxWarContribution)
					m_pGame->m_pClientList[iAttackerH]->m_iWarContribution = m_pGame->m_iMaxWarContribution;

				m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint += m_pGame->m_pClientList[iClientH]->m_iLevel / 2;

				if (m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint > m_pGame->m_iMaxConstructionPoints)
					m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint = m_pGame->m_iMaxConstructionPoints;

				//testcode
				std::snprintf(G_cTxt, sizeof(G_cTxt), "Enemy Player Killed by Player! Construction: +%d WarContribution +%d", m_pGame->m_pClientList[iClientH]->m_iLevel / 2, (iRewardExp - (iRewardExp / 3)) * 6);
				PutLogList(G_cTxt);

				m_pGame->SendNotifyMsg(0, iAttackerH, Notify::ConstructionPoint, m_pGame->m_pClientList[iAttackerH]->m_iConstructionPoint, m_pGame->m_pClientList[iAttackerH]->m_iWarContribution, 0, 0);

				if (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp) >= iEK_Level) {
					if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cMapName, 10) != 0)
						&& (m_pGame->m_bEnemyKillMode == false)) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}

					if (m_pGame->m_bEnemyKillMode) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}
				}
				m_pGame->m_pClientList[iAttackerH]->m_iRewardGold += m_pGame->iDice(1, (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)));
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold > MaxRewardGold)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = MaxRewardGold;
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold < 0)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = 0;
			}
			else {
				m_pGame->m_pClientList[iAttackerH]->m_iExp += iRewardExp;
				if (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp) >= iEK_Level) {
					if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cMapName, 10) != 0)
						&& (m_pGame->m_bEnemyKillMode == false)) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}

					if (m_pGame->m_bEnemyKillMode) {
						m_pGame->m_pClientList[iAttackerH]->m_iEnemyKillCount += m_pGame->m_iEnemyKillAdjust;
					}
				}
				m_pGame->m_pClientList[iAttackerH]->m_iRewardGold += m_pGame->iDice(1, (m_pGame->iGetExpLevel(m_pGame->m_pClientList[iClientH]->m_iExp)));
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold > MaxRewardGold)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = MaxRewardGold;
				if (m_pGame->m_pClientList[iAttackerH]->m_iRewardGold < 0)
					m_pGame->m_pClientList[iAttackerH]->m_iRewardGold = 0;
			}
		}

		m_pGame->SendNotifyMsg(0, iAttackerH, Notify::EnemyKillReward, iClientH, 0, 0, 0);

		if (m_pGame->bCheckLimitedUser(iAttackerH) == false) {
			m_pGame->SendNotifyMsg(0, iAttackerH, Notify::Exp, 0, 0, 0, 0);
		}
		m_pGame->bCheckLevelUp(iAttackerH);

		m_pGame->m_stCityStatus[m_pGame->m_pClientList[iAttackerH]->m_cSide].iWins++;
	}
}

// 05/22/2004 - Hypnotoad - register in pk log
void LootManager::ApplyCombatKilledPenalty(int iClientH, int cPenaltyLevel, bool bIsSAattacked)
{
	uint32_t iExp;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	// Crusade
	if (m_pGame->m_bIsCrusadeMode) {
		// PKcount
		if (m_pGame->m_pClientList[iClientH]->m_iPKCount > 0) {
			m_pGame->m_pClientList[iClientH]->m_iPKCount--;
			m_pGame->SendNotifyMsg(0, iClientH, Notify::PkPenalty, 0, 0, 0, 0);
			// v2.15
			m_pGame->m_pCombatManager->_bPKLog(PkLog::ReduceCriminal, 0, iClientH, 0);

		}
		return;
	}
	else {
		// PKcount
		if (m_pGame->m_pClientList[iClientH]->m_iPKCount > 0) {
			m_pGame->m_pClientList[iClientH]->m_iPKCount--;
			m_pGame->SendNotifyMsg(0, iClientH, Notify::PkPenalty, 0, 0, 0, 0);
			// v2.15
			m_pGame->m_pCombatManager->_bPKLog(PkLog::ReduceCriminal, 0, iClientH, 0);
		}

		iExp = m_pGame->iDice(1, (5 * cPenaltyLevel * m_pGame->m_pClientList[iClientH]->m_iLevel));

		if (m_pGame->m_pClientList[iClientH]->m_bIsNeutral) iExp = iExp / 3;

		// if (m_pGame->m_pClientList[iClientH]->m_iLevel == hb::shared::limits::PlayerMaxLevel) iExp = 0;

		m_pGame->m_pClientList[iClientH]->m_iExp -= iExp;
		if (m_pGame->m_pClientList[iClientH]->m_iExp < 0) m_pGame->m_pClientList[iClientH]->m_iExp = 0;

		m_pGame->SendNotifyMsg(0, iClientH, Notify::Exp, 0, 0, 0, 0);

		if (m_pGame->m_pClientList[iClientH]->m_bIsNeutral != true) {
			if (m_pGame->m_pClientList[iClientH]->m_iLevel < 80) {
				// v2.03 60 -> 80
				cPenaltyLevel--;
				if (cPenaltyLevel <= 0) cPenaltyLevel = 1;
				_PenaltyItemDrop(iClientH, cPenaltyLevel, bIsSAattacked);
			}
			else _PenaltyItemDrop(iClientH, cPenaltyLevel, bIsSAattacked);
		}
	}
}

// 05/29/2004 - Hypnotoad - Limits some items from not dropping
void LootManager::_PenaltyItemDrop(int iClientH, int iTotal, bool bIsSAattacked)
{
	int j, iRemainItem;
	char cItemIndexList[hb::shared::limits::MaxItems], cItemIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	if ((m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex != -1) && (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex] != 0)) {
		// Testcode
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->GetItemEffectType() == ItemEffectType::AlterItemDrop) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan > 0) {
				m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan--;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::CurLifeSpan, m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan, 0, 0);
			}
			m_pGame->m_pItemManager->DropItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex, -1, m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_cName);

			m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex = -1;
		}
		else {
			// v2.04 testcode
			PutLogFileList("Alter Drop Item Index Error1");
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->GetItemEffectType() == ItemEffectType::AlterItemDrop)) {
					m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex = i;
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan > 0) {
						m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan--;
						m_pGame->SendNotifyMsg(0, iClientH, Notify::CurLifeSpan, m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_wCurLifeSpan, 0, 0);
					}
					m_pGame->m_pItemManager->DropItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex, -1, m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex]->m_cName);
					m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex = -1;
					return;
				}

			goto PID_DROP;
		}
		return;
	}

PID_DROP:

	for(int i = 1; i <= iTotal; i++) {
		iRemainItem = 0;
		std::memset(cItemIndexList, 0, sizeof(cItemIndexList));

		for (j = 0; j < hb::shared::limits::MaxItems; j++)
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[j] != 0) {
				cItemIndexList[iRemainItem] = j;
				iRemainItem++;
			}

		if (iRemainItem == 0) return;
		cItemIndex = cItemIndexList[m_pGame->iDice(1, iRemainItem) - 1];


		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->GetTouchEffectType() != TouchEffectType::None) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sTouchEffectValue1 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum1) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sTouchEffectValue2 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum2) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sTouchEffectValue3 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum3)) {
		}

		else if (
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sIDnum >= 400) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sIDnum != 402) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_sIDnum <= 428)) {
		}

		else if (((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->GetItemEffectType() == ItemEffectType::AttackSpecAbility) ||
			(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->GetItemEffectType() == ItemEffectType::DefenseSpecAbility)) &&
			(bIsSAattacked == false)) {
		}

		else if ((m_pGame->m_pClientList[iClientH]->m_bIsLuckyEffect) && (m_pGame->iDice(1, 10) == 5)) {
			// 10%    .
		}

		else m_pGame->m_pItemManager->DropItemHandler(iClientH, cItemIndex, -1, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex]->m_cName);
	}
}

void LootManager::GetRewardMoneyHandler(int iClientH)
{
	int iRet, iEraseReq, iWeightLeft;
	uint32_t iRewardGoldLeft;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;


	iWeightLeft = m_pGame->_iCalcMaxLoad(iClientH) - m_pGame->iCalcTotalWeight(iClientH);

	if (iWeightLeft <= 0) return;
	iWeightLeft = iWeightLeft / 2;
	if (iWeightLeft <= 0) return;

	pItem = new CItem;
	m_pGame->m_pItemManager->_bInitItemAttr(pItem, hb::shared::item::ItemId::Gold);
	//pItem->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iRewardGold;

	// (iWeightLeft / pItem->m_wWeight)     Gold.   .
	uint32_t maxGold = static_cast<uint32_t>(iWeightLeft / m_pGame->m_pItemManager->iGetItemWeight(pItem, 1));
	if (maxGold >= m_pGame->m_pClientList[iClientH]->m_iRewardGold) {
		pItem->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iRewardGold;
		iRewardGoldLeft = 0;
	}
	else {
		// (iWeightLeft / pItem->m_wWeight) .
		pItem->m_dwCount = maxGold;
		iRewardGoldLeft = m_pGame->m_pClientList[iClientH]->m_iRewardGold - maxGold;
	}

	if (m_pGame->m_pItemManager->_bAddClientItemList(iClientH, pItem, &iEraseReq)) {

		m_pGame->m_pClientList[iClientH]->m_iRewardGold = iRewardGoldLeft;

		iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);

		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
			m_pGame->DeleteClient(iClientH, true, true);
			return;
		}

		m_pGame->SendNotifyMsg(0, iClientH, Notify::RewardGold, 0, 0, 0, 0);
	}
	else {

	}
}
