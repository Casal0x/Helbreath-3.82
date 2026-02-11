// CraftingManager.cpp: Manages potion brewing and crafting recipe processing.
// Extracted from CGame (Phase B2).

#include "CraftingManager.h"
#include "Game.h"
#include "SkillManager.h"
#include "ItemManager.h"
#include "Portion.h"
#include "Item.h"
#include "Packet/SharedPackets.h"

extern char G_cTxt[512];
extern void PutLogFileList(char* cStr);

using namespace hb::shared::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;

CraftingManager::CraftingManager()
{
	for (int i = 0; i < MaxPortionTypes; i++) {
		m_pPortionConfigList[i] = 0;
		m_pCraftingConfigList[i] = 0;
	}
}

CraftingManager::~CraftingManager()
{
	CleanupArrays();
}

void CraftingManager::InitArrays()
{
	for (int i = 0; i < MaxPortionTypes; i++) {
		m_pPortionConfigList[i] = 0;
		m_pCraftingConfigList[i] = 0;
	}
}

void CraftingManager::CleanupArrays()
{
	for (int i = 0; i < MaxPortionTypes; i++) {
		if (m_pPortionConfigList[i] != 0) delete m_pPortionConfigList[i];
		if (m_pCraftingConfigList[i] != 0) delete m_pCraftingConfigList[i];
	}
}

void CraftingManager::ReqCreatePortionHandler(int iClientH, char* pData)
{
	char cI[6], cPortionName[hb::shared::limits::ItemNameLen];
	int    iRet, j, iEraseReq, iSkillLimit, iSkillLevel, iResult, iDifficulty;
	short sItemIndex[6], sTemp;
	short  sItemNumber[6], sItemArray[12];
	bool   bDup, bFlag;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	m_pGame->m_pClientList[iClientH]->m_iSkillMsgRecvCount++;

	for(int i = 0; i < 6; i++) {
		cI[i] = -1;
		sItemIndex[i] = -1;
		sItemNumber[i] = 0;
	}

	const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandCommonItems>(
		pData, sizeof(hb::net::PacketCommandCommonItems));
	if (!pkt) return;
	for(int i = 0; i < 6; i++) {
		cI[i] = static_cast<char>(pkt->item_ids[i]);
	}

	for(int i = 0; i < 6; i++) {
		if (cI[i] >= hb::shared::limits::MaxItems) return;
		if ((cI[i] >= 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[cI[i]] == 0)) return;
	}

	for(int i = 0; i < 6; i++)
		if (cI[i] >= 0) {
			bDup = false;
			for (j = 0; j < 6; j++)
				if (sItemIndex[j] == cI[i]) {
					sItemNumber[j]++;
					bDup = true;
				}
			if (bDup == false) {
				for (j = 0; j < 6; j++)
					if (sItemIndex[j] == -1) {
						sItemIndex[j] = cI[i];
						sItemNumber[j]++;
						goto RCPH_LOOPBREAK;
					}
			RCPH_LOOPBREAK:;
			}
		}

	for(int i = 0; i < 6; i++)
		if (sItemIndex[i] != -1) {
			if (sItemIndex[i] < 0) return;
			if ((sItemIndex[i] >= 0) && (sItemIndex[i] >= hb::shared::limits::MaxItems)) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]] == 0) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_dwCount < static_cast<uint32_t>(sItemNumber[i])) return;
		}

	// . Bubble Sort
	bFlag = true;
	while (bFlag) {
		bFlag = false;
		for(int i = 0; i < 5; i++)
			if ((sItemIndex[i] != -1) && (sItemIndex[i + 1] != -1)) {
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sIDnum) <
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i + 1]]->m_sIDnum)) {
					sTemp = sItemIndex[i + 1];
					sItemIndex[i + 1] = sItemIndex[i];
					sItemIndex[i] = sTemp;
					sTemp = sItemNumber[i + 1];
					sItemNumber[i + 1] = sItemNumber[i];
					sItemNumber[i] = sTemp;
					bFlag = true;
				}
			}
	}

	j = 0;
	for(int i = 0; i < 6; i++) {
		if (sItemIndex[i] != -1)
			sItemArray[j] = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sIDnum;
		else sItemArray[j] = sItemIndex[i];
		sItemArray[j + 1] = sItemNumber[i];
		j += 2;
	}

	std::memset(cPortionName, 0, sizeof(cPortionName));

	for(int i = 0; i < MaxPortionTypes; i++)
		if (m_pPortionConfigList[i] != 0) {
			bFlag = false;
			for (j = 0; j < 12; j++)
				if (m_pPortionConfigList[i]->m_sArray[j] != sItemArray[j]) bFlag = true;

			if (bFlag == false) {
				std::memset(cPortionName, 0, sizeof(cPortionName));
				memcpy(cPortionName, m_pPortionConfigList[i]->m_cName, hb::shared::limits::ItemNameLen - 1);
				iSkillLimit = m_pPortionConfigList[i]->m_iSkillLimit;
				iDifficulty = m_pPortionConfigList[i]->m_iDifficulty;
			}
		}

	if (strlen(cPortionName) == 0) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::NoMatchingPortion, 0, 0, 0, 0);
		return;
	}

	iSkillLevel = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[12];
	if (iSkillLimit > iSkillLevel) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::LowPortionSkill, 0, 0, 0, cPortionName);
		return;
	}

	iSkillLevel -= iDifficulty;
	if (iSkillLevel <= 0) iSkillLevel = 1;

	iResult = m_pGame->iDice(1, 100);
	if (iResult > iSkillLevel) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::PortionFail, 0, 0, 0, cPortionName);
		return;
	}

	m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 12, 1);

	if (strlen(cPortionName) != 0) {
		pItem = 0;
		pItem = new CItem;
		if (pItem == 0) return;

		for(int i = 0; i < 6; i++)
			if (sItemIndex[i] != -1) {
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Consume)
					// v1.41 !!!
					m_pGame->m_pItemManager->SetItemCount(iClientH, sItemIndex[i], //     m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_cName,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_dwCount - sItemNumber[i]);
				else m_pGame->m_pItemManager->ItemDepleteHandler(iClientH, sItemIndex[i], false);
			}

		m_pGame->SendNotifyMsg(0, iClientH, Notify::PortionSuccess, 0, 0, 0, cPortionName);
		m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, (iDifficulty / 3));

		if ((m_pGame->m_pItemManager->_bInitItemAttr(pItem, cPortionName))) {
			if (m_pGame->m_pItemManager->_bAddClientItemList(iClientH, pItem, &iEraseReq)) {
				iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);
				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					break;
				}

				//if ((pItem->m_wPrice * pItem->m_dwCount) > 1000)
				//	SendMsgToLS(ServerMsgId::RequestSavePlayerData, iClientH);
			}
			else {
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
					m_pGame->m_pClientList[iClientH]->m_sY, pItem);

				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
					m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
					pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4

				iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);


				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					break;
				}
			}
		}
		else {
			delete pItem;
			pItem = 0;
		}
	}
}

///		Snoopy: Added Crafting to the same file than potions
void CraftingManager::ReqCreateCraftingHandler(int iClientH, char* pData)
{
	char cI[6], cCraftingName[hb::shared::limits::ItemNameLen];
	int    iRet, j, iEraseReq, iRiskLevel, iDifficulty, iNeededContrib = 0;
	short sTemp;
	short  sItemIndex[6], sItemPurity[6], sItemNumber[6], sItemArray[12];
	bool   bDup, bFlag, bNeedLog;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	m_pGame->m_pClientList[iClientH]->m_iSkillMsgRecvCount++;

	for(int i = 0; i < 6; i++)
	{
		cI[i] = -1;
		sItemIndex[i] = -1;
		sItemNumber[i] = 0;
		sItemPurity[i] = -1;
	}
	const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandCommonBuild>(
		pData, sizeof(hb::net::PacketCommandCommonBuild));
	if (!pkt) return;
	for(int i = 0; i < 6; i++) {
		cI[i] = static_cast<char>(pkt->item_ids[i]);
	}

	for(int i = 0; i < 6; i++)
	{
		if (cI[i] >= hb::shared::limits::MaxItems) return;
		if ((cI[i] >= 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[cI[i]] == 0)) return;
	}

	for(int i = 0; i < 6; i++)
		if (cI[i] >= 0)
		{
			bDup = false;
			for (j = 0; j < 6; j++)
				if (sItemIndex[j] == cI[i])
				{
					sItemNumber[j]++;
					bDup = true;
				}
			if (bDup == false)
			{
				for (j = 0; j < 6; j++)
					if (sItemIndex[j] == -1)
					{
						sItemIndex[j] = cI[i];
						sItemNumber[j]++;
						goto RCPH_LOOPBREAK;
					}
			RCPH_LOOPBREAK:;
			}
		}


	for(int i = 0; i < 6; i++)
		if (sItemIndex[i] != -1)
		{
			if (sItemIndex[i] < 0) return;
			if ((sItemIndex[i] >= 0) && (sItemIndex[i] >= hb::shared::limits::MaxItems)) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]] == 0) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_dwCount < static_cast<uint32_t>(sItemNumber[i])) return;
			sItemPurity[i] = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sItemSpecEffectValue2;
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::None)
				&& (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sSprite == 6)
				&& (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sSpriteFrame == 129))
			{
				sItemPurity[i] = 100; // Merien stones considered 100% purity.
			}
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Consume)
			{
				sItemPurity[i] = -1; // Diamonds / Emeralds.etc.. never have purity
			}
			if (sItemNumber[i] > 1) // No purity for stacked items
			{
				sItemPurity[i] = -1;
			}
			/*std::snprintf(G_cTxt, sizeof(G_cTxt), "Crafting: %d x %s (%d)"
				, sItemNumber[i]
				, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_cName
				, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sIDnum);
			PutLogList(G_cTxt);*/

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Equip)
				&& (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetEquipPos() == EquipPos::Neck))
			{
				iNeededContrib = 10; // Necks Crafting requires 10 contrib
			}
		}

	// Bubble Sort
	bFlag = true;
	while (bFlag)
	{
		bFlag = false;
		for(int i = 0; i < 5; i++)
			if ((sItemIndex[i] != -1) && (sItemIndex[i + 1] != -1))
			{
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sIDnum) < (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i + 1]]->m_sIDnum))
				{
					sTemp = sItemIndex[i + 1];
					sItemIndex[i + 1] = sItemIndex[i];
					sItemIndex[i] = sTemp;
					sTemp = sItemPurity[i + 1];
					sItemPurity[i + 1] = sItemPurity[i];
					sItemPurity[i] = sTemp;
					sTemp = sItemNumber[i + 1];
					sItemNumber[i + 1] = sItemNumber[i];
					sItemNumber[i] = sTemp;
					bFlag = true;
				}
			}
	}
	j = 0;
	for(int i = 0; i < 6; i++)
	{
		if (sItemIndex[i] != -1)
			sItemArray[j] = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sIDnum;
		else sItemArray[j] = sItemIndex[i];
		sItemArray[j + 1] = sItemNumber[i];
		j += 2;
	}

	// Search Crafting you wanna build
	std::memset(cCraftingName, 0, sizeof(cCraftingName));
	for(int i = 0; i < MaxPortionTypes; i++)
		if (m_pCraftingConfigList[i] != 0)
		{
			bFlag = false;
			for (j = 0; j < 12; j++)
			{
				if (m_pCraftingConfigList[i]->m_sArray[j] != sItemArray[j]) bFlag = true; // one item mismatch
			}
			if (bFlag == false) // good Crafting receipe
			{
				std::memset(cCraftingName, 0, sizeof(cCraftingName));
				memcpy(cCraftingName, m_pCraftingConfigList[i]->m_cName, hb::shared::limits::ItemNameLen - 1);
				iRiskLevel = m_pCraftingConfigList[i]->m_iSkillLimit;			// % to loose item if crafting fails
				iDifficulty = m_pCraftingConfigList[i]->m_iDifficulty;
			}
		}


	// Check if recipe is OK
	if (strlen(cCraftingName) == 0)
	{
		m_pGame->SendNotifyMsg(0, iClientH, Notify::CraftingFail, 1, 0, 0, 0); // "There is not enough material"
		return;
	}
	// Check for Contribution
	if (m_pGame->m_pClientList[iClientH]->m_iContribution < iNeededContrib)
	{
		m_pGame->SendNotifyMsg(0, iClientH, Notify::CraftingFail, 2, 0, 0, 0); // "There is not enough Contribution Point"
		return;
	}
	// Check possible Failure
	if (m_pGame->iDice(1, 100) > static_cast<uint32_t>(iDifficulty))
	{
		m_pGame->SendNotifyMsg(0, iClientH, Notify::CraftingFail, 3, 0, 0, 0); // "Crafting failed"
		// Remove parts...
		pItem = 0;
		pItem = new CItem;
		if (pItem == 0) return;
		for(int i = 0; i < 6; i++)
			if (sItemIndex[i] != -1)
			{	// Deplete any Merien Stone
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::None)
					&& (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sSprite == 6)
					&& (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_sSpriteFrame == 129))
				{
					m_pGame->m_pItemManager->ItemDepleteHandler(iClientH, sItemIndex[i], false);
				}
				else
					// Risk to deplete any other items (not stackable ones) // DEF_ITEMTYPE_CONSUME
					if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Equip)
						|| (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Material))
					{
						if (m_pGame->iDice(1, 100) < static_cast<uint32_t>(iRiskLevel))
						{
							m_pGame->m_pItemManager->ItemDepleteHandler(iClientH, sItemIndex[i], false);
						}
					}
			}
		return;
	}

	// Purity
	int iPurity, iTot = 0, iCount = 0;
	for(int i = 0; i < 6; i++)
	{
		if (sItemIndex[i] != -1)
		{
			if (sItemPurity[i] != -1)
			{
				iTot += sItemPurity[i];
				iCount++;
			}
		}
	}
	if (iCount == 0)
	{
		iPurity = 20 + m_pGame->iDice(1, 80);			// Wares have random purity (20%..100%)
		bNeedLog = false;
	}
	else
	{
		iPurity = iTot / iCount;
		iTot = (iPurity * 4) / 5;
		iCount = iPurity - iTot;
		iPurity = iTot + m_pGame->iDice(1, iCount);	// Jewel completion depends off Wares purity
		bNeedLog = true;
	}
	if (iNeededContrib != 0)
	{
		iPurity = 0;						// Necks require contribution but no purity/completion
		bNeedLog = true;
	}
	m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 18, 1);

	if (strlen(cCraftingName) != 0)
	{
		pItem = 0;
		pItem = new CItem;
		if (pItem == 0) return;
		for(int i = 0; i < 6; i++)
		{
			if (sItemIndex[i] != -1)
			{
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->GetItemType() == ItemType::Consume)
				{
					m_pGame->m_pItemManager->SetItemCount(iClientH, sItemIndex[i],
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex[i]]->m_dwCount - sItemNumber[i]);
				}
				else // So if item is not Type 5 (stackable items), you deplete item
				{
					m_pGame->m_pItemManager->ItemDepleteHandler(iClientH, sItemIndex[i], false);
				}
			}
		}
		if (iNeededContrib != 0)
		{
			m_pGame->m_pClientList[iClientH]->m_iContribution -= iNeededContrib;
			// No known msg to send info to client, so client will compute shown Contrib himself.
		}

		m_pGame->SendNotifyMsg(0, iClientH, Notify::CraftingSuccess, 0, 0, 0, 0);

		m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(2, 100);

		if ((m_pGame->m_pItemManager->_bInitItemAttr(pItem, cCraftingName)))
		{	// // Snoopy: Added Purity to Oils/Elixirs
			if (iPurity != 0)
			{
				pItem->m_sItemSpecEffectValue2 = iPurity;
				pItem->m_dwAttribute = 1;
			}
			pItem->SetTouchEffectType(TouchEffectType::ID);
			pItem->m_sTouchEffectValue1 = static_cast<short>(m_pGame->iDice(1, 100000));
			pItem->m_sTouchEffectValue2 = static_cast<short>(m_pGame->iDice(1, 100000));
			// pItem->m_sTouchEffectValue3 = GameClock::GetTimeMS();
			SYSTEMTIME SysTime;
			char cTemp[256];
			GetLocalTime(&SysTime);
			std::memset(cTemp, 0, sizeof(cTemp));
			std::snprintf(cTemp, sizeof(cTemp), "%d%2d", (short)SysTime.wMonth, (short)SysTime.wDay);
			pItem->m_sTouchEffectValue3 = atoi(cTemp);

			// SNOOPY log anything above WAREs
			if (bNeedLog)
			{
				std::snprintf(G_cTxt, sizeof(G_cTxt), "PC(%s) Crafting (%s) Purity(%d)"
					, m_pGame->m_pClientList[iClientH]->m_cCharName
					, pItem->m_cName
					, pItem->m_sItemSpecEffectValue2);
				PutLogFileList(G_cTxt);
			}
			if (m_pGame->m_pItemManager->_bAddClientItemList(iClientH, pItem, &iEraseReq))
			{
				iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);
				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					break;
				}
				//if ((pItem->m_wPrice * pItem->m_dwCount) > 1000)
				//	SendMsgToLS(ServerMsgId::RequestSavePlayerData, iClientH);
			}
			else
			{
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
					m_pGame->m_pClientList[iClientH]->m_sY, pItem);
				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
					m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
					pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute);

				iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);


				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					break;
				}
			}
		}
		else
		{
			delete pItem;
			pItem = 0;
		}
	}
}
