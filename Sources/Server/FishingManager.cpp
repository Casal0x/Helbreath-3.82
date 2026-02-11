// FishingManager.cpp: Implementation of FishingManager.
// Fish spawning, catch processing, and fishing interactions.
// Extracted from CGame (Phase B1).

#include "FishingManager.h"
#include "Game.h"
#include "SkillManager.h"
#include "ItemManager.h"
#include "DynamicObjectManager.h"
#include "Packet/SharedPackets.h"

using namespace hb::shared::net;
using namespace hb::server::config;
namespace dynamic_object = hb::shared::dynamic_object;

FishingManager::FishingManager()
{
	InitArrays();
}

FishingManager::~FishingManager()
{
	CleanupArrays();
}

void FishingManager::InitArrays()
{
	for (int i = 0; i < MaxFishs; i++)
		m_pFish[i] = 0;
}

void FishingManager::CleanupArrays()
{
	for (int i = 0; i < MaxFishs; i++)
		if (m_pFish[i] != 0) {
			delete m_pFish[i];
			m_pFish[i] = 0;
		}
}

int FishingManager::iCreateFish(char cMapIndex, short sX, short sY, short sType, CItem* pItem, int iDifficulty, uint32_t dwLastTime)
{
	int iDynamicHandle;

	if ((cMapIndex < 0) || (cMapIndex >= MaxMaps)) return 0;
	if (m_pGame->m_pMapList[cMapIndex] == 0) return 0;
	if (m_pGame->m_pMapList[cMapIndex]->bGetIsWater(sX, sY) == false) return 0;

	for(int i = 1; i < MaxFishs; i++)
		if (m_pFish[i] == 0) {
			m_pFish[i] = new class CFish(cMapIndex, sX, sY, sType, pItem, iDifficulty);
			if (m_pFish[i] == 0) return 0;

			// Dynamic Object . Owner Fish  .
			switch (pItem->m_sIDnum) {
			case 101:
			case 102:
			case 103:
			case 570:
			case 571:
			case 572:
			case 573:
			case 574:
			case 575:
			case 576:
			case 577:
				iDynamicHandle = m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(i, 0, dynamic_object::Fish, cMapIndex, sX, sY, dwLastTime);
				break;
			default:
				iDynamicHandle = m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(i, 0, dynamic_object::FishObject, cMapIndex, sX, sY, dwLastTime);
				break;
			}

			if (iDynamicHandle == 0) {
				delete m_pFish[i];
				m_pFish[i] = 0;
				return 0;
			}
			m_pFish[i]->m_sDynamicObjectHandle = iDynamicHandle;
			m_pGame->m_pMapList[cMapIndex]->m_iCurFish++;

			return i;
		}

	return 0;
}


bool FishingManager::bDeleteFish(int iHandle, int iDelMode)
{
	int iH;
	uint32_t dwTime;

	if (m_pFish[iHandle] == 0) return false;

	dwTime = GameClock::GetTimeMS();

	// DynamicObject .
	iH = m_pFish[iHandle]->m_sDynamicObjectHandle;

	if (m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH] != 0) {
		m_pGame->SendEventToNearClient_TypeB(MsgId::DynamicObject, MsgType::Reject, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_cMapIndex, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_sX, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_sY, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_sType, iH, 0, (short)0);
		m_pGame->m_pMapList[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_cMapIndex]->SetDynamicObject(0, 0, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_sX, m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_sY, dwTime);
		m_pGame->m_pMapList[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH]->m_cMapIndex]->m_iCurFish--;

		delete m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH];
		m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[iH] = 0;
	}

	for(int i = 1; i < MaxClients; i++) {
		if ((m_pGame->m_pClientList[i] != 0) && (m_pGame->m_pClientList[i]->m_bIsInitComplete) &&
			(m_pGame->m_pClientList[i]->m_iAllocatedFish == iHandle)) {
			m_pGame->SendNotifyMsg(0, i, Notify::FishCanceled, iDelMode, 0, 0, 0);
			m_pGame->m_pSkillManager->ClearSkillUsingStatus(i);
		}
	}

	delete m_pFish[iHandle];
	m_pFish[iHandle] = 0;

	return true;
}


int FishingManager::iCheckFish(int iClientH, char cMapIndex, short dX, short dY)
{

	short sDistX, sDistY;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return 0;

	if ((cMapIndex < 0) || (cMapIndex >= MaxMaps)) return 0;

	for(int i = 1; i < MaxDynamicObjects; i++)
		if (m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i] != 0) {
			sDistX = abs(m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sX - dX);
			sDistY = abs(m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sY - dY);

			if ((m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_cMapIndex == cMapIndex) &&
				((m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sType == dynamic_object::Fish) || (m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sType == dynamic_object::FishObject)) &&
				(sDistX <= 2) && (sDistY <= 2)) {
				// .       Fish  .

				if (m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner] == 0) return 0;
				if (m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_sEngagingCount >= MaxEngagingFish) return 0;

				if (m_pGame->m_pClientList[iClientH]->m_iAllocatedFish != 0) return 0;
				if (m_pGame->m_pClientList[iClientH]->m_cMapIndex != cMapIndex) return 0;
				m_pGame->m_pClientList[iClientH]->m_iAllocatedFish = m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner;
				m_pGame->m_pClientList[iClientH]->m_iFishChance = 1;
				m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[1] = true;

				m_pGame->SendNotifyMsg(0, iClientH, Notify::EventFishMode, (m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_pItem->m_wPrice / 2), m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_pItem->m_sSprite,
					m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_pItem->m_sSpriteFrame, m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_pItem->m_cName);

				m_pFish[m_pGame->m_pDynamicObjectManager->m_pDynamicObjectList[i]->m_sOwner]->m_sEngagingCount++;

				return i;
			}
		}

	return 0;
}

void FishingManager::FishProcessor()
{
	int iSkillLevel, iResult, iChangeValue;

	for(int i = 1; i < MaxClients; i++) {
		if ((m_pGame->m_pClientList[i] != 0) && (m_pGame->m_pClientList[i]->m_bIsInitComplete) &&
			(m_pGame->m_pClientList[i]->m_iAllocatedFish != 0)) {

			if (m_pFish[m_pGame->m_pClientList[i]->m_iAllocatedFish] == 0) break;

			iSkillLevel = m_pGame->m_pClientList[i]->m_cSkillMastery[1];
			iSkillLevel -= m_pFish[m_pGame->m_pClientList[i]->m_iAllocatedFish]->m_iDifficulty;
			if (iSkillLevel <= 0) iSkillLevel = 1;

			iChangeValue = iSkillLevel / 10;
			if (iChangeValue <= 0) iChangeValue = 1;
			iChangeValue = m_pGame->iDice(1, iChangeValue);

			iResult = m_pGame->iDice(1, 100);
			if (iSkillLevel > iResult) {
				m_pGame->m_pClientList[i]->m_iFishChance += iChangeValue;
				if (m_pGame->m_pClientList[i]->m_iFishChance > 99) m_pGame->m_pClientList[i]->m_iFishChance = 99;

				m_pGame->SendNotifyMsg(0, i, Notify::FishChance, m_pGame->m_pClientList[i]->m_iFishChance, 0, 0, 0);
			}
			else if (iSkillLevel < iResult) {
				m_pGame->m_pClientList[i]->m_iFishChance -= iChangeValue;
				if (m_pGame->m_pClientList[i]->m_iFishChance < 1) m_pGame->m_pClientList[i]->m_iFishChance = 1;

				m_pGame->SendNotifyMsg(0, i, Notify::FishChance, m_pGame->m_pClientList[i]->m_iFishChance, 0, 0, 0);
			}
		}
	}
}


void FishingManager::ReqGetFishThisTimeHandler(int iClientH)
{
	int iResult, iFishH;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if (m_pGame->m_pClientList[iClientH]->m_iAllocatedFish == 0) return;
	if (m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish] == 0) return;

	m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[1] = false;

	iResult = m_pGame->iDice(1, 100);
	if (m_pGame->m_pClientList[iClientH]->m_iFishChance >= iResult) {

		m_pGame->GetExp(iClientH, m_pGame->iDice(m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_iDifficulty, 5));
		m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 1, m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_iDifficulty);

		pItem = m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_pItem;
		m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_pItem = 0;

		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
			m_pGame->m_pClientList[iClientH]->m_sY,
			pItem);

		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
			m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
			pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4 color

		m_pGame->SendNotifyMsg(0, iClientH, Notify::FishSuccess, 0, 0, 0, 0);
		iFishH = m_pGame->m_pClientList[iClientH]->m_iAllocatedFish;
		m_pGame->m_pClientList[iClientH]->m_iAllocatedFish = 0;

		bDeleteFish(iFishH, 1);
		return;
	}

	m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_sEngagingCount--;
	m_pGame->SendNotifyMsg(0, iClientH, Notify::FishFail, 0, 0, 0, 0);

	m_pGame->m_pClientList[iClientH]->m_iAllocatedFish = 0;
}


void FishingManager::FishGenerator()
{
	int iP, tX, tY, iRet;
	char  cItemName[hb::shared::limits::ItemNameLen];
	int sDifficulty;
	uint32_t dwLastTime;
	CItem* pItem;

	for(int i = 0; i < MaxMaps; i++) {
		if ((m_pGame->iDice(1, 10) == 5) && (m_pGame->m_pMapList[i] != 0) &&
			(m_pGame->m_pMapList[i]->m_iCurFish < m_pGame->m_pMapList[i]->m_iMaxFish)) {

			iP = m_pGame->iDice(1, m_pGame->m_pMapList[i]->m_iTotalFishPoint) - 1;
			if ((m_pGame->m_pMapList[i]->m_FishPointList[iP].x == -1) || (m_pGame->m_pMapList[i]->m_FishPointList[iP].y == -1)) break;

			tX = m_pGame->m_pMapList[i]->m_FishPointList[iP].x + (m_pGame->iDice(1, 3) - 2);
			tY = m_pGame->m_pMapList[i]->m_FishPointList[iP].y + (m_pGame->iDice(1, 3) - 2);

			pItem = new CItem;
			if (pItem == 0) break;

			std::memset(cItemName, 0, sizeof(cItemName));
			switch (m_pGame->iDice(1, 9)) {
			case 1:   strcpy(cItemName, "RedCarp"); sDifficulty = m_pGame->iDice(1, 10) + 20; break;
			case 2:   strcpy(cItemName, "GreenCarp"); sDifficulty = m_pGame->iDice(1, 5) + 10; break;
			case 3:   strcpy(cItemName, "GoldCarp"); sDifficulty = m_pGame->iDice(1, 10) + 1;  break;
			case 4:   strcpy(cItemName, "CrucianCarp"); sDifficulty = 1;  break;
			case 5:   strcpy(cItemName, "BlueSeaBream"); sDifficulty = m_pGame->iDice(1, 15) + 1;  break;
			case 6:   strcpy(cItemName, "RedSeaBream"); sDifficulty = m_pGame->iDice(1, 18) + 1;  break;
			case 7:   strcpy(cItemName, "Salmon"); sDifficulty = m_pGame->iDice(1, 12) + 1;  break;
			case 8:   strcpy(cItemName, "GrayMullet"); sDifficulty = m_pGame->iDice(1, 10) + 1;  break;
			case 9:
				switch (m_pGame->iDice(1, 150)) {
				case 1:
				case 2:
				case 3:
					strcpy(cItemName, "PowerGreenPotion");
					sDifficulty = m_pGame->iDice(5, 4) + 30;
					break;

				case 10:
				case 11:
					strcpy(cItemName, "SuperPowerGreenPotion");
					sDifficulty = m_pGame->iDice(5, 4) + 50;
					break;

				case 20:
					strcpy(cItemName, "Dagger+2");
					sDifficulty = m_pGame->iDice(5, 4) + 30;
					break;

				case 30:
					strcpy(cItemName, "LongSword+2");
					sDifficulty = m_pGame->iDice(5, 4) + 40;
					break;

				case 40:
					strcpy(cItemName, "Scimitar+2");
					sDifficulty = m_pGame->iDice(5, 4) + 50;
					break;

				case 50:
					strcpy(cItemName, "Rapier+2");
					sDifficulty = m_pGame->iDice(5, 4) + 60;
					break;

				case 60:
					strcpy(cItemName, "Flameberge+2");
					sDifficulty = m_pGame->iDice(5, 4) + 60;
					break;

				case 70:
					strcpy(cItemName, "WarAxe+2");
					sDifficulty = m_pGame->iDice(5, 4) + 50;
					break;

				case 90:
					strcpy(cItemName, "Ruby");
					sDifficulty = m_pGame->iDice(5, 4) + 40;
					break;

				case 95:
					strcpy(cItemName, "Diamond");
					sDifficulty = m_pGame->iDice(5, 4) + 40;
					break;
				}
				break;
			}
			dwLastTime = (60000 * 10) + (m_pGame->iDice(1, 3) - 1) * (60000 * 10);

			if (m_pGame->m_pItemManager->_bInitItemAttr(pItem, cItemName)) {
				iRet = iCreateFish(i, tX, tY, 1, pItem, sDifficulty, dwLastTime);
			}
			else {
				delete pItem;
				pItem = 0;
			}
		}
	}
}


void FishingManager::ReleaseFishEngagement(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_iAllocatedFish == 0) return;

	if (m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish] != 0)
		m_pFish[m_pGame->m_pClientList[iClientH]->m_iAllocatedFish]->m_sEngagingCount--;

	m_pGame->m_pClientList[iClientH]->m_iAllocatedFish = 0;
}
