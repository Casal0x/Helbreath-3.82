// MiningManager.cpp: Implementation of MiningManager.
// Mineral spawning, mining actions, and mineral lifecycle.
// Extracted from CGame (Phase B1).

#include "MiningManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "SkillManager.h"
#include "ItemManager.h"
#include "DelayEventManager.h"
#include "DynamicObjectManager.h"
#include "Packet/SharedPackets.h"

using namespace hb::shared::net;
using namespace hb::server::config;
namespace dynamic_object = hb::shared::dynamic_object;

MiningManager::MiningManager()
{
	InitArrays();
}

MiningManager::~MiningManager()
{
	CleanupArrays();
}

void MiningManager::InitArrays()
{
	for (int i = 0; i < MaxMinerals; i++)
		m_pMineral[i] = 0;
}

void MiningManager::CleanupArrays()
{
	for (int i = 0; i < MaxMinerals; i++)
		if (m_pMineral[i] != 0) {
			delete m_pMineral[i];
			m_pMineral[i] = 0;
		}
}

void MiningManager::MineralGenerator()
{
	int iP, tX, tY, iRet;

	for(int i = 0; i < MaxMaps; i++) {
		if ((m_pGame->iDice(1, 4) == 1) && (m_pGame->m_pMapList[i] != 0) &&
			(m_pGame->m_pMapList[i]->m_bMineralGenerator) &&
			(m_pGame->m_pMapList[i]->m_iCurMineral < m_pGame->m_pMapList[i]->m_iMaxMineral)) {

			iP = m_pGame->iDice(1, m_pGame->m_pMapList[i]->m_iTotalMineralPoint) - 1;
			if ((m_pGame->m_pMapList[i]->m_MineralPointList[iP].x == -1) || (m_pGame->m_pMapList[i]->m_MineralPointList[iP].y == -1)) break;

			tX = m_pGame->m_pMapList[i]->m_MineralPointList[iP].x;
			tY = m_pGame->m_pMapList[i]->m_MineralPointList[iP].y;

			iRet = iCreateMineral(i, tX, tY, m_pGame->m_pMapList[i]->m_cMineralGeneratorLevel);
		}
	}
}

int MiningManager::iCreateMineral(char cMapIndex, int tX, int tY, char cLevel)
{
	int iDynamicHandle, iMineralType;

	if ((cMapIndex < 0) || (cMapIndex >= MaxMaps)) return 0;
	if (m_pGame->m_pMapList[cMapIndex] == 0) return 0;

	for(int i = 1; i < MaxMinerals; i++)
		if (m_pMineral[i] == 0) {
			iMineralType = m_pGame->iDice(1, cLevel);
			m_pMineral[i] = new class CMineral(iMineralType, cMapIndex, tX, tY, 1);
			if (m_pMineral[i] == 0) return 0;

			iDynamicHandle = 0;
			switch (iMineralType) {
			case 1:
			case 2:
			case 3:
			case 4:
				iDynamicHandle = m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(0, 0, dynamic_object::Mineral1, cMapIndex, tX, tY, 0, i);
				break;

			case 5:
			case 6:
				iDynamicHandle = m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(0, 0, dynamic_object::Mineral2, cMapIndex, tX, tY, 0, i);
				break;

			default:
				iDynamicHandle = m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(0, 0, dynamic_object::Mineral1, cMapIndex, tX, tY, 0, i);
				break;
			}

			if (iDynamicHandle == 0) {
				delete m_pMineral[i];
				m_pMineral[i] = 0;
				return 0;
			}
			m_pMineral[i]->m_sDynamicObjectHandle = iDynamicHandle;
			m_pMineral[i]->m_cMapIndex = cMapIndex;

			switch (iMineralType) {
			case 1: m_pMineral[i]->m_iDifficulty = 10; m_pMineral[i]->m_iRemain = 20; break;
			case 2: m_pMineral[i]->m_iDifficulty = 15; m_pMineral[i]->m_iRemain = 15; break;
			case 3: m_pMineral[i]->m_iDifficulty = 20; m_pMineral[i]->m_iRemain = 10; break;
			case 4: m_pMineral[i]->m_iDifficulty = 50; m_pMineral[i]->m_iRemain = 8; break;
			case 5: m_pMineral[i]->m_iDifficulty = 70; m_pMineral[i]->m_iRemain = 6; break;
			case 6: m_pMineral[i]->m_iDifficulty = 90; m_pMineral[i]->m_iRemain = 4; break;
			default: m_pMineral[i]->m_iDifficulty = 10; m_pMineral[i]->m_iRemain = 20; break;
			}

			m_pGame->m_pMapList[cMapIndex]->m_iCurMineral++;

			return i;
		}

	return 0;
}


void MiningManager::_CheckMiningAction(int iClientH, int dX, int dY)
{
	short sType;
	uint32_t dwRegisterTime;
	int   iDynamicIndex, iSkillLevel, iResult, iItemID;
	CItem* pItem;
	uint16_t  wWeaponType;

	iItemID = 0;

	if (m_pGame->m_pClientList[iClientH] == 0)  return;

	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetDynamicObject(dX, dY, &sType, &dwRegisterTime, &iDynamicIndex);

	if (m_pGame->m_pClientList[iClientH]->m_status.bInvisibility) {
		m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(iClientH, hb::shared::owner_class::Player, false);
		m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(iClientH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
		m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
	}

	switch (sType) {
	case dynamic_object::Mineral1:
	case dynamic_object::Mineral2:
		wWeaponType = m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType;
		if (wWeaponType == 25) {
		}
		else return;

		if (!m_pGame->m_pClientList[iClientH]->m_appearance.bIsWalking) return;

		iSkillLevel = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[0];
		if (iSkillLevel == 0) break;

		if (m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex] == 0) break;
		iSkillLevel -= m_pMineral[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_iV1]->m_iDifficulty;
		if (iSkillLevel <= 0) iSkillLevel = 1;

		iResult = m_pGame->iDice(1, 100);
		if (iResult <= iSkillLevel) {
			m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 0, 1);

			switch (m_pMineral[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_iV1]->m_cType) {
			case 1:
				switch (m_pGame->iDice(1, 5)) {
				case 1:
				case 2:
				case 3:
					iItemID = 355; // Coal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 4:
					iItemID = 357; // IronOre
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 5:
					iItemID = 507; // BlondeStone
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				}
				break;

			case 2:
				switch (m_pGame->iDice(1, 5)) {
				case 1:
				case 2:
					iItemID = 355; // Coal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 3:
				case 4:
					iItemID = 357; // IronOre
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 5:
					if (m_pGame->iDice(1, 3) == 2) {
						iItemID = 356; // SilverNugget
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 4);
					}
					else {
						iItemID = 507; // BlondeStone
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					}
					break;
				}
				break;

			case 3:
				switch (m_pGame->iDice(1, 6)) {
				case 1:
					iItemID = 355; // Coal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 2:
				case 3:
				case 4:
				case 5:
					iItemID = 357; // IronOre
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 6:
					if (m_pGame->iDice(1, 8) == 3) {
						if (m_pGame->iDice(1, 2) == 1) {
							iItemID = 356; // SilverNugget
							m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 4);
						}
						else {
							iItemID = 357; // IronOre
							m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
						}
						break;
					}
					else {
						iItemID = 357; // IronOre
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					}
					break;
				}
				break;

			case 4:
				switch (m_pGame->iDice(1, 6)) {
				case 1:
					iItemID = 355; // Coal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 2:
					if (m_pGame->iDice(1, 3) == 2) {
						iItemID = 356; // SilverNugget
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 4);
					}
					break;
				case 3:
				case 4:
				case 5:
					iItemID = 357; // IronOre
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
					break;
				case 6:
					if (m_pGame->iDice(1, 8) == 3) {
						if (m_pGame->iDice(1, 4) == 3) {
							if (m_pGame->iDice(1, 4) < 3) {
								iItemID = 508; // Mithral
								m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 15);
							}
							else {
								iItemID = 354; // GoldNugget
								m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 5);
							}
							break;
						}
						else {
							iItemID = 356; // SilverNugget
							m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 4);
						}
						break;
					}
					else {
						if (m_pGame->iDice(1, 2) == 1) {
							iItemID = 354; // GoldNugget
							m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 5);
						}
						else {
							iItemID = 357;  // IronOre
							m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, 3);
						}
						break;
					}
					break;
				}
				break;

			case 5:
				switch (m_pGame->iDice(1, 19)) {
				case 3:
					iItemID = 352; // Sapphire
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					break;
				default:
					iItemID = 358; // Crystal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					break;
				}
				break;

			case 6:
				switch (m_pGame->iDice(1, 5)) {
				case 1:
					if (m_pGame->iDice(1, 6) == 3) {
						iItemID = 353; // Emerald
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 4);
					}
					else {
						iItemID = 358; // Crystal
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					}
					break;
				case 2:
					if (m_pGame->iDice(1, 6) == 3) {
						iItemID = 352; // Saphire
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 4);
					}
					else {
						iItemID = 358; // Crystal
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					}
					break;
				case 3:
					if (m_pGame->iDice(1, 6) == 3) {
						iItemID = 351; // Ruby
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 4);
					}
					else {
						iItemID = 358; // Crystal
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					}
					break;
				case 4:
					iItemID = 358; // Crystal
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					break;
				case 5:
					if (m_pGame->iDice(1, 12) == 3) {
						iItemID = 350; // Diamond
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 5);
					}
					else {
						iItemID = 358; // Crystal
						m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 3);
					}
					break;
				}
				break;

			}

			pItem = new CItem;
			if (m_pGame->m_pItemManager->_bInitItemAttr(pItem, iItemID) == false) {
				delete pItem;
			}
			else {
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
					m_pGame->m_pClientList[iClientH]->m_sY, pItem);
				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
					m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
					pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4
			}

			m_pMineral[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_iV1]->m_iRemain--;
			if (m_pMineral[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_iV1]->m_iRemain <= 0) {
				// . Delete Mineral
				bDeleteMineral(m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_iV1);

				delete m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex];
				m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex] = 0;
			}
		}
		break;

	default:
		break;
	}
}

bool MiningManager::bDeleteMineral(int iIndex)
{
	int iDynamicIndex;
	uint32_t dwTime;

	dwTime = GameClock::GetTimeMS();

	if (m_pMineral[iIndex] == 0) return false;
	iDynamicIndex = m_pMineral[iIndex]->m_sDynamicObjectHandle;
	if (m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex] == 0) return false;

	m_pGame->SendEventToNearClient_TypeB(MsgId::DynamicObject, MsgType::Reject, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_cMapIndex,
		m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sX, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sY,
		m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sType, iDynamicIndex, 0, (short)0);
	m_pGame->m_pMapList[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_cMapIndex]->SetDynamicObject(0, 0, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sX, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sY, dwTime);
	m_pGame->m_pMapList[m_pMineral[iIndex]->m_cMapIndex]->SetTempMoveAllowedFlag(m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sX, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iDynamicIndex]->m_sY, true);

	m_pGame->m_pMapList[m_pMineral[iIndex]->m_cMapIndex]->m_iCurMineral--;

	delete m_pMineral[iIndex];
	m_pMineral[iIndex] = 0;

	return true;
}
