#include "CombatManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "WarManager.h"
#include "SkillManager.h"
#include "MagicManager.h"
#include "ItemManager.h"
#include "Item.h"
#include "EntityManager.h"
#include "LootManager.h"
#include "DelayEventManager.h"
#include "DynamicObjectManager.h"
#include "Packet/SharedPackets.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;
namespace dynamic_object = hb::shared::dynamic_object;
namespace smap = hb::server::map;
namespace sdelay = hb::server::delay_event;
using namespace hb::server::npc;

extern char G_cTxt[512];
extern void PutLogList(char* cStr);
extern void PutLogFileList(char* cStr);
extern void PutPvPLogFileList(char* cStr);
extern void PutHackLogFileList(char* cStr);

// Direction lookup tables (duplicated from Game.cpp)
static char _tmp_cTmpDirX[9] = { 0,0,1,1,1,0,-1,-1,-1 };
static char _tmp_cTmpDirY[9] = { 0,-1,-1,0,1,1,1,0,-1 };

// Combo attack bonus tables (duplicated from Game.cpp)
static int ___iCAB5[] = { 0,0, 0,1,2 };
static int ___iCAB6[] = { 0,0, 0,0,0 };
static int ___iCAB7[] = { 0,0, 1,2,3 };
static int ___iCAB8[] = { 0,0, 1,3,5 };
static int ___iCAB9[] = { 0,0, 2,4,8 };
static int ___iCAB10[] = { 0,0, 1,2,3 };

void CombatManager::RemoveFromTarget(short sTargetH, char cTargetType, int iCode)
{
	
	uint32_t dwTime = GameClock::GetTimeMS();

	for(int i = 0; i < MaxNpcs; i++)
		if (m_pGame->m_pNpcList[i] != 0) {
			if ((m_pGame->m_pNpcList[i]->m_iGuildGUID != 0) && (cTargetType == hb::shared::owner_class::Player) &&
				(m_pGame->m_pClientList[sTargetH]->m_iGuildGUID == m_pGame->m_pNpcList[i]->m_iGuildGUID)) {

				if (m_pGame->m_pNpcList[i]->m_cActionLimit == 0) {
					m_pGame->m_pNpcList[i]->m_bIsSummoned = true;
					m_pGame->m_pNpcList[i]->m_dwSummonedTime = dwTime;
				}
			}

			if ((m_pGame->m_pNpcList[i]->m_iTargetIndex == sTargetH) &&
				(m_pGame->m_pNpcList[i]->m_cTargetType == cTargetType)) {

				switch (iCode) {
				case hb::shared::magic::Invisibility:
					if (m_pGame->m_pNpcList[i]->m_cSpecialAbility == 1) {
					}
					else {
						m_pGame->m_pNpcList[i]->m_cBehavior = Behavior::Move;
						m_pGame->m_pNpcList[i]->m_iTargetIndex = 0;
						m_pGame->m_pNpcList[i]->m_cTargetType = 0;
					}
					break;

				default:
					m_pGame->m_pNpcList[i]->m_cBehavior = Behavior::Move;
					m_pGame->m_pNpcList[i]->m_iTargetIndex = 0;
					m_pGame->m_pNpcList[i]->m_cTargetType = 0;
					break;
				}
			}
		}
}


int CombatManager::iGetDangerValue(int iNpcH, short dX, short dY)
{
	int iDangerValue;
	short sOwner, sDOType;
	char  cOwnerType;
	uint32_t dwRegisterTime;

	if (m_pGame->m_pNpcList[iNpcH] == 0) return false;

	iDangerValue = 0;

	for(int ix = dX - 2; ix <= dX + 2; ix++)
		for(int iy = dY - 2; iy <= dY + 2; iy++) {
			m_pGame->m_pMapList[m_pGame->m_pNpcList[iNpcH]->m_cMapIndex]->GetOwner(&sOwner, &cOwnerType, ix, iy);
			m_pGame->m_pMapList[m_pGame->m_pNpcList[iNpcH]->m_cMapIndex]->bGetDynamicObject(ix, iy, &sDOType, &dwRegisterTime);

			if (sDOType == 1) iDangerValue++;

			switch (cOwnerType) {
			case 0:
				break;
			case hb::shared::owner_class::Player:
				if (m_pGame->m_pClientList[sOwner] == 0) break;
				if (m_pGame->m_pNpcList[iNpcH]->m_cSide != m_pGame->m_pClientList[sOwner]->m_cSide)
					iDangerValue++;
				else iDangerValue--;
				break;
			case hb::shared::owner_class::Npc:
				if (m_pGame->m_pNpcList[sOwner] == 0) break;
				if (m_pGame->m_pNpcList[iNpcH]->m_cSide != m_pGame->m_pNpcList[sOwner]->m_cSide)
					iDangerValue++;
				else iDangerValue--;
				break;
			}
		}

	return iDangerValue;
}


void CombatManager::ClientKilledHandler(int iClientH, int iAttackerH, char cAttackerType, short sDamage)
{
	char cAttackerName[hb::shared::limits::NpcNameLen];
	short sAttackerWeapon;
	int iExH;
	bool  bIsSAattacked = false;


	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return;

	// 2002-7-4
	if (memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "fight", 5) == 0) {
		m_pGame->m_pClientList[iClientH]->m_dwFightzoneDeadTime = GameClock::GetTimeMS();
		std::snprintf(G_cTxt, sizeof(G_cTxt), "Fightzone Dead Time: %d", m_pGame->m_pClientList[iClientH]->m_dwFightzoneDeadTime);
		PutLogList(G_cTxt);
	}

	m_pGame->m_pClientList[iClientH]->m_bIsKilled = true;
	// HP 0.
	m_pGame->m_pClientList[iClientH]->m_iHP = 0;

	// Snoopy: Remove all magic effects and flags
	for(int i = 0; i < hb::server::config::MaxMagicEffects; i++)
		m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[i] = 0;

	m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetIllusionMovementFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetIllusionFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetInhibitionCastingFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetPoisonFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetIceFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetBerserkFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(iClientH, hb::shared::owner_class::Player, false);
	m_pGame->m_pItemManager->SetSlateFlag(iClientH, SlateClearNotify, false);
	m_pGame->m_pStatusEffectManager->SetHasteFlag(iClientH, hb::shared::owner_class::Player, false);

	if (m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode) {
		iExH = m_pGame->m_pClientList[iClientH]->m_iExchangeH;
		m_pGame->m_pItemManager->_ClearExchangeStatus(iExH);
		m_pGame->m_pItemManager->_ClearExchangeStatus(iClientH);
	}

	// NPC    .
	RemoveFromTarget(iClientH, hb::shared::owner_class::Player);

	// Delete all summoned NPCs belonging to this player
	for (int i = 0; i < MaxNpcs; i++)
		if (m_pGame->m_pNpcList[i] != 0) {
			if ((m_pGame->m_pNpcList[i]->m_bIsSummoned) &&
				(m_pGame->m_pNpcList[i]->m_iFollowOwnerIndex == iClientH) &&
				(m_pGame->m_pNpcList[i]->m_cFollowOwnerType == hb::shared::owner_class::Player)) {
				m_pGame->m_pEntityManager->DeleteEntity(i);
			}
		}

	std::memset(cAttackerName, 0, sizeof(cAttackerName));
	switch (cAttackerType) {
	case hb::shared::owner_class::PlayerIndirect:
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[iAttackerH] != 0)
			memcpy(cAttackerName, m_pGame->m_pClientList[iAttackerH]->m_cCharName, hb::shared::limits::CharNameLen - 1);
		break;
	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[iAttackerH] != 0)
#ifdef DEF_LOCALNPCNAME
			std::snprintf(cAttackerName, sizeof(cAttackerName), "NPCNPCNPC@%d", m_pGame->m_pNpcList[iAttackerH]->m_sType);
#else 
			memcpy(cAttackerName, m_pGame->m_pNpcList[iAttackerH]->m_cNpcName, hb::shared::limits::NpcNameLen - 1);
#endif
		break;
	default:
		break;
	}

	m_pGame->SendNotifyMsg(0, iClientH, Notify::Killed, 0, 0, 0, cAttackerName);
	if (cAttackerType == hb::shared::owner_class::Player) {
		sAttackerWeapon = m_pGame->m_pClientList[iAttackerH]->m_appearance.iWeaponType;
	}
	else sAttackerWeapon = 1;
	m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Dying, sDamage, sAttackerWeapon, 0);
	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->ClearOwner(12, iClientH, hb::shared::owner_class::Player, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetDeadOwner(iClientH, hb::shared::owner_class::Player, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cType == smap::MapType::NoPenaltyNoReward) return;
	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsHeldenianMap) {
		if (m_pGame->m_pClientList[iClientH]->m_cSide == 1) {
			m_pGame->m_iHeldenianAresdenDead++;
		}
		else if (m_pGame->m_pClientList[iClientH]->m_cSide == 2) {
			m_pGame->m_iHeldenianElvineDead++;
		}
		m_pGame->m_pWarManager->UpdateHeldenianStatus();
	}

	if (cAttackerType == hb::shared::owner_class::Player) {
		// v1.432
		switch (m_pGame->m_pClientList[iAttackerH]->m_iSpecialAbilityType) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			bIsSAattacked = true;
			break;
		}

		if (iAttackerH == iClientH) return;
		if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) {
			if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {


				m_pGame->m_pLootManager->ApplyPKpenalty(iAttackerH, iClientH);
			}
			else {

				m_pGame->m_pLootManager->PK_KillRewardHandler(iAttackerH, iClientH);
			}
		}
		else {
			if (m_pGame->m_pClientList[iClientH]->m_iGuildRank == -1) {
				if (memcmp(m_pGame->m_pClientList[iAttackerH]->m_cLocation, "NONE", 4) == 0) {
					if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
						m_pGame->m_pLootManager->ApplyPKpenalty(iAttackerH, iClientH);
					}
					else {

					}
				}
				else {
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iAttackerH]->m_cLocation, 10) == 0) {
						if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
							m_pGame->m_pLootManager->ApplyPKpenalty(iAttackerH, iClientH);
						}
						else {
							m_pGame->m_pLootManager->PK_KillRewardHandler(iAttackerH, iClientH);
						}
					}
					else {
						m_pGame->m_pLootManager->EnemyKillRewardHandler(iAttackerH, iClientH);
					}
				}
			}
			else {
				// , ,   -> PK /   ->
				if (memcmp(m_pGame->m_pClientList[iAttackerH]->m_cLocation, "NONE", 4) == 0) {
					if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
						m_pGame->m_pLootManager->ApplyPKpenalty(iAttackerH, iClientH);
					}
					else {

					}
				}
				else {
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iAttackerH]->m_cLocation, 10) == 0) {
						if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
							m_pGame->m_pLootManager->ApplyPKpenalty(iAttackerH, iClientH);
						}
						else {
							m_pGame->m_pLootManager->PK_KillRewardHandler(iAttackerH, iClientH);
						}
					}
					else {
						m_pGame->m_pLootManager->EnemyKillRewardHandler(iAttackerH, iClientH);
					}
				}
			}
		}

		if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
			// Innocent
			if (memcmp(m_pGame->m_pClientList[iAttackerH]->m_cLocation, "NONE", 4) == 0) {
				//m_pGame->m_pClientList[iClientH]->m_iExp -= m_pGame->iDice(1, 100);
				//if (m_pGame->m_pClientList[iClientH]->m_iExp < 0) m_pGame->m_pClientList[iClientH]->m_iExp = 0;
				//m_pGame->SendNotifyMsg(0, iClientH, Notify::Exp, 0, 0, 0, 0);
			}
			else {
				if (memcmp(m_pGame->m_pClientList[iAttackerH]->m_cLocation, m_pGame->m_pClientList[iClientH]->m_cLocation, 10) == 0) {
					//m_pGame->m_pClientList[iClientH]->m_iExp -= m_pGame->iDice(1, 100);
					//if (m_pGame->m_pClientList[iClientH]->m_iExp < 0) m_pGame->m_pClientList[iClientH]->m_iExp = 0;
					//m_pGame->SendNotifyMsg(0, iClientH, Notify::Exp, 0, 0, 0, 0);
				}
				else {
					m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 2, bIsSAattacked);
				}
			}
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 1) && (m_pGame->m_pClientList[iClientH]->m_iPKCount <= 3)) {
			// Criminal 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 3, bIsSAattacked);
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 4) && (m_pGame->m_pClientList[iClientH]->m_iPKCount <= 11)) {
			// Murderer 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 6, bIsSAattacked);
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 12)) {
			// Slaughterer 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 12, bIsSAattacked);
		}
		char cTxt[128];
		std::memset(cTxt, 0, sizeof(cTxt));
		std::snprintf(cTxt, sizeof(cTxt), "%s killed %s", m_pGame->m_pClientList[iAttackerH]->m_cCharName, m_pGame->m_pClientList[iClientH]->m_cCharName);
		for(int killedi = 0; killedi < MaxClients; killedi++) {
			if (m_pGame->m_pClientList[killedi] != 0 && killedi != iAttackerH) {
				m_pGame->SendNotifyMsg(0, killedi, Notify::NoticeMsg, 0, 0, 0, cTxt);
			}
		}
		std::memset(cTxt, 0, sizeof(cTxt));
		std::snprintf(cTxt, sizeof(cTxt), "%s(%s) killed %s(%s) in %s(%d,%d)", m_pGame->m_pClientList[iAttackerH]->m_cCharName, m_pGame->m_pClientList[iAttackerH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName, m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cMapName, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
		PutPvPLogFileList(cTxt); // Centu : log pvp
	}
	else if (cAttackerType == hb::shared::owner_class::Npc) {

		_bPKLog(PkLog::ByNpc, iClientH, 0, cAttackerName);

		if (m_pGame->m_pClientList[iClientH]->m_iPKCount == 0) {
			// Innocent
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 1, bIsSAattacked);
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 1) && (m_pGame->m_pClientList[iClientH]->m_iPKCount <= 3)) {
			// Criminal 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 3, bIsSAattacked);
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 4) && (m_pGame->m_pClientList[iClientH]->m_iPKCount <= 11)) {
			// Murderer 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 6, bIsSAattacked);
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_iPKCount >= 12)) {
			// Slaughterer 
			m_pGame->m_pLootManager->ApplyCombatKilledPenalty(iClientH, 12, bIsSAattacked);
		}
		if (m_pGame->m_pNpcList[iAttackerH]->m_iGuildGUID != 0) {

			if (m_pGame->m_pNpcList[iAttackerH]->m_cSide != m_pGame->m_pClientList[iClientH]->m_cSide) {
				for(int i = 1; i < MaxClients; i++)
					if ((m_pGame->m_pClientList[i] != 0) && (m_pGame->m_pClientList[i]->m_iGuildGUID == m_pGame->m_pNpcList[iAttackerH]->m_iGuildGUID) &&
						(m_pGame->m_pClientList[i]->m_iCrusadeDuty == 3)) {
						m_pGame->m_pClientList[i]->m_iConstructionPoint += (m_pGame->m_pClientList[iClientH]->m_iLevel / 2);

						if (m_pGame->m_pClientList[i]->m_iConstructionPoint > m_pGame->m_iMaxConstructionPoints)
							m_pGame->m_pClientList[i]->m_iConstructionPoint = m_pGame->m_iMaxConstructionPoints;

						//testcode
						std::snprintf(G_cTxt, sizeof(G_cTxt), "Enemy Player Killed by Npc! Construction +%d", (m_pGame->m_pClientList[iClientH]->m_iLevel / 2));
						PutLogList(G_cTxt);
						m_pGame->SendNotifyMsg(0, i, Notify::ConstructionPoint, m_pGame->m_pClientList[i]->m_iConstructionPoint, m_pGame->m_pClientList[i]->m_iWarContribution, 0, 0);
						return;
					}
			}
		}
		char cTxt[128];
		std::memset(cTxt, 0, sizeof(cTxt));
		std::snprintf(cTxt, sizeof(cTxt), "%s killed %s", m_pGame->m_pNpcList[iAttackerH]->m_cNpcName, m_pGame->m_pClientList[iClientH]->m_cCharName);
		for(int Killedi = 0; Killedi < MaxClients; Killedi++) {
			if (m_pGame->m_pClientList[Killedi] != 0) {
				m_pGame->SendNotifyMsg(0, Killedi, Notify::NoticeMsg, 0, 0, 0, cTxt);
			}
		}
	}
	else if (cAttackerType == hb::shared::owner_class::PlayerIndirect) {
		_bPKLog(PkLog::ByOther, iClientH, 0, 0);
		// m_pGame->m_pClientList[iClientH]->m_iExp -= m_pGame->iDice(1, 50);
		// if (m_pGame->m_pClientList[iClientH]->m_iExp < 0) m_pGame->m_pClientList[iClientH]->m_iExp = 0;

		// m_pGame->SendNotifyMsg(0, iClientH, Notify::Exp, 0, 0, 0, 0);
	}
}


void CombatManager::Effect_Damage_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3, bool bExp, int iAttr)
{
	int iPartyID, iDamage, iSideCondition, iIndex, iRemainLife, iTemp, iMaxSuperAttack, iRepDamage;
	char cAttackerSide, cDamageMoveDir;
	uint32_t dwTime, iExp;
	double dTmp1, dTmp2, dTmp3;
	short sAtkX, sAtkY, sTgtX, sTgtY, dX, dY, sItemIndex;

	if (cAttackerType == hb::shared::owner_class::Player)
		if (m_pGame->m_pClientList[sAttackerH] == 0) return;

	if (cAttackerType == hb::shared::owner_class::Npc)
		if (m_pGame->m_pNpcList[sAttackerH] == 0) return;

	if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex] != 0) &&
		(m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsHeldenianMap == 1) && (m_pGame->m_bHeldenianInitiated)) return;

	dwTime = GameClock::GetTimeMS();
	iDamage = m_pGame->iDice(sV1, sV2) + sV3;
	if (iDamage <= 0) iDamage = 0;

	switch (cAttackerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sAttackerH]->m_cHeroArmourBonus == 2) iDamage += 4;
		if ((m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::LeftHand)] == -1) || (m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] == -1)) {
			sItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
			if ((sItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex] != 0)) {
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 732 || m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 738) {
					iDamage *= (int)1.2;
				}
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 863 || m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 864) {
					if (m_pGame->m_pClientList[sAttackerH]->m_iRating > 0) {
						iRepDamage = m_pGame->m_pClientList[sAttackerH]->m_iRating / 100;
						if (iRepDamage < 5) iRepDamage = 5;
						if (iRepDamage > 15) iRepDamage = 15;
						iDamage += iRepDamage;
					}
					if (cTargetType == hb::shared::owner_class::Player) {
						if (m_pGame->m_pClientList[sTargetH] != 0) {
							if (m_pGame->m_pClientList[sTargetH]->m_iRating < 0) {
								iRepDamage = (abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 10);
								if (iRepDamage > 10) iRepDamage = 10;
								iDamage += iRepDamage;
							}
						}
					}
				}
			}
			sItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::Neck)];
			if ((sItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex] != 0)) {
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 859) { // NecklaceOfKloness  
					if (cTargetType == hb::shared::owner_class::Player) {
						if (m_pGame->m_pClientList[sTargetH] != 0) {
							iRepDamage = (abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 20);
							if (iRepDamage > 5) iRepDamage = 5;
							iDamage += iRepDamage;
						}
					}
				}
			}
		}

		if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[sAttackerH]->m_bIsPlayerCivil) && (cTargetType == hb::shared::owner_class::Player)) return;

		dTmp1 = (double)iDamage;
		dTmp2 = (double)(m_pGame->m_pClientList[sAttackerH]->m_iMag + m_pGame->m_pClientList[sAttackerH]->m_iAngelicMag);
		dTmp2 = dTmp2 / 3.3f;
		dTmp3 = dTmp1 + (dTmp1 * (dTmp2 / 100.0f));
		iDamage = (int)(dTmp3 + 0.5f);

		iDamage += m_pGame->m_pClientList[sAttackerH]->m_iAddMagicalDamage;
		if (iDamage <= 0) iDamage = 0;

		if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone)
			iDamage += iDamage / 3;

		if (m_pGame->m_pWarManager->bCheckHeldenianMap(sAttackerH, m_pGame->m_iBTFieldMapIndex, hb::shared::owner_class::Player) == 1) {
			iDamage += iDamage / 3;
		}

		if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_bIsCrusadeMode) && (m_pGame->m_pClientList[sAttackerH]->m_iCrusadeDuty == 1)) {
			if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 80) {
				iDamage += (iDamage * 7) / 10;
			}
			else if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 100) {
				iDamage += iDamage / 2;
			}
			else
				iDamage += iDamage / 3;
		}

		cAttackerSide = m_pGame->m_pClientList[sAttackerH]->m_cSide;
		sAtkX = m_pGame->m_pClientList[sAttackerH]->m_sX;
		sAtkY = m_pGame->m_pClientList[sAttackerH]->m_sY;
		iPartyID = m_pGame->m_pClientList[sAttackerH]->m_iPartyID;
		break;

	case hb::shared::owner_class::Npc:
		cAttackerSide = m_pGame->m_pNpcList[sAttackerH]->m_cSide;
		sAtkX = m_pGame->m_pNpcList[sAttackerH]->m_sX;
		sAtkY = m_pGame->m_pNpcList[sAttackerH]->m_sY;
		break;
	}

	switch (cTargetType) {
	case hb::shared::owner_class::Player:

		if (m_pGame->m_pClientList[sTargetH] == 0) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsInitComplete == false) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return;

		// GM mode damage immunity
		if (m_pGame->m_pClientList[sTargetH]->m_bIsGMMode)
		{
			uint32_t dwNow = GameClock::GetTimeMS();
			if (dwNow - m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime > 2000)
			{
				m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime = dwNow;
				m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::DamageImmune, 0, 0);
			}
			return;
		}

		if (m_pGame->m_pClientList[sTargetH]->m_status.bSlateInvincible) return;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_bIsCrusadeMode == false) &&
			(m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsPlayerCivil)) return;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsNeutral) &&
			(m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsOwnLocation)) return;

		if ((dwTime - m_pGame->m_pClientList[sTargetH]->m_dwTime) > (uint32_t)m_pGame->m_iLagProtectionInterval) return;
		if ((m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->m_bIsAttackEnabled == false)) return;
		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_bIsNeutral) && (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0)) return;
		if ((m_pGame->m_pClientList[sTargetH]->m_iPartyID != 0) && (iPartyID == m_pGame->m_pClientList[sTargetH]->m_iPartyID)) return;
		m_pGame->m_pClientList[sTargetH]->m_dwLogoutHackCheck = dwTime;

		if (cAttackerType == hb::shared::owner_class::Player) {
			if (m_pGame->m_pClientList[sAttackerH]->m_bIsSafeAttackMode) {
				iSideCondition = iGetPlayerRelationship(sAttackerH, sTargetH);
				if ((iSideCondition == 7) || (iSideCondition == 2) || (iSideCondition == 6)) {

				}
				else {
					if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone) {
						if (m_pGame->m_pClientList[sAttackerH]->m_iGuildGUID != m_pGame->m_pClientList[sTargetH]->m_iGuildGUID) {

						}
						else return;
					}
					else return;
				}
			}

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->iGetAttribute(m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY, 0x00000005) != 0) return;
		}

		m_pGame->m_pSkillManager->ClearSkillUsingStatus(sTargetH);

		switch (iAttr) {
		case 1:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsEarth != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsEarth;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 2:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsAir != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsAir;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 3:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsFire != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsFire;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 4:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsWater != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsWater;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		default: break;
		}

		iIndex = m_pGame->m_pClientList[sTargetH]->m_iMagicDamageSaveItemIndex;
		if ((iIndex != -1) && (iIndex >= 0) && (iIndex < hb::shared::limits::MaxItems)) {

			switch (m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_sIDnum) {
			case 335:
				dTmp1 = (double)iDamage;
				dTmp2 = dTmp1 * 0.2f;
				dTmp3 = dTmp1 - dTmp2;
				iDamage = (int)(dTmp3 + 0.5f);
				break;

			case 337:
				dTmp1 = (double)iDamage;
				dTmp2 = dTmp1 * 0.1f;
				dTmp3 = dTmp1 - dTmp2;
				iDamage = (int)(dTmp3 + 0.5f);
				break;
			}
			if (iDamage <= 0) iDamage = 0;

			iRemainLife = m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan;
			if (iRemainLife <= iDamage) {
				m_pGame->m_pItemManager->ItemDepleteHandler(sTargetH, iIndex, true);
			}
			else {
				m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan -= iDamage;
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::CurLifeSpan, iIndex, m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan, 0, 0);
			}
		}

		if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsMD != 0) {
			dTmp1 = (double)iDamage;
			dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsMD;
			dTmp3 = (dTmp2 / 100.0f) * dTmp1;
			iDamage = iDamage - (int)dTmp3;
		}

		if (cTargetType == hb::shared::owner_class::Player) {
			iDamage -= (m_pGame->iDice(1, m_pGame->m_pClientList[sTargetH]->m_iVit / 10) - 1);
			if (iDamage <= 0) iDamage = 0;
		}

		if ((m_pGame->m_pClientList[sTargetH]->m_bIsLuckyEffect) &&
			(m_pGame->iDice(1, 10) == 5) && (m_pGame->m_pClientList[sTargetH]->m_iHP <= iDamage)) {
			iDamage = m_pGame->m_pClientList[sTargetH]->m_iHP - 1;
		}

		if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] == 2)
			iDamage = iDamage / 2;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsSpecialAbilityEnabled)) {
			switch (m_pGame->m_pClientList[sTargetH]->m_iSpecialAbilityType) {
			case 51:
			case 52:
				return;
			}
		}

		m_pGame->m_pClientList[sTargetH]->m_iHP -= iDamage;
		// Interrupt spell casting on damage
		if (iDamage > 0) {
			m_pGame->m_pClientList[sTargetH]->m_dwLastDamageTakenTime = GameClock::GetTimeMS();
			if (m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime) {
				m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime = false;
				m_pGame->m_pClientList[sTargetH]->m_iSpellCount = -1;
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::SpellInterrupted, 0, 0, 0, 0);
			}
		}
		if (m_pGame->m_pClientList[sTargetH]->m_iHP <= 0) {
			ClientKilledHandler(sTargetH, sAttackerH, cAttackerType, iDamage);
		}
		else {
			if (iDamage > 0) {
				if (m_pGame->m_pClientList[sTargetH]->m_iAddTransMana > 0) {
					dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddTransMana;
					dTmp2 = (double)iDamage;
					dTmp3 = (dTmp1 / 100.0f) * dTmp2 + 1.0f;

					iTemp = m_pGame->iGetMaxMP(sTargetH);
					m_pGame->m_pClientList[sTargetH]->m_iMP += (int)dTmp3;
					if (m_pGame->m_pClientList[sTargetH]->m_iMP > iTemp) m_pGame->m_pClientList[sTargetH]->m_iMP = iTemp;
				}

				if (m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical > 0) {
					if (m_pGame->iDice(1, 100) <= static_cast<uint32_t>(m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical)) {
						iMaxSuperAttack = (m_pGame->m_pClientList[sTargetH]->m_iLevel / 10);
						if (m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft < iMaxSuperAttack) m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft++;
						m_pGame->SendNotifyMsg(0, sTargetH, Notify::SuperAttackLeft, 0, 0, 0, 0);
					}
				}

				m_pGame->SendNotifyMsg(0, sTargetH, Notify::Hp, 0, 0, 0, 0);
				m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);

				if (m_pGame->m_pClientList[sTargetH]->m_bSkillUsingStatus[19] != true) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->ClearOwner(0, sTargetH, hb::shared::owner_class::Player, m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY);
					m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Player, m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY);
				}

				if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);
					m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
				}
			}
		}

		sTgtX = m_pGame->m_pClientList[sTargetH]->m_sX;
		sTgtY = m_pGame->m_pClientList[sTargetH]->m_sY;
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sTargetH] == 0) return;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0) return;
		if ((m_pGame->m_bIsCrusadeMode) && (cAttackerSide == m_pGame->m_pNpcList[sTargetH]->m_cSide)) return;

		sTgtX = m_pGame->m_pNpcList[sTargetH]->m_sX;
		sTgtY = m_pGame->m_pNpcList[sTargetH]->m_sY;

		switch (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit) {
		case 1:
		case 2:
			return;

		case 4:
			if (sTgtX == sAtkX) {
				if (sTgtY == sAtkY) return;
				else if (sTgtY > sAtkY) cDamageMoveDir = 5;
				else if (sTgtY < sAtkY) cDamageMoveDir = 1;
			}
			else if (sTgtX > sAtkX) {
				if (sTgtY == sAtkY)     cDamageMoveDir = 3;
				else if (sTgtY > sAtkY) cDamageMoveDir = 4;
				else if (sTgtY < sAtkY) cDamageMoveDir = 2;
			}
			else if (sTgtX < sAtkX) {
				if (sTgtY == sAtkY)     cDamageMoveDir = 7;
				else if (sTgtY > sAtkY) cDamageMoveDir = 6;
				else if (sTgtY < sAtkY) cDamageMoveDir = 8;
			}

			dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
			dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

			if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) {
				cDamageMoveDir = static_cast<char>(m_pGame->iDice(1, 8));
				dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
				dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];
				if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) return;
			}

			m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->ClearOwner(5, sTargetH, hb::shared::owner_class::Npc, m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY);
			m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Npc, dX, dY);
			m_pGame->m_pNpcList[sTargetH]->m_sX = dX;
			m_pGame->m_pNpcList[sTargetH]->m_sY = dY;
			m_pGame->m_pNpcList[sTargetH]->m_cDir = cDamageMoveDir;

			m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Move, 0, 0, 0);

			dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
			dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

			if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) {
				cDamageMoveDir = static_cast<char>(m_pGame->iDice(1, 8));
				dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
				dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

				if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) return;
			}

			m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->ClearOwner(5, sTargetH, hb::shared::owner_class::Npc, m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY);
			m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Npc, dX, dY);
			m_pGame->m_pNpcList[sTargetH]->m_sX = dX;
			m_pGame->m_pNpcList[sTargetH]->m_sY = dY;
			m_pGame->m_pNpcList[sTargetH]->m_cDir = cDamageMoveDir;

			m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Move, 0, 0, 0);

			if (m_pGame->m_pWarManager->bCheckEnergySphereDestination(sTargetH, sAttackerH, cAttackerType)) {
				// Use EntityManager for NPC deletion
				if (m_pGame->m_pEntityManager != NULL)
					m_pGame->m_pEntityManager->DeleteEntity(sTargetH);
			}
			return;
		}

		if (cAttackerType == hb::shared::owner_class::Player) {
			switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
			case 40:
			case 41:
				if ((m_pGame->m_pClientList[sAttackerH]->m_cSide == 0) || (m_pGame->m_pNpcList[sTargetH]->m_cSide == m_pGame->m_pClientList[sAttackerH]->m_cSide)) return;
				break;
			}
		}

		switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
		case 67: // McGaffin
		case 68: // Perry
		case 69: // Devlin
			return;
		}

		if (m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage > 0) {
			dTmp1 = (double)iDamage;
			dTmp2 = (double)(m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage) / 100.0f;
			dTmp3 = dTmp1 * dTmp2;
			dTmp2 = dTmp1 - dTmp3;
			iDamage = (int)dTmp2;
			if (iDamage < 0) iDamage = 1;
		}

		if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] == 2)
			iDamage = iDamage / 2;

		m_pGame->m_pNpcList[sTargetH]->m_iHP -= iDamage;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP < 0) {
			// Use EntityManager for NPC death handling
			if (m_pGame->m_pEntityManager != NULL)
				m_pGame->m_pEntityManager->OnEntityKilled(sTargetH, sAttackerH, cAttackerType, iDamage);
		}
		else {
			switch (cAttackerType) {
			case hb::shared::owner_class::Player:
				if ((m_pGame->m_pNpcList[sTargetH]->m_sType != 21) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 55) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 56)
					&& (m_pGame->m_pNpcList[sTargetH]->m_cSide == cAttackerSide)) return;
				break;

			case hb::shared::owner_class::Npc:
				if (m_pGame->m_pNpcList[sAttackerH]->m_cSide == m_pGame->m_pNpcList[sTargetH]->m_cSide) return;
				break;
			}

			m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);

			if ((m_pGame->iDice(1, 3) == 2) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 0)) {
				if ((cAttackerType == hb::shared::owner_class::Npc) &&
					(m_pGame->m_pNpcList[sAttackerH]->m_sType == m_pGame->m_pNpcList[sTargetH]->m_sType) &&
					(m_pGame->m_pNpcList[sAttackerH]->m_cSide == m_pGame->m_pNpcList[sTargetH]->m_cSide)) return;

				m_pGame->m_pNpcList[sTargetH]->m_cBehavior = Behavior::Attack;
				m_pGame->m_pNpcList[sTargetH]->m_sBehaviorTurnCount = 0;
				m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex = sAttackerH;
				m_pGame->m_pNpcList[sTargetH]->m_cTargetType = cAttackerType;

				m_pGame->m_pNpcList[sTargetH]->m_dwTime = dwTime;

				if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
					m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Npc, hb::shared::magic::HoldObject);
				}

				if ((m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > 0) && (m_pGame->m_pNpcList[sTargetH]->m_bIsSummoned != true) &&
					(cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] != 0)) {
					if (m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > static_cast<uint32_t>(iDamage)) {
						iExp = iDamage;
						if ((m_pGame->m_bIsCrusadeMode) && (iExp > 10)) iExp = 10;

						if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp > 0) {
							dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
							dTmp2 = (double)iExp;
							dTmp3 = (dTmp1 / 100.0f) * dTmp2;
							iExp += (uint32_t)dTmp3;
						}

						if (m_pGame->m_pClientList[sAttackerH]->m_iLevel > 100) {
							switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
							case 55:
							case 56:
								iExp = 0;
								break;
							default: break;
							}
						}

						if (bExp)
							m_pGame->GetExp(sAttackerH, iExp, true);
						else m_pGame->GetExp(sAttackerH, (iExp / 2), true);
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp -= iDamage;
					}
					else {
						iExp = m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp;
						if ((m_pGame->m_bIsCrusadeMode) && (iExp > 10)) iExp = 10;

						if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp > 0) {
							dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
							dTmp2 = (double)iExp;
							dTmp3 = (dTmp1 / 100.0f) * dTmp2;
							iExp += (uint32_t)dTmp3;
						}

						if (m_pGame->m_pClientList[sAttackerH]->m_iLevel > 100) {
							switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
							case 55:
							case 56:
								iExp = 0;
								break;
							default: break;
							}
						}

						if (bExp)
							m_pGame->GetExp(sAttackerH, iExp, true);
						else m_pGame->GetExp(sAttackerH, (iExp / 2), true);
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp = 0;
					}
				}
			}
		}
		break;
	}
}


void CombatManager::Effect_Damage_Spot_DamageMove(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sAtkX, short sAtkY, short sV1, short sV2, short sV3, bool bExp, int iAttr)
{
	int iDamage, iSideCondition, iIndex, iRemainLife, iTemp, iMaxSuperAttack;
	uint32_t dwTime, wWeaponType;
	char cAttackerSide, cDamageMoveDir;
	double dTmp1, dTmp2, dTmp3;
	int iPartyID, iMoveDamage;
	short sTgtX, sTgtY;

	if (cAttackerType == hb::shared::owner_class::Player)
		if (m_pGame->m_pClientList[sAttackerH] == 0) return;

	if (cAttackerType == hb::shared::owner_class::Npc)
		if (m_pGame->m_pNpcList[sAttackerH] == 0) return;

	dwTime = GameClock::GetTimeMS();
	sTgtX = 0;
	sTgtY = 0;

	iDamage = m_pGame->iDice(sV1, sV2) + sV3;
	if (iDamage <= 0) iDamage = 0;

	iPartyID = 0;

	switch (cAttackerType) {
	case hb::shared::owner_class::Player:
		dTmp1 = (double)iDamage;
		dTmp2 = (double)(m_pGame->m_pClientList[sAttackerH]->m_iMag + m_pGame->m_pClientList[sAttackerH]->m_iAngelicMag);

		dTmp2 = dTmp2 / 3.3f;
		dTmp3 = dTmp1 + (dTmp1 * (dTmp2 / 100.0f));
		iDamage = (int)(dTmp3 + 0.5f);
		if (iDamage <= 0) iDamage = 0;

		// v1.432 2001 4 7 13 7
		iDamage += m_pGame->m_pClientList[sAttackerH]->m_iAddMagicalDamage;

		if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone)
			iDamage += iDamage / 3;

		// Crusade :     1.33
		if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_bIsCrusadeMode) && (m_pGame->m_pClientList[sAttackerH]->m_iCrusadeDuty == 1))
		{
			if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 80)
			{
				iDamage += (iDamage * 7) / 10;
			}
			else if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 100)
			{
				iDamage += iDamage / 2;
			}
			else iDamage += iDamage / 3;
		}

		if (m_pGame->m_pClientList[sAttackerH]->m_cHeroArmourBonus == 2) {
			iDamage += 4;
		}

		wWeaponType = m_pGame->m_pClientList[sAttackerH]->m_appearance.iWeaponType;
		if (wWeaponType == 34) {
			iDamage += iDamage / 3;
		}

		if (m_pGame->m_pWarManager->bCheckHeldenianMap(sAttackerH, m_pGame->m_iBTFieldMapIndex, hb::shared::owner_class::Player) == 1) {
			iDamage += iDamage / 3;
		}

		cAttackerSide = m_pGame->m_pClientList[sAttackerH]->m_cSide;

		iPartyID = m_pGame->m_pClientList[sAttackerH]->m_iPartyID;
		break;

	case hb::shared::owner_class::Npc:
		cAttackerSide = m_pGame->m_pNpcList[sAttackerH]->m_cSide;
		break;
	}

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsInitComplete == false) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return;

		// GM mode damage immunity
		if (m_pGame->m_pClientList[sTargetH]->m_bIsGMMode)
		{
			uint32_t dwNow = GameClock::GetTimeMS();
			if (dwNow - m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime > 2000)
			{
				m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime = dwNow;
				m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::DamageImmune, 0, 0);
			}
			return;
		}

		if ((dwTime - m_pGame->m_pClientList[sTargetH]->m_dwTime) > (uint32_t)m_pGame->m_iLagProtectionInterval) return;
		if (m_pGame->m_pClientList[sTargetH]->m_cMapIndex == -1) return;
		if ((m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->m_bIsAttackEnabled == false)) return;
		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_bIsNeutral) && (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0)) return;

		if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsPlayerCivil)) return;
		if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_bIsPlayerCivil)) return;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsNeutral) && (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsPlayerCivil)) return;

		// 01-12-17
		if ((m_pGame->m_pClientList[sTargetH]->m_iPartyID != 0) && (iPartyID == m_pGame->m_pClientList[sTargetH]->m_iPartyID)) return;
		m_pGame->m_pClientList[sTargetH]->m_dwLogoutHackCheck = dwTime;

		if (cAttackerType == hb::shared::owner_class::Player) {

			if (m_pGame->m_pClientList[sAttackerH]->m_bIsSafeAttackMode) {
				iSideCondition = iGetPlayerRelationship(sAttackerH, sTargetH);
				if ((iSideCondition == 7) || (iSideCondition == 2) || (iSideCondition == 6)) {
				}
				else {
					if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone) {
						if (m_pGame->m_pClientList[sAttackerH]->m_iGuildGUID != m_pGame->m_pClientList[sTargetH]->m_iGuildGUID) {
						}
						else return;
					}
					else return;
				}
			}

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->iGetAttribute(m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY, 0x00000005) != 0) return;
		}

		m_pGame->m_pSkillManager->ClearSkillUsingStatus(sTargetH);

		switch (iAttr) {
		case 1:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsEarth != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsEarth;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 2:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsAir != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsAir;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 3:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsFire != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsFire;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		case 4:
			if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsWater != 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsWater;
				dTmp3 = (dTmp2 / 100.0f) * dTmp1;
				iDamage = iDamage - (int)(dTmp3);
				if (iDamage < 0) iDamage = 0;
			}
			break;

		default: break;
		}

		iIndex = m_pGame->m_pClientList[sTargetH]->m_iMagicDamageSaveItemIndex;
		if ((iIndex != -1) && (iIndex >= 0) && (iIndex < hb::shared::limits::MaxItems)) {

			switch (m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_sIDnum) {
			case 335:
				dTmp1 = (double)iDamage;
				dTmp2 = dTmp1 * 0.2f;
				dTmp3 = dTmp1 - dTmp2;
				iDamage = (int)(dTmp3 + 0.5f);
				break;

			case 337:
				dTmp1 = (double)iDamage;
				dTmp2 = dTmp1 * 0.1f;
				dTmp3 = dTmp1 - dTmp2;
				iDamage = (int)(dTmp3 + 0.5f);
				break;
			}
			if (iDamage <= 0) iDamage = 0;

			iRemainLife = m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan;
			if (iRemainLife <= iDamage) {
				m_pGame->m_pItemManager->ItemDepleteHandler(sTargetH, iIndex, true);
			}
			else {
				m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan -= iDamage;
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::CurLifeSpan, iIndex, m_pGame->m_pClientList[sTargetH]->m_pItemList[iIndex]->m_wCurLifeSpan, 0, 0);
			}
		}

		if (m_pGame->m_pClientList[sTargetH]->m_iAddAbsMD != 0) {
			dTmp1 = (double)iDamage;
			dTmp2 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddAbsMD;
			dTmp3 = (dTmp2 / 100.0f) * dTmp1;
			iDamage = iDamage - (int)dTmp3;
		}

		if (cTargetType == hb::shared::owner_class::Player) {
			iDamage -= (m_pGame->iDice(1, m_pGame->m_pClientList[sTargetH]->m_iVit / 10) - 1);
			if (iDamage <= 0) iDamage = 0;
		}

		if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] == 2)
			iDamage = iDamage / 2;

		if ((m_pGame->m_pClientList[sTargetH]->m_bIsLuckyEffect) &&
			(m_pGame->iDice(1, 10) == 5) && (m_pGame->m_pClientList[sTargetH]->m_iHP <= iDamage)) {
			iDamage = m_pGame->m_pClientList[sTargetH]->m_iHP - 1;
		}

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsSpecialAbilityEnabled)) {
			switch (m_pGame->m_pClientList[sTargetH]->m_iSpecialAbilityType) {
			case 51:
			case 52:
				return;
			}
		}

		m_pGame->m_pClientList[sTargetH]->m_iHP -= iDamage;
		// Interrupt spell casting on damage
		if (iDamage > 0) {
			m_pGame->m_pClientList[sTargetH]->m_dwLastDamageTakenTime = GameClock::GetTimeMS();
			if (m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime) {
				m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime = false;
				m_pGame->m_pClientList[sTargetH]->m_iSpellCount = -1;
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::SpellInterrupted, 0, 0, 0, 0);
			}
		}
		if (m_pGame->m_pClientList[sTargetH]->m_iHP <= 0) {
			ClientKilledHandler(sTargetH, sAttackerH, cAttackerType, iDamage);
		}
		else {
			if (iDamage > 0) {
				if (m_pGame->m_pClientList[sTargetH]->m_iAddTransMana > 0) {
					dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddTransMana;
					dTmp2 = (double)iDamage;
					dTmp3 = (dTmp1 / 100.0f) * dTmp2 + 1.0f;

					iTemp = m_pGame->iGetMaxMP(sTargetH);
					m_pGame->m_pClientList[sTargetH]->m_iMP += (int)dTmp3;
					if (m_pGame->m_pClientList[sTargetH]->m_iMP > iTemp) m_pGame->m_pClientList[sTargetH]->m_iMP = iTemp;
				}

				if (m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical > 0) {
					if (m_pGame->iDice(1, 100) <= static_cast<uint32_t>(m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical)) {
						iMaxSuperAttack = (m_pGame->m_pClientList[sTargetH]->m_iLevel / 10);
						if (m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft < iMaxSuperAttack) m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft++;
						m_pGame->SendNotifyMsg(0, sTargetH, Notify::SuperAttackLeft, 0, 0, 0, 0);
					}
				}

				if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone))
					iMoveDamage = 80;
				else iMoveDamage = 50;

				if (iDamage >= iMoveDamage) {
			///		char cDamageMoveDir;
					sTgtX = m_pGame->m_pClientList[sTargetH]->m_sX;
					sTgtY = m_pGame->m_pClientList[sTargetH]->m_sY;

					if (sTgtX == sAtkX) {
						if (sTgtY == sAtkY)     goto EDSD_SKIPDAMAGEMOVE;
						else if (sTgtY > sAtkY) cDamageMoveDir = 5;
						else if (sTgtY < sAtkY) cDamageMoveDir = 1;
					}
					else if (sTgtX > sAtkX) {
						if (sTgtY == sAtkY)     cDamageMoveDir = 3;
						else if (sTgtY > sAtkY) cDamageMoveDir = 4;
						else if (sTgtY < sAtkY) cDamageMoveDir = 2;
					}
					else if (sTgtX < sAtkX) {
						if (sTgtY == sAtkY)     cDamageMoveDir = 7;
						else if (sTgtY > sAtkY) cDamageMoveDir = 6;
						else if (sTgtY < sAtkY) cDamageMoveDir = 8;
					}

					m_pGame->m_pClientList[sTargetH]->m_iLastDamage = iDamage;
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::Hp, 0, 0, 0, 0);
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::DamageMove, cDamageMoveDir, iDamage, 0, 0);
				}
				else {
				EDSD_SKIPDAMAGEMOVE:
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::Hp, 0, 0, 0, 0);
					m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);
				}

				if (m_pGame->m_pClientList[sTargetH]->m_bSkillUsingStatus[19] != true) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->ClearOwner(0, sTargetH, hb::shared::owner_class::Player, m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY);
					m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Player, m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY);
				}

				if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
					// Hold-Person    .     .
					// 1: Hold-Person 
					// 2: Paralize
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);

					m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
				}
			}
		}
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sTargetH] == 0) return;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0) return;
		if ((m_pGame->m_bIsCrusadeMode) && (cAttackerSide == m_pGame->m_pNpcList[sTargetH]->m_cSide)) return;

		switch (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit) {
		case 1:
		case 2:
		case 4:
			return;
		}

		if (cAttackerType == hb::shared::owner_class::Player) {
			switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
			case 40:
			case 41:
				if ((m_pGame->m_pClientList[sAttackerH]->m_cSide == 0) || (m_pGame->m_pNpcList[sTargetH]->m_cSide == m_pGame->m_pClientList[sAttackerH]->m_cSide)) return;
				break;
			}
		}

		switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
		case 67: // McGaffin
		case 68: // Perry
		case 69: // Devlin
			iDamage = 0;
			break;
		}

		// (AbsDamage 0 )    .
		if (m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage > 0) {
			dTmp1 = (double)iDamage;
			dTmp2 = (double)(m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage) / 100.0f;
			dTmp3 = dTmp1 * dTmp2;
			dTmp2 = dTmp1 - dTmp3;
			iDamage = (int)dTmp2;
			if (iDamage < 0) iDamage = 1;
		}

		if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] == 2)
			iDamage = iDamage / 2;

		m_pGame->m_pNpcList[sTargetH]->m_iHP -= iDamage;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP < 0) {
			// NPC .
			m_pGame->m_pEntityManager->OnEntityKilled(sTargetH, sAttackerH, cAttackerType, iDamage);
		}
		else {

			switch (cAttackerType) {
			case hb::shared::owner_class::Player:
				if ((m_pGame->m_pNpcList[sTargetH]->m_sType != 21) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 55) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 56)
					&& (m_pGame->m_pNpcList[sTargetH]->m_cSide == cAttackerSide)) return;
				break;

			case hb::shared::owner_class::Npc:
				if (m_pGame->m_pNpcList[sAttackerH]->m_cSide == m_pGame->m_pNpcList[sTargetH]->m_cSide) return;
				break;
			}

			m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);

			if ((m_pGame->iDice(1, 3) == 2) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 0)) {

				if ((cAttackerType == hb::shared::owner_class::Npc) &&
					(m_pGame->m_pNpcList[sAttackerH]->m_sType == m_pGame->m_pNpcList[sTargetH]->m_sType) &&
					(m_pGame->m_pNpcList[sAttackerH]->m_cSide == m_pGame->m_pNpcList[sTargetH]->m_cSide)) return;

				// ActionLimit 1   .   .
				m_pGame->m_pNpcList[sTargetH]->m_cBehavior = Behavior::Attack;
				m_pGame->m_pNpcList[sTargetH]->m_sBehaviorTurnCount = 0;
				m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex = sAttackerH;
				m_pGame->m_pNpcList[sTargetH]->m_cTargetType = cAttackerType;


				// Damage    .
				m_pGame->m_pNpcList[sTargetH]->m_dwTime = dwTime;

				if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
					// Hold    .
					m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Npc, hb::shared::magic::HoldObject);
				}

				//Crusade
				uint32_t iExp;

				// NPC           .
				if ((m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > 0) && (m_pGame->m_pNpcList[sTargetH]->m_bIsSummoned != true) &&
					(cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] != 0)) {
					// ExpStock .      .
					if (m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > static_cast<uint32_t>(iDamage)) {
						// Crusade
						iExp = iDamage;
						if ((m_pGame->m_bIsCrusadeMode) && (iExp > 10)) iExp = 10;

						if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp > 0) {
							dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
							dTmp2 = (double)iExp;
							dTmp3 = (dTmp1 / 100.0f) * dTmp2;
							iExp += (uint32_t)dTmp3;
						}

						if (m_pGame->m_pClientList[sAttackerH]->m_iLevel > 100) {
							switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
							case 55:
							case 56:
								iExp = 0;
								break;
							default: break;
							}
						}

						if (bExp)
							m_pGame->GetExp(sAttackerH, iExp); //m_pGame->m_pClientList[sAttackerH]->m_iExpStock += iExp;     //iDamage;
						else m_pGame->GetExp(sAttackerH, (iExp / 2)); //m_pGame->m_pClientList[sAttackerH]->m_iExpStock += (iExp/2); //(iDamage/2);
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp -= iDamage;
					}
					else {
						// Crusade
						iExp = m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp;
						if ((m_pGame->m_bIsCrusadeMode) && (iExp > 10)) iExp = 10;

						if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp > 0) {
							dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
							dTmp2 = (double)iExp;
							dTmp3 = (dTmp1 / 100.0f) * dTmp2;
							iExp += (uint32_t)dTmp3;
						}

						if (m_pGame->m_pClientList[sAttackerH]->m_iLevel > 100) {
							switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
							case 55:
							case 56:
								iExp = 0;
								break;
							default: break;
							}
						}


						if (bExp)
							m_pGame->GetExp(sAttackerH, iExp); //m_pGame->m_pClientList[sAttackerH]->m_iExpStock += iExp;     //m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp;
						else m_pGame->GetExp(sAttackerH, (iExp / 2)); //m_pGame->m_pClientList[sAttackerH]->m_iExpStock += (iExp/2); //(m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp/2);
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp = 0;
					}
				}
			}
		}
		break;
	}
}


void CombatManager::Effect_HpUp_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3)
{
	int iHP, iMaxHP;
	uint32_t dwTime = GameClock::GetTimeMS();

	if (cAttackerType == hb::shared::owner_class::Player)
		if (m_pGame->m_pClientList[sAttackerH] == 0) return;

	iHP = m_pGame->iDice(sV1, sV2) + sV3;

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return;
		iMaxHP = (3 * m_pGame->m_pClientList[sTargetH]->m_iVit) + (2 * m_pGame->m_pClientList[sTargetH]->m_iLevel) + ((m_pGame->m_pClientList[sTargetH]->m_iStr + m_pGame->m_pClientList[sTargetH]->m_iAngelicStr) / 2);
		if (m_pGame->m_pClientList[sTargetH]->m_iSideEffect_MaxHPdown != 0)
			iMaxHP = iMaxHP - (iMaxHP / m_pGame->m_pClientList[sTargetH]->m_iSideEffect_MaxHPdown);
		if (m_pGame->m_pClientList[sTargetH]->m_iHP < iMaxHP) {
			m_pGame->m_pClientList[sTargetH]->m_iHP += iHP;
			if (m_pGame->m_pClientList[sTargetH]->m_iHP > iMaxHP) m_pGame->m_pClientList[sTargetH]->m_iHP = iMaxHP;
			if (m_pGame->m_pClientList[sTargetH]->m_iHP <= 0)     m_pGame->m_pClientList[sTargetH]->m_iHP = 1;
			m_pGame->SendNotifyMsg(0, sTargetH, Notify::Hp, 0, 0, 0, 0);
		}
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sTargetH] == 0) return;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0) return;
		if (m_pGame->m_pNpcList[sTargetH]->m_bIsKilled) return;
		iMaxHP = m_pGame->m_pNpcList[sTargetH]->m_iHitDice * 4;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP < iMaxHP) {
			m_pGame->m_pNpcList[sTargetH]->m_iHP += iHP;
			if (m_pGame->m_pNpcList[sTargetH]->m_iHP > iMaxHP) m_pGame->m_pNpcList[sTargetH]->m_iHP = iMaxHP;
			if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0)     m_pGame->m_pNpcList[sTargetH]->m_iHP = 1;
		}
		break;
	}
}


void CombatManager::Effect_SpDown_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3)
{
	int iSP, iMaxSP;
	uint32_t dwTime = GameClock::GetTimeMS();

	if (cAttackerType == hb::shared::owner_class::Player)
		if (m_pGame->m_pClientList[sAttackerH] == 0) return;

	iSP = m_pGame->iDice(sV1, sV2) + sV3;

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return;

		// New 19/05/2004
		// Is the user having an invincibility slate
		if (m_pGame->m_pClientList[sTargetH]->m_status.bSlateInvincible) return;

		iMaxSP = (2 * (m_pGame->m_pClientList[sTargetH]->m_iStr + m_pGame->m_pClientList[sTargetH]->m_iAngelicStr)) + (2 * m_pGame->m_pClientList[sTargetH]->m_iLevel);
		if (m_pGame->m_pClientList[sTargetH]->m_iSP > 0) {

			//v1.42 
			if (m_pGame->m_pClientList[sTargetH]->m_iTimeLeft_FirmStaminar == 0) {
				m_pGame->m_pClientList[sTargetH]->m_iSP -= iSP;
				if (m_pGame->m_pClientList[sTargetH]->m_iSP < 0) m_pGame->m_pClientList[sTargetH]->m_iSP = 0;
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::Sp, 0, 0, 0, 0);
			}
		}
		break;

	case hb::shared::owner_class::Npc:
		// NPC   .
		break;
	}
}


void CombatManager::Effect_SpUp_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3)
{
	int iSP, iMaxSP;
	uint32_t dwTime = GameClock::GetTimeMS();

	if (cAttackerType == hb::shared::owner_class::Player)
		if (m_pGame->m_pClientList[sAttackerH] == 0) return;

	iSP = m_pGame->iDice(sV1, sV2) + sV3;

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return;

		iMaxSP = (2 * (m_pGame->m_pClientList[sTargetH]->m_iStr + m_pGame->m_pClientList[sTargetH]->m_iAngelicStr)) + (2 * m_pGame->m_pClientList[sTargetH]->m_iLevel);
		if (m_pGame->m_pClientList[sTargetH]->m_iSP < iMaxSP) {
			m_pGame->m_pClientList[sTargetH]->m_iSP += iSP;

			if (m_pGame->m_pClientList[sTargetH]->m_iSP > iMaxSP)
				m_pGame->m_pClientList[sTargetH]->m_iSP = iMaxSP;

			m_pGame->SendNotifyMsg(0, sTargetH, Notify::Sp, 0, 0, 0, 0);
		}
		break;

	case hb::shared::owner_class::Npc:
		// NPC   .
		break;
	}
}


/*********************************************************************************************************************
**  int bool CombatManager::bCheckResistingMagicSuccess(char cAttackerDir, short sTargetH, char cTargetType, int iHitRatio) **
**  description			:: calculates if a player resists magic														**
**  last updated		:: November 20, 2004; 8:42 PM; Hypnotoad													**
**	return value		:: bool																						**
**  commentary			::	-	hero armor for target mages adds 50 magic resistance								**
**							-	10000 or more it ratio will deduct 10000 hit ratio									**
**							-	invincible tablet is 100% magic resistance											**
**********************************************************************************************************************/
bool CombatManager::bCheckResistingMagicSuccess(char cAttackerDir, short sTargetH, char cTargetType, int iHitRatio)
{
	double dTmp1, dTmp2, dTmp3;
	int    iTargetMagicResistRatio, iDestHitRatio, iResult;
	char   cTargetDir, cProtect;

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return false;
		if (m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->m_bIsAttackEnabled == false) return false;

		if (m_pGame->m_pClientList[sTargetH]->m_status.bSlateInvincible) return true;
		cTargetDir = m_pGame->m_pClientList[sTargetH]->m_cDir;
		iTargetMagicResistRatio = m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[3] + m_pGame->m_pClientList[sTargetH]->m_iAddMR;
		if ((m_pGame->m_pClientList[sTargetH]->m_iMag + m_pGame->m_pClientList[sTargetH]->m_iAngelicMag) > 50)
			iTargetMagicResistRatio += ((m_pGame->m_pClientList[sTargetH]->m_iMag + m_pGame->m_pClientList[sTargetH]->m_iAngelicMag) - 50);
		iTargetMagicResistRatio += m_pGame->m_pClientList[sTargetH]->m_iAddResistMagic;
		cProtect = m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect];
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sTargetH] == 0) return false;
		cTargetDir = m_pGame->m_pNpcList[sTargetH]->m_cDir;
		iTargetMagicResistRatio = m_pGame->m_pNpcList[sTargetH]->m_cResistMagic;
		cProtect = m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect];
		break;
	}

	if (cProtect == 5) return true;

	if ((iHitRatio < 1000) && (cProtect == 2)) return true;
	if (iHitRatio >= 10000) iHitRatio -= 10000;
	if (iTargetMagicResistRatio < 1) iTargetMagicResistRatio = 1;
	if (sTargetH < MaxClients)
	{
		if ((cAttackerDir != 0) && (m_pGame->m_pClientList[sTargetH] != 0) && (m_pGame->m_pClientList[sTargetH]->m_cHeroArmourBonus == 2)) {
			iHitRatio += 50;
		}
	}

	dTmp1 = (double)(iHitRatio);
	dTmp2 = (double)(iTargetMagicResistRatio);
	dTmp3 = (dTmp1 / dTmp2) * 50.0f;
	iDestHitRatio = (int)(dTmp3);

	if (iDestHitRatio < m_pGame->m_iMinimumHitRatio) iDestHitRatio = m_pGame->m_iMinimumHitRatio;
	if (iDestHitRatio > m_pGame->m_iMaximumHitRatio) iDestHitRatio = m_pGame->m_iMaximumHitRatio;
	if (iDestHitRatio >= 100) return false;

	iResult = m_pGame->iDice(1, 100);
	if (iResult <= iDestHitRatio) return false;

	if (cTargetType == hb::shared::owner_class::Player)
		m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sTargetH, 3, 1);
	return true;
}


bool CombatManager::bCheckResistingIceSuccess(char cAttackerDir, short sTargetH, char cTargetType, int iHitRatio)
{
	int    iTargetIceResistRatio, iResult;

	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sTargetH] == 0) return false;

		iTargetIceResistRatio = m_pGame->m_pClientList[sTargetH]->m_iAddAbsWater * 2;
		if (m_pGame->m_pClientList[sTargetH]->m_dwWarmEffectTime == 0) {
		}
		else if ((GameClock::GetTimeMS() - m_pGame->m_pClientList[sTargetH]->m_dwWarmEffectTime) < 1000 * 30) return true;
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sTargetH] == 0) return false;
		iTargetIceResistRatio = (m_pGame->m_pNpcList[sTargetH]->m_cResistMagic) - (m_pGame->m_pNpcList[sTargetH]->m_cResistMagic / 3); // . NPC    70%
		break;
	}

	if (iTargetIceResistRatio < 1) iTargetIceResistRatio = 1;

	iResult = m_pGame->iDice(1, 100);
	if (iResult <= iTargetIceResistRatio) return true;

	return false;
}


bool CombatManager::bAnalyzeCriminalAction(int iClientH, short dX, short dY, bool bIsCheck)
{
	int   iNamingValue, tX, tY;
	short sOwnerH;
	char  cOwnerType, cName[hb::shared::limits::CharNameLen], cNpcName[hb::shared::limits::NpcNameLen];
	char  cNpcWaypoint[11];

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return false;
	if (m_pGame->m_bIsCrusadeMode) return false;

	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

	if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0)) {
		if (_bGetIsPlayerHostile(iClientH, sOwnerH) != true) {
			if (bIsCheck) return true;

			std::memset(cNpcName, 0, sizeof(cNpcName));
			if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "aresden") == 0)
				strcpy(cNpcName, "Guard-Aresden");
			else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elvine") == 0)
				strcpy(cNpcName, "Guard-Elvine");
			else  if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "default") == 0)
				strcpy(cNpcName, "Guard-Neutral");

			iNamingValue = m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->iGetEmptyNamingValue();
			if (iNamingValue == -1) {

			}
			else {
				std::memset(cNpcWaypoint, 0, sizeof(cNpcWaypoint));
				std::memset(cName, 0, sizeof(cName));
				std::snprintf(cName, sizeof(cName), "XX%d", iNamingValue);
				cName[0] = '_';
				cName[1] = m_pGame->m_pClientList[iClientH]->m_cMapIndex + 65;

				tX = (int)m_pGame->m_pClientList[iClientH]->m_sX;
				tY = (int)m_pGame->m_pClientList[iClientH]->m_sY;
				int iNpcConfigId = m_pGame->GetNpcConfigIdByName(cNpcName);
				if (m_pGame->bCreateNewNpc(iNpcConfigId, cName, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, 0, 0, MoveType::Random,
					&tX, &tY, cNpcWaypoint, 0, 0, -1, false, true) == false) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetNamingValueEmpty(iNamingValue);
				}
				else {
					if (m_pGame->m_pEntityManager != 0) m_pGame->m_pEntityManager->bSetNpcAttackMode(cName, iClientH, hb::shared::owner_class::Player, true);
				}
			}
		}
	}
	return false;
}


bool CombatManager::_bGetIsPlayerHostile(int iClientH, int sOwnerH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (m_pGame->m_pClientList[sOwnerH] == 0) return false;

	if (iClientH == sOwnerH) return true;

	if (m_pGame->m_pClientList[iClientH]->m_cSide == 0) {
		if (m_pGame->m_pClientList[sOwnerH]->m_iPKCount != 0)
			return true;
		else return false;
	}
	else {
		if (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[sOwnerH]->m_cSide) {
			if (m_pGame->m_pClientList[sOwnerH]->m_cSide == 0) {
				if (m_pGame->m_pClientList[sOwnerH]->m_iPKCount != 0)
					return true;
				else return false;
			}
			else return true;
		}
		else {
			if (m_pGame->m_pClientList[sOwnerH]->m_iPKCount != 0)
				return true;
			else return false;
		}
	}

	return false;
}


void CombatManager::PoisonEffect(int iClientH, int iV1)
{
	int iPoisonLevel, iDamage, iPrevHP, iProb;

	if (m_pGame->m_pClientList[iClientH] == 0)     return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	iPoisonLevel = m_pGame->m_pClientList[iClientH]->m_iPoisonLevel;

	iDamage = m_pGame->iDice(1, iPoisonLevel);

	iPrevHP = m_pGame->m_pClientList[iClientH]->m_iHP;
	m_pGame->m_pClientList[iClientH]->m_iHP -= iDamage;
	if (m_pGame->m_pClientList[iClientH]->m_iHP <= 0) m_pGame->m_pClientList[iClientH]->m_iHP = 1;

	if (iPrevHP != m_pGame->m_pClientList[iClientH]->m_iHP)
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Hp, 0, 0, 0, 0);


	iProb = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[23] - 10 + m_pGame->m_pClientList[iClientH]->m_iAddPR;
	if (iProb <= 10) iProb = 10;
	if (m_pGame->iDice(1, 100) <= static_cast<uint32_t>(iProb)) {
		m_pGame->m_pClientList[iClientH]->m_bIsPoisoned = false;
		m_pGame->m_pStatusEffectManager->SetPoisonFlag(iClientH, hb::shared::owner_class::Player, false); // remove poison aura after effect complete
		m_pGame->SendNotifyMsg(0, iClientH, Notify::MagicEffectOff, hb::shared::magic::Poison, 0, 0, 0);
	}
}

bool CombatManager::bCheckResistingPoisonSuccess(short sOwnerH, char cOwnerType)
{
	int iResist, iResult;

	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return false;
		iResist = m_pGame->m_pClientList[sOwnerH]->m_cSkillMastery[23] + m_pGame->m_pClientList[sOwnerH]->m_iAddPR;
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return false;
		iResist = 0;
		break;
	}

	iResult = m_pGame->iDice(1, 100);
	if (iResult >= iResist)
		return false;

	if (cOwnerType == hb::shared::owner_class::Player)
		m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sOwnerH, 23, 1);

	return true;
}


int CombatManager::iGetPlayerRelationship(int iClientH, int iOpponentH)
{
	int iRet;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return 0;

	if (m_pGame->m_pClientList[iOpponentH] == 0) return 0;
	if (m_pGame->m_pClientList[iOpponentH]->m_bIsInitComplete == false) return 0;

	iRet = 0;

	if (m_pGame->m_pClientList[iClientH]->m_iPKCount != 0) {
		if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iOpponentH]->m_cLocation, 10) == 0) &&
			(memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) != 0) && (memcmp(m_pGame->m_pClientList[iOpponentH]->m_cLocation, "NONE", 4) != 0)) {
			iRet = 7;
		}
		else iRet = 2;
	}
	else if (m_pGame->m_pClientList[iOpponentH]->m_iPKCount != 0) {
		if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iOpponentH]->m_cLocation, 10) == 0) &&
			(memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) != 0))
			iRet = 6;
		else iRet = 2;
	}
	else {
		if (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[iOpponentH]->m_cSide) {
			if ((m_pGame->m_pClientList[iClientH]->m_cSide != 0) && (m_pGame->m_pClientList[iOpponentH]->m_cSide != 0)) {
				// 0(Traveler)  .
				iRet = 2;
			}
			else {
				iRet = 0;
			}
		}
		else {
			if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cGuildName, m_pGame->m_pClientList[iOpponentH]->m_cGuildName, 20) == 0) &&
				(memcmp(m_pGame->m_pClientList[iClientH]->m_cGuildName, "NONE", 4) != 0)) {
				if (m_pGame->m_pClientList[iOpponentH]->m_iGuildRank == 0)
					iRet = 5;
				else iRet = 3;
			}
			else
				if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[iOpponentH]->m_cLocation, 10) == 0) &&
					(memcmp(m_pGame->m_pClientList[iClientH]->m_cGuildName, "NONE", 4) != 0) &&
					(memcmp(m_pGame->m_pClientList[iOpponentH]->m_cGuildName, "NONE", 4) != 0) &&
					(memcmp(m_pGame->m_pClientList[iClientH]->m_cGuildName, m_pGame->m_pClientList[iOpponentH]->m_cGuildName, 20) != 0)) {
					iRet = 4;
				}
				else iRet = 1;
		}
	}

	return iRet;
}


EntityRelationship CombatManager::GetPlayerRelationship(int iOwnerH, int iViewerH)
{
	if (m_pGame->m_pClientList[iOwnerH] == 0 || m_pGame->m_pClientList[iViewerH] == 0)
		return EntityRelationship::Neutral;

	// Viewer is PK → everyone is enemy to them
	if (m_pGame->m_pClientList[iViewerH]->m_iPKCount != 0)
		return EntityRelationship::Enemy;

	// Target is PK
	if (m_pGame->m_pClientList[iOwnerH]->m_iPKCount != 0)
		return EntityRelationship::PK;

	// No faction = neutral
	if (m_pGame->m_pClientList[iOwnerH]->m_cSide == 0 || m_pGame->m_pClientList[iViewerH]->m_cSide == 0)
		return EntityRelationship::Neutral;

	// Same faction = friendly
	if (m_pGame->m_pClientList[iOwnerH]->m_cSide == m_pGame->m_pClientList[iViewerH]->m_cSide)
		return EntityRelationship::Friendly;

	// Different factions
	if (m_pGame->m_bIsCrusadeMode)
		return EntityRelationship::Enemy;

	// Both are combatants (non-hunter) = enemy
	if (!m_pGame->m_pClientList[iViewerH]->m_bIsPlayerCivil && !m_pGame->m_pClientList[iOwnerH]->m_bIsPlayerCivil)
		return EntityRelationship::Enemy;

	return EntityRelationship::Neutral;
}


void CombatManager::_CheckAttackType(int iClientH, short* spType)
{
	uint16_t wType;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	wType = m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType;

	switch (*spType) {
	case 2:
		// Effect  .
		if (m_pGame->m_pClientList[iClientH]->m_cArrowIndex == -1) *spType = 0;
		if (wType < 40) *spType = 1;
		break;

	case 20:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] < 100) *spType = 1;
		break;

	case 21:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] < 100) *spType = 1;
		break;

	case 22:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] < 100) *spType = 1;
		break;

	case 23:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)   *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] < 100) *spType = 1;
		break;

	case 24:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] < 100) *spType = 1;
		break;

	case 25:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] < 100) *spType = 2;
		if (m_pGame->m_pClientList[iClientH]->m_cArrowIndex == -1)      *spType = 0;
		if (wType < 40) *spType = 1;
		break;

	case 26:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] < 100) *spType = 1;
		break;

	case 27:
		if (m_pGame->m_pClientList[iClientH]->m_iSuperAttackLeft <= 0)  *spType = 1;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] < 100) *spType = 1;
		break;
	}
}


void CombatManager::CheckFireBluring(char cMapIndex, int sX, int sY)
{
	int iItemNum;
	char  cItemColor;
	CItem* pItem;
	short sIDNum;
	uint32_t dwAttr;

	for(int ix = sX - 1; ix <= sX + 1; ix++)
		for(int iy = sY - 1; iy <= sY + 1; iy++) {
			iItemNum = m_pGame->m_pMapList[cMapIndex]->iCheckItem(ix, iy);

			switch (iItemNum) {
			case 355:
				pItem = m_pGame->m_pMapList[cMapIndex]->pGetItem(ix, iy, &sIDNum, &cItemColor, &dwAttr);
				if (pItem != 0) delete pItem;
				m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(0, 0, dynamic_object::Fire, cMapIndex, ix, iy, 6000);

				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::SetItem, cMapIndex,
					ix, iy, sIDNum, 0, cItemColor, dwAttr);
				break;
			}
		}
}


int CombatManager::_iGetWeaponSkillType(int iClientH)
{
	uint16_t wWeaponType;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;

	wWeaponType = m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType;

	if (wWeaponType == 0) {
		return 5;
	}
	else if ((wWeaponType >= 1) && (wWeaponType <= 2)) {
		return 7;
	}
	else if ((wWeaponType > 2) && (wWeaponType < 20)) {
		if (wWeaponType == 7)
			return 9;
		else return 8;
	}
	else if ((wWeaponType >= 20) && (wWeaponType < 30)) {
		return 10;
	}
	else if ((wWeaponType >= 30) && (wWeaponType < 35)) {
		return 14;
	}
	else if ((wWeaponType >= 35) && (wWeaponType < 40)) {
		return 21;
	}
	else if (wWeaponType >= 40) {
		return 6;
	}

	return 1;
}


int CombatManager::iGetComboAttackBonus(int iSkill, int iComboCount)
{
	if (iComboCount <= 1) return 0;
	if (iComboCount > 6) return 0;
	switch (iSkill) {
	case 5:
		return ___iCAB5[iComboCount];
		break;
	case 6:
		return ___iCAB6[iComboCount];
		break;
	case 7:
		return ___iCAB7[iComboCount];
		break;
	case 8:
		return ___iCAB8[iComboCount];
		break;
	case 9:
		return ___iCAB9[iComboCount];
		break;
	case 10:
		return ___iCAB10[iComboCount];
		break;
	case 14:
		return ___iCAB6[iComboCount];
		break;
	case 21:
		return ___iCAB10[iComboCount];
		break;
	}

	return 0;
}


void CombatManager::ArmorLifeDecrement(int iAttackerH, int iTargetH, char cOwnerType, int iValue)
{
	int iTemp;

	if (m_pGame->m_pClientList[iAttackerH] == 0) return;
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[iTargetH] == 0) return;
		break;

	case hb::shared::owner_class::Npc:	return;
	default: return;
	}

	if (m_pGame->m_pClientList[iAttackerH]->m_cSide == m_pGame->m_pClientList[iTargetH]->m_cSide) return;

	iTemp = m_pGame->m_pClientList[iTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Body)];
	if ((iTemp != -1) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp] != 0)) {
		if ((m_pGame->m_pClientList[iTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan -= iValue;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
		}
		if ((m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan <= 0) || (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 64000)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan = 0;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
			m_pGame->m_pItemManager->ReleaseItemHandler(iTargetH, iTemp, true);
		}
	}

	iTemp = m_pGame->m_pClientList[iTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Pants)];
	if ((iTemp != -1) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp] != 0)) {

		if ((m_pGame->m_pClientList[iTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan -= iValue;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
		}
		if ((m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan <= 0) || (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 64000)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan = 0;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
			m_pGame->m_pItemManager->ReleaseItemHandler(iTargetH, iTemp, true);
		}
	}

	iTemp = m_pGame->m_pClientList[iTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Leggings)];
	if ((iTemp != -1) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp] != 0)) {

		if ((m_pGame->m_pClientList[iTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan -= iValue;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
		}
		if ((m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan <= 0) || (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 64000)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan = 0;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
			m_pGame->m_pItemManager->ReleaseItemHandler(iTargetH, iTemp, true);
		}
	}

	iTemp = m_pGame->m_pClientList[iTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Arms)];
	if ((iTemp != -1) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp] != 0)) {

		if ((m_pGame->m_pClientList[iTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan -= iValue;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
		}
		if ((m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan <= 0) || (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 64000)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan = 0;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
			m_pGame->m_pItemManager->ReleaseItemHandler(iTargetH, iTemp, true);
		}
	}

	iTemp = m_pGame->m_pClientList[iTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Head)];
	if ((iTemp != -1) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp] != 0)) {

		if ((m_pGame->m_pClientList[iTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan -= iValue;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
		}
		if ((m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan <= 0) || (m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 64000)) {
			m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan = 0;
			m_pGame->SendNotifyMsg(0, iTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[iTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
			m_pGame->m_pItemManager->ReleaseItemHandler(iTargetH, iTemp, true);
		}
	}
}


bool CombatManager::_bPKLog(int iAction, int iAttackerH, int iVictumH, char* pNPC)
{
	char  cTxt[1024], cTemp1[120], cTemp2[120];

	std::memset(cTxt, 0, sizeof(cTxt));
	std::memset(cTemp1, 0, sizeof(cTemp1));
	std::memset(cTemp2, 0, sizeof(cTemp2));

	if (m_pGame->m_pClientList[iVictumH] == 0) return false;

	switch (iAction) {

	case PkLog::ReduceCriminal:
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tReduce\tCC(%d)\t%s(%d %d)\t", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName, m_pGame->m_pClientList[iVictumH]->m_iPKCount,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY);
		break;

	case PkLog::ByPlayer:
		if (m_pGame->m_pClientList[iAttackerH] == 0) return false;
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tKilled by PC\t \t%s(%d %d)\t(%s) PC(%s)", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY, m_pGame->m_pClientList[iAttackerH]->m_cIPaddress, m_pGame->m_pClientList[iAttackerH]->m_cCharName);
		break;
	case PkLog::ByPk:
		if (m_pGame->m_pClientList[iAttackerH] == 0) return false;
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tKilled by PK\tCC(%d)\t%s(%d %d)\t(%s) PC(%s)", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName, m_pGame->m_pClientList[iAttackerH]->m_iPKCount,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY, m_pGame->m_pClientList[iAttackerH]->m_cIPaddress, m_pGame->m_pClientList[iAttackerH]->m_cCharName);
		break;
	case PkLog::ByEnemy:
		if (m_pGame->m_pClientList[iAttackerH] == 0) return false;
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tKilled by Enemy\t \t%s(%d %d)\t(%s) PC(%s)", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY, m_pGame->m_pClientList[iAttackerH]->m_cIPaddress, m_pGame->m_pClientList[iAttackerH]->m_cCharName);
		break;
	case PkLog::ByNpc:
		if (pNPC == 0) return false;
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tKilled by NPC\t \t%s(%d %d)\tNPC(%s)", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY, pNPC);
		break;
	case PkLog::ByOther:
		std::snprintf(cTxt, sizeof(cTxt), "(%s) PC(%s)\tKilled by Other\t \t%s(%d %d)\tUnknown", m_pGame->m_pClientList[iVictumH]->m_cIPaddress, m_pGame->m_pClientList[iVictumH]->m_cCharName,
			m_pGame->m_pClientList[iVictumH]->m_cMapName, m_pGame->m_pClientList[iVictumH]->m_sX, m_pGame->m_pClientList[iVictumH]->m_sY);
		break;
	default:
		return false;
	}
	PutPvPLogFileList(cTxt);
	return true;
}


bool CombatManager::bCheckClientAttackFrequency(int iClientH, uint32_t dwClientTime)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return false;

	if (m_pGame->m_pClientList[iClientH]->m_dwAttackFreqTime == 0)
		m_pGame->m_pClientList[iClientH]->m_dwAttackFreqTime = dwClientTime;
	else {
		uint32_t dwTimeGap = dwClientTime - m_pGame->m_pClientList[iClientH]->m_dwAttackFreqTime;
		m_pGame->m_pClientList[iClientH]->m_dwAttackFreqTime = dwClientTime;

		// Compute expected minimum swing time from player's weapon speed and status effects.
		// Must match client-side animation timing (PlayerAnim::Attack: sMaxFrame=7, frames 0-7 = 8 durations @ 78ms base).
		constexpr int ATTACK_FRAME_DURATIONS = 8;
		constexpr int BASE_FRAME_TIME = 78;
		constexpr int RUN_FRAME_TIME = 39;
		constexpr int TOLERANCE_MS = 50;

		const auto& status = m_pGame->m_pClientList[iClientH]->m_status;
		int iAttackDelay = status.iAttackDelay;  // 0 = full swing (STR meets weapon req)
		bool bHaste = status.bHaste;
		bool bFrozen = status.bFrozen;

		int effectiveFrameTime = BASE_FRAME_TIME + (iAttackDelay * 12);
		if (bFrozen) effectiveFrameTime += BASE_FRAME_TIME >> 2;
		if (bHaste)  effectiveFrameTime -= static_cast<int>(RUN_FRAME_TIME / 2.3);

		int expectedSwingTime = ATTACK_FRAME_DURATIONS * effectiveFrameTime;
		int threshold = expectedSwingTime - TOLERANCE_MS;
		if (threshold < 200) threshold = 200;

		if (dwTimeGap < static_cast<uint32_t>(threshold)) {
			try
			{
				std::snprintf(G_cTxt, sizeof(G_cTxt), "Swing Hack: (%s) Player: (%s) - attacking at irregular rates. Gap: %ums, Min: %dms",
					m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName,
					dwTimeGap, expectedSwingTime);
				PutHackLogFileList(G_cTxt);
				m_pGame->DeleteClient(iClientH, true, true);
			}
			catch (...)
			{
			}
			return false;
		}
	}

	return false;
}


bool CombatManager::bCalculateEnduranceDecrement(short sTargetH, short sAttackerH, char cTargetType, int iArmorType)
{
	int iDownValue = 1, iHammerChance = 100, iItemIndex;
	uint16_t wWeaponType;

	if (m_pGame->m_pClientList[sTargetH] == 0) return false;
	if (sAttackerH > MaxClients) return false;
	if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] == 0)) return false;
	wWeaponType = m_pGame->m_pClientList[sAttackerH]->m_appearance.iWeaponType;		// sAttackerH was 2536 == null
	if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_cSide != m_pGame->m_pClientList[sAttackerH]->m_cSide)) {
		switch (m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill) {
		case 14:
			if ((wWeaponType == 31) || (wWeaponType == 32)) {
				iItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
				if ((iItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex] != 0)) {
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 761) { // BattleHammer 
						iDownValue = 30;
					}
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 762) { // GiantBattleHammer
						iDownValue = 35;
					}
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 843) { // BarbarianHammer
						iDownValue = 30;
					}
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 745) { // MasterBattleHammer
						iDownValue = 30;
					}
					break;
				}
			}
			else {
				iDownValue = 20;
			}
			break;
		case 10:
			iDownValue = 3;
			break;
		default:
			iDownValue = 1;
			break;
		}
	}
	if (m_pGame->m_pClientList[sTargetH]->m_bIsSpecialAbilityEnabled) {
		switch (m_pGame->m_pClientList[sTargetH]->m_iSpecialAbilityType) {
		case 52:
			iDownValue = 0;
			iHammerChance = 0;
			break;
		}
	}
	if ((m_pGame->m_pClientList[sTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan > 0)) {
		m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan -= iDownValue;
		m_pGame->SendNotifyMsg(0, sTargetH, Notify::CurLifeSpan, iArmorType, m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan, 0, 0);
	}
	if (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan <= 0) {
		m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan = 0;
		m_pGame->SendNotifyMsg(0, sTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_cEquipPos, iArmorType, 0, 0);
		m_pGame->m_pItemManager->ReleaseItemHandler(sTargetH, iArmorType, true);
		return true;
	}
	if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill == 14) && (iHammerChance == 100)) {
		if (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wMaxLifeSpan < 2000) {
			iHammerChance = m_pGame->iDice(6, (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wMaxLifeSpan - m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan));
		}
		else {
			iHammerChance = m_pGame->iDice(4, (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wMaxLifeSpan - m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan));
		}
		if ((wWeaponType == 31) || (wWeaponType == 32)) {
			iItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
			if ((iItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex] != 0)) {
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 761) { // BattleHammer 
					iHammerChance -= iHammerChance >> 1;
				}
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 762) { // GiantBattleHammer
					iHammerChance = (((iHammerChance * 5) + 7) >> 3);
				}
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 843) { // BarbarianHammer
					iHammerChance = (((iHammerChance * 5) + 7) >> 3);
				}
				if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[iItemIndex]->m_sIDnum == 745) { // MasterBattleHammer
					iHammerChance = (((iHammerChance * 5) + 7) >> 3);
				}
			}
			else {
				iHammerChance = ((iHammerChance + 3) >> 2);
			}
			switch (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_sIDnum) {
			case 621:
			case 622:
				iHammerChance = 0;
				break;
			}
			if (m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_wCurLifeSpan < iHammerChance) {
				m_pGame->m_pItemManager->ReleaseItemHandler(sTargetH, iArmorType, true);
				m_pGame->SendNotifyMsg(0, sTargetH, Notify::ItemReleased, m_pGame->m_pClientList[sTargetH]->m_pItemList[iArmorType]->m_cEquipPos, iArmorType, 0, 0);
			}
		}
	}
	return true;
}


uint32_t CombatManager::iCalculateAttackEffect(short sTargetH, char cTargetType, short sAttackerH, char cAttackerType, int tdX, int tdY, int iAttackMode, bool bNearAttack, bool bIsDash, bool bArrowUse)
{
	int    iAP_SM, iAP_L, iAttackerHitRatio, iTargetDefenseRatio, iDestHitRatio, iResult, iAP_Abs_Armor, iAP_Abs_Shield;
	char   cAttackerName[hb::shared::limits::NpcNameLen], cAttackerDir, cAttackerSide, cTargetDir, cProtect, cCropSkill, cFarmingSkill;
	short  sWeaponIndex, sAttackerWeapon, dX, dY, sX, sY, sAtkX, sAtkY, sTgtX, sTgtY;
	uint32_t  dwTime;
	uint16_t   wWeaponType;
	double dTmp1, dTmp2, dTmp3;
	bool   bKilled;
	bool   bNormalMissileAttack;
	bool   bIsAttackerBerserk;
	int    iKilledDice, iDamage, iExp, iWepLifeOff, iSideCondition, iMaxSuperAttack, iWeaponSkill, iComboBonus, iTemp;
	int    iAttackerHP, iMoveDamage, iRepDamage;
	char   cAttackerSA;
	int    iAttackerSAvalue, iHitPoint;
	char   cDamageMoveDir;
	int    iPartyID, iConstructionPoint, iWarContribution, tX, tY, iDst1, iDst2;
	short sItemIndex;
	short sSkillUsed;

	dwTime = GameClock::GetTimeMS();
	bKilled = false;
	iExp = 0;
	iPartyID = 0;
	bNormalMissileAttack = false;
	std::memset(cAttackerName, 0, sizeof(cAttackerName));
	cAttackerSA = 0;
	iAttackerSAvalue = 0;
	wWeaponType = 0;

	switch (cAttackerType) {
	case hb::shared::owner_class::Player:

		if (m_pGame->m_pClientList[sAttackerH] == 0) return 0;
		if ((m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsAttackEnabled == false)) return 0;
		if ((m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex] == 0) && (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsHeldenianMap) && (m_pGame->m_bIsHeldenianMode)) return 0;
		if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[sAttackerH]->m_bIsPlayerCivil) && (cTargetType == hb::shared::owner_class::Player)) return 0;

		if (m_pGame->m_pClientList[sAttackerH]->m_status.bInvisibility) {
			m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sAttackerH, hb::shared::owner_class::Player, false);
			m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sAttackerH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
			m_pGame->m_pClientList[sAttackerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
		}

		if (!m_pGame->m_pClientList[sAttackerH]->m_appearance.bIsWalking) return 0;

		iAP_SM = 0;
		iAP_L = 0;

		wWeaponType = m_pGame->m_pClientList[sAttackerH]->m_appearance.iWeaponType;

		sSkillUsed = m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill;
		if ((bIsDash) && (m_pGame->m_pClientList[sAttackerH]->m_cSkillMastery[sSkillUsed] != 100) && (wWeaponType != 25) && (wWeaponType != 27)) {
			try
			{
				std::snprintf(G_cTxt, sizeof(G_cTxt), "TSearch Fullswing Hack: (%s) Player: (%s) - dashing with only (%d) weapon skill.", m_pGame->m_pClientList[sAttackerH]->m_cIPaddress, m_pGame->m_pClientList[sAttackerH]->m_cCharName, m_pGame->m_pClientList[sAttackerH]->m_cSkillMastery[sSkillUsed]);
				PutHackLogFileList(G_cTxt);
				m_pGame->DeleteClient(sAttackerH, true, true);
			}
			catch (...)
			{

			}
			return 0;
		}

		cAttackerSide = m_pGame->m_pClientList[sAttackerH]->m_cSide;

		if (wWeaponType == 0) {
			iAP_SM = iAP_L = m_pGame->iDice(1, ((m_pGame->m_pClientList[sAttackerH]->m_iStr + m_pGame->m_pClientList[sAttackerH]->m_iAngelicStr) / 12));
			if (iAP_SM <= 0) iAP_SM = 1;
			if (iAP_L <= 0) iAP_L = 1;
			iAttackerHitRatio = m_pGame->m_pClientList[sAttackerH]->m_iHitRatio + m_pGame->m_pClientList[sAttackerH]->m_cSkillMastery[5];
			m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill = 5;

		}
		else if ((wWeaponType >= 1) && (wWeaponType < 40)) {
			iAP_SM = m_pGame->iDice(m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceThrow_SM, m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceRange_SM);
			iAP_L = m_pGame->iDice(m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceThrow_L, m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceRange_L);

			iAP_SM += m_pGame->m_pClientList[sAttackerH]->m_cAttackBonus_SM;
			iAP_L += m_pGame->m_pClientList[sAttackerH]->m_cAttackBonus_L;

			iAttackerHitRatio = m_pGame->m_pClientList[sAttackerH]->m_iHitRatio;

			dTmp1 = (double)iAP_SM;
			dTmp2 = (double)(m_pGame->m_pClientList[sAttackerH]->m_iStr + m_pGame->m_pClientList[sAttackerH]->m_iAngelicStr);

			dTmp2 = dTmp2 / 5.0f;
			dTmp3 = dTmp1 + (dTmp1 * (dTmp2 / 100.0f));
			iAP_SM = (int)(dTmp3 + 0.5f);

			dTmp1 = (double)iAP_L;
			dTmp2 = (double)(m_pGame->m_pClientList[sAttackerH]->m_iStr + m_pGame->m_pClientList[sAttackerH]->m_iAngelicStr);

			dTmp2 = dTmp2 / 5.0f;
			dTmp3 = dTmp1 + (dTmp1 * (dTmp2 / 100.0f));
			iAP_L = (int)(dTmp3 + 0.5f);
		}
		else if (wWeaponType >= 40) {
			iAP_SM = m_pGame->iDice(m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceThrow_SM, m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceRange_SM);
			iAP_L = m_pGame->iDice(m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceThrow_L, m_pGame->m_pClientList[sAttackerH]->m_cAttackDiceRange_L);

			iAP_SM += m_pGame->m_pClientList[sAttackerH]->m_cAttackBonus_SM;
			iAP_L += m_pGame->m_pClientList[sAttackerH]->m_cAttackBonus_L;

			iAttackerHitRatio = m_pGame->m_pClientList[sAttackerH]->m_iHitRatio;
			bNormalMissileAttack = true;

			iAP_SM += m_pGame->iDice(1, ((m_pGame->m_pClientList[sAttackerH]->m_iStr + m_pGame->m_pClientList[sAttackerH]->m_iAngelicStr) / 20));
			iAP_L += m_pGame->iDice(1, ((m_pGame->m_pClientList[sAttackerH]->m_iStr + m_pGame->m_pClientList[sAttackerH]->m_iAngelicStr) / 20));
		}

		iAttackerHitRatio += 50;
		if (iAP_SM <= 0) iAP_SM = 1;
		if (iAP_L <= 0) iAP_L = 1;

		if (m_pGame->m_pClientList[sAttackerH]->m_iCustomItemValue_Attack != 0) {
			if ((m_pGame->m_pClientList[sAttackerH]->m_iMinAP_SM != 0) && (iAP_SM < m_pGame->m_pClientList[sAttackerH]->m_iMinAP_SM)) {
				iAP_SM = m_pGame->m_pClientList[sAttackerH]->m_iMinAP_SM;
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_iMinAP_L != 0) && (iAP_L < m_pGame->m_pClientList[sAttackerH]->m_iMinAP_L)) {
				iAP_L = m_pGame->m_pClientList[sAttackerH]->m_iMinAP_L;
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_SM != 0) && (iAP_SM > m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_SM)) {
				iAP_SM = m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_SM;
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_L != 0) && (iAP_L > m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_L)) {
				iAP_L = m_pGame->m_pClientList[sAttackerH]->m_iMaxAP_L;
			}
		}

		if (m_pGame->m_pClientList[sAttackerH]->m_cHeroArmourBonus == 1) {
			iAttackerHitRatio += 100;
			iAP_SM += 5;
			iAP_L += 5;
		}

		sItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
		if ((sItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex] != 0)) {
			if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 851) || // KlonessEsterk 
				(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 863) || // KlonessWand(MS.20)
				(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 864)) { // KlonessWand(MS.10) 
				if (m_pGame->m_pClientList[sAttackerH]->m_iRating > 0) {
					iRepDamage = m_pGame->m_pClientList[sAttackerH]->m_iRating / 100;
					if (iRepDamage < 5) iRepDamage = 5;
					if (iRepDamage > 15) iRepDamage = 15;
					iAP_SM += iRepDamage;
					iAP_L += iRepDamage;
				}
				if (cTargetType == hb::shared::owner_class::Player) {
					if (m_pGame->m_pClientList[sTargetH] == 0) return 0;
					if (m_pGame->m_pClientList[sTargetH]->m_iRating < 0) {
						iRepDamage = (abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 10);
						if (iRepDamage > 10) iRepDamage = 10;
						iAP_SM += iRepDamage;
						iAP_L += iRepDamage;
					}
				}
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 732) || // BerserkWand(MS.20)
				(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 738)) { // BerserkWand(MS.10)
				iAP_SM += 1;
				iAP_L += 1;
			}
		}

		sItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
		if ((sItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex] != 0)) {
			if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 847) &&
				(m_pGame->m_cDayOrNight == 2)) {
				iAP_SM += 4;
				iAP_L += 4;
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 848) &&
				(m_pGame->m_cDayOrNight == 1)) {
				iAP_SM += 4;
				iAP_L += 4;
			}
			if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 849) || // KlonessBlade 
				(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 850)) { // KlonessAxe
				if (m_pGame->m_pClientList[sAttackerH]->m_iRating > 0) {
					iRepDamage = m_pGame->m_pClientList[sAttackerH]->m_iRating / 100;
					if (iRepDamage < 5) iRepDamage = 5;
					if (iRepDamage > 15) iRepDamage = 15;
					iAP_SM += iRepDamage;
					iAP_L += iRepDamage;
				}
				if (cTargetType == hb::shared::owner_class::Player) {
					if (m_pGame->m_pClientList[sTargetH] == 0) return 0;
					if (m_pGame->m_pClientList[sTargetH]->m_iRating < 0) {
						iRepDamage = (abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 10);
						if (iRepDamage > 10) iRepDamage = 10;
						iAP_SM += iRepDamage;
						iAP_L += iRepDamage;
					}
				}
			}
		}

		sItemIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::Neck)];
		if ((sItemIndex != -1) && (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex] != 0)) {
			if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sItemIndex]->m_sIDnum == 859) { // NecklaceOfKloness  
				if (cTargetType == hb::shared::owner_class::Player) {
					if (m_pGame->m_pClientList[sTargetH] == 0) return 0;
					iRepDamage = (abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 20);
					if (iRepDamage > 5) iRepDamage = 5;
					iAP_SM += iRepDamage;
					iAP_L += iRepDamage;
				}
			}
		}

		cAttackerDir = m_pGame->m_pClientList[sAttackerH]->m_cDir;
		strcpy(cAttackerName, m_pGame->m_pClientList[sAttackerH]->m_cCharName);

		if (m_pGame->m_pClientList[sAttackerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] != 0)
			bIsAttackerBerserk = true;
		else bIsAttackerBerserk = false;

		if ((bArrowUse != true) && (m_pGame->m_pClientList[sAttackerH]->m_iSuperAttackLeft > 0) && (iAttackMode >= 20)) {

			dTmp1 = (double)iAP_SM;
			dTmp2 = (double)m_pGame->m_pClientList[sAttackerH]->m_iLevel;
			dTmp3 = dTmp2 / 100.0f;
			dTmp2 = dTmp1 * dTmp3;
			iTemp = (int)(dTmp2 + 0.5f);
			iAP_SM += iTemp;

			dTmp1 = (double)iAP_L;
			dTmp2 = (double)m_pGame->m_pClientList[sAttackerH]->m_iLevel;
			dTmp3 = dTmp2 / 100.0f;
			dTmp2 = dTmp1 * dTmp3;
			iTemp = (int)(dTmp2 + 0.5f);
			iAP_L += iTemp;

			switch (m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill) {
			case 5:  iAP_SM += (iAP_SM / 10); iAP_L += (iAP_L / 10); iAttackerHitRatio += 20; break; // Boxing
			case 6:  iAP_SM += (iAP_SM / 10); iAP_L += (iAP_L / 10); iAttackerHitRatio += 30; break; // Bow
			case 7:  iAP_SM += (iAP_SM / 10); iAP_L += (iAP_L / 10); iAttackerHitRatio += 40; break; // Dagger/SS
			case 8:  iAP_SM += (iAP_SM / 10); iAP_L += (iAP_L / 10); iAttackerHitRatio += 30; break; // Long Sword
			case 9:  iAP_SM += (iAP_SM / 7);  iAP_L += (iAP_L / 7);  iAttackerHitRatio += 30; break; // Fencing
			case 10: iAP_SM += (iAP_SM / 5);  iAP_L += (iAP_L / 5);                           break; // Axe
			case 14: iAP_SM += (iAP_SM / 5);  iAP_L += (iAP_L / 5);  iAttackerHitRatio += 20; break; // Hammer
			case 21: iAP_SM += (iAP_SM / 5);  iAP_L += (iAP_L / 5);  iAttackerHitRatio += 50; break; // Wand
			default: break;
			}
			iAttackerHitRatio += 100;
			iAttackerHitRatio += m_pGame->m_pClientList[sAttackerH]->m_iCustomItemValue_Attack;
		}

		if (bIsDash) {

			iAttackerHitRatio += 20;

			switch (m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill) {
			case 8:  iAP_SM += (iAP_SM / 10); iAP_L += (iAP_L / 10); break;
			case 10: iAP_SM += (iAP_SM / 5); iAP_L += (iAP_L / 5); break;
			case 14: iAP_SM += (iAP_SM / 5); iAP_L += (iAP_L / 5); break;
			default: break;
			}
		}

		iAttackerHP = m_pGame->m_pClientList[sAttackerH]->m_iHP;
		iAttackerHitRatio += m_pGame->m_pClientList[sAttackerH]->m_iAddAR;

		sAtkX = m_pGame->m_pClientList[sAttackerH]->m_sX;
		sAtkY = m_pGame->m_pClientList[sAttackerH]->m_sY;
		iPartyID = m_pGame->m_pClientList[sAttackerH]->m_iPartyID;
		break;

	case hb::shared::owner_class::Npc:

		if (m_pGame->m_pNpcList[sAttackerH] == 0) return 0;
		if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sAttackerH]->m_cMapIndex]->m_bIsAttackEnabled == false) return 0;

		if (m_pGame->m_pNpcList[sAttackerH]->m_status.bInvisibility) {
			m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sAttackerH, hb::shared::owner_class::Npc, false);
			m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sAttackerH, hb::shared::owner_class::Npc, hb::shared::magic::Invisibility);
			m_pGame->m_pNpcList[sAttackerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
		}

		cAttackerSide = m_pGame->m_pNpcList[sAttackerH]->m_cSide;
		iAP_SM = 0;
		iAP_L = 0;

		if (m_pGame->m_pNpcList[sAttackerH]->m_cAttackDiceThrow >= 0)
		{
			iAP_L = iAP_SM = m_pGame->iDice(
				static_cast<uint32_t>(m_pGame->m_pNpcList[sAttackerH]->m_cAttackDiceThrow),
				static_cast<uint32_t>(m_pGame->m_pNpcList[sAttackerH]->m_cAttackDiceRange)
			);
		}

		iAttackerHitRatio = m_pGame->m_pNpcList[sAttackerH]->m_iHitRatio;

		cAttackerDir = m_pGame->m_pNpcList[sAttackerH]->m_cDir;
		memcpy(cAttackerName, m_pGame->m_pNpcList[sAttackerH]->m_cNpcName, hb::shared::limits::NpcNameLen - 1);

		if (m_pGame->m_pNpcList[sAttackerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] != 0)
			bIsAttackerBerserk = true;
		else bIsAttackerBerserk = false;

		iAttackerHP = m_pGame->m_pNpcList[sAttackerH]->m_iHP;
		cAttackerSA = m_pGame->m_pNpcList[sAttackerH]->m_cSpecialAbility;

		sAtkX = m_pGame->m_pNpcList[sAttackerH]->m_sX;
		sAtkY = m_pGame->m_pNpcList[sAttackerH]->m_sY;
		break;
	}

	switch (cTargetType) {
	case hb::shared::owner_class::Player:

		if (m_pGame->m_pClientList[sTargetH] == 0) return 0;
		if (m_pGame->m_pClientList[sTargetH]->m_bIsKilled) return 0;

		// GM mode damage immunity
		if (m_pGame->m_pClientList[sTargetH]->m_bIsGMMode)
		{
			uint32_t dwNow = GameClock::GetTimeMS();
			if (dwNow - m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime > 2000)
			{
				m_pGame->m_pClientList[sTargetH]->m_dwLastGMImmuneNotifyTime = dwNow;
				m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::DamageImmune, 0, 0);
			}
			return 0;
		}

		if (m_pGame->m_pClientList[sTargetH]->m_status.bSlateInvincible) return 0;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_bIsCrusadeMode == false) &&
			(m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsPlayerCivil)) return 0;

		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sTargetH]->m_bIsNeutral) &&
			(m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsOwnLocation)) return 0;

		if ((m_pGame->m_pClientList[sTargetH]->m_sX != tdX) || (m_pGame->m_pClientList[sTargetH]->m_sY != tdY)) return 0;


		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_bIsNeutral)
			&& (m_pGame->m_pClientList[sTargetH]->m_iPKCount == 0)) return 0;

		if ((m_pGame->m_pClientList[sTargetH]->m_iPartyID != 0) && (iPartyID == m_pGame->m_pClientList[sTargetH]->m_iPartyID)) return 0;

		cTargetDir = m_pGame->m_pClientList[sTargetH]->m_cDir;
		iTargetDefenseRatio = m_pGame->m_pClientList[sTargetH]->m_iDefenseRatio;
		m_pGame->m_pClientList[sTargetH]->m_dwLogoutHackCheck = dwTime;
		if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH]->m_bIsSafeAttackMode)) {
			iSideCondition = iGetPlayerRelationship(sAttackerH, sTargetH);
			if ((iSideCondition == 7) || (iSideCondition == 2) || (iSideCondition == 6)) {
				iAP_SM = iAP_SM / 2;
				iAP_L = iAP_L / 2;
			}
			else {
				if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone) {
					if (m_pGame->m_pClientList[sAttackerH]->m_iGuildGUID == m_pGame->m_pClientList[sTargetH]->m_iGuildGUID) return 0;
					else {
						iAP_SM = iAP_SM / 2;
						iAP_L = iAP_L / 2;
					}
				}
				else return 0;
			}
		}

		iTargetDefenseRatio += m_pGame->m_pClientList[sTargetH]->m_iAddDR;


		sTgtX = m_pGame->m_pClientList[sTargetH]->m_sX;
		sTgtY = m_pGame->m_pClientList[sTargetH]->m_sY;
		break;

	case hb::shared::owner_class::Npc:

		if (m_pGame->m_pNpcList[sTargetH] == 0) return 0;
		if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0) return 0;

		if ((m_pGame->m_pNpcList[sTargetH]->m_sX != tdX) || (m_pGame->m_pNpcList[sTargetH]->m_sY != tdY)) return 0;

		cTargetDir = m_pGame->m_pNpcList[sTargetH]->m_cDir;
		iTargetDefenseRatio = m_pGame->m_pNpcList[sTargetH]->m_iDefenseRatio;

		if (cAttackerType == hb::shared::owner_class::Player) {
			switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
			case 40:
			case 41:
				if ((m_pGame->m_pClientList[sAttackerH]->m_cSide == 0) || (m_pGame->m_pNpcList[sTargetH]->m_cSide == m_pGame->m_pClientList[sAttackerH]->m_cSide)) return 0;
				break;
			}

			if ((wWeaponType == 25) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 5) && (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount > 0)) {
				if (m_pGame->m_pClientList[sAttackerH]->m_iCrusadeDuty != 2) break;

				switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
				case 36:
				case 37:
				case 38:
				case 39:
					switch (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount) {
					case 1:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 0;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
						case 36: iConstructionPoint = 700; iWarContribution = 700; break;
						case 37: iConstructionPoint = 700; iWarContribution = 700; break;
						case 38: iConstructionPoint = 500; iWarContribution = 500; break;
						case 39: iConstructionPoint = 500; iWarContribution = 500; break;
						}

						m_pGame->m_pClientList[sAttackerH]->m_iWarContribution += iWarContribution;
						if (m_pGame->m_pClientList[sAttackerH]->m_iWarContribution > m_pGame->m_iMaxWarContribution)
							m_pGame->m_pClientList[sAttackerH]->m_iWarContribution = m_pGame->m_iMaxWarContribution;
						std::snprintf(G_cTxt, sizeof(G_cTxt), "Construction Complete! WarContribution: +%d", iWarContribution);
						PutLogList(G_cTxt);
						m_pGame->SendNotifyMsg(0, sAttackerH, Notify::ConstructionPoint, m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint, m_pGame->m_pClientList[sAttackerH]->m_iWarContribution, 0, 0);
						break;
					case 5:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 1;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						break;
					case 10:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 2;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						break;
					}
					break;
				}

				m_pGame->m_pNpcList[sTargetH]->m_iBuildCount--;
				if (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount <= 0) {
					m_pGame->m_pNpcList[sTargetH]->m_iBuildCount = 0;
				}
				return 0;
			}
			if ((wWeaponType == 27) && (m_pGame->m_pNpcList[sTargetH]->m_cCropType != 0) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 5) && (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount > 0)) {
				cFarmingSkill = m_pGame->m_pClientList[sAttackerH]->m_cSkillMastery[2];
				cCropSkill = m_pGame->m_pNpcList[sTargetH]->m_cCropSkill;
				if (cFarmingSkill < 20) return 0;
				if (m_pGame->m_pClientList[sAttackerH]->m_iLevel < 20) return 0;
				switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
				case 64:
					switch (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount) {
					case 1:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 3;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						//sub_4B67E0
						m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sAttackerH, 2, cFarmingSkill <= cCropSkill + 10);
						m_pGame->m_pStatusEffectManager->_CheckFarmingAction(sAttackerH, sTargetH, 1);
						// Use EntityManager for NPC deletion
						if (m_pGame->m_pEntityManager != NULL)
							m_pGame->m_pEntityManager->DeleteEntity(sTargetH);
						return 0;
					case 8:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 3;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sAttackerH, 2, cFarmingSkill <= cCropSkill + 10);
						m_pGame->m_pStatusEffectManager->_CheckFarmingAction(sAttackerH, sTargetH, 0);
						break;
					case 18:
						m_pGame->m_pNpcList[sTargetH]->m_appearance.iSpecialFrame = 2;
						m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
						m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sAttackerH, 2, cFarmingSkill <= cCropSkill + 10);
						m_pGame->m_pStatusEffectManager->_CheckFarmingAction(sAttackerH, sTargetH, 0);
						break;

					}
					break;
				}
				m_pGame->m_pNpcList[sTargetH]->m_iBuildCount--;
				if (m_pGame->m_pNpcList[sTargetH]->m_iBuildCount <= 0) {
					m_pGame->m_pNpcList[sTargetH]->m_iBuildCount = 0;
				}
				return 0;
			}
		}

		sTgtX = m_pGame->m_pNpcList[sTargetH]->m_sX;
		sTgtY = m_pGame->m_pNpcList[sTargetH]->m_sY;
		break;
	}

	if ((cAttackerType == hb::shared::owner_class::Player) && (cTargetType == hb::shared::owner_class::Player)) {

		sX = m_pGame->m_pClientList[sAttackerH]->m_sX;
		sY = m_pGame->m_pClientList[sAttackerH]->m_sY;

		dX = m_pGame->m_pClientList[sTargetH]->m_sX;
		dY = m_pGame->m_pClientList[sTargetH]->m_sY;

		if (m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->iGetAttribute(sX, sY, 0x00000006) != 0) return 0;
		if (m_pGame->m_pMapList[m_pGame->m_pClientList[sTargetH]->m_cMapIndex]->iGetAttribute(dX, dY, 0x00000006) != 0) return 0;
	}

	if (cAttackerType == hb::shared::owner_class::Player) {
		if ((m_pGame->m_pClientList[sAttackerH]->m_iDex + m_pGame->m_pClientList[sAttackerH]->m_iAngelicDex) > 50) {
			iAttackerHitRatio += ((m_pGame->m_pClientList[sAttackerH]->m_iDex + m_pGame->m_pClientList[sAttackerH]->m_iAngelicDex) - 50);
		}
	}

	if (wWeaponType >= 40) {
		switch (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cWhetherStatus) {
		case 0:	break;
		case 1:	iAttackerHitRatio -= (iAttackerHitRatio / 20); break;
		case 2:	iAttackerHitRatio -= (iAttackerHitRatio / 10); break;
		case 3:	iAttackerHitRatio -= (iAttackerHitRatio / 4);  break;
		}
	}

	if (iAttackerHitRatio < 0)   iAttackerHitRatio = 0;
	switch (cTargetType) {
	case hb::shared::owner_class::Player:
		cProtect = m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect];
		break;

	case hb::shared::owner_class::Npc:
		cProtect = m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect];
		break;
	}

	if (cAttackerType == hb::shared::owner_class::Player) {
		if (m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1) {
			if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)]] == 0) {
				m_pGame->m_pClientList[sAttackerH]->m_bIsItemEquipped[m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)]] = false;
				m_pGame->DeleteClient(sAttackerH, true, true);
				return 0;
			}

			if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)]]->GetItemEffectType() == ItemEffectType::AttackArrow) {
				if (m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex == -1) {
					return 0;
				}
				else {
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex] == 0)
						return 0;

					if (bArrowUse != true)
						m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex]->m_dwCount--;
					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex]->m_dwCount <= 0) {

						m_pGame->m_pItemManager->ItemDepleteHandler(sAttackerH, m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex, false);
						m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex = m_pGame->m_pItemManager->_iGetArrowItemIndex(sAttackerH);
					}
					else {
						m_pGame->SendNotifyMsg(0, sAttackerH, Notify::SetItemCount, m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex, m_pGame->m_pClientList[sAttackerH]->m_pItemList[m_pGame->m_pClientList[sAttackerH]->m_cArrowIndex]->m_dwCount, false, 0);
						m_pGame->iCalcTotalWeight(sAttackerH);
					}
				}
				if (cProtect == 1) return 0;
			}
			else {
				switch (cProtect) {
				case 3: iTargetDefenseRatio += 40;  break;
				case 4: iTargetDefenseRatio += 100; break;
				}
				if (iTargetDefenseRatio < 0) iTargetDefenseRatio = 1;
			}
		}
	}
	else {
		switch (cProtect) {
		case 1:
			switch (m_pGame->m_pNpcList[sAttackerH]->m_sType) {
			case 54:
				if ((abs(sTgtX - m_pGame->m_pNpcList[sAttackerH]->m_sX) >= 1) || (abs(sTgtY - m_pGame->m_pNpcList[sAttackerH]->m_sY) >= 1)) return 0;
			}
			break;
		case 3: iTargetDefenseRatio += 40;  break;
		case 4: iTargetDefenseRatio += 100; break;
		}
		if (iTargetDefenseRatio < 0) iTargetDefenseRatio = 1;
	}

	if (cAttackerDir == cTargetDir) iTargetDefenseRatio = iTargetDefenseRatio / 2;
	if (iTargetDefenseRatio < 1)   iTargetDefenseRatio = 1;

	dTmp1 = (double)(iAttackerHitRatio);
	dTmp2 = (double)(iTargetDefenseRatio);
	dTmp3 = (dTmp1 / dTmp2) * 50.0f;
	iDestHitRatio = (int)(dTmp3);

	if (iDestHitRatio < m_pGame->m_iMinimumHitRatio) iDestHitRatio = m_pGame->m_iMinimumHitRatio;
	if (iDestHitRatio > m_pGame->m_iMaximumHitRatio) iDestHitRatio = m_pGame->m_iMaximumHitRatio;

	if ((bIsAttackerBerserk) && (iAttackMode < 20)) {
		iAP_SM = iAP_SM * 2;
		iAP_L = iAP_L * 2;
	}

	if (cAttackerType == hb::shared::owner_class::Player) {
		iAP_SM += m_pGame->m_pClientList[sAttackerH]->m_iAddPhysicalDamage;
		iAP_L += m_pGame->m_pClientList[sAttackerH]->m_iAddPhysicalDamage;
	}

	if (bNearAttack) {
		iAP_SM = iAP_SM / 2;
		iAP_L = iAP_L / 2;
	}

	if (cTargetType == hb::shared::owner_class::Player) {
		iAP_SM -= (m_pGame->iDice(1, m_pGame->m_pClientList[sTargetH]->m_iVit / 10) - 1);
		iAP_L -= (m_pGame->iDice(1, m_pGame->m_pClientList[sTargetH]->m_iVit / 10) - 1);
	}

	if (cAttackerType == hb::shared::owner_class::Player) {
		if (iAP_SM <= 1) iAP_SM = 1;
		if (iAP_L <= 1) iAP_L = 1;
	}
	else {
		if (iAP_SM <= 0) iAP_SM = 0;
		if (iAP_L <= 0) iAP_L = 0;
	}

	iResult = m_pGame->iDice(1, 100);

	if (iResult <= iDestHitRatio) {
		if (cAttackerType == hb::shared::owner_class::Player) {

			if (((m_pGame->m_pClientList[sAttackerH]->m_iHungerStatus <= 10) || (m_pGame->m_pClientList[sAttackerH]->m_iSP <= 0)) && (m_pGame->iDice(1, 10) == 5)) return false;
			m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount++;
			if (m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount < 0) m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount = 0;
			if (m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount > 4) m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount = 1;
			iWeaponSkill = _iGetWeaponSkillType(sAttackerH);
			iComboBonus = iGetComboAttackBonus(iWeaponSkill, m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount);

			if ((m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount > 1) && (m_pGame->m_pClientList[sAttackerH]->m_iAddCD != 0))
				iComboBonus += m_pGame->m_pClientList[sAttackerH]->m_iAddCD;

			iAP_SM += iComboBonus;
			iAP_L += iComboBonus;

			switch (m_pGame->m_pClientList[sAttackerH]->m_iSpecialWeaponEffectType) {
			case 0: break;
			case 1:
				if ((m_pGame->m_pClientList[sAttackerH]->m_iSuperAttackLeft > 0) && (iAttackMode >= 20)) {
					iAP_SM += m_pGame->m_pClientList[sAttackerH]->m_iSpecialWeaponEffectValue;
					iAP_L += m_pGame->m_pClientList[sAttackerH]->m_iSpecialWeaponEffectValue;
				}
				break;

			case 2:
				cAttackerSA = 61;
				iAttackerSAvalue = m_pGame->m_pClientList[sAttackerH]->m_iSpecialWeaponEffectValue * 5;
				break;

			case 3:
				cAttackerSA = 62;
				break;
			}

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone) {
				iAP_SM += iAP_SM / 3;
				iAP_L += iAP_L / 3;
			}

			if (m_pGame->m_pWarManager->bCheckHeldenianMap(sAttackerH, m_pGame->m_iBTFieldMapIndex, hb::shared::owner_class::Player) == 1) {
				iAP_SM += iAP_SM / 3;
				iAP_L += iAP_L / 3;
			}

			if ((cTargetType == hb::shared::owner_class::Player) && (m_pGame->m_bIsCrusadeMode) && (m_pGame->m_pClientList[sAttackerH]->m_iCrusadeDuty == 1)) {
				if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 80) {
					iAP_SM += iAP_SM;
					iAP_L += iAP_L;
				}
				else if (m_pGame->m_pClientList[sAttackerH]->m_iLevel <= 100) {
					iAP_SM += (iAP_SM * 7) / 10;
					iAP_L += (iAP_L * 7) / 10;
				}
				else {
					iAP_SM += iAP_SM / 3;
					iAP_L += iAP_L / 3;
				}
			}
		}

		switch (cTargetType) {
		case hb::shared::owner_class::Player:
			m_pGame->m_pSkillManager->ClearSkillUsingStatus(sTargetH);
			if ((dwTime - m_pGame->m_pClientList[sTargetH]->m_dwTime) > (uint32_t)m_pGame->m_iLagProtectionInterval) {
				return 0;
			}
			else {
				switch (cAttackerSA) {
				case 62:
					if (m_pGame->m_pClientList[sTargetH]->m_iRating < 0) {
						iTemp = abs(m_pGame->m_pClientList[sTargetH]->m_iRating) / 10;
						if (iTemp > 10) iTemp = 10;
						iAP_SM += iTemp;
					}
					break;
				}

				iAP_Abs_Armor = 0;
				iAP_Abs_Shield = 0;
				iTemp = m_pGame->iDice(1, 10000);
				if ((iTemp >= 1) && (iTemp < 5000))           iHitPoint = 1;
				else if ((iTemp >= 5000) && (iTemp < 7500))   iHitPoint = 2;
				else if ((iTemp >= 7500) && (iTemp < 9000))   iHitPoint = 3;
				else if ((iTemp >= 9000) && (iTemp <= 10000)) iHitPoint = 4;

				switch (iHitPoint) {
				case 1:
					if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Body)] > 0) {
						if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Body)] >= 80)
							dTmp1 = 80.0f;
						else dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Body)];
						dTmp2 = (double)iAP_SM;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;
						iAP_Abs_Armor = (int)dTmp3;
					}
					break;
				case 2:
					if ((m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Pants)] +
						m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Leggings)]) > 0) {
						if ((m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Pants)] +
							m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Leggings)]) >= 80)
							dTmp1 = 80.0f;
						else dTmp1 = (double)(m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Pants)] + m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Leggings)]);
						dTmp2 = (double)iAP_SM;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;

						iAP_Abs_Armor = (int)dTmp3;
					}
					break;

				case 3:
					if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Arms)] > 0) {
						if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Arms)] >= 80)
							dTmp1 = 80.0f;
						else dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Arms)];
						dTmp2 = (double)iAP_SM;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;

						iAP_Abs_Armor = (int)dTmp3;
					}
					break;

				case 4:
					if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Head)] > 0) {
						if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Head)] >= 80)
							dTmp1 = 80.0f;
						else dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Head)];
						dTmp2 = (double)iAP_SM;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;

						iAP_Abs_Armor = (int)dTmp3;
					}
					break;
				}

				if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Shield > 0) {
					if (m_pGame->iDice(1, 100) <= (m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[11])) {
						m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sTargetH, 11, 1);
						if (m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Shield >= 80)
							dTmp1 = 80.0f;
						else dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iDamageAbsorption_Shield;
						dTmp2 = (double)iAP_SM;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;

						iAP_Abs_Shield = (int)dTmp3;

						iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::LeftHand)];
						if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
							if ((m_pGame->m_pClientList[sTargetH]->m_cSide != 0) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan > 0)) {
								m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan--;
								m_pGame->SendNotifyMsg(0, sTargetH, Notify::CurLifeSpan, iTemp, m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan, 0, 0);
							}
							if (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp]->m_wCurLifeSpan == 0) {
								m_pGame->SendNotifyMsg(0, sTargetH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp]->m_cEquipPos, iTemp, 0, 0);
								m_pGame->m_pItemManager->ReleaseItemHandler(sTargetH, iTemp, true);
							}
						}
					}
				}

				iAP_SM = iAP_SM - (iAP_Abs_Armor + iAP_Abs_Shield);
				if (iAP_SM <= 0) iAP_SM = 1;

				if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] != 0) && (m_pGame->m_pClientList[sAttackerH]->m_bIsSpecialAbilityEnabled)) {
					switch (m_pGame->m_pClientList[sAttackerH]->m_iSpecialAbilityType) {
					case 0: break;
					case 1:
						iTemp = (m_pGame->m_pClientList[sTargetH]->m_iHP / 2);
						if (iTemp > iAP_SM) iAP_SM = iTemp;
						if (iAP_SM <= 0) iAP_SM = 1;
						break;
					case 2:
						if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
							m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
							m_pGame->m_pStatusEffectManager->SetIceFlag(sTargetH, hb::shared::owner_class::Player, true);
							m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + 30000,
								sTargetH, hb::shared::owner_class::Player, 0, 0, 0, 1, 0, 0);
							m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
						}
						break;
					case 3:
						if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] == 0) {
							m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 2;
							m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::HoldObject, dwTime + 10000,
								sTargetH, hb::shared::owner_class::Player, 0, 0, 0, 10, 0, 0);
							m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOn, hb::shared::magic::HoldObject, 10, 0, 0);
						}
						break;
					case 4:
						iAP_SM = (m_pGame->m_pClientList[sTargetH]->m_iHP);
						break;
					case 5:
						m_pGame->m_pClientList[sAttackerH]->m_iHP += iAP_SM;
						if (m_pGame->iGetMaxHP(sAttackerH) < m_pGame->m_pClientList[sAttackerH]->m_iHP) m_pGame->m_pClientList[sAttackerH]->m_iHP = m_pGame->iGetMaxHP(sAttackerH);
						m_pGame->SendNotifyMsg(0, sAttackerH, Notify::Hp, 0, 0, 0, 0);
						break;
					}
				}

				if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] != 0) && (m_pGame->m_pClientList[sTargetH]->m_bIsSpecialAbilityEnabled)) {
					switch (m_pGame->m_pClientList[sTargetH]->m_iSpecialAbilityType) {
					case 50:
						if (m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1)
							sWeaponIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
						else sWeaponIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
						if (sWeaponIndex != -1)	m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan = 0;
						break;
					case 51:
						if (iHitPoint == m_pGame->m_pClientList[sTargetH]->m_iSpecialAbilityEquipPos)
							iAP_SM = 0;
						break;
					case 52:
						iAP_SM = 0;
						break;
					}
				}

				if ((m_pGame->m_pClientList[sTargetH]->m_bIsLuckyEffect) &&
					(m_pGame->iDice(1, 10) == 5) && (m_pGame->m_pClientList[sTargetH]->m_iHP <= iAP_SM)) {
					iAP_SM = m_pGame->m_pClientList[sTargetH]->m_iHP - 1;
				}

				switch (iHitPoint) {
				case 1:
					iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Body)];
					if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
						bCalculateEnduranceDecrement(sTargetH, sAttackerH, cTargetType, iTemp);
					}
					break;

				case 2:
					iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Pants)];
					if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
						bCalculateEnduranceDecrement(sTargetH, sAttackerH, cTargetType, iTemp);
					}
					else {
						iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Leggings)];
						if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
							bCalculateEnduranceDecrement(sTargetH, sAttackerH, cTargetType, iTemp);
						}
					}
					break;

				case 3:
					iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Arms)];
					if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
						bCalculateEnduranceDecrement(sTargetH, sAttackerH, cTargetType, iTemp);
					}
					break;

				case 4:
					iTemp = m_pGame->m_pClientList[sTargetH]->m_sItemEquipmentStatus[ToInt(EquipPos::Head)];
					if ((iTemp != -1) && (m_pGame->m_pClientList[sTargetH]->m_pItemList[iTemp] != 0)) {
						bCalculateEnduranceDecrement(sTargetH, sAttackerH, cTargetType, iTemp);
					}
					break;
				}

				if ((cAttackerSA == 2) && (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] != 0)) {
					m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOff, hb::shared::magic::Protect, m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect], 0, 0);
					switch (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect]) {
					case 1:
						m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(sTargetH, hb::shared::owner_class::Player, false);
						break;
					case 2:
					case 5:
						m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(sTargetH, hb::shared::owner_class::Player, false);
						break;
					case 3:
					case 4:
						m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(sTargetH, hb::shared::owner_class::Player, false);
						break;
					}
					m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Player, hb::shared::magic::Protect);
				}

				if ((m_pGame->m_pClientList[sTargetH]->m_bIsPoisoned == false) &&
					((cAttackerSA == 5) || (cAttackerSA == 6) || (cAttackerSA == 61))) {
					if (bCheckResistingPoisonSuccess(sTargetH, hb::shared::owner_class::Player) == false) {
						m_pGame->m_pClientList[sTargetH]->m_bIsPoisoned = true;
						if (cAttackerSA == 5)		m_pGame->m_pClientList[sTargetH]->m_iPoisonLevel = 15;
						else if (cAttackerSA == 6)  m_pGame->m_pClientList[sTargetH]->m_iPoisonLevel = 40;
						else if (cAttackerSA == 61) m_pGame->m_pClientList[sTargetH]->m_iPoisonLevel = iAttackerSAvalue;

						m_pGame->m_pClientList[sTargetH]->m_dwPoisonTime = dwTime;
						m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOn, hb::shared::magic::Poison, m_pGame->m_pClientList[sTargetH]->m_iPoisonLevel, 0, 0);
						m_pGame->m_pStatusEffectManager->SetPoisonFlag(sTargetH, hb::shared::owner_class::Player, true);
					}
				}

				m_pGame->m_pClientList[sTargetH]->m_iHP -= iAP_SM;
				// Interrupt spell casting on damage
				if (iAP_SM > 0) {
					m_pGame->m_pClientList[sTargetH]->m_dwLastDamageTakenTime = GameClock::GetTimeMS();
					if (m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime) {
						m_pGame->m_pClientList[sTargetH]->m_bMagicPauseTime = false;
						m_pGame->m_pClientList[sTargetH]->m_iSpellCount = -1;
						m_pGame->SendNotifyMsg(0, sTargetH, Notify::SpellInterrupted, 0, 0, 0, 0);
					}
				}
				if (m_pGame->m_pClientList[sTargetH]->m_iHP <= 0) {
					if (cAttackerType == hb::shared::owner_class::Player)
						bAnalyzeCriminalAction(sAttackerH, m_pGame->m_pClientList[sTargetH]->m_sX, m_pGame->m_pClientList[sTargetH]->m_sY);
					ClientKilledHandler(sTargetH, sAttackerH, cAttackerType, iAP_SM);
					bKilled = true;
					iKilledDice = m_pGame->m_pClientList[sTargetH]->m_iLevel;
				}
				else {
					if (iAP_SM > 0) {
						if (m_pGame->m_pClientList[sTargetH]->m_iAddTransMana > 0) {
							dTmp1 = (double)m_pGame->m_pClientList[sTargetH]->m_iAddTransMana;
							dTmp2 = (double)iAP_SM;
							dTmp3 = (dTmp1 / 100.0f) * dTmp2;
							iTemp = m_pGame->iGetMaxMP(sTargetH);
							m_pGame->m_pClientList[sTargetH]->m_iMP += (int)dTmp3;
							if (m_pGame->m_pClientList[sTargetH]->m_iMP > iTemp) m_pGame->m_pClientList[sTargetH]->m_iMP = iTemp;
						}
						if (m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical > 0) {
							if (m_pGame->iDice(1, 100) <= static_cast<uint32_t>(m_pGame->m_pClientList[sTargetH]->m_iAddChargeCritical)) {
								iMaxSuperAttack = (m_pGame->m_pClientList[sTargetH]->m_iLevel / 10);
								if (m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft < iMaxSuperAttack) m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft++;
								m_pGame->SendNotifyMsg(0, sTargetH, Notify::SuperAttackLeft, 0, 0, 0, 0);
							}
						}

						m_pGame->SendNotifyMsg(0, sTargetH, Notify::Hp, 0, 0, 0, 0);

						if (cAttackerType == hb::shared::owner_class::Player)
							sAttackerWeapon = m_pGame->m_pClientList[sAttackerH]->m_appearance.iWeaponType;
						else sAttackerWeapon = 1;

						if ((cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_bIsFightZone))
							iMoveDamage = 60;
						else iMoveDamage = 40;

						if (iAP_SM >= iMoveDamage) {
							if (sTgtX == sAtkX) {
								if (sTgtY == sAtkY)     goto CAE_SKIPDAMAGEMOVE;
								else if (sTgtY > sAtkY) cDamageMoveDir = 5;
								else if (sTgtY < sAtkY) cDamageMoveDir = 1;
							}
							else if (sTgtX > sAtkX) {
								if (sTgtY == sAtkY)     cDamageMoveDir = 3;
								else if (sTgtY > sAtkY) cDamageMoveDir = 4;
								else if (sTgtY < sAtkY) cDamageMoveDir = 2;
							}
							else if (sTgtX < sAtkX) {
								if (sTgtY == sAtkY)     cDamageMoveDir = 7;
								else if (sTgtY > sAtkY) cDamageMoveDir = 6;
								else if (sTgtY < sAtkY) cDamageMoveDir = 8;
							}
							m_pGame->m_pClientList[sTargetH]->m_iLastDamage = iAP_SM;

							m_pGame->SendNotifyMsg(0, sTargetH, Notify::DamageMove, cDamageMoveDir, iAP_SM, sAttackerWeapon, 0);
						}
						else {
						CAE_SKIPDAMAGEMOVE:
							int iProb;
							if (cAttackerType == hb::shared::owner_class::Player) {
								switch (m_pGame->m_pClientList[sAttackerH]->m_sUsingWeaponSkill) {
								case 6: iProb = 3500; break;
								case 8: iProb = 1000; break;
								case 9: iProb = 2900; break;
								case 10: iProb = 2500; break;
								case 14: iProb = 2000; break;
								case 21: iProb = 2000; break;
								default: iProb = 1; break;
								}
							}
							else iProb = 1;

							if (m_pGame->iDice(1, 10000) >= static_cast<uint32_t>(iProb))
								m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, iAP_SM, sAttackerWeapon, 0);
						}

						if (m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] == 1) {
							m_pGame->SendNotifyMsg(0, sTargetH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);
							m_pGame->m_pClientList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
							m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
						}

						m_pGame->m_pClientList[sTargetH]->m_iSuperAttackCount++;
						if (m_pGame->m_pClientList[sTargetH]->m_iSuperAttackCount > 14) {
							m_pGame->m_pClientList[sTargetH]->m_iSuperAttackCount = 0;
							iMaxSuperAttack = (m_pGame->m_pClientList[sTargetH]->m_iLevel / 10);
							if (m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft < iMaxSuperAttack) m_pGame->m_pClientList[sTargetH]->m_iSuperAttackLeft++;
							m_pGame->SendNotifyMsg(0, sTargetH, Notify::SuperAttackLeft, 0, 0, 0, 0);
						}
					}
				}
			}
			break;

		case hb::shared::owner_class::Npc:
			if (m_pGame->m_pNpcList[sTargetH]->m_cBehavior == Behavior::Dead) return 0;
			if (m_pGame->m_pNpcList[sTargetH]->m_bIsKilled) return 0;
			if (m_pGame->m_bIsCrusadeMode) {
				if (cAttackerSide == m_pGame->m_pNpcList[sTargetH]->m_cSide) {
					switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
					case 40:
					case 41:
					case 43:
					case 44:
					case 45:
					case 46:
					case 47:
					case 51:
						return 0;

					default: break;
					}
				}
				else {
					switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
					case 41:
						if (cAttackerSide != 0) {
							m_pGame->m_pNpcList[sTargetH]->m_iV1 += iAP_L;
							if (m_pGame->m_pNpcList[sTargetH]->m_iV1 > 500) {
								m_pGame->m_pNpcList[sTargetH]->m_iV1 = 0;
								m_pGame->m_pNpcList[sTargetH]->m_iManaStock--;
								if (m_pGame->m_pNpcList[sTargetH]->m_iManaStock <= 0) m_pGame->m_pNpcList[sTargetH]->m_iManaStock = 0;
								std::snprintf(G_cTxt, sizeof(G_cTxt), "ManaStock down: %d", m_pGame->m_pNpcList[sTargetH]->m_iManaStock);
								PutLogList(G_cTxt);
							}
						}
						break;
					}
				}
			}
			switch (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit) {
			case 1:
			case 2:
				return 0;
			}

			if (m_pGame->m_pNpcList[sTargetH]->m_cSize == 0)
				iDamage = iAP_SM;
			else iDamage = iAP_L;

			if (m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage < 0) {
				dTmp1 = (double)iDamage;
				dTmp2 = (double)(abs(m_pGame->m_pNpcList[sTargetH]->m_iAbsDamage)) / 100.0f;
				dTmp3 = dTmp1 * dTmp2;
				dTmp2 = dTmp1 - dTmp3;
				iDamage = (int)dTmp2;
				if (iDamage < 0) iDamage = 1;
				else if ((m_pGame->m_pNpcList[sTargetH]->m_sType == 31) && (cAttackerType == 1) && (m_pGame->m_pClientList[sAttackerH] != 0) && (m_pGame->m_pClientList[sAttackerH]->m_iSpecialAbilityType == 7))
					iDamage += m_pGame->iDice(3, 2);
			}


			if ((cAttackerSA == 2) && (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] != 0)) {
				switch (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect]) {
				case 1:
					m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(sTargetH, hb::shared::owner_class::Npc, false);
					break;
				case 2:
				case 5:
					m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(sTargetH, hb::shared::owner_class::Npc, false);
					break;
				case 3:
				case 4:
					m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(sTargetH, hb::shared::owner_class::Npc, false);
					break;
				}
				m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::Protect] = 0;
				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Npc, hb::shared::magic::Protect);
			}

			switch (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit) {
			case 0:
			case 3:
			case 5:
				m_pGame->m_pNpcList[sTargetH]->m_iHP -= iDamage;
				break;
			}

			if (m_pGame->m_pNpcList[sTargetH]->m_iHP <= 0) {
				m_pGame->m_pEntityManager->OnEntityKilled(sTargetH, sAttackerH, cAttackerType, iDamage);
				bKilled = true;
				iKilledDice = m_pGame->m_pNpcList[sTargetH]->m_iHitDice;
			}
			else {
				if ((m_pGame->m_pNpcList[sTargetH]->m_sType != 21) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 55) && (m_pGame->m_pNpcList[sTargetH]->m_sType != 56)
					&& (m_pGame->m_pNpcList[sTargetH]->m_cSide == cAttackerSide)) goto CAE_SKIPCOUNTERATTACK;

				if (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit != 0) goto CAE_SKIPCOUNTERATTACK;
				if (m_pGame->m_pNpcList[sTargetH]->m_bIsPermAttackMode) goto CAE_SKIPCOUNTERATTACK;
				if ((m_pGame->m_pNpcList[sTargetH]->m_bIsSummoned) && (m_pGame->m_pNpcList[sTargetH]->m_iSummonControlMode == 1)) goto CAE_SKIPCOUNTERATTACK;
				if (m_pGame->m_pNpcList[sTargetH]->m_sType == 51) goto CAE_SKIPCOUNTERATTACK;

				if (m_pGame->iDice(1, 3) == 2) {
					if (m_pGame->m_pNpcList[sTargetH]->m_cBehavior == Behavior::Attack) {
						tX = tY = 0;
						switch (m_pGame->m_pNpcList[sTargetH]->m_cTargetType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex] != 0) {
								tX = m_pGame->m_pClientList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex]->m_sX;
								tY = m_pGame->m_pClientList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex]->m_sY;
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex] != 0) {
								tX = m_pGame->m_pNpcList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex]->m_sX;
								tY = m_pGame->m_pNpcList[m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex]->m_sY;
							}
							break;
						}

						iDst1 = (m_pGame->m_pNpcList[sTargetH]->m_sX - tX) * (m_pGame->m_pNpcList[sTargetH]->m_sX - tX) + (m_pGame->m_pNpcList[sTargetH]->m_sY - tY) * (m_pGame->m_pNpcList[sTargetH]->m_sY - tY);

						tX = tY = 0;
						switch (cAttackerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sAttackerH] != 0) {
								tX = m_pGame->m_pClientList[sAttackerH]->m_sX;
								tY = m_pGame->m_pClientList[sAttackerH]->m_sY;
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sAttackerH] != 0) {
								tX = m_pGame->m_pNpcList[sAttackerH]->m_sX;
								tY = m_pGame->m_pNpcList[sAttackerH]->m_sY;
							}
							break;
						}

						iDst2 = (m_pGame->m_pNpcList[sTargetH]->m_sX - tX) * (m_pGame->m_pNpcList[sTargetH]->m_sX - tX) + (m_pGame->m_pNpcList[sTargetH]->m_sY - tY) * (m_pGame->m_pNpcList[sTargetH]->m_sY - tY);

						if (iDst2 <= iDst1) {
							m_pGame->m_pNpcList[sTargetH]->m_cBehavior = Behavior::Attack;
							m_pGame->m_pNpcList[sTargetH]->m_sBehaviorTurnCount = 0;
							m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex = sAttackerH;
							m_pGame->m_pNpcList[sTargetH]->m_cTargetType = cAttackerType;
						}
					}
					else {
						m_pGame->m_pNpcList[sTargetH]->m_cBehavior = Behavior::Attack;
						m_pGame->m_pNpcList[sTargetH]->m_sBehaviorTurnCount = 0;
						m_pGame->m_pNpcList[sTargetH]->m_iTargetIndex = sAttackerH;
						m_pGame->m_pNpcList[sTargetH]->m_cTargetType = cAttackerType;
					}
				}

			CAE_SKIPCOUNTERATTACK:

				if ((m_pGame->iDice(1, 3) == 2) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 0))
					m_pGame->m_pNpcList[sTargetH]->m_dwTime = dwTime;

				if (cAttackerType == hb::shared::owner_class::Player)
					sAttackerWeapon = m_pGame->m_pClientList[sAttackerH]->m_appearance.iWeaponType;
				else sAttackerWeapon = 1;

				if ((wWeaponType < 40) && (m_pGame->m_pNpcList[sTargetH]->m_cActionLimit == 4)) {
					if (sTgtX == sAtkX) {
						if (sTgtY == sAtkY)     goto CAE_SKIPDAMAGEMOVE2;
						else if (sTgtY > sAtkY) cDamageMoveDir = 5;
						else if (sTgtY < sAtkY) cDamageMoveDir = 1;
					}
					else if (sTgtX > sAtkX) {
						if (sTgtY == sAtkY)     cDamageMoveDir = 3;
						else if (sTgtY > sAtkY) cDamageMoveDir = 4;
						else if (sTgtY < sAtkY) cDamageMoveDir = 2;
					}
					else if (sTgtX < sAtkX) {
						if (sTgtY == sAtkY)     cDamageMoveDir = 7;
						else if (sTgtY > sAtkY) cDamageMoveDir = 6;
						else if (sTgtY < sAtkY) cDamageMoveDir = 8;
					}

					dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
					dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

					if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) {
						cDamageMoveDir = static_cast<char>(m_pGame->iDice(1, 8));
						dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
						dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

						if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) goto CAE_SKIPDAMAGEMOVE2;
					}

					m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->ClearOwner(5, sTargetH, hb::shared::owner_class::Npc, m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY);
					m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Npc, dX, dY);
					m_pGame->m_pNpcList[sTargetH]->m_sX = dX;
					m_pGame->m_pNpcList[sTargetH]->m_sY = dY;
					m_pGame->m_pNpcList[sTargetH]->m_cDir = cDamageMoveDir;

					m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Move, 0, 0, 0);

					dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
					dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];

					if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) {
						cDamageMoveDir = static_cast<char>(m_pGame->iDice(1, 8));
						dX = m_pGame->m_pNpcList[sTargetH]->m_sX + _tmp_cTmpDirX[cDamageMoveDir];
						dY = m_pGame->m_pNpcList[sTargetH]->m_sY + _tmp_cTmpDirY[cDamageMoveDir];
						if (m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bGetMoveable(dX, dY, 0) == false) goto CAE_SKIPDAMAGEMOVE2;
					}

					m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->ClearOwner(5, sTargetH, hb::shared::owner_class::Npc, m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY);
					m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->SetOwner(sTargetH, hb::shared::owner_class::Npc, dX, dY);
					m_pGame->m_pNpcList[sTargetH]->m_sX = dX;
					m_pGame->m_pNpcList[sTargetH]->m_sY = dY;
					m_pGame->m_pNpcList[sTargetH]->m_cDir = cDamageMoveDir;

					m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Move, 0, 0, 0);

					if (m_pGame->m_pWarManager->bCheckEnergySphereDestination(sTargetH, sAttackerH, cAttackerType)) {
						if (cAttackerType == hb::shared::owner_class::Player) {
							iExp = (m_pGame->m_pNpcList[sTargetH]->m_iExp / 3);
							if (m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > 0)
								iExp += m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp;

							if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp != 0) {
								dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
								dTmp2 = (double)iExp;
								dTmp3 = (dTmp1 / 100.0f) * dTmp2;
								iExp += (int)dTmp3;
							}

							if ((m_pGame->m_bIsCrusadeMode) && (iExp > 10)) iExp = 10;

							m_pGame->GetExp(sAttackerH, iExp);

							// Use EntityManager for NPC deletion
							if (m_pGame->m_pEntityManager != NULL)
								m_pGame->m_pEntityManager->DeleteEntity(sTargetH);
							return false;
						}
					}

				CAE_SKIPDAMAGEMOVE2:;
				}
				else {
					m_pGame->SendEventToNearClient_TypeA(sTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, sAttackerWeapon, 0);
				}

				if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] == 1) {
					m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
					m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Npc, hb::shared::magic::HoldObject);
				}
				else if (m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] == 2) {
					if ((m_pGame->m_pNpcList[sTargetH]->m_iHitDice > 50) && (m_pGame->iDice(1, 10) == 5)) {
						m_pGame->m_pNpcList[sTargetH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
						m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sTargetH, hb::shared::owner_class::Npc, hb::shared::magic::HoldObject);
					}
				}

				if ((m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > 0) && (m_pGame->m_pNpcList[sTargetH]->m_bIsSummoned != true) &&
					(cAttackerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sAttackerH] != 0)) {
					if (m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp > static_cast<uint32_t>(iDamage)) {
						iExp = iDamage;
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp -= iDamage;
					}
					else {
						iExp = m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp;
						m_pGame->m_pNpcList[sTargetH]->m_iNoDieRemainExp = 0;
					}

					if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp != 0) {
						dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
						dTmp2 = (double)iExp;
						dTmp3 = (dTmp1 / 100.0f) * dTmp2;
						iExp += (int)dTmp3;
					}

					if (m_pGame->m_bIsCrusadeMode) iExp = iExp / 3;

					if (m_pGame->m_pClientList[sAttackerH]->m_iLevel > 100) {
						switch (m_pGame->m_pNpcList[sTargetH]->m_sType) {
						case 55:
						case 56:
							iExp = 0;
							break;
						default: break;
						}
					}
				}
			}
			break;
		}

		if (cAttackerType == hb::shared::owner_class::Player) {
			if (m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1)
				sWeaponIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
			else sWeaponIndex = m_pGame->m_pClientList[sAttackerH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];

			if ((sWeaponIndex != -1) && (bArrowUse != true)) {
				if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex] != 0) &&
					(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_sIDnum != 231)) {
					if (bKilled == false)
						m_pGame->m_pItemManager->CalculateSSN_ItemIndex(sAttackerH, sWeaponIndex, 1);
					else {
						if (m_pGame->m_pClientList[sAttackerH]->m_iHP <= 3)
							m_pGame->m_pItemManager->CalculateSSN_ItemIndex(sAttackerH, sWeaponIndex, m_pGame->iDice(1, iKilledDice) * 2);
						else m_pGame->m_pItemManager->CalculateSSN_ItemIndex(sAttackerH, sWeaponIndex, m_pGame->iDice(1, iKilledDice));
					}
				}

				if ((m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex] != 0) &&
					(m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wMaxLifeSpan != 0)) {
					iWepLifeOff = 1;
					if ((wWeaponType >= 1) && (wWeaponType < 40)) {
						switch (m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->m_cWhetherStatus) {
						case 0:	break;
						case 1:	if (m_pGame->iDice(1, 3) == 1) iWepLifeOff++; break;
						case 2:	if (m_pGame->iDice(1, 2) == 1) iWepLifeOff += m_pGame->iDice(1, 2); break;
						case 3:	if (m_pGame->iDice(1, 2) == 1) iWepLifeOff += m_pGame->iDice(1, 3); break;
						}
					}

					if (m_pGame->m_pClientList[sAttackerH]->m_cSide != 0) {
						if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan < iWepLifeOff)
							m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan = 0;
						else m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan -= iWepLifeOff;

						m_pGame->SendNotifyMsg(0, sAttackerH, Notify::CurLifeSpan, sWeaponIndex, m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan, 0, 0);
					}

					if (m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_wCurLifeSpan == 0) {
						m_pGame->SendNotifyMsg(0, sAttackerH, Notify::ItemLifeSpanEnd, m_pGame->m_pClientList[sAttackerH]->m_pItemList[sWeaponIndex]->m_cEquipPos, sWeaponIndex, 0, 0);
						m_pGame->m_pItemManager->ReleaseItemHandler(sAttackerH, sWeaponIndex, true);
					}
				}
			}
			else {
				if (wWeaponType == 0) {
					m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(sAttackerH, 5, 1);
				}
			}
		}
	}
	else {
		if (cAttackerType == hb::shared::owner_class::Player) {
			m_pGame->m_pClientList[sAttackerH]->m_iComboAttackCount = 0;
		}
	}

	return iExp;
}

