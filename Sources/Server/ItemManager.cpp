#include "ItemManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "WarManager.h"
#include "SkillManager.h"
#include "MagicManager.h"
#include "Item.h"
#include "CombatManager.h"
#include "EntityManager.h"
#include "DynamicObjectManager.h"
#include "DelayEventManager.h"
#include "LootManager.h"
#include "CraftingManager.h"
#include "QuestManager.h"
#include "GuildManager.h"
#include "FishingManager.h"
#include "MiningManager.h"
#include "Packet/SharedPackets.h"
#include "ObjectIDRange.h"
#include "Skill.h"
#include "GameConfigSqliteStore.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "StringCompat.h"
#include "TimeUtils.h"

using namespace hb::shared::net;

using hb::log_channel;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;
namespace dynamic_object = hb::shared::dynamic_object;
namespace smap = hb::server::map;
namespace sdelay = hb::server::delay_event;
using namespace hb::server::npc;
using namespace hb::server::skill;

extern char G_cTxt[512];
extern char G_cData50000[50000];

static std::string format_item_info(CItem* pItem)
{
	if (pItem == nullptr) return "(null)";
	char buf[256];
	std::snprintf(buf, sizeof(buf), "%s(count=%u attr=0x%08X touch=%d:%d:%d:%d)",
		pItem->m_cName,
		pItem->m_dwCount,
		pItem->m_dwAttribute,
		pItem->m_sTouchEffectType,
		pItem->m_sTouchEffectValue1,
		pItem->m_sTouchEffectValue2,
		pItem->m_sTouchEffectValue3);
	return buf;
}

static bool is_item_suspicious(CItem* pItem)
{
	if (pItem == nullptr) return false;
	if (pItem->m_sIDnum == 90) return false; // Gold
	if (pItem->m_dwAttribute != 0 && pItem->GetTouchEffectType() != TouchEffectType::ID)
		return true;
	if (pItem->GetTouchEffectType() == TouchEffectType::None && pItem->m_dwAttribute != 0)
		return true;
	return false;
}

// Helper function (duplicated from Game.cpp - static file-scope function)
static void NormalizeItemName(const char* src, char* dst, size_t dstSize)
{
	size_t j = 0;
	for (size_t i = 0; src[i] && j < dstSize - 1; ++i) {
		if (src[i] != ' ' && src[i] != '_') {
			dst[j++] = src[i];
		}
	}
	dst[j] = '\0';
}

bool ItemManager::bSendClientItemConfigs(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) {
		return false;
	}

	// Calculate how many items per packet - keep packets small (~7KB) for reliable delivery
	constexpr size_t maxPacketSize = 7000;
	constexpr size_t headerSize = sizeof(hb::net::PacketItemConfigHeader);
	constexpr size_t entrySize = sizeof(hb::net::PacketItemConfigEntry);
	constexpr size_t maxEntriesPerPacket = (maxPacketSize - headerSize) / entrySize;

	// First count total items
	int totalItems = 0;
	for(int i = 0; i < MaxItemTypes; i++) {
		if (m_pGame->m_pItemConfigList[i] != 0) {
			totalItems++;
		}
	}

	// Send items in packets
	int itemsSent = 0;
	int packetIndex = 0;

	while (itemsSent < totalItems) {
		// Build packet
		std::memset(G_cData50000, 0, sizeof(G_cData50000));

		auto* pktHeader = reinterpret_cast<hb::net::PacketItemConfigHeader*>(G_cData50000);
		pktHeader->header.msg_id = MsgId::ItemConfigContents;
		pktHeader->header.msg_type = MsgType::Confirm;
		pktHeader->totalItems = static_cast<uint16_t>(totalItems);
		pktHeader->packetIndex = static_cast<uint16_t>(packetIndex);

		auto* entries = reinterpret_cast<hb::net::PacketItemConfigEntry*>(G_cData50000 + headerSize);

		uint16_t entriesInPacket = 0;
		int configIndex = 0;
		int skipped = 0;

		// Find items for this packet
		for(int i = 0; i < MaxItemTypes && entriesInPacket < maxEntriesPerPacket; i++) {
			if (m_pGame->m_pItemConfigList[i] == 0) {
				continue;
			}

			// Skip items already sent in previous packets
			if (skipped < itemsSent) {
				skipped++;
				continue;
			}

			const CItem* item = m_pGame->m_pItemConfigList[i];
			auto& entry = entries[entriesInPacket];

			entry.itemId = item->m_sIDnum;
			std::memset(entry.name, 0, sizeof(entry.name));
			std::snprintf(entry.name, sizeof(entry.name), "%s", item->m_cName);
			entry.itemType = item->m_cItemType;
			entry.equipPos = item->m_cEquipPos;
			entry.effectType = item->m_sItemEffectType;
			entry.effectValue1 = item->m_sItemEffectValue1;
			entry.effectValue2 = item->m_sItemEffectValue2;
			entry.effectValue3 = item->m_sItemEffectValue3;
			entry.effectValue4 = item->m_sItemEffectValue4;
			entry.effectValue5 = item->m_sItemEffectValue5;
			entry.effectValue6 = item->m_sItemEffectValue6;
			entry.maxLifeSpan = item->m_wMaxLifeSpan;
			entry.specialEffect = item->m_sSpecialEffect;
			entry.sprite = item->m_sSprite;
			entry.spriteFrame = item->m_sSpriteFrame;
			entry.price = item->m_bIsForSale ? static_cast<int32_t>(item->m_wPrice) : -static_cast<int32_t>(item->m_wPrice);
			entry.weight = item->m_wWeight;
			entry.apprValue = item->m_cApprValue;
			entry.speed = item->m_cSpeed;
			entry.levelLimit = item->m_sLevelLimit;
			entry.genderLimit = item->m_cGenderLimit;
			entry.specialEffectValue1 = item->m_sSpecialEffectValue1;
			entry.specialEffectValue2 = item->m_sSpecialEffectValue2;
			entry.relatedSkill = item->m_sRelatedSkill;
			entry.category = item->m_cCategory;
			entry.itemColor = item->m_cItemColor;

			entriesInPacket++;
		}

		pktHeader->itemCount = entriesInPacket;
		size_t packetSize = headerSize + (entriesInPacket * entrySize);

		int iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(G_cData50000, static_cast<int>(packetSize));
		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			hb::logger::log("Failed to send item configs: Client({}) Packet({})", iClientH, packetIndex);
			m_pGame->DeleteClient(iClientH, true, true);
			delete m_pGame->m_pClientList[iClientH];
			m_pGame->m_pClientList[iClientH] = 0;
			return false;
		}

		itemsSent += entriesInPacket;
		packetIndex++;
	}

	return true;
}

const DropTable* ItemManager::GetDropTable(int id) const
{
	if (id <= 0) {
		return nullptr;
	}
	auto it = m_pGame->m_DropTables.find(id);
	if (it == m_pGame->m_DropTables.end()) {
		return nullptr;
	}
	return &it->second;
}

void ItemManager::_ClearItemConfigList()
{
	for(int i = 0; i < MaxItemTypes; i++) {
		if (m_pGame->m_pItemConfigList[i] != 0) {
			delete m_pGame->m_pItemConfigList[i];
			m_pGame->m_pItemConfigList[i] = 0;
		}
	}
}

bool ItemManager::_bInitItemAttr(CItem* pItem, const char* pItemName)
{
	
	char cTmpName[hb::shared::limits::NpcNameLen];
	char cNormalizedInput[21];
	char cNormalizedConfig[21];

	std::memset(cTmpName, 0, sizeof(cTmpName));
	strcpy(cTmpName, pItemName);

	// Normalize the input name for comparison (client may send "MagicStaff" while DB has "Magic Staff")
	NormalizeItemName(cTmpName, cNormalizedInput, sizeof(cNormalizedInput));

	for(int i = 0; i < MaxItemTypes; i++)
		if (m_pGame->m_pItemConfigList[i] != 0) {
			// Normalize the config name for comparison
			NormalizeItemName(m_pGame->m_pItemConfigList[i]->m_cName, cNormalizedConfig, sizeof(cNormalizedConfig));
			if (hb_stricmp(cNormalizedInput, cNormalizedConfig) == 0) {
				std::memset(pItem->m_cName, 0, sizeof(pItem->m_cName));
				strcpy(pItem->m_cName, m_pGame->m_pItemConfigList[i]->m_cName);
				pItem->m_cItemType = m_pGame->m_pItemConfigList[i]->m_cItemType;
				pItem->m_cEquipPos = m_pGame->m_pItemConfigList[i]->m_cEquipPos;
				pItem->m_sItemEffectType = m_pGame->m_pItemConfigList[i]->m_sItemEffectType;
				pItem->m_sItemEffectValue1 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue1;
				pItem->m_sItemEffectValue2 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue2;
				pItem->m_sItemEffectValue3 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue3;
				pItem->m_sItemEffectValue4 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue4;
				pItem->m_sItemEffectValue5 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue5;
				pItem->m_sItemEffectValue6 = m_pGame->m_pItemConfigList[i]->m_sItemEffectValue6;
				pItem->m_wMaxLifeSpan = m_pGame->m_pItemConfigList[i]->m_wMaxLifeSpan;
				pItem->m_wCurLifeSpan = pItem->m_wMaxLifeSpan;
				pItem->m_sSpecialEffect = m_pGame->m_pItemConfigList[i]->m_sSpecialEffect;

				pItem->m_sSprite = m_pGame->m_pItemConfigList[i]->m_sSprite;
				pItem->m_sSpriteFrame = m_pGame->m_pItemConfigList[i]->m_sSpriteFrame;
				pItem->m_wPrice = m_pGame->m_pItemConfigList[i]->m_wPrice;
				pItem->m_wWeight = m_pGame->m_pItemConfigList[i]->m_wWeight;
				pItem->m_cApprValue = m_pGame->m_pItemConfigList[i]->m_cApprValue;
				pItem->m_cSpeed = m_pGame->m_pItemConfigList[i]->m_cSpeed;
				pItem->m_sLevelLimit = m_pGame->m_pItemConfigList[i]->m_sLevelLimit;
				pItem->m_cGenderLimit = m_pGame->m_pItemConfigList[i]->m_cGenderLimit;

				pItem->m_sSpecialEffectValue1 = m_pGame->m_pItemConfigList[i]->m_sSpecialEffectValue1;
				pItem->m_sSpecialEffectValue2 = m_pGame->m_pItemConfigList[i]->m_sSpecialEffectValue2;

				pItem->m_sRelatedSkill = m_pGame->m_pItemConfigList[i]->m_sRelatedSkill;
				pItem->m_cCategory = m_pGame->m_pItemConfigList[i]->m_cCategory;
				pItem->m_sIDnum = m_pGame->m_pItemConfigList[i]->m_sIDnum;

				pItem->m_bIsForSale = m_pGame->m_pItemConfigList[i]->m_bIsForSale;
				pItem->m_cItemColor = m_pGame->m_pItemConfigList[i]->m_cItemColor;

				return true;
			}
		}

	return false;
}

bool ItemManager::_bInitItemAttr(CItem* pItem, int iItemID)
{
	if (iItemID < 0 || iItemID >= MaxItemTypes) return false;
	if (m_pGame->m_pItemConfigList[iItemID] == nullptr) return false;

	CItem* pConfig = m_pGame->m_pItemConfigList[iItemID];

	std::memset(pItem->m_cName, 0, sizeof(pItem->m_cName));
	strcpy(pItem->m_cName, pConfig->m_cName);
	pItem->m_cItemType = pConfig->m_cItemType;
	pItem->m_cEquipPos = pConfig->m_cEquipPos;
	pItem->m_sItemEffectType = pConfig->m_sItemEffectType;
	pItem->m_sItemEffectValue1 = pConfig->m_sItemEffectValue1;
	pItem->m_sItemEffectValue2 = pConfig->m_sItemEffectValue2;
	pItem->m_sItemEffectValue3 = pConfig->m_sItemEffectValue3;
	pItem->m_sItemEffectValue4 = pConfig->m_sItemEffectValue4;
	pItem->m_sItemEffectValue5 = pConfig->m_sItemEffectValue5;
	pItem->m_sItemEffectValue6 = pConfig->m_sItemEffectValue6;
	pItem->m_wMaxLifeSpan = pConfig->m_wMaxLifeSpan;
	pItem->m_wCurLifeSpan = pItem->m_wMaxLifeSpan;
	pItem->m_sSpecialEffect = pConfig->m_sSpecialEffect;
	pItem->m_sSprite = pConfig->m_sSprite;
	pItem->m_sSpriteFrame = pConfig->m_sSpriteFrame;
	pItem->m_wPrice = pConfig->m_wPrice;
	pItem->m_wWeight = pConfig->m_wWeight;
	pItem->m_cApprValue = pConfig->m_cApprValue;
	pItem->m_cSpeed = pConfig->m_cSpeed;
	pItem->m_sLevelLimit = pConfig->m_sLevelLimit;
	pItem->m_cGenderLimit = pConfig->m_cGenderLimit;
	pItem->m_sSpecialEffectValue1 = pConfig->m_sSpecialEffectValue1;
	pItem->m_sSpecialEffectValue2 = pConfig->m_sSpecialEffectValue2;
	pItem->m_sRelatedSkill = pConfig->m_sRelatedSkill;
	pItem->m_cCategory = pConfig->m_cCategory;
	pItem->m_sIDnum = pConfig->m_sIDnum;
	pItem->m_bIsForSale = pConfig->m_bIsForSale;
	pItem->m_cItemColor = pConfig->m_cItemColor;

	return true;
}

void ItemManager::DropItemHandler(int iClientH, short sItemIndex, int iAmount, const char* pItemName, bool bByPlayer)
{
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;
	if ((iAmount != -1) && (iAmount < 0)) return;

	if (((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Consume) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow)) &&
		(iAmount == -1))
		iAmount = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount;

	if (((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Consume) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow)) &&
		(((int)m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount - iAmount) > 0)) {
		pItem = new CItem;
		if (_bInitItemAttr(pItem, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cName) == false) {
			delete pItem;
			return;
		}
		else {
			if (iAmount <= 0) {
				delete pItem;
				return;
			}
			pItem->m_dwCount = (uint32_t)iAmount;
		}

		if ((uint32_t)iAmount > m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount) {
			delete pItem;
			return;
		}

		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount -= iAmount;

		// v1.41 !!!
		SetItemCount(iClientH, sItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount);

		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
			m_pGame->m_pClientList[iClientH]->m_sY, pItem);

		// v1.411 
		// v2.17 2002-7-31
		if (bByPlayer)
			_bItemLog(ItemLogAction::Drop, iClientH, (int)-1, pItem);
		else
			_bItemLog(ItemLogAction::Drop, iClientH, (int)-1, pItem, true);

		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
			m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
			pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4 color

		m_pGame->SendNotifyMsg(0, iClientH, Notify::DropItemFinCountChanged, sItemIndex, iAmount, 0, 0);
	}
	else {

		ReleaseItemHandler(iClientH, sItemIndex, true);

		// v2.17
		if (m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex])
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);

		// v1.432
		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::AlterItemDrop) &&
			(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan == 0)) {
			delete m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex];
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] = 0;
		}
		else {
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
				m_pGame->m_pClientList[iClientH]->m_sY,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

			// v1.41
			// v2.17 2002-7-31
			if (bByPlayer)
				_bItemLog(ItemLogAction::Drop, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);
			else
				_bItemLog(ItemLogAction::Drop, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex], true);

			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
				0,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); //v1.4 color
		}

		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] = 0;
		m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] = false;

		m_pGame->SendNotifyMsg(0, iClientH, Notify::DropItemFinEraseItem, sItemIndex, iAmount, 0, 0);

		m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);
	}

	m_pGame->iCalcTotalWeight(iClientH);
}

int ItemManager::iClientMotion_GetItem_Handler(int iClientH, short sX, short sY, char cDir)
{
	char  cRemainItemColor;
	int   iRet, iEraseReq;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;
	if ((cDir <= 0) || (cDir > 8))       return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return 0;

	if ((sX != m_pGame->m_pClientList[iClientH]->m_sX) || (sY != m_pGame->m_pClientList[iClientH]->m_sY)) return 2;

	int iStX, iStY;
	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex] != 0) {
		iStX = m_pGame->m_pClientList[iClientH]->m_sX / 20;
		iStY = m_pGame->m_pClientList[iClientH]->m_sY / 20;
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iPlayerActivity++;

		switch (m_pGame->m_pClientList[iClientH]->m_cSide) {
		case 0: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iNeutralActivity++; break;
		case 1: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iAresdenActivity++; break;
		case 2: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iElvineActivity++;  break;
		}
	}

	m_pGame->m_pSkillManager->ClearSkillUsingStatus(iClientH);

	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->ClearOwner(0, iClientH, hb::shared::owner_class::Player, sX, sY);
	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetOwner(iClientH, hb::shared::owner_class::Player, sX, sY);

	short sIDNum;
	uint32_t dwAttribute;
	pItem = m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->pGetItem(sX, sY, &sIDNum, &cRemainItemColor, &dwAttribute);
	if (pItem != 0) {
		if (_bAddClientItemList(iClientH, pItem, &iEraseReq)) {

			_bItemLog(ItemLogAction::Get, iClientH, 0, pItem);

			iRet = SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return 0;
			}

			// Broadcast remaining item state to nearby clients (clears tile if no items remain)
			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::SetItem,
				m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				sX, sY, sIDNum, 0, cRemainItemColor, dwAttribute);
		}
		else
		{
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(sX, sY, pItem);

			iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return 0;
			}
		}
	}

	{
		hb::net::PacketResponseMotionHeader pkt{};
		pkt.header.msg_id = MsgId::ResponseMotion;
		pkt.header.msg_type = Confirm::MotionConfirm;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	switch (iRet) {
	case sock::Event::QueueFull:
	case sock::Event::SocketError:
	case sock::Event::CriticalError:
	case sock::Event::SocketClosed:
		m_pGame->DeleteClient(iClientH, true, true);
		return 0;
	}

	return 1;
}

bool ItemManager::_bAddClientItemList(int iClientH, CItem* pItem, int* pDelReq)
{

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (pItem == 0) return false;

	if ((pItem->GetItemType() == ItemType::Consume) || (pItem->GetItemType() == ItemType::Arrow)) {
		if ((m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(pItem, pItem->m_dwCount)) > m_pGame->_iCalcMaxLoad(iClientH))
			return false;
	}
	else {
		if ((m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(pItem, 1)) > m_pGame->_iCalcMaxLoad(iClientH))
			return false;
	}

	if ((pItem->GetItemType() == ItemType::Consume) || (pItem->GetItemType() == ItemType::Arrow)) {
		for(int i = 0; i < hb::shared::limits::MaxItems; i++)
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) &&
				(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum == pItem->m_sIDnum)) {
				m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_dwCount += pItem->m_dwCount;
				//delete pItem;
				*pDelReq = 1;

				m_pGame->iCalcTotalWeight(iClientH);

				return true;
			}
	}

	for(int i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] == 0) {

			m_pGame->m_pClientList[iClientH]->m_pItemList[i] = pItem;
			m_pGame->m_pClientList[iClientH]->m_ItemPosList[i].x = 40;
			m_pGame->m_pClientList[iClientH]->m_ItemPosList[i].y = 30;

			*pDelReq = 0;

			if (pItem->GetItemType() == ItemType::Arrow)
				m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);

			m_pGame->iCalcTotalWeight(iClientH);

			return true;
		}

	return false;
}

int ItemManager::_bAddClientBulkItemList(int iClientH, const char* pItemName, int iAmount)
{
	if (m_pGame->m_pClientList[iClientH] == nullptr) return 0;
	if (pItemName == nullptr || iAmount < 1) return 0;

	int iCreated = 0;
	CItem* pFirstItem = nullptr;

	for (int i = 0; i < iAmount; i++)
	{
		CItem* pItem = new CItem();
		if (!_bInitItemAttr(pItem, pItemName))
		{
			delete pItem;
			break;
		}

		// Weight check
		if ((m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(pItem, 1)) > m_pGame->_iCalcMaxLoad(iClientH))
		{
			delete pItem;
			break;
		}

		// Find an empty slot directly (no merge)
		bool bAdded = false;
		for (int j = 0; j < hb::shared::limits::MaxItems; j++)
		{
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[j] == nullptr)
			{
				m_pGame->m_pClientList[iClientH]->m_pItemList[j] = pItem;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[j].x = 40;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[j].y = 30;
				m_pGame->iCalcTotalWeight(iClientH);
				if (pFirstItem == nullptr) pFirstItem = pItem;
				iCreated++;
				bAdded = true;
				break;
			}
		}

		if (!bAdded)
		{
			delete pItem;
			break;
		}
	}

	// Send one bulk notification with total count
	if (iCreated > 0 && pFirstItem != nullptr)
	{
		hb::net::PacketNotifyItemObtained pkt{};
		pkt.header.msg_id = MsgId::Notify;
		pkt.header.msg_type = Notify::ItemObtainedBulk;
		pkt.is_new = 1;
		memcpy(pkt.name, pFirstItem->m_cName, sizeof(pkt.name));
		pkt.count = iCreated;
		pkt.item_type = pFirstItem->m_cItemType;
		pkt.equip_pos = pFirstItem->m_cEquipPos;
		pkt.is_equipped = 0;
		pkt.level_limit = pFirstItem->m_sLevelLimit;
		pkt.gender_limit = pFirstItem->m_cGenderLimit;
		pkt.cur_lifespan = pFirstItem->m_wCurLifeSpan;
		pkt.weight = pFirstItem->m_wWeight;
		pkt.sprite = pFirstItem->m_sSprite;
		pkt.sprite_frame = pFirstItem->m_sSpriteFrame;
		pkt.item_color = pFirstItem->m_cItemColor;
		pkt.spec_value2 = static_cast<uint8_t>(pFirstItem->m_sItemSpecEffectValue2);
		pkt.attribute = pFirstItem->m_dwAttribute;
		pkt.item_id = pFirstItem->m_sIDnum;
		pkt.max_lifespan = pFirstItem->m_wMaxLifeSpan;
		m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}

	return iCreated;
}

bool ItemManager::bEquipItemHandler(int iClientH, short sItemIndex, bool bNotify)
{
	char cHeroArmorType;
	EquipPos cEquipPos;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return false;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return false;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() != ItemType::Equip) return false;

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan == 0) return false;

	if (((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00000001) == 0) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sLevelLimit > m_pGame->m_pClientList[iClientH]->m_iLevel)) return false;

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cGenderLimit != 0) {
		switch (m_pGame->m_pClientList[iClientH]->m_sType) {
		case 1:
		case 2:
		case 3:
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cGenderLimit != 1) return false;
			break;
		case 4:
		case 5:
		case 6:
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cGenderLimit != 2) return false;
			break;
		}
	}

	if (iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex], 1) > (m_pGame->m_pClientList[iClientH]->m_iStr + m_pGame->m_pClientList[iClientH]->m_iAngelicStr) * 100) return false;

	cEquipPos = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetEquipPos();

	if ((cEquipPos == EquipPos::Body) || (cEquipPos == EquipPos::Leggings) ||
		(cEquipPos == EquipPos::Arms) || (cEquipPos == EquipPos::Head)) {
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue4) {
		case 10: // Str
			if ((m_pGame->m_pClientList[iClientH]->m_iStr + m_pGame->m_pClientList[iClientH]->m_iAngelicStr) < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		case 11: // Dex
			if ((m_pGame->m_pClientList[iClientH]->m_iDex + m_pGame->m_pClientList[iClientH]->m_iAngelicDex) < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		case 12: // Vit
			if (m_pGame->m_pClientList[iClientH]->m_iVit < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		case 13: // Int
			if ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		case 14: // Mag
			if ((m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		case 15: // Chr
			if (m_pGame->m_pClientList[iClientH]->m_iCharisma < m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], true);
				return false;
			}
			break;
		}
	}

	if (cEquipPos == EquipPos::TwoHand) {
		// Stormbringer
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 845) {
			if ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) < 65) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos, sItemIndex, 0, 0);
				ReleaseItemHandler(iClientH, sItemIndex, true);
				return false;
			}
		}
	}

	if (cEquipPos == EquipPos::RightHand) {
		// Resurrection wand(MS.10) or Resurrection wand(MS.20)
		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 865) || (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 866)) {
			if ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) > 99 && (m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) > 99 && m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime < 1) {
				m_pGame->m_pClientList[iClientH]->m_cMagicMastery[94] = true;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::StateChangeSuccess, 0, 0, 0, 0);
			}
		}
	}

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::AttackSpecAbility) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::DefenseSpecAbility)) {

		if ((m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType != 0)) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos != m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos], 0, 0);
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos], true);
			}
		}
	}

	if (cEquipPos == EquipPos::None) return false;

	if (cEquipPos == EquipPos::TwoHand) {
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] != -1)
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], false);
		else {
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1)
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)], false);
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::LeftHand)] != -1)
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::LeftHand)], false);
		}
	}
	else {
		if ((cEquipPos == EquipPos::LeftHand) || (cEquipPos == EquipPos::RightHand)) {
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1)
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)], false);
		}

		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] != -1)
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], false);
	}

	if (cEquipPos == EquipPos::FullBody) {
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Head)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Head)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Body)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Body)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Arms)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Arms)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Leggings)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Leggings)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Pants)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Pants)], false);
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Back)] != -1) {
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Back)], false);
		}
	}
	else {
		if (cEquipPos == EquipPos::Head || cEquipPos == EquipPos::Body || cEquipPos == EquipPos::Arms ||
			cEquipPos == EquipPos::Leggings || cEquipPos == EquipPos::Pants || cEquipPos == EquipPos::Back) {
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::FullBody)] != -1) {
				ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::FullBody)], false);
			}
		}
		if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] != -1)
			ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)], false);
	}

	m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] = sItemIndex;
	m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] = true;

	hb::shared::entity::ApplyEquipAppearance(
		m_pGame->m_pClientList[iClientH]->m_appearance,
		cEquipPos,
		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cApprValue,
		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor);

	// Weapon-specific: compute attack delay and reset combo
	if (cEquipPos == EquipPos::RightHand || cEquipPos == EquipPos::TwoHand) {
		int speed = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cSpeed;
		speed -= ((m_pGame->m_pClientList[iClientH]->m_iStr + m_pGame->m_pClientList[iClientH]->m_iAngelicStr) / 13);
		if (speed < 0) speed = 0;
		m_pGame->m_pClientList[iClientH]->m_status.iAttackDelay = static_cast<uint8_t>(speed);
		m_pGame->m_pClientList[iClientH]->m_iComboAttackCount = 0;
	}

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::AttackSpecAbility) {
		m_pGame->m_pClientList[iClientH]->m_appearance.iShieldGlare = 0;
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect) {
		case 0: break;
		case 1:
			m_pGame->m_pClientList[iClientH]->m_appearance.iShieldGlare = 1;
			break;

		case 2:
			m_pGame->m_pClientList[iClientH]->m_appearance.iShieldGlare = 3;
			break;

		case 3:
			m_pGame->m_pClientList[iClientH]->m_appearance.iShieldGlare = 2;
			break;
		}
	}

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::DefenseSpecAbility) {
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponGlare = 0;
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect) {
		case 0:
			break;
		case 50:
		case 51:
		case 52:
			m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponGlare = 1;
			break;
		default:
			// m_sAppr4
			// 0x0002 Green
			// 0x0003 ice element
			// 0x0004 sparkle
			// 0x0005 sparkle green gm
			// 0x0006 sparkle green
			break;
		}
	}

	cHeroArmorType = _cCheckHeroItemEquipped(iClientH);
	if (cHeroArmorType != 0x0FFFFFFFF) m_pGame->m_pClientList[iClientH]->m_cHeroArmourBonus = cHeroArmorType;

	m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
	CalcTotalItemEffect(iClientH, sItemIndex, bNotify);
	return true;

}

void ItemManager::RequestPurchaseItemHandler(int iClientH, const char* pItemName, int iNum, int iItemId)
{
	CItem* pItem;
	uint32_t dwGoldCount, dwItemCount;
	uint16_t wTempPrice;
	int   iRet, iEraseReq, iGoldWeight;
	int   iCost, iDiscountRatio, iDiscountCost;
	double dTmp1, dTmp2, dTmp3;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	//if ( (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) != 0) &&
	//	 (memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, m_pGame->m_pClientList[iClientH]->m_cLocation, 10) != 0) ) return;

	if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) != 0) {
		if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "are", 3) == 0) {
			if ((memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, "aresden", 7) == 0) ||
				(memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, "arefarm", 7) == 0)) {

			}
			else return;
		}

		if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "elv", 3) == 0) {
			if ((memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, "elvine", 6) == 0) ||
				(memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, "elvfarm", 7) == 0)) {

			}
			else return;
		}
	}

	// New 18/05/2004
	if (m_pGame->m_pClientList[iClientH]->m_pIsProcessingAllowed == false) return;

	// Determine item ID and count from client-provided item ID
	short sItemID = 0;
	dwItemCount = 1;

	if (iItemId > 0 && iItemId < MaxItemTypes) {
		sItemID = static_cast<short>(iItemId);
	}
	else {
		// No valid item ID provided
		return;
	}

	for(int i = 1; i <= iNum; i++) {

		pItem = new CItem;
		bool bInitOk = _bInitItemAttr(pItem, sItemID);
		if (bInitOk == false) {
			delete pItem;
		}
		else {

			if (pItem->m_bIsForSale == false) {
				delete pItem;
				return;
			}

			pItem->m_dwCount = dwItemCount;

			iCost = pItem->m_wPrice * pItem->m_dwCount;

			dwGoldCount = dwGetItemCountByID(iClientH, hb::shared::item::ItemId::Gold);

			iDiscountRatio = ((m_pGame->m_pClientList[iClientH]->m_iCharisma - 10) / 4);

			// 2.03 Discount Method
			// Charisma
			// iDiscountRatio = (m_pGame->m_pClientList[iClientH]->m_iCharisma / 4) -1;
			// if (iDiscountRatio == 0) iDiscountRatio = 1;

			dTmp1 = (double)(iDiscountRatio);
			dTmp2 = dTmp1 / 100.0f;
			dTmp1 = (double)iCost;
			dTmp3 = dTmp1 * dTmp2;
			iDiscountCost = (int)dTmp3;

			if (iDiscountCost >= (iCost / 2)) iDiscountCost = (iCost / 2) - 1;

			if (dwGoldCount < (uint32_t)(iCost - iDiscountCost)) {
				delete pItem;

				{
					hb::net::PacketNotifyNotEnoughGold pkt{};
					pkt.header.msg_id = MsgId::Notify;
					pkt.header.msg_type = Notify::NotEnoughGold;
					pkt.item_index = -1;
					iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
				}
				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					return;
				}
				return;
			}

			if (_bAddClientItemList(iClientH, pItem, &iEraseReq)) {
				if (m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad < 0) m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad = 0;

				wTempPrice = (iCost - iDiscountCost);
				iRet = SendItemNotifyMsg(iClientH, Notify::ItemPurchased, pItem, wTempPrice);
				if (iEraseReq == 1) delete pItem;

				// Gold  .      .
				iGoldWeight = SetItemCountByID(iClientH, hb::shared::item::ItemId::Gold, dwGoldCount - wTempPrice);
				m_pGame->iCalcTotalWeight(iClientH);

				m_pGame->m_stCityStatus[m_pGame->m_pClientList[iClientH]->m_cSide].iFunds += wTempPrice;

				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					return;
				}
			}
			else
			{
				delete pItem;

				m_pGame->iCalcTotalWeight(iClientH);

				iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					return;
				}
			}
		}
	}
}

void ItemManager::GiveItemHandler(int iClientH, short sItemIndex, int iAmount, short dX, short dY, uint16_t wObjectID, const char* pItemName)
{
	int iRet, iEraseReq;
	short sOwnerH;
	char cOwnerType, cCharName[hb::shared::limits::NpcNameLen];
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (iAmount <= 0) return;

	std::memset(cCharName, 0, sizeof(cCharName));

	if (((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Consume) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow)) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount > (uint32_t)iAmount)) {

		pItem = new CItem;
		if (_bInitItemAttr(pItem, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cName) == false) {
			delete pItem;
			return;
		}
		else {
			pItem->m_dwCount = iAmount;
		}

		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount -= iAmount;

		SetItemCount(iClientH, sItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount);

		// dX, dY     .
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

		if (wObjectID != 0) {
			if (hb::shared::object_id::IsPlayerID(wObjectID)) {
				if ((wObjectID > 0) && (wObjectID < MaxClients)) {
					if (m_pGame->m_pClientList[wObjectID] != 0) {
						if ((uint16_t)sOwnerH != wObjectID) sOwnerH = 0;
					}
				}
			}
			else {
				// NPC
				uint16_t npcIdx = hb::shared::object_id::ToNpcIndex(wObjectID);
				if (hb::shared::object_id::IsNpcID(wObjectID) && (npcIdx > 0) && (npcIdx < MaxNpcs)) {
					if (m_pGame->m_pNpcList[npcIdx] != 0) {
						if ((uint16_t)sOwnerH != npcIdx) sOwnerH = 0;
					}
				}
			}
		}

		if (sOwnerH == 0) {
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, pItem);

			// v1.411
			_bItemLog(ItemLogAction::Drop, iClientH, 0, pItem);

			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
				pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); //v1.4 color
		}
		else {
			if (cOwnerType == hb::shared::owner_class::Player) {
				memcpy(cCharName, m_pGame->m_pClientList[sOwnerH]->m_cCharName, hb::shared::limits::CharNameLen - 1);

				if (sOwnerH == iClientH) {
					delete pItem;
					return;
				}

				if (_bAddClientItemList(sOwnerH, pItem, &iEraseReq)) {
					iRet = SendItemNotifyMsg(sOwnerH, Notify::ItemObtained, pItem, 0);
					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(sOwnerH, true, true);
						break;
					}

					// v1.4
					m_pGame->SendNotifyMsg(0, iClientH, Notify::GiveItemFinCountChanged, sItemIndex, iAmount, 0, cCharName);
				}
				else {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
						m_pGame->m_pClientList[iClientH]->m_sY,
						pItem);

					// v1.411  
					_bItemLog(ItemLogAction::Drop, iClientH, 0, pItem);

					m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
						m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
						pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); //v1.4 color

					{
						iRet = SendItemNotifyMsg(sOwnerH, Notify::CannotCarryMoreItem, 0, 0);
					}

					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(sOwnerH, true, true);
						break;
					}

					m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotGiveItem, sItemIndex, iAmount, 0, cCharName);
				}

			}
			else {
				// NPC  .
				memcpy(cCharName, m_pGame->m_pNpcList[sOwnerH]->m_cNpcName, hb::shared::limits::NpcNameLen - 1);

				if (m_pGame->m_pNpcList[sOwnerH]->m_iNpcConfigId == 58) { // Warehouse Keeper
					// NPC     .
					if (bSetItemToBankItem(iClientH, pItem) == false) {
						m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotItemToBank, 0, 0, 0, 0);

						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, pItem);

						// v1.411  
						_bItemLog(ItemLogAction::Drop, iClientH, 0, pItem);

						m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
							m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
							pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4 color
					}
				}
				else {
					// NPC       .
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, pItem);

					// v1.411  
					_bItemLog(ItemLogAction::Drop, iClientH, 0, pItem);

					m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
						m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
						pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4 color
				}
			}
		}
	}
	else {

		ReleaseItemHandler(iClientH, sItemIndex, true);

		if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow)
			m_pGame->m_pClientList[iClientH]->m_cArrowIndex = -1;

		// dX, dY     .
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY); // dX, dY   .         .

		if (wObjectID != 0) {
			if (hb::shared::object_id::IsPlayerID(wObjectID)) {
				if ((wObjectID > 0) && (wObjectID < MaxClients)) {
					if (m_pGame->m_pClientList[wObjectID] != 0) {
						if ((uint16_t)sOwnerH != wObjectID) sOwnerH = 0;
					}
				}
			}
			else {
				// NPC
				uint16_t npcIdx = hb::shared::object_id::ToNpcIndex(wObjectID);
				if (hb::shared::object_id::IsNpcID(wObjectID) && (npcIdx > 0) && (npcIdx < MaxNpcs)) {
					if (m_pGame->m_pNpcList[npcIdx] != 0) {
						if ((uint16_t)sOwnerH != npcIdx) sOwnerH = 0;
					}
				}
			}
		}

		if (sOwnerH == 0) {
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
				m_pGame->m_pClientList[iClientH]->m_sY,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);
			// v1.411  
			_bItemLog(ItemLogAction::Drop, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
				0,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); // v1.4 color

			m_pGame->SendNotifyMsg(0, iClientH, Notify::DropItemFinEraseItem, sItemIndex, iAmount, 0, 0);
		}
		else {
			// . @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

			if (cOwnerType == hb::shared::owner_class::Player) {
				memcpy(cCharName, m_pGame->m_pClientList[sOwnerH]->m_cCharName, hb::shared::limits::CharNameLen - 1);
				pItem = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex];

				if (pItem->m_sIDnum == 88) {

					// iClientH  sOwnerH   .
					// sOwnerH         .
					if ((m_pGame->m_pClientList[iClientH]->m_iGuildRank == -1) &&
						(memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) != 0) &&
						(memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pClientList[sOwnerH]->m_cLocation, 10) == 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iGuildRank == 0)) {
						m_pGame->SendNotifyMsg(iClientH, sOwnerH, Notify::QueryJoinGuildReqPermission, 0, 0, 0, 0);
						m_pGame->SendNotifyMsg(0, iClientH, Notify::GiveItemFinEraseItem, sItemIndex, 1, 0, cCharName);

						_bItemLog(ItemLogAction::Deplete, iClientH, (int)-1, pItem);

						goto REMOVE_ITEM_PROCEDURE;
					}
				}

				if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 89)) {

					// iClientH  sOwnerH   .
					// sOwnerH  iClientH    iClientH
					if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cGuildName, m_pGame->m_pClientList[sOwnerH]->m_cGuildName, 20) == 0) &&
						(m_pGame->m_pClientList[iClientH]->m_iGuildRank != -1) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iGuildRank == 0)) {
						m_pGame->SendNotifyMsg(iClientH, sOwnerH, Notify::QueryDismissGuildReqPermission, 0, 0, 0, 0);
						m_pGame->SendNotifyMsg(0, iClientH, Notify::GiveItemFinEraseItem, sItemIndex, 1, 0, cCharName);

						_bItemLog(ItemLogAction::Deplete, iClientH, (int)-1, pItem);

						goto REMOVE_ITEM_PROCEDURE;
					}
				}

				if (_bAddClientItemList(sOwnerH, pItem, &iEraseReq)) {

					_bItemLog(ItemLogAction::Give, iClientH, sOwnerH, pItem);

					iRet = SendItemNotifyMsg(sOwnerH, Notify::ItemObtained, pItem, 0);
					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(sOwnerH, true, true);
						break;
					}
				}
				else {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
						m_pGame->m_pClientList[iClientH]->m_sY,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);
					_bItemLog(ItemLogAction::Drop, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

					m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
						m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
						0,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); // v1.4 color

					{
						iRet = SendItemNotifyMsg(sOwnerH, Notify::CannotCarryMoreItem, 0, 0);
					}

					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(sOwnerH, true, true);
						break;
					}

					std::memset(cCharName, 0, sizeof(cCharName));
				}
			}
			else {
				memcpy(cCharName, m_pGame->m_pNpcList[sOwnerH]->m_cNpcName, hb::shared::limits::NpcNameLen - 1);

				if (m_pGame->m_pNpcList[sOwnerH]->m_iNpcConfigId == 58) { // Warehouse Keeper
					if (bSetItemToBankItem(iClientH, sItemIndex) == false) {
						m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotItemToBank, 0, 0, 0, 0);

						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
							m_pGame->m_pClientList[iClientH]->m_sY,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

						_bItemLog(ItemLogAction::Drop, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

						m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
							m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
							0,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); // v1.4 color
					}
				}
				else if (m_pGame->m_pNpcList[sOwnerH]->m_iNpcConfigId == 56) { // Shop Keeper
					if ((m_pGame->m_bIsCrusadeMode == false) && (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 89)) {

						if ((m_pGame->m_pClientList[iClientH]->m_iGuildRank != 0) && (m_pGame->m_pClientList[iClientH]->m_iGuildRank != -1)) {
							m_pGame->SendNotifyMsg(iClientH, iClientH, CommonType::DismissGuildApprove, 0, 0, 0, 0);

							std::memset(m_pGame->m_pClientList[iClientH]->m_cGuildName, 0, sizeof(m_pGame->m_pClientList[iClientH]->m_cGuildName));
							memcpy(m_pGame->m_pClientList[iClientH]->m_cGuildName, "NONE", 4);
							m_pGame->m_pClientList[iClientH]->m_iGuildRank = -1;
							m_pGame->m_pClientList[iClientH]->m_iGuildGUID = -1;

							m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);

							m_pGame->m_pClientList[iClientH]->m_iExp -= 300;
							if (m_pGame->m_pClientList[iClientH]->m_iExp < 0) m_pGame->m_pClientList[iClientH]->m_iExp = 0;
							m_pGame->SendNotifyMsg(0, iClientH, Notify::Exp, 0, 0, 0, 0);
						}

						delete m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex];
					}
					else {
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
							m_pGame->m_pClientList[iClientH]->m_sY,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

						_bItemLog(ItemLogAction::Drop, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

						m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
							m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
							0,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
							m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); // v1.4 color

						std::memset(cCharName, 0, sizeof(cCharName));

					}
				}
				else {
					// NPC       .

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
						m_pGame->m_pClientList[iClientH]->m_sY,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

					_bItemLog(ItemLogAction::Drop, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

					m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
						m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum,
						0,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
						m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute); // v1.4 color

					std::memset(cCharName, 0, sizeof(cCharName));
				}
			}

			m_pGame->SendNotifyMsg(0, iClientH, Notify::GiveItemFinEraseItem, sItemIndex, iAmount, 0, cCharName);
		}

	REMOVE_ITEM_PROCEDURE:

		if (m_pGame->m_pClientList[iClientH] == 0) return;

		// . delete !
		m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] = 0;
		m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] = false;

		m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);
	}

	m_pGame->iCalcTotalWeight(iClientH);
}

int ItemManager::SetItemCount(int iClientH, int iItemIndex, uint32_t dwCount)
{
	uint16_t wWeight;

	if (m_pGame->m_pClientList[iClientH] == 0) return -1;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] == 0) return -1;

	wWeight = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 1);//m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wWeight;

	if (dwCount == 0) {
		ItemDepleteHandler(iClientH, iItemIndex, false);
	}
	else {
		m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwCount = dwCount;
		m_pGame->SendNotifyMsg(0, iClientH, Notify::SetItemCount, iItemIndex, dwCount, (char)true, 0);
	}

	return wWeight;
}

uint32_t ItemManager::dwGetItemCountByID(int iClientH, short sItemID)
{
	if (m_pGame->m_pClientList[iClientH] == nullptr) return 0;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++) {
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != nullptr &&
		    m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum == sItemID) {
			return m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_dwCount;
		}
	}

	return 0;
}

int ItemManager::SetItemCountByID(int iClientH, short sItemID, uint32_t dwCount)
{
	if (m_pGame->m_pClientList[iClientH] == nullptr) return -1;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++) {
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != nullptr &&
		    m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum == sItemID) {

			uint16_t wWeight = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemList[i], 1);

			if (dwCount == 0) {
				ItemDepleteHandler(iClientH, i, false);
			}
			else {
				m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_dwCount = dwCount;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::SetItemCount, i, dwCount, (char)true, 0);
			}

			return wWeight;
		}
	}

	return -1;
}

void ItemManager::ReleaseItemHandler(int iClientH, short sItemIndex, bool bNotice)
{
	char cHeroArmorType;
	EquipPos cEquipPos;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() != ItemType::Equip) return;

	if (m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] == false) return;

	cHeroArmorType = _cCheckHeroItemEquipped(iClientH);
	if (cHeroArmorType != 0x0FFFFFFFF) m_pGame->m_pClientList[iClientH]->m_cHeroArmourBonus = 0;

	cEquipPos = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetEquipPos();
	if (cEquipPos == EquipPos::RightHand) {
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] != 0) {
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 865) || (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 866)) {
				m_pGame->m_pClientList[iClientH]->m_cMagicMastery[94] = false;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::StateChangeSuccess, 0, 0, 0, 0);
			}
		}
	}

	// Appr .
	switch (cEquipPos) {
	case EquipPos::RightHand:
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponColor = 0;
		m_pGame->m_pClientList[iClientH]->m_status.iAttackDelay = 0;
		break;

	case EquipPos::LeftHand:
		m_pGame->m_pClientList[iClientH]->m_appearance.iShieldType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iShieldColor = 0;
		break;

	case EquipPos::TwoHand:
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponColor = 0;
		break;

	case EquipPos::Body:
		m_pGame->m_pClientList[iClientH]->m_appearance.iArmorType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.bHideArmor = false;
		m_pGame->m_pClientList[iClientH]->m_appearance.iArmorColor = 0;
		break;

	case EquipPos::Back:
		m_pGame->m_pClientList[iClientH]->m_appearance.iMantleType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iMantleColor = 0;
		break;

	case EquipPos::Arms:
		m_pGame->m_pClientList[iClientH]->m_appearance.iArmArmorType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iArmColor = 0;
		break;

	case EquipPos::Pants:
		m_pGame->m_pClientList[iClientH]->m_appearance.iPantsType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iPantsColor = 0;
		break;

	case EquipPos::Leggings:
		m_pGame->m_pClientList[iClientH]->m_appearance.iBootsType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iBootsColor = 0;
		break;

	case EquipPos::Head:
		m_pGame->m_pClientList[iClientH]->m_appearance.iHelmType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iHelmColor = 0;
		break;

	case EquipPos::FullBody:
		m_pGame->m_pClientList[iClientH]->m_appearance.iArmorType = 0;
		m_pGame->m_pClientList[iClientH]->m_appearance.iMantleColor = 0;
		break;
	}

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::AttackSpecAbility) {
		m_pGame->m_pClientList[iClientH]->m_appearance.iShieldGlare = 0;
	}

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType() == ItemEffectType::DefenseSpecAbility) {
		m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponGlare = 0;
	}

	m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] = false;
	m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(cEquipPos)] = -1;

	if (bNotice)
		m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);

	CalcTotalItemEffect(iClientH, sItemIndex, true);
}

void ItemManager::RequestRetrieveItemHandler(int iClientH, char* pData)
{
	char cBankItemIndex;
	int j, iRet, iItemWeight;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	const auto* pkt = hb::net::PacketCast<hb::net::PacketRequestRetrieveItem>(
		pData, sizeof(hb::net::PacketRequestRetrieveItem));
	if (!pkt) return;
	cBankItemIndex = static_cast<char>(pkt->item_slot);
	//wh remove
	//if (m_pGame->m_pClientList[iClientH]->m_bIsInsideWarehouse == false) return;

	if ((cBankItemIndex < 0) || (cBankItemIndex >= hb::shared::limits::MaxBankItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex] == 0) {
		// Bank item missing.
		hb::net::PacketResponseRetrieveItem pkt{};
		pkt.header.msg_id = MsgId::ResponseRetrieveItem;
		pkt.header.msg_type = MsgType::Reject;
		pkt.bank_index = 0;
		pkt.item_index = 0;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	else {
		/*
		if ( (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->GetItemType() == ItemType::Consume) ||
			 (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->GetItemType() == ItemType::Arrow) ) {
			//iItemWeight = m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_wWeight * m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_dwCount;
			iItemWeight = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex], m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_dwCount);
		}
		else iItemWeight = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex], 1); //m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_wWeight;
		*/
		// v1.432
		iItemWeight = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex], m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_dwCount);

		if ((iItemWeight + m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad) > m_pGame->_iCalcMaxLoad(iClientH)) {
		// Notify cannot carry more items.
			iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				break;
			}
			return;
		}

		if ((m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->GetItemType() == ItemType::Consume) ||
			(m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->GetItemType() == ItemType::Arrow)) {
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) &&
					(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cItemType == m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_cItemType) &&
					(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum == m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_sIDnum)) {
					// v1.41 !!! 
					SetItemCount(iClientH, i, m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_dwCount + m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex]->m_dwCount);

					delete m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex];
					m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex] = 0;

					for (j = 0; j <= hb::shared::limits::MaxBankItems - 2; j++) {
						if ((m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1] != 0) && (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j] == 0)) {
							m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j] = m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1];

							m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1] = 0;
						}
					}

					// Send retrieve confirmation.
					hb::net::PacketResponseRetrieveItem pkt{};
					pkt.header.msg_id = MsgId::ResponseRetrieveItem;
					pkt.header.msg_type = MsgType::Confirm;
					pkt.bank_index = cBankItemIndex;
					pkt.item_index = static_cast<int8_t>(i);

					m_pGame->iCalcTotalWeight(iClientH);
					m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);

					iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));

					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(iClientH, true, true);
						return;
					}
					return;
				}

			goto RRIH_NOQUANTITY;
		}
		else {
		RRIH_NOQUANTITY:
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] == 0) {
					m_pGame->m_pClientList[iClientH]->m_pItemList[i] = m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex];
					// v1.3 1-27 12:22
					m_pGame->m_pClientList[iClientH]->m_ItemPosList[i].x = 40;
					m_pGame->m_pClientList[iClientH]->m_ItemPosList[i].y = 30;

					m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[i] = false;

					m_pGame->m_pClientList[iClientH]->m_pItemInBankList[cBankItemIndex] = 0;

					for (j = 0; j <= hb::shared::limits::MaxBankItems - 2; j++) {
						if ((m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1] != 0) && (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j] == 0)) {
							m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j] = m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1];

							m_pGame->m_pClientList[iClientH]->m_pItemInBankList[j + 1] = 0;
						}
					}

					// Send retrieve confirmation.
					hb::net::PacketResponseRetrieveItem pktConfirm{};
					pktConfirm.header.msg_id = MsgId::ResponseRetrieveItem;
					pktConfirm.header.msg_type = MsgType::Confirm;
					pktConfirm.bank_index = cBankItemIndex;
					pktConfirm.item_index = static_cast<int8_t>(i);

					m_pGame->iCalcTotalWeight(iClientH);
					m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);

					iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pktConfirm), sizeof(pktConfirm));
					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(iClientH, true, true);
						return;
					}
					return;
				}
			// No empty inventory slot.
			hb::net::PacketResponseRetrieveItem pktReject{};
			pktReject.header.msg_id = MsgId::ResponseRetrieveItem;
			pktReject.header.msg_type = MsgType::Reject;
			pktReject.bank_index = 0;
			pktReject.item_index = 0;
			iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pktReject), sizeof(pktReject));
		}
	}

	switch (iRet) {
	case sock::Event::QueueFull:
	case sock::Event::SocketError:
	case sock::Event::CriticalError:
	case sock::Event::SocketClosed:
		m_pGame->DeleteClient(iClientH, true, true);
		return;
	}
}

bool ItemManager::bSetItemToBankItem(int iClientH, short sItemIndex)
{
	int iRet;
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return false;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return false;
	//wh remove
	//if (m_pGame->m_pClientList[iClientH]->m_bIsInsideWarehouse == false) return false;

	for(int i = 0; i < hb::shared::limits::MaxBankItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[i] == 0) {

			m_pGame->m_pClientList[iClientH]->m_pItemInBankList[i] = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex];
			pItem = m_pGame->m_pClientList[iClientH]->m_pItemInBankList[i];
			// !!!      NULL .
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] = 0;

			m_pGame->iCalcTotalWeight(iClientH);

			{
				hb::net::PacketNotifyItemToBank pkt{};
				pkt.header.msg_id = MsgId::Notify;
				pkt.header.msg_type = Notify::ItemToBank;
				pkt.bank_index = static_cast<uint8_t>(i);
				pkt.is_new = 1;
				memcpy(pkt.name, pItem->m_cName, sizeof(pkt.name));
				pkt.count = pItem->m_dwCount;
				pkt.item_type = pItem->m_cItemType;
				pkt.equip_pos = pItem->m_cEquipPos;
				pkt.is_equipped = 0;
				pkt.level_limit = pItem->m_sLevelLimit;
				pkt.gender_limit = pItem->m_cGenderLimit;
				pkt.cur_lifespan = pItem->m_wCurLifeSpan;
				pkt.weight = pItem->m_wWeight;
				pkt.sprite = pItem->m_sSprite;
				pkt.sprite_frame = pItem->m_sSpriteFrame;
				pkt.item_color = pItem->m_cItemColor;
				pkt.item_effect_value2 = pItem->m_sItemEffectValue2;
				pkt.attribute = pItem->m_dwAttribute;
				pkt.spec_effect_value2 = static_cast<uint8_t>(pItem->m_sItemSpecEffectValue2);
				pkt.item_id = pItem->m_sIDnum;
				pkt.max_lifespan = pItem->m_wMaxLifeSpan;
				iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
			}
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				// . v1.41  .
				// m_pGame->DeleteClient(iClientH, true, true);
				return true;
			}

			return true;
		}

	return false;
}

void ItemManager::CalculateSSN_ItemIndex(int iClientH, short sWeaponIndex, int iValue)
{
	short sSkillIndex;
	int   iOldSSN, iSSNpoint, iWeaponIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sWeaponIndex] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return;

	sSkillIndex = m_pGame->m_pClientList[iClientH]->m_pItemList[sWeaponIndex]->m_sRelatedSkill;
	if ((sSkillIndex < 0) || (sSkillIndex >= hb::shared::limits::MaxSkillType)) return;
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] == 0) return;

	iOldSSN = m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex];
	m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] += iValue;

	iSSNpoint = m_pGame->m_iSkillSSNpoint[m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] + 1];

	if ((m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] < 100) &&
		(m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] > iSSNpoint)) {

		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]++;

		switch (sSkillIndex) {
		case 0:  // Mining
		case 5:  // Hand-Attack
		case 13: // Manufacturing
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iStr + m_pGame->m_pClientList[iClientH]->m_iAngelicStr) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 3: // Magic-Resistance
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > (m_pGame->m_pClientList[iClientH]->m_iLevel * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 4:  // Magic
		case 18: // Crafting
		case 21: // Staff-Attack
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 1:  // Fishing
		case 6:  // Archery
		case 7:  // Short-Sword
		case 8:  // Long-Sword
		case 9:  // Fencing 
		case 10: // Axe-Attack
		case 11: // Shield        	
		case 14: // Hammer 
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iDex + m_pGame->m_pClientList[iClientH]->m_iAngelicDex) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 2:	 // Farming
		case 12: // Alchemy
		case 15:
		case 19: // Pretend-Corpse
		case 20: // Enchanting
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 23: // Poison-Resistance
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > (m_pGame->m_pClientList[iClientH]->m_iVit * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		default:
			m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;
		}

		if (m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] == 0) {
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1) {
				iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sSkillIndex) {
					m_pGame->m_pClientList[iClientH]->m_iHitRatio++;
				}
			}

			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1) {
				iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sSkillIndex) {
					// Mace    .  1 .
					m_pGame->m_pClientList[iClientH]->m_iHitRatio++;
				}
			}
		}

		if (m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] == 0) {
			// SKill  600     1 .
			m_pGame->m_pSkillManager->bCheckTotalSkillMasteryPoints(iClientH, sSkillIndex);
			// Skill    .
			m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, sSkillIndex, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex], 0, 0);
		}
	}
}

int ItemManager::_iGetArrowItemIndex(int iClientH)
{
	

	if (m_pGame->m_pClientList[iClientH] == 0) return -1;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {

			// Arrow  1     .
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->GetItemType() == ItemType::Arrow) &&
				(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_dwCount > 0))
				return i;
		}

	return -1;
}

void ItemManager::ItemDepleteHandler(int iClientH, short sItemIndex, bool bIsUseItemResult)
{

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;

	_bItemLog(ItemLogAction::Deplete, iClientH, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);

	ReleaseItemHandler(iClientH, sItemIndex, true);

	m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemDepletedEraseItem, sItemIndex, (int)bIsUseItemResult, 0, 0);

	delete m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex];
	m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] = 0;

	m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex] = false;

	// !!! BUG POINT
	// . ArrowIndex     .
	m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);

	m_pGame->iCalcTotalWeight(iClientH);
}

void ItemManager::UseItemHandler(int iClientH, short sItemIndex, short dX, short dY, short sDestItemID)
{
	int iTemp, iMax, iV1, iV2, iV3, iSEV1, iEffectResult = 0;
	uint32_t dwTime;
	short sTemp, sTmpType;
	char cSlateType[20];

	dwTime = GameClock::GetTimeMS();
	std::memset(cSlateType, 0, sizeof(cSlateType));

	//testcode
	//std::snprintf(G_cTxt, sizeof(G_cTxt), "%d", sDestItemID);
	//PutLogList(G_cTxt);

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseDeplete) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UsePerm) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Eat) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseSkill) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseDepleteDest)) {
	}
	else return;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseDeplete) ||
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Eat)) {

		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
		case ItemEffectType::Warm:

			if (m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 1) {
				//	m_pGame->m_pStatusEffectManager->SetIceFlag(iClientH, hb::shared::owner_class::Player, false);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(iClientH, hb::shared::owner_class::Player, hb::shared::magic::Ice);

				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (1 * 1000),
					iClientH, hb::shared::owner_class::Player, 0, 0, 0, 1, 0, 0);

				//				m_pGame->SendNotifyMsg(0, iClientH, Notify::MagicEffectOff, hb::shared::magic::Ice, 0, 0, 0);
			}

			m_pGame->m_pClientList[iClientH]->m_dwWarmEffectTime = dwTime;
			break;

		case ItemEffectType::Lottery:
			// EV1(:  100) EV2( ) EV3( )
			iTemp = m_pGame->iDice(1, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1);
			if (iTemp == m_pGame->iDice(1, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1)) {

			}
			else {

			}
			break;

		case ItemEffectType::Slates:
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] != 0) {
				// Full Ancient Slate ??
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum == 867) {
					// Slates dont work on Heldenian Map
					switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2) {
					case 2: // Bezerk slate
						m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] = true;
						m_pGame->m_pStatusEffectManager->SetBerserkFlag(iClientH, hb::shared::owner_class::Player, true);
						m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Berserk, dwTime + (1000 * 600),
							iClientH, hb::shared::owner_class::Player, 0, 0, 0, 1, 0, 0);
						m_pGame->SendNotifyMsg(0, iClientH, Notify::MagicEffectOn, hb::shared::magic::Berserk, 1, 0, 0);
						strcpy(cSlateType, "Berserk");
						break;

					case 1: // Invincible slate
						if (strlen(cSlateType) == 0) {
							strcpy(cSlateType, "Invincible");
						}
					case 3: // Mana slate
						if (strlen(cSlateType) == 0) {
							strcpy(cSlateType, "Mana");
						}
					case 4: // Exp slate
						if (strlen(cSlateType) == 0) {
							strcpy(cSlateType, "Exp");
						}
						SetSlateFlag(iClientH, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2, true);
						m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::AncientTablet, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2,
							dwTime + (1000 * 600), iClientH, hb::shared::owner_class::Player, 0, 0, 0, 1, 0, 0);
						switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2) {
						case 1:
							iEffectResult = 4;
							break;
						case 3:
							iEffectResult = 5;
							break;
						case 4:
							iEffectResult = 6;
							break;
						}
					}
					//if (strlen(cSlateType) > 0)
					//	_bItemLog(ItemLogAction::Use, iClientH, strlen(cSlateType), m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]);
				}
			}
			break;
		case ItemEffectType::HP:
			iMax = m_pGame->iGetMaxHP(iClientH);
			if (m_pGame->m_pClientList[iClientH]->m_iHP < iMax) {

				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1 == 0) {
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3;
				}
				else {
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue3;
				}

				m_pGame->m_pClientList[iClientH]->m_iHP += (m_pGame->iDice(iV1, iV2) + iV3);
				if (m_pGame->m_pClientList[iClientH]->m_iHP > iMax) m_pGame->m_pClientList[iClientH]->m_iHP = iMax;
				if (m_pGame->m_pClientList[iClientH]->m_iHP <= 0)   m_pGame->m_pClientList[iClientH]->m_iHP = 1;

				iEffectResult = 1;
			}
			break;

		case ItemEffectType::MP:
			iMax = m_pGame->iGetMaxMP(iClientH);

			if (m_pGame->m_pClientList[iClientH]->m_iMP < iMax) {

				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1 == 0) {
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3;
				}
				else
				{
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue3;
				}

				m_pGame->m_pClientList[iClientH]->m_iMP += (m_pGame->iDice(iV1, iV2) + iV3);
				if (m_pGame->m_pClientList[iClientH]->m_iMP > iMax)
					m_pGame->m_pClientList[iClientH]->m_iMP = iMax;

				iEffectResult = 2;
			}
			break;
		case ItemEffectType::CritKomm:
			//CritInc(iClientH);
			break;
		case ItemEffectType::SP:
			iMax = m_pGame->iGetMaxSP(iClientH);

			if (m_pGame->m_pClientList[iClientH]->m_iSP < iMax) {

				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1 == 0) {
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3;
				}
				else {
					iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1;
					iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;
					iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue3;
				}

				m_pGame->m_pClientList[iClientH]->m_iSP += (m_pGame->iDice(iV1, iV2) + iV3);
				if (m_pGame->m_pClientList[iClientH]->m_iSP > iMax)
					m_pGame->m_pClientList[iClientH]->m_iSP = iMax;

				iEffectResult = 3;
			}

			if (m_pGame->m_pClientList[iClientH]->m_bIsPoisoned) {
				m_pGame->m_pClientList[iClientH]->m_bIsPoisoned = false;
				m_pGame->m_pStatusEffectManager->SetPoisonFlag(iClientH, hb::shared::owner_class::Player, false); // removes poison aura when using a revitalizing potion
				m_pGame->SendNotifyMsg(0, iClientH, Notify::MagicEffectOff, hb::shared::magic::Poison, 0, 0, 0);
			}
			break;

		case ItemEffectType::HPStock:
			iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
			iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
			iV3 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3;

			m_pGame->m_pClientList[iClientH]->m_iHPstock += m_pGame->iDice(iV1, iV2) + iV3;
			if (m_pGame->m_pClientList[iClientH]->m_iHPstock < 0)   m_pGame->m_pClientList[iClientH]->m_iHPstock = 0;
			if (m_pGame->m_pClientList[iClientH]->m_iHPstock > 500) m_pGame->m_pClientList[iClientH]->m_iHPstock = 500;

			m_pGame->m_pClientList[iClientH]->m_iHungerStatus += m_pGame->iDice(iV1, iV2) + iV3;
			if (m_pGame->m_pClientList[iClientH]->m_iHungerStatus > 100) m_pGame->m_pClientList[iClientH]->m_iHungerStatus = 100;
			if (m_pGame->m_pClientList[iClientH]->m_iHungerStatus < 0)   m_pGame->m_pClientList[iClientH]->m_iHungerStatus = 0;
			break;

		case ItemEffectType::StudySkill:
			iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
			iV2 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
			iSEV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue1;
			// iV1  Skill . iV2  , iSEV1    ()
			if (iSEV1 == 0) {
				m_pGame->m_pSkillManager->TrainSkillResponse(true, iClientH, iV1, iV2);
			}
			else {
				m_pGame->m_pSkillManager->TrainSkillResponse(true, iClientH, iV1, iSEV1);
			}
			break;

		case ItemEffectType::StudyMagic:
			// iV1   .
			iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
			if (m_pGame->m_pMagicConfigList[iV1] != 0)
				m_pGame->m_pMagicManager->RequestStudyMagicHandler(iClientH, m_pGame->m_pMagicConfigList[iV1]->m_cName, false);
			break;

			/*case ItemEffectType::Lottery:
				iLottery = m_pGame->iDice(1, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->
				break;*/

				// New 15/05/2004 Changed
		case ItemEffectType::Magic:
			if (m_pGame->m_pClientList[iClientH]->m_status.bInvisibility) {
				m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(iClientH, hb::shared::owner_class::Player, false);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(iClientH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
				m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
			}

			switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1) {
			case 1:
				// Recall    .
				// testcode
				m_pGame->RequestTeleportHandler(iClientH, "1   ");
				break;

			case 2:
				m_pGame->m_pMagicManager->PlayerMagicHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, 32, true);
				break;

			case 3:
				if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsFightZone == false)
					m_pGame->m_pMagicManager->PlayerMagicHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, 34, true);
				break;

			case 4:
				// fixed location teleportation:
				switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2) {
				case 1:
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "bisle", 5) != 0) {
						//v1.42
						ItemDepleteHandler(iClientH, sItemIndex, true);
						m_pGame->RequestTeleportHandler(iClientH, "2   ", "bisle", -1, -1);
					}
					break;
				case 2: //lotery
					ItemDepleteHandler(iClientH, sItemIndex, true);
					m_pGame->LoteryHandler(iClientH);
					break;

				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
					hb::time::local_time SysTime{};

					SysTime = hb::time::local_time::now();
					if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sTouchEffectValue1 != SysTime.month) ||
						(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sTouchEffectValue2 != SysTime.day) ||
						(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sTouchEffectValue3 <= SysTime.hour)) {
					}
					else {
						char cDestMapName[hb::shared::limits::MapNameLen]{};
						int zoneNum = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2 - 10;
						std::snprintf(cDestMapName, sizeof(cDestMapName), "fightzone%d", zoneNum % 10);
						if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, cDestMapName, 10) != 0) {
							//v1.42
							ItemDepleteHandler(iClientH, sItemIndex, true);
							m_pGame->RequestTeleportHandler(iClientH, "2   ", cDestMapName, -1, -1);
						}
					}
					break;
				}
				break;

			case 5:
				m_pGame->m_pMagicManager->PlayerMagicHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, 31, true,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2);
				break;
			}
			break;

		case ItemEffectType::FirmStamina:
			m_pGame->m_pClientList[iClientH]->m_iTimeLeft_FirmStaminar += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
			if (m_pGame->m_pClientList[iClientH]->m_iTimeLeft_FirmStaminar > 20 * 30) m_pGame->m_pClientList[iClientH]->m_iTimeLeft_FirmStaminar = 20 * 30;
			break;

		case ItemEffectType::ChangeAttr:
			switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1) {
			case 1:
				m_pGame->m_pClientList[iClientH]->m_cHairColor++;
				if (m_pGame->m_pClientList[iClientH]->m_cHairColor > 15) m_pGame->m_pClientList[iClientH]->m_cHairColor = 0;

				m_pGame->m_pClientList[iClientH]->m_appearance.iHairStyle = m_pGame->m_pClientList[iClientH]->m_cHairStyle;
				m_pGame->m_pClientList[iClientH]->m_appearance.iHairColor = m_pGame->m_pClientList[iClientH]->m_cHairColor;
				m_pGame->m_pClientList[iClientH]->m_appearance.iUnderwearType = m_pGame->m_pClientList[iClientH]->m_cUnderwear;
				break;

			case 2:
				m_pGame->m_pClientList[iClientH]->m_cHairStyle++;
				if (m_pGame->m_pClientList[iClientH]->m_cHairStyle > 7) m_pGame->m_pClientList[iClientH]->m_cHairStyle = 0;

				m_pGame->m_pClientList[iClientH]->m_appearance.iHairStyle = m_pGame->m_pClientList[iClientH]->m_cHairStyle;
				m_pGame->m_pClientList[iClientH]->m_appearance.iHairColor = m_pGame->m_pClientList[iClientH]->m_cHairColor;
				m_pGame->m_pClientList[iClientH]->m_appearance.iUnderwearType = m_pGame->m_pClientList[iClientH]->m_cUnderwear;
				break;

			case 3:
				// Appearance , .
				m_pGame->m_pClientList[iClientH]->m_cSkin++;
				if (m_pGame->m_pClientList[iClientH]->m_cSkin > 3)
					m_pGame->m_pClientList[iClientH]->m_cSkin = 1;

				if (m_pGame->m_pClientList[iClientH]->m_cSex == 1)      sTemp = 1;
				else if (m_pGame->m_pClientList[iClientH]->m_cSex == 2) sTemp = 4;

				switch (m_pGame->m_pClientList[iClientH]->m_cSkin) {
				case 2:	sTemp += 1; break;
				case 3:	sTemp += 2; break;
				}
				m_pGame->m_pClientList[iClientH]->m_sType = sTemp;
				break;

			case 4:
				sTemp = m_pGame->m_pClientList[iClientH]->m_appearance.iHelmType;
				if (sTemp == 0) {
					// sTemp 0  , ,     .    .
					if (m_pGame->m_pClientList[iClientH]->m_cSex == 1)
						m_pGame->m_pClientList[iClientH]->m_cSex = 2;
					else m_pGame->m_pClientList[iClientH]->m_cSex = 1;

					// Appearance , .
					if (m_pGame->m_pClientList[iClientH]->m_cSex == 1) {
						sTmpType = 1;
					}
					else if (m_pGame->m_pClientList[iClientH]->m_cSex == 2) {
						sTmpType = 4;
					}

					switch (m_pGame->m_pClientList[iClientH]->m_cSkin) {
					case 1:
						break;
					case 2:
						sTmpType += 1;
						break;
					case 3:
						sTmpType += 2;
						break;
					}

					m_pGame->m_pClientList[iClientH]->m_sType = sTmpType;
					m_pGame->m_pClientList[iClientH]->m_appearance.iHairStyle = m_pGame->m_pClientList[iClientH]->m_cHairStyle;
					m_pGame->m_pClientList[iClientH]->m_appearance.iHairColor = m_pGame->m_pClientList[iClientH]->m_cHairColor;
					m_pGame->m_pClientList[iClientH]->m_appearance.iUnderwearType = m_pGame->m_pClientList[iClientH]->m_cUnderwear;
				}
				break;
			}

			m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
			break;
		}
		// *** Request Teleport Handler           .
		ItemDepleteHandler(iClientH, sItemIndex, true);

		switch (iEffectResult) {
		case 1:
			m_pGame->SendNotifyMsg(0, iClientH, Notify::Hp, 0, 0, 0, 0);
			break;
		case 2:
			m_pGame->SendNotifyMsg(0, iClientH, Notify::Mp, 0, 0, 0, 0);
			break;
		case 3:
			m_pGame->SendNotifyMsg(0, iClientH, Notify::Sp, 0, 0, 0, 0);
			break;
		case 4: // Invincible
			m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateInvincible, 0, 0, 0, 0);
			break;
		case 5: // Mana
			m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateMana, 0, 0, 0, 0);
			break;
		case 6: // EXP
			m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateExp, 0, 0, 0, 0);
			break;
		default:
			break;
		}
	}
	else if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseDepleteDest) {
		// dX, dY       .
		if (_bDepleteDestTypeItemUseEffect(iClientH, dX, dY, sItemIndex, sDestItemID))
			ItemDepleteHandler(iClientH, sItemIndex, true);
	}
	else if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::Arrow) {
		m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);
	}
	else if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UsePerm) {
		// .     . (ex: )
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
		case ItemEffectType::ShowLocation:
			iV1 = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
			switch (iV1) {
			case 1:
				if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "aresden") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 1, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elvine") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 2, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "middleland") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 3, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "default") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 4, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "huntzone2") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 5, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "huntzone1") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 6, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "huntzone4") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 7, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "huntzone3") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 8, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "arefarm") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 9, 0, 0);
				else if (strcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elvfarm") == 0)
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 10, 0, 0);
				else m_pGame->SendNotifyMsg(0, iClientH, Notify::ShowMap, iV1, 0, 0, 0);
				break;
			}
			break;
		}
	}
	else if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemType() == ItemType::UseSkill) {

		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) ||
			(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan <= 0) ||
			(m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill])) {
			return;
		}
		else {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wMaxLifeSpan != 0) {
				m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan--;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::CurLifeSpan, sItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan, 0, 0);
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan <= 0) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemLifeSpanEnd, ToInt(EquipPos::None), sItemIndex, 0, 0);
				}
				else {
					// ID . v1.12
					int iSkillUsingTimeID = (int)GameClock::GetTimeMS();

					m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::UseItemSkill, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill,
						dwTime + m_pGame->m_pSkillConfigList[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill]->m_sValue2 * 1000,
						iClientH, hb::shared::owner_class::Player, m_pGame->m_pClientList[iClientH]->m_cMapIndex, dX, dY,
						m_pGame->m_pClientList[iClientH]->m_cSkillMastery[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill], iSkillUsingTimeID, 0);

					m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill] = true;
					m_pGame->m_pClientList[iClientH]->m_iSkillUsingTimeID[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill] = iSkillUsingTimeID; //v1.12
				}
			}
		}
	}
}

bool ItemManager::bSetItemToBankItem(int iClientH, CItem* pItem)
{
	int iRet;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (pItem == 0) return false;
	//wh remove
	//if (m_pGame->m_pClientList[iClientH]->m_bIsInsideWarehouse == false) return false;

	for(int i = 0; i < hb::shared::limits::MaxBankItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemInBankList[i] == 0) {

			m_pGame->m_pClientList[iClientH]->m_pItemInBankList[i] = pItem;

			{
				hb::net::PacketNotifyItemToBank pkt{};
				pkt.header.msg_id = MsgId::Notify;
				pkt.header.msg_type = Notify::ItemToBank;
				pkt.bank_index = static_cast<uint8_t>(i);
				pkt.is_new = 1;
				memcpy(pkt.name, pItem->m_cName, sizeof(pkt.name));
				pkt.count = pItem->m_dwCount;
				pkt.item_type = pItem->m_cItemType;
				pkt.equip_pos = pItem->m_cEquipPos;
				pkt.is_equipped = 0;
				pkt.level_limit = pItem->m_sLevelLimit;
				pkt.gender_limit = pItem->m_cGenderLimit;
				pkt.cur_lifespan = pItem->m_wCurLifeSpan;
				pkt.weight = pItem->m_wWeight;
				pkt.sprite = pItem->m_sSprite;
				pkt.sprite_frame = pItem->m_sSpriteFrame;
				pkt.item_color = pItem->m_cItemColor;
				pkt.item_effect_value2 = pItem->m_sItemEffectValue2;
				pkt.attribute = pItem->m_dwAttribute;
				pkt.spec_effect_value2 = static_cast<uint8_t>(pItem->m_sItemSpecEffectValue2);
				pkt.item_id = pItem->m_sIDnum;
				pkt.max_lifespan = pItem->m_wMaxLifeSpan;
				iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
			}
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				// . v1.41  .
				// m_pGame->DeleteClient(iClientH, true, true);
				return true;
			}

			return true;
		}

	return false;
}

int ItemManager::iCalculateUseSkillItemEffect(int iOwnerH, char cOwnerType, char cOwnerSkill, int iSkillNum, char cMapIndex, int dX, int dY)
{
	CItem* pItem;
	char  cItemName[hb::shared::limits::ItemNameLen];
	short lX, lY;
	int   iResult, iFish;

	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[iOwnerH] == 0) return 0;
		if (m_pGame->m_pClientList[iOwnerH]->m_cMapIndex != cMapIndex) return 0;
		lX = m_pGame->m_pClientList[iOwnerH]->m_sX;
		lY = m_pGame->m_pClientList[iOwnerH]->m_sY;
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[iOwnerH] == 0) return 0;
		if (m_pGame->m_pNpcList[iOwnerH]->m_cMapIndex != cMapIndex) return 0;
		lX = m_pGame->m_pNpcList[iOwnerH]->m_sX;
		lY = m_pGame->m_pNpcList[iOwnerH]->m_sY;
		break;
	}

	if (cOwnerSkill == 0) return 0;

	// 100       1D105
	iResult = m_pGame->iDice(1, 105);
	if (cOwnerSkill <= iResult)	return 0;

	if (m_pGame->m_pMapList[cMapIndex]->bGetIsWater(dX, dY) == false) return 0;

	if (cOwnerType == hb::shared::owner_class::Player)
		m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iOwnerH, iSkillNum, 1);

	switch (m_pGame->m_pSkillConfigList[iSkillNum]->m_sType) {
	case EffectType::Taming:
		// : dX, dY   .
		m_pGame->m_pSkillManager->_TamingHandler(iOwnerH, iSkillNum, cMapIndex, dX, dY);
		break;

	case EffectType::Get:
		std::memset(cItemName, 0, sizeof(cItemName));
		bool bIsFish = false;
		switch (m_pGame->m_pSkillConfigList[iSkillNum]->m_sValue1) {
		case 1:
			std::snprintf(cItemName, sizeof(cItemName), "Meat");
			break;

		case 2:
			if (cOwnerType == hb::shared::owner_class::Player) {
				iFish = m_pGame->m_pFishingManager->iCheckFish(iOwnerH, cMapIndex, dX, dY);
				if (iFish == 0) {
					std::snprintf(cItemName, sizeof(cItemName), "Fish");
					bIsFish = true;
				}
			}
			else {
				std::snprintf(cItemName, sizeof(cItemName), "Fish");
				bIsFish = true;
			}
			break;
		}

		if (strlen(cItemName) != 0) {

			if (bIsFish) {
				m_pGame->SendNotifyMsg(0, iOwnerH, Notify::FishSuccess, 0, 0, 0, 0);
				m_pGame->m_pClientList[iOwnerH]->m_iExpStock += m_pGame->iDice(1, 2);
			}

			pItem = new CItem;
			if (pItem == 0) return 0;
			if (_bInitItemAttr(pItem, cItemName)) {
				m_pGame->m_pMapList[cMapIndex]->bSetItem(lX, lY, pItem);

				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, cMapIndex,
					lX, lY, pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); //v1.4
			}
		}
		break;
	}

	return 1;
}

void ItemManager::ReqSellItemHandler(int iClientH, char cItemID, char cSellToWhom, int iNum, const char* pItemName)
{
	char cItemCategory;
	short sRemainLife;
	int   iPrice;
	double d1, d2, d3;
	bool   bNeutral;
	uint32_t  dwSWEType, dwSWEValue, dwAddPrice1, dwAddPrice2, dwMul1, dwMul2;
	CItem* m_pGold;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((cItemID < 0) || (cItemID >= 50)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID] == 0) return;
	if (iNum <= 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwCount < static_cast<uint32_t>(iNum)) return;

	m_pGame->iCalcTotalWeight(iClientH);

	m_pGold = new CItem;
	_bInitItemAttr(m_pGold, hb::shared::item::ItemId::Gold);

	// v1.42
	bNeutral = false;
	if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) bNeutral = true;
	switch (cSellToWhom) {
	case 15:
	case 24:
		cItemCategory = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cCategory;
		// 12-22
		if ((cItemCategory >= 11) && (cItemCategory <= 50)) {

			iPrice = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2) * iNum;
			sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;

			if (bNeutral) iPrice = iPrice / 2;
			if (iPrice <= 0)    iPrice = 1;
			if (iPrice > 1000000) iPrice = 1000000;

			if (m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(m_pGold, iPrice) > m_pGame->_iCalcMaxLoad(iClientH)) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotSellItem, cItemID, 4, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
			}
			else m_pGame->SendNotifyMsg(0, iClientH, Notify::SellItemPrice, cItemID, sRemainLife, iPrice, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName, iNum);
		}
		else if ((cItemCategory >= 1) && (cItemCategory <= 10)) {
			sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;

			if (sRemainLife == 0) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotSellItem, cItemID, 2, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
			}
			else {
				d1 = (double)sRemainLife;
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan != 0)
					d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan;
				else d2 = 1.0f;
				d3 = (d1 / d2) * 0.5f;
				d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice;
				d3 = d3 * d2;

				iPrice = (int)d3;
				iPrice = iPrice * iNum;

				dwAddPrice1 = 0;
				dwAddPrice2 = 0;
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00F00000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00F00000) >> 20;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x000F0000) >> 16;

					switch (dwSWEType) {
					case 6: dwMul1 = 2; break;
					case 8: dwMul1 = 2; break;
					case 5: dwMul1 = 3; break;
					case 1: dwMul1 = 4; break;
					case 7: dwMul1 = 5; break;
					case 2: dwMul1 = 6; break;
					case 3: dwMul1 = 15; break;
					case 9: dwMul1 = 20; break;
					default: dwMul1 = 1; break;
					}

					d1 = (double)iPrice * dwMul1;
					switch (dwSWEValue) {
					case 1: d2 = 10.0f; break;
					case 2: d2 = 20.0f; break;
					case 3: d2 = 30.0f; break;
					case 4: d2 = 35.0f; break;
					case 5: d2 = 40.0f; break;
					case 6: d2 = 50.0f; break;
					case 7: d2 = 100.0f; break;
					case 8: d2 = 200.0f; break;
					case 9: d2 = 300.0f; break;
					case 10: d2 = 400.0f; break;
					case 11: d2 = 500.0f; break;
					case 12: d2 = 700.0f; break;
					case 13: d2 = 900.0f; break;
					default: d2 = 0.0f; break;
					}
					d3 = d1 * (d2 / 100.0f);

					dwAddPrice1 = (int)(d1 + d3);
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x0000F000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x0000F000) >> 12;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00000F00) >> 8;

					switch (dwSWEType) {
					case 1:
					case 12: dwMul2 = 2; break;

					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7: dwMul2 = 4; break;

					case 8:
					case 9:
					case 10:
					case 11: dwMul2 = 6; break;
					}

					d1 = (double)iPrice * dwMul2;
					switch (dwSWEValue) {
					case 1: d2 = 10.0f; break;
					case 2: d2 = 20.0f; break;
					case 3: d2 = 30.0f; break;
					case 4: d2 = 35.0f; break;
					case 5: d2 = 40.0f; break;
					case 6: d2 = 50.0f; break;
					case 7: d2 = 100.0f; break;
					case 8: d2 = 200.0f; break;
					case 9: d2 = 300.0f; break;
					case 10: d2 = 400.0f; break;
					case 11: d2 = 500.0f; break;
					case 12: d2 = 700.0f; break;
					case 13: d2 = 900.0f; break;
					default: d2 = 0.0f; break;
					}
					d3 = d1 * (d2 / 100.0f);

					dwAddPrice2 = (int)(d1 + d3);
				}

				iPrice = iPrice + (dwAddPrice1 - (dwAddPrice1 / 3)) + (dwAddPrice2 - (dwAddPrice2 / 3));

				if (bNeutral) iPrice = iPrice / 2;
				if (iPrice <= 0)    iPrice = 1;
				if (iPrice > 1000000) iPrice = 1000000;

				if (m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(m_pGold, iPrice) > m_pGame->_iCalcMaxLoad(iClientH)) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotSellItem, cItemID, 4, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
				}
				else m_pGame->SendNotifyMsg(0, iClientH, Notify::SellItemPrice, cItemID, sRemainLife, iPrice, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName, iNum);
			}
		}
		else m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotSellItem, cItemID, 1, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
		break;

	default:
		break;
	}
	if (m_pGold != 0) delete m_pGold;
}

void ItemManager::ReqSellItemConfirmHandler(int iClientH, char cItemID, int iNum, const char* pString)
{
	CItem* pItemGold;
	short sRemainLife;
	int   iPrice;
	double d1, d2, d3;
	char cItemCategory;
	uint32_t dwMul1, dwMul2, dwSWEType, dwSWEValue, dwAddPrice1, dwAddPrice2;
	int    iEraseReq, iRet;
	bool   bNeutral;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((cItemID < 0) || (cItemID >= 50)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID] == 0) return;
	if (iNum <= 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwCount < static_cast<uint32_t>(iNum)) return;

	// New 18/05/2004
	if (m_pGame->m_pClientList[iClientH]->m_pIsProcessingAllowed == false) return;

	m_pGame->iCalcTotalWeight(iClientH);
	cItemCategory = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cCategory;

	// v1.42
	bNeutral = false;
	if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) bNeutral = true;

	iPrice = 0;
	if ((cItemCategory >= 1) && (cItemCategory <= 10)) {
		sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;

		if (sRemainLife <= 0) {
			return;
		}
		else {
			d1 = (double)sRemainLife;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan != 0)
				d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan;
			else d2 = 1.0f;
			d3 = (d1 / d2) * 0.5f;
			d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice;
			d3 = d3 * d2;

			iPrice = (short)d3;
			iPrice = iPrice * iNum;

			dwAddPrice1 = 0;
			dwAddPrice2 = 0;
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00F00000) != 0) {
				dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00F00000) >> 20;
				dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x000F0000) >> 16;

				// 0-None 1- 2- 3- 4-
				// 5- 6- 7- 8- 9-
				switch (dwSWEType) {
				case 6: dwMul1 = 2; break;
				case 8: dwMul1 = 2; break;
				case 5: dwMul1 = 3; break;
				case 1: dwMul1 = 4; break;
				case 7: dwMul1 = 5; break;
				case 2: dwMul1 = 6; break;
				case 3: dwMul1 = 15; break;
				case 9: dwMul1 = 20; break;
				default: dwMul1 = 1; break;
				}

				d1 = (double)iPrice * dwMul1;
				switch (dwSWEValue) {
				case 1: d2 = 10.0f; break;
				case 2: d2 = 20.0f; break;
				case 3: d2 = 30.0f; break;
				case 4: d2 = 35.0f; break;
				case 5: d2 = 40.0f; break;
				case 6: d2 = 50.0f; break;
				case 7: d2 = 100.0f; break;
				case 8: d2 = 200.0f; break;
				case 9: d2 = 300.0f; break;
				case 10: d2 = 400.0f; break;
				case 11: d2 = 500.0f; break;
				case 12: d2 = 700.0f; break;
				case 13: d2 = 900.0f; break;
				default: d2 = 0.0f; break;
				}
				d3 = d1 * (d2 / 100.0f);
				dwAddPrice1 = (int)(d1 + d3);
			}

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x0000F000) != 0) {
				dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x0000F000) >> 12;
				dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwAttribute & 0x00000F00) >> 8;

				// (1),  (2),  (3), HP  (4), SP  (5)
				// MP  (6),  (7),   (8),   (9)
				// (10),   (11),  Gold(12)
				switch (dwSWEType) {
				case 1:
				case 12: dwMul2 = 2; break;

				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7: dwMul2 = 4; break;

				case 8:
				case 9:
				case 10:
				case 11: dwMul2 = 6; break;
				}

				d1 = (double)iPrice * dwMul2;
				switch (dwSWEValue) {
				case 1: d2 = 10.0f; break;
				case 2: d2 = 20.0f; break;
				case 3: d2 = 30.0f; break;
				case 4: d2 = 35.0f; break;
				case 5: d2 = 40.0f; break;
				case 6: d2 = 50.0f; break;
				case 7: d2 = 100.0f; break;
				case 8: d2 = 200.0f; break;
				case 9: d2 = 300.0f; break;
				case 10: d2 = 400.0f; break;
				case 11: d2 = 500.0f; break;
				case 12: d2 = 700.0f; break;
				case 13: d2 = 900.0f; break;
				default: d2 = 0.0f; break;
				}
				d3 = d1 * (d2 / 100.0f);
				dwAddPrice2 = (int)(d1 + d3);
			}

			iPrice = iPrice + (dwAddPrice1 - (dwAddPrice1 / 3)) + (dwAddPrice2 - (dwAddPrice2 / 3));

			if (bNeutral) iPrice = iPrice / 2;
			if (iPrice <= 0) iPrice = 1;
			if (iPrice > 1000000) iPrice = 1000000; // New 06/05/2004

			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemSold, cItemID, 0, 0, 0);

			_bItemLog(ItemLogAction::Sell, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]);

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->GetItemType() == ItemType::Consume) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->GetItemType() == ItemType::Arrow)) {
				// v1.41 !!!
				SetItemCount(iClientH, cItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwCount - iNum);
			}
			else ItemDepleteHandler(iClientH, cItemID, false);
		}
	}
	else
		if ((cItemCategory >= 11) && (cItemCategory <= 50)) {
			iPrice = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2;
			iPrice = iPrice * iNum;

			if (bNeutral) iPrice = iPrice / 2;
			if (iPrice <= 0) iPrice = 1;
			if (iPrice > 1000000) iPrice = 1000000; // New 06/05/2004

			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemSold, cItemID, 0, 0, 0);

			_bItemLog(ItemLogAction::Sell, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]);

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->GetItemType() == ItemType::Consume) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->GetItemType() == ItemType::Arrow)) {
				// v1.41 !!!
				SetItemCount(iClientH, cItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_dwCount - iNum);
			}
			else ItemDepleteHandler(iClientH, cItemID, false);
		}

	// Gold .    0     .
	if (iPrice <= 0) return;

	pItemGold = new CItem;
	_bInitItemAttr(pItemGold, hb::shared::item::ItemId::Gold);
	pItemGold->m_dwCount = iPrice;

	if (_bAddClientItemList(iClientH, pItemGold, &iEraseReq)) {

		iRet = SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItemGold, 0);

		m_pGame->iCalcTotalWeight(iClientH);

		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			m_pGame->DeleteClient(iClientH, true, true);
			break;
		}
	}
	else {
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
			m_pGame->m_pClientList[iClientH]->m_sY, pItemGold);

		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
			m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
			pItemGold->m_sIDnum, 0, pItemGold->m_cItemColor, pItemGold->m_dwAttribute); // v1.4 color

		m_pGame->iCalcTotalWeight(iClientH);

		iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			m_pGame->DeleteClient(iClientH, true, true);
			return;
		}
	}
}

void ItemManager::ReqRepairItemHandler(int iClientH, char cItemID, char cRepairWhom, const char* pString)
{
	char cItemCategory;
	short sRemainLife, sPrice;
	double d1, d2, d3;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((cItemID < 0) || (cItemID >= 50)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID] == 0) return;

	cItemCategory = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cCategory;

	if ((cItemCategory >= 1) && (cItemCategory <= 10)) {

		if (cRepairWhom != 24) {
			m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotRepairItem, cItemID, 2, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
			return;
		}

		sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;
		if (sRemainLife == 0) {
			sPrice = static_cast<short>(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2);
		}
		else {
			d1 = (double)sRemainLife;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan != 0)
				d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan;
			else d2 = 1.0f;
			d3 = (d1 / d2) * 0.5f;
			d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice;
			d3 = d3 * d2;

			sPrice = static_cast<short>((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2) - d3);
		}

		m_pGame->SendNotifyMsg(0, iClientH, Notify::RepairItemPrice, cItemID, sRemainLife, sPrice, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
	}
	else if (((cItemCategory >= 43) && (cItemCategory <= 50)) || ((cItemCategory >= 11) && (cItemCategory <= 12))) {

		if (cRepairWhom != 15) {
			m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotRepairItem, cItemID, 2, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
			return;
		}

		sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;
		if (sRemainLife == 0) {
			sPrice = static_cast<short>(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2);
		}
		else {
			d1 = (double)sRemainLife;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan != 0)
				d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan;
			else d2 = 1.0f;
			d3 = (d1 / d2) * 0.5f;
			d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice;
			d3 = d3 * d2;

			sPrice = static_cast<short>((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2) - d3);
		}

		m_pGame->SendNotifyMsg(0, iClientH, Notify::RepairItemPrice, cItemID, sRemainLife, sPrice, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
	}
	else {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::CannotRepairItem, cItemID, 1, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cName);
	}
}

void ItemManager::ReqRepairItemCofirmHandler(int iClientH, char cItemID, const char* pString)
{
	short    sRemainLife, sPrice;
	char cItemCategory;
	double   d1, d2, d3;
	uint32_t dwGoldCount;
	int      iRet, iGoldWeight;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	if ((cItemID < 0) || (cItemID >= 50)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID] == 0) return;

	// New 18/05/2004
	if (m_pGame->m_pClientList[iClientH]->m_pIsProcessingAllowed == false) return;

	//testcode
	//PutLogList("Repair!");

	cItemCategory = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_cCategory;

	if (((cItemCategory >= 1) && (cItemCategory <= 10)) || ((cItemCategory >= 43) && (cItemCategory <= 50)) ||
		((cItemCategory >= 11) && (cItemCategory <= 12))) {

		sRemainLife = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan;
		if (sRemainLife == 0) {
			sPrice = static_cast<short>(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2);
		}
		else {
			d1 = (double)abs(sRemainLife);
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan != 0)
				d2 = (double)abs(m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan);
			else d2 = 1.0f;
			d3 = (d1 / d2) * 0.5f;
			d2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice;
			d3 = d3 * d2;

			sPrice = static_cast<short>((m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wPrice / 2) - d3);
		}

		// sPrice         .
		dwGoldCount = dwGetItemCountByID(iClientH, hb::shared::item::ItemId::Gold);

		if (dwGoldCount < (uint32_t)sPrice) {
			// Gold     .   .
			{
				hb::net::PacketNotifyNotEnoughGold pkt{};
				pkt.header.msg_id = MsgId::Notify;
				pkt.header.msg_type = Notify::NotEnoughGold;
				pkt.item_index = static_cast<int8_t>(cItemID);
				iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
			}
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return;
			}
			return;
		}
		else {

			// . !BUG POINT  .      .
			m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan = m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wMaxLifeSpan;
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemRepaired, cItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID]->m_wCurLifeSpan, 0, 0);

			iGoldWeight = SetItemCountByID(iClientH, hb::shared::item::ItemId::Gold, dwGoldCount - sPrice);

			m_pGame->iCalcTotalWeight(iClientH);

			m_pGame->m_stCityStatus[m_pGame->m_pClientList[iClientH]->m_cSide].iFunds += sPrice;
		}
	}
	else {
	}
}

void ItemManager::CalcTotalItemEffect(int iClientH, int iEquipItemID, bool bNotify)
{
	short sItemIndex;
	int iArrowIndex, iPrevSAType, iTemp;
	EquipPos cEquipPos;
	double dV1, dV2, dV3;
	uint32_t  dwSWEType, dwSWEValue;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	if ((m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1) &&
		(m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1)) {

		if (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)]] != 0) {
			m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)]] = false;
			m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] = -1;
		}
	}

	m_pGame->m_pClientList[iClientH]->m_iAngelicStr = 0; // By Snoopy81
	m_pGame->m_pClientList[iClientH]->m_iAngelicInt = 0; // By Snoopy81
	m_pGame->m_pClientList[iClientH]->m_iAngelicDex = 0; // By Snoopy81
	m_pGame->m_pClientList[iClientH]->m_iAngelicMag = 0; // By Snoopy81	
	m_pGame->m_pStatusEffectManager->SetAngelFlag(iClientH, hb::shared::owner_class::Player, 0, 0);

	m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM = 0;
	m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM = 0;
	m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM = 0;

	m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L = 0;
	m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L = 0;
	m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L = 0;

	m_pGame->m_pClientList[iClientH]->m_iHitRatio = 0;
	m_pGame->m_pClientList[iClientH]->m_iDefenseRatio = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
	m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Shield = 0;

	for(int i = 0; i < DEF_MAXITEMEQUIPPOS; i++) {
		m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[i] = 0;
	}

	m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddResistMagic = 0;

	m_pGame->m_pClientList[iClientH]->m_iAddPhysicalDamage = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddMagicalDamage = 0;

	m_pGame->m_pClientList[iClientH]->m_bIsLuckyEffect = false;
	m_pGame->m_pClientList[iClientH]->m_iMagicDamageSaveItemIndex = -1;
	m_pGame->m_pClientList[iClientH]->m_iSideEffect_MaxHPdown = 0;

	m_pGame->m_pClientList[iClientH]->m_iAddAbsAir = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddAbsEarth = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddAbsFire = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddAbsWater = 0;

	m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack = 0;
	m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Defense = 0;

	m_pGame->m_pClientList[iClientH]->m_iMinAP_SM = 0;
	m_pGame->m_pClientList[iClientH]->m_iMinAP_L = 0;

	m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM = 0;
	m_pGame->m_pClientList[iClientH]->m_iMaxAP_L = 0;

	m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectType = 0;	// : 0-None 1- 2- 3- 4-
	m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectValue = 0;

	m_pGame->m_pClientList[iClientH]->m_iAddHP = m_pGame->m_pClientList[iClientH]->m_iAddSP = m_pGame->m_pClientList[iClientH]->m_iAddMP = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddAR = m_pGame->m_pClientList[iClientH]->m_iAddPR = m_pGame->m_pClientList[iClientH]->m_iAddDR = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddMR = m_pGame->m_pClientList[iClientH]->m_iAddAbsPD = m_pGame->m_pClientList[iClientH]->m_iAddAbsMD = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddCD = m_pGame->m_pClientList[iClientH]->m_iAddExp = m_pGame->m_pClientList[iClientH]->m_iAddGold = 0;

	iPrevSAType = m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType;

	m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType = 0;
	m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityLastSec = 0;
	m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos = 0;

	m_pGame->m_pClientList[iClientH]->m_iAddTransMana = 0;
	m_pGame->m_pClientList[iClientH]->m_iAddChargeCritical = 0;

	m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex = -1;
	for (sItemIndex = 0; sItemIndex < hb::shared::limits::MaxItems; sItemIndex++)
	{
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] != 0) {
			switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
			case ItemEffectType::AlterItemDrop:
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan > 0) {
					m_pGame->m_pClientList[iClientH]->m_iAlterItemDropIndex = sItemIndex;
				}
				break;
			}
		}
	}

	for (sItemIndex = 0; sItemIndex < hb::shared::limits::MaxItems; sItemIndex++)
	{
		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] != 0) &&
			(m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[sItemIndex])) {

			cEquipPos = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetEquipPos();

			switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {

			case ItemEffectType::MagicDamageSave:
				m_pGame->m_pClientList[iClientH]->m_iMagicDamageSaveItemIndex = sItemIndex;
				break;

			case ItemEffectType::AttackSpecAbility:
			case ItemEffectType::AttackDefense:
			case ItemEffectType::AttackManaSave:
			case ItemEffectType::AttackMaxHPDown:
			case ItemEffectType::Attack:
				m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1);
				m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2);
				m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3);
				m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue4);
				m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5);
				m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue6);

				iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
				//testcode
				//std::snprintf(G_cTxt, sizeof(G_cTxt), "Add Damage: %d", iTemp);
				//PutLogList(G_cTxt);

				m_pGame->m_pClientList[iClientH]->m_iAddPhysicalDamage += iTemp;
				m_pGame->m_pClientList[iClientH]->m_iAddMagicalDamage += iTemp;

				m_pGame->m_pClientList[iClientH]->m_iHitRatio += m_pGame->m_pClientList[iClientH]->m_cSkillMastery[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill];

				//m_pGame->m_pClientList[iClientH]->m_iHitRatio_ItemEffect_SM += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSM_HitRatio;
				//m_pGame->m_pClientList[iClientH]->m_iHitRatio_ItemEffect_L  += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sL_HitRatio;
				m_pGame->m_pClientList[iClientH]->m_sUsingWeaponSkill = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill;

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00000001) != 0) {
					m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;
					if (m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack > 100)
						m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack = 100;

					if (m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack < -100)
						m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack = -100;

					if (m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack > 0) {
						dV2 = (double)m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack;
						dV1 = (dV2 / 100.0f) * (5.0f);
						m_pGame->m_pClientList[iClientH]->m_iMinAP_SM = m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM +
							m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM + (int)dV1;

						m_pGame->m_pClientList[iClientH]->m_iMinAP_L = m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L +
							m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L + (int)dV1;

						if (m_pGame->m_pClientList[iClientH]->m_iMinAP_SM < 1) m_pGame->m_pClientList[iClientH]->m_iMinAP_SM = 1;
						if (m_pGame->m_pClientList[iClientH]->m_iMinAP_L < 1)  m_pGame->m_pClientList[iClientH]->m_iMinAP_L = 1;

						if (m_pGame->m_pClientList[iClientH]->m_iMinAP_SM > (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM))
							m_pGame->m_pClientList[iClientH]->m_iMinAP_SM = (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM);

						if (m_pGame->m_pClientList[iClientH]->m_iMinAP_L > (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L))
							m_pGame->m_pClientList[iClientH]->m_iMinAP_L = (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L);

						//testcode
						//std::snprintf(G_cTxt, sizeof(G_cTxt), "MinAP: %d %d +(%d)", m_pGame->m_pClientList[iClientH]->m_iMinAP_SM, m_pGame->m_pClientList[iClientH]->m_iMinAP_L, (int)dV1);
						//PutLogList(G_cTxt);
					}
					else if (m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack < 0) {
						dV2 = (double)m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Attack;
						dV1 = (dV2 / 100.0f) * (5.0f);
						m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM = m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM
							+ m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM + (int)dV1;

						m_pGame->m_pClientList[iClientH]->m_iMaxAP_L = m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L
							+ m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L + (int)dV1;

						if (m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM < 1) m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM = 1;
						if (m_pGame->m_pClientList[iClientH]->m_iMaxAP_L < 1)  m_pGame->m_pClientList[iClientH]->m_iMaxAP_L = 1;

						if (m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM < (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM))
							m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM = (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM);

						if (m_pGame->m_pClientList[iClientH]->m_iMaxAP_L < (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L))
							m_pGame->m_pClientList[iClientH]->m_iMaxAP_L = (m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L * m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L + m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L);

						//testcode
						//std::snprintf(G_cTxt, sizeof(G_cTxt), "MaxAP: %d %d +(%d)", m_pGame->m_pClientList[iClientH]->m_iMaxAP_SM, m_pGame->m_pClientList[iClientH]->m_iMaxAP_L, (int)dV1);
						//PutLogList(G_cTxt);
					}
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00F00000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00F00000) >> 20;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x000F0000) >> 16;

					// 0-None 1- 2- 3- 4-
					// 5- 6- 7- 8- 9- 10-
					m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectType = (int)dwSWEType;
					m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectValue = (int)dwSWEValue;

					switch (dwSWEType) {
					case 7:
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM++;
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L++;
						break;

					case 9:
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM += 2;
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L += 2;
						break;
					}
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x0000F000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x0000F000) >> 12;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00000F00) >> 8;

					// (1),  (2),  (3), HP  (4), SP  (5)
					// MP  (6),  (7),   (8),   (9)
					// (10),   (11),  Gold(12)

					switch (dwSWEType) {
					case 0:  break;
					case 1:  m_pGame->m_pClientList[iClientH]->m_iAddPR += (int)dwSWEValue * 7; break;
					case 2:  m_pGame->m_pClientList[iClientH]->m_iAddAR += (int)dwSWEValue * 7; break;
					case 3:  m_pGame->m_pClientList[iClientH]->m_iAddDR += (int)dwSWEValue * 7; break;
					case 4:  m_pGame->m_pClientList[iClientH]->m_iAddHP += (int)dwSWEValue * 7; break;
					case 5:  m_pGame->m_pClientList[iClientH]->m_iAddSP += (int)dwSWEValue * 7; break;
					case 6:  m_pGame->m_pClientList[iClientH]->m_iAddMP += (int)dwSWEValue * 7; break;
					case 7:  m_pGame->m_pClientList[iClientH]->m_iAddMR += (int)dwSWEValue * 7; break;
					case 8:  m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos] += (int)dwSWEValue * 3; break;
					case 9:  m_pGame->m_pClientList[iClientH]->m_iAddAbsMD += (int)dwSWEValue * 3; break;
					case 10: m_pGame->m_pClientList[iClientH]->m_iAddCD += (int)dwSWEValue; break;
					case 11: m_pGame->m_pClientList[iClientH]->m_iAddExp += (int)dwSWEValue * 10; break;
					case 12: m_pGame->m_pClientList[iClientH]->m_iAddGold += (int)dwSWEValue * 10; break;
					}

					switch (dwSWEType) {
					case 9: if (m_pGame->m_pClientList[iClientH]->m_iAddAbsMD > 80) m_pGame->m_pClientList[iClientH]->m_iAddAbsMD = 80; break;
					}
				}

				switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
				case ItemEffectType::AttackMaxHPDown:
					m_pGame->m_pClientList[iClientH]->m_iSideEffect_MaxHPdown = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect;
					break;

				case ItemEffectType::AttackManaSave:
					// :    80%
					m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue4;
					if (m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio > 80) m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio = 80;
					break;

				case ItemEffectType::AttackDefense:
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Body)] += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect;
					break;

				case ItemEffectType::AttackSpecAbility:
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect;
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityLastSec = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffectValue1;
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos = ToInt(cEquipPos);

					if ((bNotify) && (iEquipItemID == (int)sItemIndex))
						m_pGame->SendNotifyMsg(0, iClientH, Notify::SpecialAbilityStatus, 2, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime, 0);
					break;
				}
				break;

			case ItemEffectType::AddEffect:
				switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1) {
				case 1:
					m_pGame->m_pClientList[iClientH]->m_iAddResistMagic += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 2:
					m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					if (m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio > 80) m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio = 80;
					break;

				case 3:
					m_pGame->m_pClientList[iClientH]->m_iAddPhysicalDamage += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 4:
					m_pGame->m_pClientList[iClientH]->m_iDefenseRatio += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 5:
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2 != 0)
						m_pGame->m_pClientList[iClientH]->m_bIsLuckyEffect = true;
					else m_pGame->m_pClientList[iClientH]->m_bIsLuckyEffect = false;
					break;

				case 6:
					m_pGame->m_pClientList[iClientH]->m_iAddMagicalDamage += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 7:
					m_pGame->m_pClientList[iClientH]->m_iAddAbsAir += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 8:
					m_pGame->m_pClientList[iClientH]->m_iAddAbsEarth += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 9:
					m_pGame->m_pClientList[iClientH]->m_iAddAbsFire += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 10:
					// . (2  )
					m_pGame->m_pClientList[iClientH]->m_iAddAbsWater += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 11:
					m_pGame->m_pClientList[iClientH]->m_iAddPR += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 12:
					m_pGame->m_pClientList[iClientH]->m_iHitRatio += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2;
					break;

				case 13: // Magin Ruby		Characters Hp recovery rate(% applied) added by the purity formula.
					m_pGame->m_pClientList[iClientH]->m_iAddHP += (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 / 5);
					break;

				case 14: // Magin Diamond	Attack probability(physical&magic) added by the purity formula.
					m_pGame->m_pClientList[iClientH]->m_iAddAR += (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 / 5);
					break;

				case 15: // Magin Emerald	Magical damage decreased(% applied) by the purity formula.	
					m_pGame->m_pClientList[iClientH]->m_iAddAbsMD += (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 / 10);
					if (m_pGame->m_pClientList[iClientH]->m_iAddAbsMD > 80) m_pGame->m_pClientList[iClientH]->m_iAddAbsMD = 80;
					break;

				case 30: // Magin Sapphire	Phisical damage decreased(% applied) by the purity formula.	
					iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 / 10);
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Head)] += iTemp;
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Body)] += iTemp;
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Arms)] += iTemp;
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[ToInt(EquipPos::Pants)] += iTemp;
					break;

					/*Functions rates confirm.
					Magic Diamond: Completion rate / 5 = Functions rate. ? Maximum 20. (not%)
					Magic Ruby: Completion rate / 5 = Functions rate.(%) ? Maximum 20%.
					Magic Emerald: Completion rate / 10 = Functions rate.(%) ? Maximum 10%.
					Magic Sapphire: Completion rate / 10 = Functions rate.(%) ? Maximum 10%.*/

					// ******* Angel Code - Begin ******* //			
				case 16: // Angel STR//AngelicPandent(STR)
					iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
					m_pGame->m_pClientList[iClientH]->m_iAngelicStr = iTemp + 1;
					m_pGame->m_pStatusEffectManager->SetAngelFlag(iClientH, hb::shared::owner_class::Player, 1, iTemp);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::SettingSuccess, 0, 0, 0, 0);
					break;
				case 17: // Angel DEX //AngelicPandent(DEX)
					iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
					m_pGame->m_pClientList[iClientH]->m_iAngelicDex = iTemp + 1;
					m_pGame->m_pStatusEffectManager->SetAngelFlag(iClientH, hb::shared::owner_class::Player, 2, iTemp);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::SettingSuccess, 0, 0, 0, 0);
					break;
				case 18: // Angel INT//AngelicPandent(INT)
					iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
					m_pGame->m_pClientList[iClientH]->m_iAngelicInt = iTemp + 1;
					m_pGame->m_pStatusEffectManager->SetAngelFlag(iClientH, hb::shared::owner_class::Player, 3, iTemp);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::SettingSuccess, 0, 0, 0, 0);
					break;
				case 19: // Angel MAG//AngelicPandent(MAG)
					iTemp = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
					m_pGame->m_pClientList[iClientH]->m_iAngelicMag = iTemp + 1;
					m_pGame->m_pStatusEffectManager->SetAngelFlag(iClientH, hb::shared::owner_class::Player, 4, iTemp);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::SettingSuccess, 0, 0, 0, 0);
					break;

				}
				break;

			case ItemEffectType::AttackArrow:
				if ((m_pGame->m_pClientList[iClientH]->m_cArrowIndex != -1) &&
					(m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cArrowIndex] == 0)) {
					// ArrowIndex  . ( )
					m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);
				}
				else if (m_pGame->m_pClientList[iClientH]->m_cArrowIndex == -1)
					m_pGame->m_pClientList[iClientH]->m_cArrowIndex = _iGetArrowItemIndex(iClientH);

				if (m_pGame->m_pClientList[iClientH]->m_cArrowIndex == -1) {
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM = 0;
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM = 0;
					m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM = 0;
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L = 0;
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L = 0;
					m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L = 0;
				}
				else {
					iArrowIndex = m_pGame->m_pClientList[iClientH]->m_cArrowIndex;
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1);
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2);
					m_pGame->m_pClientList[iClientH]->m_cAttackBonus_SM = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue3);
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceThrow_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue4);
					m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue5);
					m_pGame->m_pClientList[iClientH]->m_cAttackBonus_L = static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue6);
				}

				m_pGame->m_pClientList[iClientH]->m_iHitRatio += m_pGame->m_pClientList[iClientH]->m_cSkillMastery[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill];
				break;

			case ItemEffectType::DefenseSpecAbility:
			case ItemEffectType::Defense:
				m_pGame->m_pClientList[iClientH]->m_iDefenseRatio += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00000001) != 0) {
					m_pGame->m_pClientList[iClientH]->m_iCustomItemValue_Defense += m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;

					dV2 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2;
					dV3 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1;
					dV1 = (double)(dV2 / 100.0f) * dV3;

					dV1 = dV1 / 2.0f;
					m_pGame->m_pClientList[iClientH]->m_iDefenseRatio += (int)dV1;
					if (m_pGame->m_pClientList[iClientH]->m_iDefenseRatio <= 0) m_pGame->m_pClientList[iClientH]->m_iDefenseRatio = 1;

					//testcode
					//std::snprintf(G_cTxt, sizeof(G_cTxt), "Custom-Defense: %d", (int)dV1);
					//PutLogList(G_cTxt);
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00F00000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00F00000) >> 20;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x000F0000) >> 16;

					// 0-None 1- 2- 3- 4-
					// 5- 6- 7- 8- 9- 10- 11- 12-

					switch (dwSWEType) {
					case 7:
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM++;
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L++;
						break;

					case 9:
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_SM += 2;
						m_pGame->m_pClientList[iClientH]->m_cAttackDiceRange_L += 2;
						break;

						// v2.04 
					case 11:
						m_pGame->m_pClientList[iClientH]->m_iAddTransMana += dwSWEValue;
						if (m_pGame->m_pClientList[iClientH]->m_iAddTransMana > 13) m_pGame->m_pClientList[iClientH]->m_iAddTransMana = 13;
						break;

					case 12:
						m_pGame->m_pClientList[iClientH]->m_iAddChargeCritical += dwSWEValue;
						if (m_pGame->m_pClientList[iClientH]->m_iAddChargeCritical > 20) m_pGame->m_pClientList[iClientH]->m_iAddChargeCritical = 20;
						break;
					}
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x0000F000) != 0) {
					dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x0000F000) >> 12;
					dwSWEValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute & 0x00000F00) >> 8;

					// (1),  (2),  (3), HP  (4), SP  (5)
					// MP  (6),  (7),   (8),   (9)
					// (10),   (11),  Gold(12)

					switch (dwSWEType) {
					case 0:  break;
					case 1:  m_pGame->m_pClientList[iClientH]->m_iAddPR += (int)dwSWEValue * 7; break;
					case 2:  m_pGame->m_pClientList[iClientH]->m_iAddAR += (int)dwSWEValue * 7; break;
					case 3:  m_pGame->m_pClientList[iClientH]->m_iAddDR += (int)dwSWEValue * 7; break;
					case 4:  m_pGame->m_pClientList[iClientH]->m_iAddHP += (int)dwSWEValue * 7; break;
					case 5:  m_pGame->m_pClientList[iClientH]->m_iAddSP += (int)dwSWEValue * 7; break;
					case 6:  m_pGame->m_pClientList[iClientH]->m_iAddMP += (int)dwSWEValue * 7; break;
					case 7:  m_pGame->m_pClientList[iClientH]->m_iAddMR += (int)dwSWEValue * 7; break;
					case 8:  m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos] += (int)dwSWEValue * 3; break;
					case 9:  m_pGame->m_pClientList[iClientH]->m_iAddAbsMD += (int)dwSWEValue * 3; break;
					case 10: m_pGame->m_pClientList[iClientH]->m_iAddCD += (int)dwSWEValue; break;
					case 11: m_pGame->m_pClientList[iClientH]->m_iAddExp += (int)dwSWEValue * 10; break;
					case 12: m_pGame->m_pClientList[iClientH]->m_iAddGold += (int)dwSWEValue * 10; break;
					}

					switch (dwSWEType) {
					case 9: if (m_pGame->m_pClientList[iClientH]->m_iAddAbsMD > 80) m_pGame->m_pClientList[iClientH]->m_iAddAbsMD = 80; break;
					}
				}

				switch (cEquipPos) {
				case EquipPos::LeftHand:
					// .  70%
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Shield = (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1) - (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1) / 3;
					break;
				default:
					// .  70%  <- v1.43 100% . V2!
					m_pGame->m_pClientList[iClientH]->m_iDamageAbsorption_Armor[m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cEquipPos] += (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2);
					break;
				}

				switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
				case ItemEffectType::DefenseSpecAbility:
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffect;
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityLastSec = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpecialEffectValue1;
					m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityEquipPos = ToInt(cEquipPos);

					if ((bNotify) && (iEquipItemID == (int)sItemIndex))
						m_pGame->SendNotifyMsg(0, iClientH, Notify::SpecialAbilityStatus, 2, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime, 0);
					break;
				}
				break;
			}
		}
	}

	// Snoopy: Bonus for Angels	
	m_pGame->m_pClientList[iClientH]->m_iDefenseRatio += m_pGame->m_pClientList[iClientH]->m_iAngelicDex * 2;
	if (m_pGame->m_pClientList[iClientH]->m_iHP > m_pGame->iGetMaxHP(iClientH)) m_pGame->m_pClientList[iClientH]->m_iHP = m_pGame->iGetMaxHP(iClientH);
	if (m_pGame->m_pClientList[iClientH]->m_iMP > m_pGame->iGetMaxMP(iClientH)) m_pGame->m_pClientList[iClientH]->m_iMP = m_pGame->iGetMaxMP(iClientH);
	if (m_pGame->m_pClientList[iClientH]->m_iSP > m_pGame->iGetMaxSP(iClientH)) m_pGame->m_pClientList[iClientH]->m_iSP = m_pGame->iGetMaxSP(iClientH);

	//v1.432
	if ((iPrevSAType != 0) && (m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType == 0) && (bNotify)) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::SpecialAbilityStatus, 4, 0, 0, 0);
		if (m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled) {
			m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled = false;
			m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime = SpecialAbilityTimeSec;
			m_pGame->m_pClientList[iClientH]->m_appearance.iEffectType = 0;
			m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		}
	}

	if ((iPrevSAType != 0) && (m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType != 0) &&
		(iPrevSAType != m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType) && (bNotify)) {
		if (m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled) {
			m_pGame->SendNotifyMsg(0, iClientH , Notify::SpecialAbilityStatus, 3, 0, 0, 0);
			m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled = false;
			m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime = SpecialAbilityTimeSec;
			m_pGame->m_pClientList[iClientH]->m_appearance.iEffectType = 0;
			m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		}
	}
}

bool ItemManager::_bDepleteDestTypeItemUseEffect(int iClientH, int dX, int dY, short sItemIndex, short sDestItemID)
{
	int bRet;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return false;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return false;

	switch (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->GetItemEffectType()) {
	case ItemEffectType::OccupyFlag:
		bRet = m_pGame->m_pWarManager->__bSetOccupyFlag(m_pGame->m_pClientList[iClientH]->m_cMapIndex, dX, dY,
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1,
			0, iClientH);
		if (bRet) {
			m_pGame->GetExp(iClientH, (m_pGame->iDice(m_pGame->m_pClientList[iClientH]->m_iLevel, 10)));
		}
		else {
			m_pGame->SendNotifyMsg(0, iClientH, Notify::NotFlagSpot, 0, 0, 0, 0);
		}
		return bRet;

		// crusade
	case ItemEffectType::ConstructionKit:
		// .   . m_sItemEffectValue1:  , m_sItemEffectValue2:
		bRet = m_pGame->m_pWarManager->__bSetConstructionKit(m_pGame->m_pClientList[iClientH]->m_cMapIndex, dX, dY,
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1,
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2,
			iClientH);
		if (bRet) {
		}
		else {
		}
		return bRet;

	case ItemEffectType::Dye:
		if ((sDestItemID >= 0) && (sDestItemID < hb::shared::limits::MaxItems)) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID] != 0) {
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 11) ||
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 12)) {
					m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor =
						static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor, 0, 0);
					return true;
				}
				else {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, -1, 0, 0);
					return false;
				}
			}
		}
		break;

	case ItemEffectType::ArmorDye:
		if ((sDestItemID >= 0) && (sDestItemID < hb::shared::limits::MaxItems)) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID] != 0) {
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 6) ||
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 15) ||
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 13)) {
					m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor =
						static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor, 0, 0);
					return true;
				}
				else {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, -1, 0, 0);
					return false;
				}
			}
		}
		break;

	case ItemEffectType::WeaponDye:
		if ((sDestItemID >= 0) && (sDestItemID < hb::shared::limits::MaxItems)) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID] != 0) {
				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 1) ||
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 3) ||
					(m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cCategory == 8)) {
					m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor =
						static_cast<char>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1);
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, m_pGame->m_pClientList[iClientH]->m_pItemList[sDestItemID]->m_cItemColor, 0, 0);
					return true;
				}
				else {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemColorChange, sDestItemID, -1, 0, 0);
					return false;
				}
			}
		}
		break;

	case ItemEffectType::Farming:
		bRet = bPlantSeedBag(m_pGame->m_pClientList[iClientH]->m_cMapIndex, dX, dY,
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue1,
			m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemEffectValue2,
			iClientH);
		return bRet;

	default:
		break;
	}

	return true;
}

void ItemManager::GetHeroMantleHandler(int iClientH, int iItemID, const char* pString)
{
	int   iNum, iRet, iEraseReq;
	char cItemName[hb::shared::limits::ItemNameLen];
	CItem* pItem;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 100) return;
	if (m_pGame->m_pClientList[iClientH]->m_cSide == 0) return;
	if (_iGetItemSpaceLeft(iClientH) == 0) {
		SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);
		return;
	}

	//Prevents a crash if item dosent exist
	if (m_pGame->m_pItemConfigList[iItemID] == 0)  return;

	switch (iItemID) {
		// Hero Cape
	case 400: //Aresden HeroCape
	case 401: //Elvine HeroCape
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 300) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 300;
		break;

		// Hero Helm
	case 403: //Aresden HeroHelm(M)
	case 404: //Aresden HeroHelm(W)
	case 405: //Elvine HeroHelm(M)
	case 406: //Elvine HeroHelm(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 150) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 150;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 20) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 20;
		break;

		// Hero Cap
	case 407: //Aresden HeroCap(M)
	case 408: //Aresden HeroCap(W)
	case 409: //Elvine HeroHelm(M)
	case 410: //Elvine HeroHelm(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 100) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 100;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 20) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 20;
		break;

		// Hero Armour
	case 411: //Aresden HeroArmour(M)
	case 412: //Aresden HeroArmour(W)
	case 413: //Elvine HeroArmour(M)
	case 414: //Elvine HeroArmour(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 300) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 300;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 30) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 30;
		break;

		// Hero Robe
	case 415: //Aresden HeroRobe(M)
	case 416: //Aresden HeroRobe(W)
	case 417: //Elvine HeroRobe(M)
	case 418: //Elvine HeroRobe(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 200) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 200;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 20) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 20;
		break;

		// Hero Hauberk
	case 419: //Aresden HeroHauberk(M)
	case 420: //Aresden HeroHauberk(W)
	case 421: //Elvine HeroHauberk(M)
	case 422: //Elvine HeroHauberk(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 100) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 100;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 10) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 10;
		break;

		// Hero Leggings
	case 423: //Aresden HeroLeggings(M)
	case 424: //Aresden HeroLeggings(W)
	case 425: //Elvine HeroLeggings(M)
	case 426: //Elvine HeroLeggings(W)
		if (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 150) return;
		m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 150;
		if (m_pGame->m_pClientList[iClientH]->m_iContribution < 15) return;
		m_pGame->m_pClientList[iClientH]->m_iContribution -= 15;
		break;

	default:
		return;
		break;
	}

	std::memset(cItemName, 0, sizeof(cItemName));
	memcpy(cItemName, m_pGame->m_pItemConfigList[iItemID]->m_cName, hb::shared::limits::ItemNameLen - 1);
	// ReqPurchaseItemHandler
	iNum = 1;
	for(int i = 1; i <= iNum; i++)
	{
		pItem = new CItem;
		if (_bInitItemAttr(pItem, cItemName) == false)
		{
			delete pItem;
		}
		else {

			if (_bAddClientItemList(iClientH, pItem, &iEraseReq)) {
				if (m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad < 0) m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad = 0;

				hb::logger::log<log_channel::events>("Get HeroItem : Char({}) Player-EK({}) Player-Contr({}) Hero Obtained({})", m_pGame->m_pClientList[iClientH]->m_cCharName, m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount, m_pGame->m_pClientList[iClientH]->m_iContribution, cItemName);

				pItem->SetTouchEffectType(TouchEffectType::UniqueOwner);
				pItem->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				pItem->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				pItem->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iRet = SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);

				m_pGame->iCalcTotalWeight(iClientH);

				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:
					m_pGame->DeleteClient(iClientH, true, true);
					return;
				}

				m_pGame->SendNotifyMsg(0, iClientH, Notify::EnemyKills, m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount, 0, 0, 0);
			}
			else
			{
				delete pItem;

				m_pGame->iCalcTotalWeight(iClientH);

				iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

				switch (iRet) {
				case sock::Event::QueueFull:
				case sock::Event::SocketError:
				case sock::Event::CriticalError:
				case sock::Event::SocketClosed:

					m_pGame->DeleteClient(iClientH, true, true);
					return;
				}
			}
		}
	}
}

void ItemManager::_SetItemPos(int iClientH, char* pData)
{
	char cItemIndex;
	short sX, sY;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	const auto* req = hb::net::PacketCast<hb::net::PacketRequestSetItemPos>(pData, sizeof(hb::net::PacketRequestSetItemPos));
	if (!req) return;
	cItemIndex = static_cast<char>(req->dir);
	sX = req->x;
	sY = req->y;

	if (sY < -10) sY = -10;

	if ((cItemIndex < 0) || (cItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemIndex] != 0) {
		m_pGame->m_pClientList[iClientH]->m_ItemPosList[cItemIndex].x = sX;
		m_pGame->m_pClientList[iClientH]->m_ItemPosList[cItemIndex].y = sY;
	}
}

void ItemManager::CheckUniqueItemEquipment(int iClientH)
{
	int iDamage;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->GetTouchEffectType() == TouchEffectType::UniqueOwner) &&
				(m_pGame->m_pClientList[iClientH]->m_bIsItemEquipped[i])) {
				// Touch Effect Type DEF_ITET_OWNER Touch Effect Value 1, 2, 3    .

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sTouchEffectValue1 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum1) &&
					(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sTouchEffectValue2 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum2) &&
					(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sTouchEffectValue3 == m_pGame->m_pClientList[iClientH]->m_sCharIDnum3)) {
				}
				else {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemReleased, m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cEquipPos, i, 0, 0);
					ReleaseItemHandler(iClientH, i, true);
					iDamage = m_pGame->iDice(10, 10);
					m_pGame->m_pClientList[iClientH]->m_iHP -= iDamage;
					if (m_pGame->m_pClientList[iClientH]->m_iHP <= 0) {
						m_pGame->m_pCombatManager->ClientKilledHandler(iClientH, 0, 0, iDamage);
					}
				}
			}
		}
}

void ItemManager::ExchangeItemHandler(int iClientH, short sItemIndex, int iAmount, short dX, short dY, uint16_t wObjectID, const char* pItemName)
{
	short sOwnerH;
	char  cOwnerType;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if ((sItemIndex < 0) || (sItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwCount < static_cast<uint32_t>(iAmount)) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode) return;
	if (wObjectID >= MaxClients) return;

	// dX, dY     .
	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

	if ((sOwnerH != 0) && (cOwnerType == hb::shared::owner_class::Player)) {

		if (wObjectID != 0) {
			if (hb::shared::object_id::IsPlayerID(wObjectID)) {
				if (m_pGame->m_pClientList[wObjectID] != 0) {
					if ((uint16_t)sOwnerH != wObjectID) sOwnerH = 0;
				}
			}
			else sOwnerH = 0;
		}

		if ((sOwnerH == 0) || (m_pGame->m_pClientList[sOwnerH] == 0)) {
			_ClearExchangeStatus(iClientH);
		}
		else {
			if ((m_pGame->m_pClientList[sOwnerH]->m_bIsExchangeMode) || (m_pGame->m_pClientList[sOwnerH]->m_appearance.bIsWalking) ||
				(m_pGame->m_pMapList[m_pGame->m_pClientList[sOwnerH]->m_cMapIndex]->m_bIsFightZone)) {
				_ClearExchangeStatus(iClientH);
			}
			else {
				m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode = true;
				m_pGame->m_pClientList[iClientH]->m_iExchangeH = sOwnerH;
				std::memset(m_pGame->m_pClientList[iClientH]->m_cExchangeName, 0, sizeof(m_pGame->m_pClientList[iClientH]->m_cExchangeName));
				strcpy(m_pGame->m_pClientList[iClientH]->m_cExchangeName, m_pGame->m_pClientList[sOwnerH]->m_cCharName);

				//Clear items in the list
				m_pGame->m_pClientList[iClientH]->iExchangeCount = 0;
				m_pGame->m_pClientList[sOwnerH]->iExchangeCount = 0;
				for(int i = 0; i < 4; i++) {
					//Clear the trader
					m_pGame->m_pClientList[iClientH]->m_sExchangeItemID[i] = 0;
					m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i] = -1;
					m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i] = 0;
					//Clear the guy we're trading with
					m_pGame->m_pClientList[sOwnerH]->m_sExchangeItemID[i] = 0;
					m_pGame->m_pClientList[sOwnerH]->m_cExchangeItemIndex[i] = -1;
					m_pGame->m_pClientList[sOwnerH]->m_iExchangeItemAmount[i] = 0;
				}

				m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[m_pGame->m_pClientList[iClientH]->iExchangeCount] = (char)sItemIndex;
				m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[m_pGame->m_pClientList[iClientH]->iExchangeCount] = iAmount;

				m_pGame->m_pClientList[iClientH]->m_sExchangeItemID[m_pGame->m_pClientList[iClientH]->iExchangeCount] = m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum;

				m_pGame->m_pClientList[sOwnerH]->m_bIsExchangeMode = true;
				m_pGame->m_pClientList[sOwnerH]->m_iExchangeH = iClientH;
				std::memset(m_pGame->m_pClientList[sOwnerH]->m_cExchangeName, 0, sizeof(m_pGame->m_pClientList[sOwnerH]->m_cExchangeName));
				strcpy(m_pGame->m_pClientList[sOwnerH]->m_cExchangeName, m_pGame->m_pClientList[iClientH]->m_cCharName);

				m_pGame->m_pClientList[iClientH]->iExchangeCount++;
				m_pGame->SendNotifyMsg(iClientH, iClientH, Notify::OpenExchangeWindow, sItemIndex + 1000, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpriteFrame, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cName, iAmount, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wMaxLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 + 100,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute,
					reinterpret_cast<char*>(static_cast<intptr_t>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum)));

				m_pGame->SendNotifyMsg(iClientH, sOwnerH, Notify::OpenExchangeWindow, sItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sSpriteFrame, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cName, iAmount, m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wCurLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_wMaxLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sItemSpecEffectValue2 + 100,
					m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_dwAttribute,
					reinterpret_cast<char*>(static_cast<intptr_t>(m_pGame->m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sIDnum)));
			}
		}
	}
	else {
		// NPC    .
		_ClearExchangeStatus(iClientH);

	}
}

void ItemManager::SetExchangeItem(int iClientH, int iItemIndex, int iAmount)
{
	int iExH;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;
	if (m_pGame->m_pClientList[iClientH]->iExchangeCount > 4) return;	//only 4 items trade

	if ((m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode) && (m_pGame->m_pClientList[iClientH]->m_iExchangeH != 0)) {
		iExH = m_pGame->m_pClientList[iClientH]->m_iExchangeH;
		if ((m_pGame->m_pClientList[iExH] == 0) || (hb_strnicmp(m_pGame->m_pClientList[iClientH]->m_cExchangeName, m_pGame->m_pClientList[iExH]->m_cCharName, hb::shared::limits::CharNameLen - 1) != 0)) {

		}
		else {
			if ((iItemIndex < 0) || (iItemIndex >= hb::shared::limits::MaxItems)) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] == 0) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwCount < static_cast<uint32_t>(iAmount)) return;

			//No Duplicate items
			for(int i = 0; i < m_pGame->m_pClientList[iClientH]->iExchangeCount; i++) {
				if (m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i] == (char)iItemIndex) {
					_ClearExchangeStatus(iExH);
					_ClearExchangeStatus(iClientH);
					return;
				}
			}

			m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[m_pGame->m_pClientList[iClientH]->iExchangeCount] = (char)iItemIndex;
			m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[m_pGame->m_pClientList[iClientH]->iExchangeCount] = iAmount;

			m_pGame->m_pClientList[iClientH]->m_sExchangeItemID[m_pGame->m_pClientList[iClientH]->iExchangeCount] = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum;

			m_pGame->m_pClientList[iClientH]->iExchangeCount++;
			m_pGame->SendNotifyMsg(iClientH, iClientH, Notify::SetExchangeItem, iItemIndex + 1000, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName, iAmount, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 + 100,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
				reinterpret_cast<char*>(static_cast<intptr_t>(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum)));

			m_pGame->SendNotifyMsg(iClientH, iExH, Notify::SetExchangeItem, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName, iAmount, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 + 100,
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
				reinterpret_cast<char*>(static_cast<intptr_t>(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum)));
		}
	}
	else {
	}
}

void ItemManager::ConfirmExchangeItem(int iClientH)
{
	int iExH;
	int iItemWeightA, iItemWeightB, iWeightLeftA, iWeightLeftB, iAmountLeft;
	CItem* pItemA[4], * pItemB[4], * pItemAcopy[4], * pItemBcopy[4];

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;

	if ((m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode) && (m_pGame->m_pClientList[iClientH]->m_iExchangeH != 0)) {
		iExH = m_pGame->m_pClientList[iClientH]->m_iExchangeH;

		if (iClientH == iExH) return;

		if (m_pGame->m_pClientList[iExH] != 0) {
			if ((hb_strnicmp(m_pGame->m_pClientList[iClientH]->m_cExchangeName, m_pGame->m_pClientList[iExH]->m_cCharName, hb::shared::limits::CharNameLen - 1) != 0) ||
				(m_pGame->m_pClientList[iExH]->m_bIsExchangeMode != true) ||
				(hb_strnicmp(m_pGame->m_pClientList[iExH]->m_cExchangeName, m_pGame->m_pClientList[iClientH]->m_cCharName, hb::shared::limits::CharNameLen - 1) != 0)) {
				_ClearExchangeStatus(iClientH);
				_ClearExchangeStatus(iExH);
				return;
			}
			else {
				m_pGame->m_pClientList[iClientH]->m_bIsExchangeConfirm = true;
				if (m_pGame->m_pClientList[iExH]->m_bIsExchangeConfirm) {

					//Check all items
					for(int i = 0; i < m_pGame->m_pClientList[iClientH]->iExchangeCount; i++) {
						if ((m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]] == 0) ||
							(m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_sIDnum != m_pGame->m_pClientList[iClientH]->m_sExchangeItemID[i])) {
							_ClearExchangeStatus(iClientH);
							_ClearExchangeStatus(iExH);
							return;
						}
					}
					for(int i = 0; i < m_pGame->m_pClientList[iExH]->iExchangeCount; i++) {
						if ((m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]] == 0) ||
							(m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_sIDnum != m_pGame->m_pClientList[iExH]->m_sExchangeItemID[i])) {
							_ClearExchangeStatus(iClientH);
							_ClearExchangeStatus(iExH);
							return;
						}
					}

					iWeightLeftA = m_pGame->_iCalcMaxLoad(iClientH) - m_pGame->iCalcTotalWeight(iClientH);
					iWeightLeftB = m_pGame->_iCalcMaxLoad(iExH) - m_pGame->iCalcTotalWeight(iExH);

					//Calculate weight for items
					iItemWeightA = 0;
					for(int i = 0; i < m_pGame->m_pClientList[iClientH]->iExchangeCount; i++) {
						iItemWeightA = iGetItemWeight(m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]],
							m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i]);
					}
					iItemWeightB = 0;
					for(int i = 0; i < m_pGame->m_pClientList[iExH]->iExchangeCount; i++) {
						iItemWeightB = iGetItemWeight(m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]],
							m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i]);
					}

					//See if the other person can take the item weightload
					if ((iWeightLeftA < iItemWeightB) || (iWeightLeftB < iItemWeightA)) {
						_ClearExchangeStatus(iClientH);
						_ClearExchangeStatus(iExH);
						return;
					}

					for(int i = 0; i < m_pGame->m_pClientList[iClientH]->iExchangeCount; i++) {
						if ((m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Consume) ||
							(m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Arrow)) {

							if (static_cast<uint32_t>(m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i]) >
								m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_dwCount) {
								_ClearExchangeStatus(iClientH);
								_ClearExchangeStatus(iExH);
								return;
							}
							pItemA[i] = new CItem;
							_bInitItemAttr(pItemA[i], m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_cName);
							pItemA[i]->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i];

							pItemAcopy[i] = new CItem;
							_bInitItemAttr(pItemAcopy[i], m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_cName);
							bCopyItemContents(pItemAcopy[i], pItemA[i]);
							pItemAcopy[i]->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i];
						}
						else {
							pItemA[i] = (CItem*)m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]];
							pItemA[i]->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i];

							pItemAcopy[i] = new CItem;
							_bInitItemAttr(pItemAcopy[i], m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_cName);
							bCopyItemContents(pItemAcopy[i], pItemA[i]);
							pItemAcopy[i]->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i];
						}
					}

					for(int i = 0; i < m_pGame->m_pClientList[iExH]->iExchangeCount; i++) {
						if ((m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Consume) ||
							(m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Arrow)) {

							if (static_cast<uint32_t>(m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i]) >
								m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_dwCount) {
								_ClearExchangeStatus(iClientH);
								_ClearExchangeStatus(iExH);
								return;
							}
							pItemB[i] = new CItem;
							_bInitItemAttr(pItemB[i], m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_cName);
							pItemB[i]->m_dwCount = m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i];

							pItemBcopy[i] = new CItem;
							_bInitItemAttr(pItemBcopy[i], m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_cName);
							bCopyItemContents(pItemBcopy[i], pItemB[i]);
							pItemBcopy[i]->m_dwCount = m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i];
						}
						else {
							pItemB[i] = (CItem*)m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]];
							pItemB[i]->m_dwCount = m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i];

							pItemBcopy[i] = new CItem;
							_bInitItemAttr(pItemBcopy[i], m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_cName);
							bCopyItemContents(pItemBcopy[i], pItemB[i]);
							pItemBcopy[i]->m_dwCount = m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i];
						}
					}

					for(int i = 0; i < m_pGame->m_pClientList[iExH]->iExchangeCount; i++) {
						bAddItem(iClientH, pItemB[i], 0);
						_bItemLog(ItemLogAction::Exchange, iExH, iClientH, pItemBcopy[i]);
						delete pItemBcopy[i];
						pItemBcopy[i] = 0;
						if ((m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Consume) ||
							(m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Arrow)) {
							iAmountLeft = (int)m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]]->m_dwCount - m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i];
							if (iAmountLeft < 0) iAmountLeft = 0;
							// v1.41 !!!
							SetItemCount(iExH, m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i], iAmountLeft);
							// m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex]->m_cName, iAmountLeft);
						}
						else {
							ReleaseItemHandler(iExH, m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i], true);
							m_pGame->SendNotifyMsg(0, iExH, Notify::GiveItemFinEraseItem, m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i], m_pGame->m_pClientList[iExH]->m_iExchangeItemAmount[i], 0, m_pGame->m_pClientList[iClientH]->m_cCharName);
							m_pGame->m_pClientList[iExH]->m_pItemList[m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i]] = 0;
						}
					}

					for(int i = 0; i < m_pGame->m_pClientList[iClientH]->iExchangeCount; i++) {
						bAddItem(iExH, pItemA[i], 0);
						_bItemLog(ItemLogAction::Exchange, iClientH, iExH, pItemAcopy[i]);
						delete pItemAcopy[i];
						pItemAcopy[i] = 0;

						if ((m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Consume) ||
							(m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->GetItemType() == ItemType::Arrow)) {
							iAmountLeft = (int)m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]]->m_dwCount - m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i];
							if (iAmountLeft < 0) iAmountLeft = 0;
							// v1.41 !!!
							SetItemCount(iClientH, m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i], iAmountLeft);
							// m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex]->m_cName, iAmountLeft);
						}
						else {
							ReleaseItemHandler(iClientH, m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i], true);
							m_pGame->SendNotifyMsg(0, iClientH, Notify::GiveItemFinEraseItem, m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i], m_pGame->m_pClientList[iClientH]->m_iExchangeItemAmount[i], 0, m_pGame->m_pClientList[iExH]->m_cCharName);
							m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i]] = 0;
						}
					}

					m_pGame->m_pClientList[iClientH]->m_bIsExchangeMode = false;
					m_pGame->m_pClientList[iClientH]->m_bIsExchangeConfirm = false;
					std::memset(m_pGame->m_pClientList[iClientH]->m_cExchangeName, 0, sizeof(m_pGame->m_pClientList[iClientH]->m_cExchangeName));
					m_pGame->m_pClientList[iClientH]->m_iExchangeH = 0;
					m_pGame->m_pClientList[iClientH]->iExchangeCount = 0;

					m_pGame->m_pClientList[iExH]->m_bIsExchangeMode = false;
					m_pGame->m_pClientList[iExH]->m_bIsExchangeConfirm = false;
					std::memset(m_pGame->m_pClientList[iExH]->m_cExchangeName, 0, sizeof(m_pGame->m_pClientList[iExH]->m_cExchangeName));
					m_pGame->m_pClientList[iExH]->m_iExchangeH = 0;
					m_pGame->m_pClientList[iExH]->iExchangeCount = 0;

					for(int i = 0; i < 4; i++) {
						m_pGame->m_pClientList[iClientH]->m_cExchangeItemIndex[i] = -1;
						m_pGame->m_pClientList[iExH]->m_cExchangeItemIndex[i] = -1;
					}

					m_pGame->SendNotifyMsg(0, iClientH, Notify::ExchangeItemComplete, 0, 0, 0, 0);
					m_pGame->SendNotifyMsg(0, iExH, Notify::ExchangeItemComplete, 0, 0, 0, 0);

					m_pGame->iCalcTotalWeight(iClientH);
					m_pGame->iCalcTotalWeight(iExH);
					return;
				}
			}
		}
		else {
			_ClearExchangeStatus(iClientH);
			return;
		}
	}
}

int ItemManager::_iGetItemSpaceLeft(int iClientH)
{
	int iTotalItem;

	iTotalItem = 0;
	for(int i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) iTotalItem++;

	return (hb::shared::limits::MaxItems - iTotalItem);
}

bool ItemManager::bAddItem(int iClientH, CItem* pItem, char cMode)
{
	int iRet, iEraseReq;

	if (_bAddClientItemList(iClientH, pItem, &iEraseReq)) {
		iRet = SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);

		return true;
	}
	else {
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX,
			m_pGame->m_pClientList[iClientH]->m_sY,
			pItem);

		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
			m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY,
			pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); //v1.4 color

		iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

		return true;
	}

	return false;
}

int ItemManager::SendItemNotifyMsg(int iClientH, uint16_t wMsgType, CItem* pItem, int iV1)
{
	int iRet = 0;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;

	switch (wMsgType) {
	case Notify::ItemObtained:
	{
		hb::net::PacketNotifyItemObtained pkt{};
		pkt.header.msg_id = MsgId::Notify;
		pkt.header.msg_type = wMsgType;
		pkt.is_new = 1;
		memcpy(pkt.name, pItem->m_cName, sizeof(pkt.name));
		pkt.count = pItem->m_dwCount;
		pkt.item_type = pItem->m_cItemType;
		pkt.equip_pos = pItem->m_cEquipPos;
		pkt.is_equipped = 0;
		pkt.level_limit = pItem->m_sLevelLimit;
		pkt.gender_limit = pItem->m_cGenderLimit;
		pkt.cur_lifespan = pItem->m_wCurLifeSpan;
		pkt.weight = pItem->m_wWeight;
		pkt.sprite = pItem->m_sSprite;
		pkt.sprite_frame = pItem->m_sSpriteFrame;
		pkt.item_color = pItem->m_cItemColor;
		pkt.spec_value2 = static_cast<uint8_t>(pItem->m_sItemSpecEffectValue2);
		pkt.attribute = pItem->m_dwAttribute;
		pkt.item_id = pItem->m_sIDnum;
		pkt.max_lifespan = pItem->m_wMaxLifeSpan;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	break;

	case Notify::ItemPurchased:
	{
		hb::net::PacketNotifyItemPurchased pkt{};
		pkt.header.msg_id = MsgId::Notify;
		pkt.header.msg_type = wMsgType;
		pkt.is_new = 1;
		memcpy(pkt.name, pItem->m_cName, sizeof(pkt.name));
		pkt.count = pItem->m_dwCount;
		pkt.item_type = pItem->m_cItemType;
		pkt.equip_pos = pItem->m_cEquipPos;
		pkt.is_equipped = 0;
		pkt.level_limit = pItem->m_sLevelLimit;
		pkt.gender_limit = pItem->m_cGenderLimit;
		pkt.cur_lifespan = pItem->m_wCurLifeSpan;
		pkt.weight = pItem->m_wWeight;
		pkt.sprite = pItem->m_sSprite;
		pkt.sprite_frame = pItem->m_sSpriteFrame;
		pkt.item_color = pItem->m_cItemColor;
		pkt.cost = static_cast<uint16_t>(iV1);
		pkt.item_id = pItem->m_sIDnum;
		pkt.max_lifespan = pItem->m_wMaxLifeSpan;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	break;

	case Notify::CannotCarryMoreItem:
	{
		hb::net::PacketNotifyEmpty pkt{};
		pkt.header.msg_id = MsgId::Notify;
		pkt.header.msg_type = wMsgType;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	break;
	}

	return iRet;
}

bool ItemManager::_bCheckItemReceiveCondition(int iClientH, CItem* pItem)
{
	

	if (m_pGame->m_pClientList[iClientH] == 0) return false;

	if (m_pGame->m_pClientList[iClientH]->m_iCurWeightLoad + iGetItemWeight(pItem, pItem->m_dwCount) > m_pGame->_iCalcMaxLoad(iClientH))
		return false;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++)
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] == 0) return true;

	return false;
}

void ItemManager::BuildItemHandler(int iClientH, char* pData)
{
	char cName[hb::shared::limits::ItemNameLen], cElementItemID[6];
	int    x, z, iMatch, iCount, iPlayerSkillLevel, iResult, iTotalValue, iResultValue, iTemp, iItemCount[hb::shared::limits::MaxItems];
	CItem* pItem;
	bool   bFlag, bItemFlag[6];
	double dV1, dV2, dV3;
	uint32_t  dwTemp, dwTemp2;
	uint16_t   wTemp;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	m_pGame->m_pClientList[iClientH]->m_iSkillMsgRecvCount++;

	const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandCommonBuild>(
		pData, sizeof(hb::net::PacketCommandCommonBuild));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, sizeof(pkt->name));

	//testcode
	//PutLogList(cName);

	std::memset(cElementItemID, 0, sizeof(cElementItemID));
	for(int i = 0; i < 6; i++) {
		cElementItemID[i] = static_cast<char>(pkt->item_ids[i]);
	}

	bFlag = true;
	while (bFlag) {
		bFlag = false;
		for(int i = 0; i <= 4; i++)
			if ((cElementItemID[i] == -1) && (cElementItemID[i + 1] != -1)) {
				cElementItemID[i] = cElementItemID[i + 1];
				cElementItemID[i + 1] = -1;
				bFlag = true;
			}
	}

	for(int i = 0; i < 6; i++) bItemFlag[i] = false;

	//testcode
	//std::snprintf(G_cTxt, sizeof(G_cTxt), "%d %d %d %d %d %d", cElementItemID[0], cElementItemID[1], cElementItemID[2],
	//	     cElementItemID[3], cElementItemID[4], cElementItemID[5]);
	//PutLogList(G_cTxt);

	iPlayerSkillLevel = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13];
	iResult = m_pGame->iDice(1, 100);

	if (iResult > iPlayerSkillLevel) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::BuildItemFail, 0, 0, 0, 0);
		return;
	}

	for(int i = 0; i < 6; i++)
		if (cElementItemID[i] != -1) {
			// Item ID.
			if ((cElementItemID[i] < 0) || (cElementItemID[i] > hb::shared::limits::MaxItems)) return;
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[i]] == 0) return;
		}

	for(int i = 0; i < hb::shared::limits::MaxBuildItems; i++)
		if (m_pGame->m_pBuildItemList[i] != 0) {
			if (memcmp(m_pGame->m_pBuildItemList[i]->m_cName, cName, hb::shared::limits::ItemNameLen - 1) == 0) {

				if (m_pGame->m_pBuildItemList[i]->m_iSkillLimit > m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13]) return;

				for (x = 0; x < hb::shared::limits::MaxItems; x++)
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[x] != 0)
						iItemCount[x] = m_pGame->m_pClientList[iClientH]->m_pItemList[x]->m_dwCount;
					else iItemCount[x] = 0;

				iMatch = 0;
				iTotalValue = 0;

				for (x = 0; x < 6; x++) {
					if (m_pGame->m_pBuildItemList[i]->m_iMaterialItemCount[x] == 0) {
						iMatch++;
					}
					else {
						for (z = 0; z < 6; z++)
							if ((cElementItemID[z] != -1) && (bItemFlag[z] == false)) {

								if ((m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[z]]->m_sIDnum == m_pGame->m_pBuildItemList[i]->m_iMaterialItemID[x]) &&
									(m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[z]]->m_dwCount >=
										static_cast<uint32_t>(m_pGame->m_pBuildItemList[i]->m_iMaterialItemCount[x])) &&
									(iItemCount[cElementItemID[z]] > 0)) {
									iTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[z]]->m_sItemSpecEffectValue2;
									if (iTemp > m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13]) {
										iTemp = iTemp - (iTemp - m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13]) / 2;
									}

									iTotalValue += (iTemp * m_pGame->m_pBuildItemList[i]->m_iMaterialItemValue[x]);
									iItemCount[cElementItemID[z]] -= m_pGame->m_pBuildItemList[i]->m_iMaterialItemCount[x];
									iMatch++;
									bItemFlag[z] = true;

									goto BIH_LOOPBREAK;
								}
							}
					BIH_LOOPBREAK:;
					}
				}

				// iMatch 6     .
				if (iMatch != 6) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::BuildItemFail, 0, 0, 0, 0);
					return;
				}

				dV2 = (double)m_pGame->m_pBuildItemList[i]->m_iMaxValue;
				if (iTotalValue <= 0)
					dV3 = 1.0f;
				else dV3 = (double)iTotalValue;
				dV1 = (double)(dV3 / dV2) * 100.0f;

				iTotalValue = (int)dV1;

				pItem = new CItem;
				if (_bInitItemAttr(pItem, m_pGame->m_pBuildItemList[i]->m_cName) == false) {
					delete pItem;
					return;
				}

				// Custom-Made
				dwTemp = pItem->m_dwAttribute;
				dwTemp = dwTemp & 0xFFFFFFFE;
				dwTemp = dwTemp | 0x00000001;
				pItem->m_dwAttribute = dwTemp;

				if (pItem->GetItemType() == ItemType::Material) {
					iTemp = m_pGame->iDice(1, (iPlayerSkillLevel / 2) + 1) - 1;
					pItem->m_sItemSpecEffectValue2 = (iPlayerSkillLevel / 2) + iTemp;
					pItem->SetTouchEffectType(TouchEffectType::ID);
					pItem->m_sTouchEffectValue1 = static_cast<short>(m_pGame->iDice(1, 100000));
					pItem->m_sTouchEffectValue2 = static_cast<short>(m_pGame->iDice(1, 100000));
					pItem->m_sTouchEffectValue3 = static_cast<short>(GameClock::GetTimeMS());

				}
				else {
					dwTemp = pItem->m_dwAttribute;
					dwTemp = dwTemp & 0x0000FFFF;

					dwTemp2 = (uint16_t)m_pGame->m_pBuildItemList[i]->m_wAttribute;
					dwTemp2 = dwTemp2 << 16;

					dwTemp = dwTemp | dwTemp2;
					pItem->m_dwAttribute = dwTemp;

					iResultValue = (iTotalValue - m_pGame->m_pBuildItemList[i]->m_iAverageValue);
					// : SpecEffectValue1 , SpecEffectValue2

					// 1.   ()
					if (iResultValue > 0) {
						dV2 = (double)iResultValue;
						dV3 = (double)(100 - m_pGame->m_pBuildItemList[i]->m_iAverageValue);
						dV1 = (dV2 / dV3) * 100.0f;
						pItem->m_sItemSpecEffectValue2 = (int)dV1;
					}
					else if (iResultValue < 0) {
						dV2 = (double)(iResultValue);
						dV3 = (double)(m_pGame->m_pBuildItemList[i]->m_iAverageValue);
						dV1 = (dV2 / dV3) * 100.0f;
						pItem->m_sItemSpecEffectValue2 = (int)dV1;
					}
					else pItem->m_sItemSpecEffectValue2 = 0;

					dV2 = (double)pItem->m_sItemSpecEffectValue2;
					dV3 = (double)pItem->m_wMaxLifeSpan;
					dV1 = (dV2 / 100.0f) * dV3;

					iTemp = (int)pItem->m_wMaxLifeSpan;
					iTemp += (int)dV1;

					pItem->SetTouchEffectType(TouchEffectType::ID);
					pItem->m_sTouchEffectValue1 = static_cast<short>(m_pGame->iDice(1, 100000));
					pItem->m_sTouchEffectValue2 = static_cast<short>(m_pGame->iDice(1, 100000));
					pItem->m_sTouchEffectValue3 = static_cast<short>(GameClock::GetTimeMS());

					if (iTemp <= 0)
						wTemp = 1;
					else wTemp = (uint16_t)iTemp;

					if (wTemp <= pItem->m_wMaxLifeSpan * 2) {
						pItem->m_wMaxLifeSpan = wTemp;
						pItem->m_sItemSpecEffectValue1 = (short)wTemp;
						pItem->m_wCurLifeSpan = pItem->m_wMaxLifeSpan;
					}
					else pItem->m_sItemSpecEffectValue1 = (short)pItem->m_wMaxLifeSpan;

					// Custom-Item  2.
					pItem->m_cItemColor = 2;
				}

				//testcode
				hb::logger::log("Custom-Item({}) Value({}) Life({}/{})", pItem->m_cName, pItem->m_sItemSpecEffectValue2, pItem->m_wCurLifeSpan, pItem->m_wMaxLifeSpan);

				bAddItem(iClientH, pItem, 0);
				m_pGame->SendNotifyMsg(0, iClientH, Notify::BuildItemSuccess, pItem->m_sItemSpecEffectValue2, pItem->m_cItemType, 0, 0); // Integer

				for (x = 0; x < 6; x++)
					if (cElementItemID[x] != -1) {
						if (m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[x]] == 0) {
							// ### BUG POINT!!!
							hb::logger::log<log_channel::events>("(?) Char({}) ElementItemID({})", m_pGame->m_pClientList[iClientH]->m_cCharName, cElementItemID[x]);
						}
						else {
							iCount = m_pGame->m_pClientList[iClientH]->m_pItemList[cElementItemID[x]]->m_dwCount - m_pGame->m_pBuildItemList[i]->m_iMaterialItemCount[x];
							if (iCount < 0) iCount = 0;
							SetItemCount(iClientH, cElementItemID[x], iCount);
						}
					}

				if (m_pGame->m_pBuildItemList[i]->m_iMaxSkill > m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13])
					m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 13, 1);

				m_pGame->GetExp(iClientH, m_pGame->iDice(1, (m_pGame->m_pBuildItemList[i]->m_iSkillLimit / 4))); //m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->iDice(1, (m_pGame->m_pBuildItemList[i]->m_iSkillLimit/4));

				return;
			}
		}

}

void ItemManager::_AdjustRareItemValue(CItem* pItem)
{
	uint32_t dwSWEType, dwSWEValue;
	double dV1, dV2, dV3;

	if ((pItem->m_dwAttribute & 0x00F00000) != 0) {
		dwSWEType = (pItem->m_dwAttribute & 0x00F00000) >> 20;
		dwSWEValue = (pItem->m_dwAttribute & 0x000F0000) >> 16;
		// 0-None 1- 2- 3-
		// 5- 6- 7- 8- 9-
		switch (dwSWEType) {
		case 0: break;

		case 5:
			pItem->m_cSpeed--;
			if (pItem->m_cSpeed < 0) pItem->m_cSpeed = 0;
			break;

		case 6:
			dV2 = (double)pItem->m_wWeight;
			dV3 = (double)(dwSWEValue * 4);
			dV1 = (dV3 / 100.0f) * dV2;
			pItem->m_wWeight -= (int)dV1;

			if (pItem->m_wWeight < 1) pItem->m_wWeight = 1;
			break;

		case 8:
		case 9:
			dV2 = (double)pItem->m_wMaxLifeSpan;
			dV3 = (double)(dwSWEValue * 7);
			dV1 = (dV3 / 100.0f) * dV2;
			pItem->m_wMaxLifeSpan += (int)dV1;
			break;
		}
	}
}

int ItemManager::RollAttributeValue()
{
	// Weighted roll for values 1-13 (original distribution)
	static const int weights[] = { 10000, 7400, 5000, 3000, 2000, 1000, 500, 400, 300, 200, 100, 70, 30 };
	static const int totalWeight = 30000;

	int roll = rand() % totalWeight;
	int cumulative = 0;
	for(int i = 0; i < 13; i++) {
		cumulative += weights[i];
		if (roll < cumulative) return i + 1;
	}
	return 1;
}

bool ItemManager::GenerateItemAttributes(CItem* pItem)
{
	if (pItem == nullptr) return false;

	AttributePrefixType primaryType = AttributePrefixType::None;
	int primaryValue = 0;
	SecondaryEffectType secondaryType = SecondaryEffectType::None;
	int secondaryValue = 0;
	int itemColor = 0;

	if (pItem->GetItemEffectType() == ItemEffectType::Attack) {
		// Attack weapons - roll primary prefix
		int roll = rand() % 10000;
		int cumul = 0;

		struct { int weight; AttributePrefixType type; int color; int minVal; } attackPrimary[] = {
			{ 299,  AttributePrefixType::Light,      2, 4 },
			{ 700,  AttributePrefixType::Strong,     3, 2 },
			{ 1500, AttributePrefixType::Critical,   5, 5 },
			{ 2000, AttributePrefixType::Agile,      1, 0 },
			{ 2000, AttributePrefixType::Righteous,  7, 0 },
			{ 1600, AttributePrefixType::Poisoning,  4, 4 },
			{ 1600, AttributePrefixType::Sharp,      6, 0 },
			{ 301,  AttributePrefixType::Ancient,    8, 0 },
		};

		for (auto& entry : attackPrimary) {
			cumul += entry.weight;
			if (roll < cumul) {
				primaryType = entry.type;
				itemColor = entry.color;
				primaryValue = RollAttributeValue();
				if (primaryValue < entry.minVal) primaryValue = entry.minVal;
				break;
			}
		}

		// Secondary effect - 40% chance (original rate)
		if (rand() % 100 < 40) {
		int secRoll = rand() % 10000;
		int secCumul = 0;

		struct { int weight; SecondaryEffectType type; int minVal; int maxVal; int fixedVal; } attackSecondary[] = {
			{ 4999, SecondaryEffectType::HittingProb,       3, 0, 0 },
			{ 3500, SecondaryEffectType::ConsecutiveAttack,  0, 7, 0 },
			{ 1000, SecondaryEffectType::GoldBonus,          0, 0, 5 },
			{ 501,  SecondaryEffectType::ExperienceBonus,    0, 0, 2 },
		};

		for (auto& entry : attackSecondary) {
			secCumul += entry.weight;
			if (secRoll < secCumul) {
				secondaryType = entry.type;
				if (entry.fixedVal > 0) {
					secondaryValue = entry.fixedVal;
				} else {
					secondaryValue = RollAttributeValue();
					if (secondaryValue < entry.minVal) secondaryValue = entry.minVal;
					if (entry.maxVal > 0 && secondaryValue > entry.maxVal) secondaryValue = entry.maxVal;
				}
				break;
			}
		}
		} // end 40% secondary chance
	}
	else if (pItem->GetItemEffectType() == ItemEffectType::Defense) {
		// Defense armor - roll primary prefix
		int roll = rand() % 10000;
		int cumul = 0;

		struct { int weight; AttributePrefixType type; int minVal; bool halved; } defensePrimary[] = {
			{ 5999, AttributePrefixType::Strong,         2, false },
			{ 3000, AttributePrefixType::Light,          4, false },
			{ 555,  AttributePrefixType::ManaConverting,  0, true },
			{ 446,  AttributePrefixType::CritChance,      0, true },
		};

		for (auto& entry : defensePrimary) {
			cumul += entry.weight;
			if (roll < cumul) {
				primaryType = entry.type;
				itemColor = 0;
				primaryValue = RollAttributeValue();
				if (entry.halved) primaryValue = primaryValue / 2;
				if (primaryValue < entry.minVal) primaryValue = entry.minVal;
				break;
			}
		}

		// Secondary effect - 40% chance (original rate)
		if (rand() % 100 < 40) {
		int secRoll = rand() % 10001;
		int secCumul = 0;

		struct { int weight; SecondaryEffectType type; int minVal; } defenseSecondary[] = {
			{ 1000, SecondaryEffectType::DefenseRatio,      3 },
			{ 3000, SecondaryEffectType::PoisonResistance,  3 },
			{ 1500, SecondaryEffectType::SPRecovery,        0 },
			{ 1000, SecondaryEffectType::HPRecovery,        0 },
			{ 1000, SecondaryEffectType::MPRecovery,        0 },
			{ 1900, SecondaryEffectType::MagicResistance,   3 },
			{ 400,  SecondaryEffectType::PhysicalAbsorb,    3 },
			{ 201,  SecondaryEffectType::MagicAbsorb,       3 },
		};

		for (auto& entry : defenseSecondary) {
			secCumul += entry.weight;
			if (secRoll < secCumul) {
				secondaryType = entry.type;
				secondaryValue = RollAttributeValue();
				if (secondaryValue < entry.minVal) secondaryValue = entry.minVal;
				break;
			}
		}
		} // end 40% secondary chance
	}
	else if (pItem->GetItemEffectType() == ItemEffectType::AttackManaSave) {
		// AttackManaSave - always type Special
		primaryType = AttributePrefixType::Special;
		itemColor = 5;
		primaryValue = RollAttributeValue();

		// Secondary effect - 40% chance (original rate)
		// Same secondary pool as attack weapons
		if (rand() % 100 < 40) {
		int secRoll = rand() % 10000;
		int secCumul = 0;

		struct { int weight; SecondaryEffectType type; int minVal; int maxVal; int fixedVal; } manaSaveSecondary[] = {
			{ 4999, SecondaryEffectType::HittingProb,       3, 0, 0 },
			{ 3500, SecondaryEffectType::ConsecutiveAttack,  0, 7, 0 },
			{ 1000, SecondaryEffectType::GoldBonus,          0, 0, 5 },
			{ 501,  SecondaryEffectType::ExperienceBonus,    0, 0, 2 },
		};

		for (auto& entry : manaSaveSecondary) {
			secCumul += entry.weight;
			if (secRoll < secCumul) {
				secondaryType = entry.type;
				if (entry.fixedVal > 0) {
					secondaryValue = entry.fixedVal;
				} else {
					secondaryValue = RollAttributeValue();
					if (secondaryValue < entry.minVal) secondaryValue = entry.minVal;
					if (entry.maxVal > 0 && secondaryValue > entry.maxVal) secondaryValue = entry.maxVal;
				}
				break;
			}
		}
		} // end 40% secondary chance
	}
	else {
		// Item has no applicable effect type
		return false;
	}

	// Clamp values to nibble range (0-15)
	if (primaryValue > 15) primaryValue = 15;
	if (secondaryValue > 15) secondaryValue = 15;

	pItem->m_cItemColor = (char)itemColor;
	pItem->m_dwAttribute = BuildAttribute(
		false, // customMade = false for drops
		primaryType,
		(uint8_t)primaryValue,
		secondaryType,
		(uint8_t)secondaryValue,
		0 // no enchant bonus
	);

	_AdjustRareItemValue(pItem);
	return true;
}

void ItemManager::RequestSellItemListHandler(int iClientH, char* pData)
{
	int iAmount;
	char cIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	const auto* req = hb::net::PacketCast<hb::net::PacketRequestSellItemList>(pData, sizeof(hb::net::PacketRequestSellItemList));
	if (!req) return;

	for(int i = 0; i < 12; i++) {
		cIndex = static_cast<char>(req->entries[i].index);
		iAmount = req->entries[i].amount;

		if ((cIndex == -1) || (cIndex < 0) || (cIndex >= hb::shared::limits::MaxItems)) return;
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[cIndex] == 0) return;

		// cIndex   .
		ReqSellItemConfirmHandler(iClientH, cIndex, iAmount, 0);
		if (m_pGame->m_pClientList[iClientH] == 0) return;
	}
}

int ItemManager::iGetItemWeight(CItem* pItem, int iCount)
{
	int iWeight;

	// . Gold   20 1
	iWeight = (pItem->m_wWeight);
	if (iCount < 0) iCount = 1;
	iWeight = iWeight * iCount;
	if (pItem->m_sIDnum == 90) iWeight = iWeight / 20;
	if (iWeight <= 0) iWeight = 1;

	return iWeight;
}

bool ItemManager::bCopyItemContents(CItem* pCopy, CItem* pOriginal)
{
	if (pOriginal == 0) return false;
	if (pCopy == 0) return false;

	pCopy->m_sIDnum = pOriginal->m_sIDnum;
	pCopy->m_cItemType = pOriginal->m_cItemType;
	pCopy->m_cEquipPos = pOriginal->m_cEquipPos;
	pCopy->m_sItemEffectType = pOriginal->m_sItemEffectType;
	pCopy->m_sItemEffectValue1 = pOriginal->m_sItemEffectValue1;
	pCopy->m_sItemEffectValue2 = pOriginal->m_sItemEffectValue2;
	pCopy->m_sItemEffectValue3 = pOriginal->m_sItemEffectValue3;
	pCopy->m_sItemEffectValue4 = pOriginal->m_sItemEffectValue4;
	pCopy->m_sItemEffectValue5 = pOriginal->m_sItemEffectValue5;
	pCopy->m_sItemEffectValue6 = pOriginal->m_sItemEffectValue6;
	pCopy->m_wMaxLifeSpan = pOriginal->m_wMaxLifeSpan;
	pCopy->m_sSpecialEffect = pOriginal->m_sSpecialEffect;

	//short m_sSM_HitRatio, m_sL_HitRatio;
	pCopy->m_sSpecialEffectValue1 = pOriginal->m_sSpecialEffectValue1;
	pCopy->m_sSpecialEffectValue2 = pOriginal->m_sSpecialEffectValue2;

	pCopy->m_sSprite = pOriginal->m_sSprite;
	pCopy->m_sSpriteFrame = pOriginal->m_sSpriteFrame;

	pCopy->m_cApprValue = pOriginal->m_cApprValue;
	pCopy->m_cSpeed = pOriginal->m_cSpeed;

	pCopy->m_wPrice = pOriginal->m_wPrice;
	pCopy->m_wWeight = pOriginal->m_wWeight;
	pCopy->m_sLevelLimit = pOriginal->m_sLevelLimit;
	pCopy->m_cGenderLimit = pOriginal->m_cGenderLimit;

	pCopy->m_sRelatedSkill = pOriginal->m_sRelatedSkill;

	pCopy->m_cCategory = pOriginal->m_cCategory;
	pCopy->m_bIsForSale = pOriginal->m_bIsForSale;

	pCopy->m_dwCount = pOriginal->m_dwCount;
	pCopy->m_sTouchEffectType = pOriginal->m_sTouchEffectType;
	pCopy->m_sTouchEffectValue1 = pOriginal->m_sTouchEffectValue1;
	pCopy->m_sTouchEffectValue2 = pOriginal->m_sTouchEffectValue2;
	pCopy->m_sTouchEffectValue3 = pOriginal->m_sTouchEffectValue3;
	pCopy->m_cItemColor = pOriginal->m_cItemColor;
	pCopy->m_sItemSpecEffectValue1 = pOriginal->m_sItemSpecEffectValue1;
	pCopy->m_sItemSpecEffectValue2 = pOriginal->m_sItemSpecEffectValue2;
	pCopy->m_sItemSpecEffectValue3 = pOriginal->m_sItemSpecEffectValue3;
	pCopy->m_wCurLifeSpan = pOriginal->m_wCurLifeSpan;
	pCopy->m_dwAttribute = pOriginal->m_dwAttribute;

	return true;
}

bool ItemManager::_bItemLog(int iAction, int iGiveH, int iRecvH, CItem* pItem, bool bForceItemLog)
{
	if (pItem == 0) return false;
	if (m_pGame->m_pClientList[iGiveH] == 0) return false;

	switch (iAction) {

	case ItemLogAction::Exchange:
		if (m_pGame->m_pClientList[iRecvH] == 0) return false;
		hb::logger::log<log_channel::trade>("{}{} IP({}) Exchange {} at {}({},{}) -> {}", is_item_suspicious(pItem) ? "[SUSPICIOUS] " : "", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY, m_pGame->m_pClientList[iRecvH]->m_cCharName);
		break;

	case ItemLogAction::Give:
		if (m_pGame->m_pClientList[iRecvH] == 0) return false;
		hb::logger::log<log_channel::trade>("{}{} IP({}) Give {} at {}({},{}) -> {}", is_item_suspicious(pItem) ? "[SUSPICIOUS] " : "", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY, m_pGame->m_pClientList[iRecvH]->m_cCharName);
		break;

	case ItemLogAction::Drop:
		hb::logger::log<log_channel::drops>("{} IP({}) Drop {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Get:
		hb::logger::log<log_channel::drops>("{} IP({}) Get {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Make:
		hb::logger::log<log_channel::crafting>("{} IP({}) Make {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Deplete:
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, "Deplete", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Buy:
		hb::logger::log<log_channel::shop>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, "Buy", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Sell:
		hb::logger::log<log_channel::shop>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, "Sell", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Retrieve:
		hb::logger::log<log_channel::bank>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, "Retrieve", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::Deposit:
		hb::logger::log<log_channel::bank>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, "Deposit", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::UpgradeFail:
		hb::logger::log<log_channel::upgrades>("{} IP({}) Upgrade {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, false ? "Success" : "Fail", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	case ItemLogAction::UpgradeSuccess:
		hb::logger::log<log_channel::upgrades>("{} IP({}) Upgrade {} {} at {}({},{})", m_pGame->m_pClientList[iGiveH]->m_cCharName, m_pGame->m_pClientList[iGiveH]->m_cIPaddress, true ? "Success" : "Fail", format_item_info(pItem), m_pGame->m_pClientList[iGiveH]->m_cMapName, m_pGame->m_pClientList[iGiveH]->m_sX, m_pGame->m_pClientList[iGiveH]->m_sY);
		break;

	default:
		return false;
	}
	return true;
}

bool ItemManager::_bItemLog(int iAction, int iClientH, char* cName, CItem* pItem)
{
	if (pItem == 0) return false;
	if (_bCheckGoodItem(pItem) == false) return false;
	if (iAction != ItemLogAction::NewGenDrop)
	{
		if (m_pGame->m_pClientList[iClientH] == 0) return false;
	}
	char cTemp1[120];
	std::memset(cTemp1, 0, sizeof(cTemp1));
	if (m_pGame->m_pClientList[iClientH] != 0) m_pGame->m_pClientList[iClientH]->m_pXSock->iGetPeerAddress(cTemp1);

	switch (iAction) {

	case ItemLogAction::NewGenDrop:
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", cName ? cName : "Unknown", "", "NpcDrop", format_item_info(pItem), "", 0, 0);
		break;

	case ItemLogAction::SkillLearn:
	case ItemLogAction::MagicLearn:
		if (cName == 0) return false;
		if (m_pGame->m_pClientList[iClientH] == 0) return false;
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iClientH]->m_cCharName, cTemp1, "Learn", format_item_info(pItem), m_pGame->m_pClientList[iClientH]->m_cMapName, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
		break;

	case ItemLogAction::SummonMonster:
		if (cName == 0) return false;
		if (m_pGame->m_pClientList[iClientH] == 0) return false;
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iClientH]->m_cCharName, cTemp1, "Summon", format_item_info(pItem), m_pGame->m_pClientList[iClientH]->m_cMapName, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
		break;

	case ItemLogAction::Poisoned:
		if (m_pGame->m_pClientList[iClientH] == 0) return false;
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iClientH]->m_cCharName, cTemp1, "Poisoned", format_item_info(pItem), m_pGame->m_pClientList[iClientH]->m_cMapName, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
		break;

	case ItemLogAction::Repair:
		if (cName == 0) return false;
		if (m_pGame->m_pClientList[iClientH] == 0) return false;
		hb::logger::log<log_channel::items_misc>("{} IP({}) {} {} at {}({},{})", m_pGame->m_pClientList[iClientH]->m_cCharName, cTemp1, "Repair", format_item_info(pItem), m_pGame->m_pClientList[iClientH]->m_cMapName, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
		break;

	default:
		return false;
	}
	return true;
}

bool ItemManager::_bCheckGoodItem(CItem* pItem)
{
	if (pItem == 0) return false;

	if (pItem->m_sIDnum == 90)
	{
		if (pItem->m_dwCount > 10000) return true;  // Gold  10000   .
		else return false;
	}
	switch (pItem->m_sIDnum) {
		// case 90: // Gold
	case 259:
	case 290:
	case 291:
	case 292:
	case 300:
	case 305:
	case 308:
	case 311:
	case 334:
	case 335:
	case 336:
	case 338:
	case 380:
	case 381:
	case 382:
	case 391:
	case 400:
	case 401:
	case 490:
	case 491:
	case 492:
	case 508:
	case 581:
	case 610:
	case 611:
	case 612:
	case 613:
	case 614:
	case 616:
	case 618:

	case 620:
	case 621:
	case 622:
	case 623:

	case 630:
	case 631:

	case 632:
	case 633:
	case 634:
	case 635:
	case 636:
	case 637:
	case 638:
	case 639:
	case 640:
	case 641:

	case 642:
	case 643:

	case 644:
	case 645:
	case 646:
	case 647:

	case 650:
	case 654:
	case 655:
	case 656:
	case 657:

	case 700:
	case 701:
	case 702:
	case 703:
	case 704:
	case 705:
	case 706:
	case 707:
	case 708:
	case 709:
	case 710:
	case 711:
	case 712:
	case 713:
	case 714:
	case 715:

	case 720:
	case 721:
	case 722:
	case 723:

	case 724:
	case 725:
	case 726:
	case 727:
	case 728:
	case 729:
	case 730:
	case 731:
	case 732:
	case 733:

	case 734:
	case 735:

	case 736:
	case 737:
	case 738:
	case 924:

		return true;
		break;
	default:
		if ((pItem->m_dwAttribute & 0xF0F0F001) == 0) return false;
		else if (pItem->m_sIDnum > 30) return true;
		else return false;
	}
}

bool ItemManager::bCheckAndConvertPlusWeaponItem(int iClientH, int iItemIndex)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] == 0) return false;

	switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
	case 4:  // Dagger +1
	case 9:  // Short Sword +1
	case 13: // Main Gauge +1
	case 16: // Gradius +1
	case 18: // Long Sword +1
	case 19: // Long Sword +2
	case 21: // Excaliber +1
	case 24: // Sabre +1
	case 26: // Scimitar +1
	case 27: // Scimitar +2
	case 29: // Falchoin +1
	case 30: // Falchion +2
	case 32: // Esterk +1
	case 33: // Esterk +2
	case 35: // Rapier +1
	case 36: // Rapier +2
	case 39: // Broad Sword +1
	case 40: // Broad Sword +2
	case 43: // Bastad Sword +1
	case 44: // Bastad Sword +2
	case 47: // Claymore +1
	case 48: // Claymore +2
	case 51: // Great Sword +1
	case 52: // Great Sword +2
	case 55: // Flameberge +1
	case 56: // Flameberge +2
	case 60: // Light Axe +1
	case 61: // Light Axe +2
	case 63: // Tomahoc +1
	case 64: // Tomohoc +2
	case 66: // Sexon Axe +1
	case 67: // Sexon Axe +2
	case 69: // Double Axe +1
	case 70: // Double Axe +2
	case 72: // War Axe +1
	case 73: // War Axe +2

	case 580: // Battle Axe +1
	case 581: // Battle Axe +2
	case 582: // Sabre +2
		return true;
		break;
	}
	return false;
}

void ItemManager::ReqCreateSlateHandler(int iClientH, char* pData)
{
	int iRet;
	char cItemID[4], ctr[4];
	char cSlateColour;
	bool bIsSlatePresent = false;
	CItem* pItem;
	int iSlateType, iEraseReq;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsOnServerChange) return;

	for(int i = 0; i < 4; i++) {
		cItemID[i] = 0;
		ctr[i] = 0;
	}
	const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandCommonItems>(
		pData, sizeof(hb::net::PacketCommandCommonItems));
	if (!pkt) return;

	// 14% chance of creating slates
	if (m_pGame->iDice(1, 100) < static_cast<uint32_t>(m_pGame->m_sSlateSuccessRate)) bIsSlatePresent = true;

	try {
		// make sure slates really exist
		for(int i = 0; i < 4; i++) {
			cItemID[i] = static_cast<char>(pkt->item_ids[i]);

			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]] == 0 || cItemID[i] > hb::shared::limits::MaxItems) {
				bIsSlatePresent = false;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateCreateFail, 0, 0, 0, 0);
				return;
			}

			//No duping
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]]->m_sIDnum == 868)
				ctr[0] = 1;
			else if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]]->m_sIDnum == 869)
				ctr[1] = 1;
			else if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]]->m_sIDnum == 870)
				ctr[2] = 1;
			else if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]]->m_sIDnum == 871)
				ctr[3] = 1;
		}
	}
	catch (...) {
		//Crash Hacker Caught
		bIsSlatePresent = false;
		m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateCreateFail, 0, 0, 0, 0);
		hb::logger::warn<log_channel::security>("Slate hack: IP={} player={}, creating slates without required item", m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName);
		m_pGame->DeleteClient(iClientH, true, true);
		return;
	}

	// Are all 4 slates present ??
	if (ctr[0] != 1 || ctr[1] != 1 || ctr[2] != 1 || ctr[3] != 1) {
		bIsSlatePresent = false;
		return;
	}

	// if we failed, kill everything
	if (!bIsSlatePresent) {
		for(int i = 0; i < 4; i++) {
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]] != 0) {
				ItemDepleteHandler(iClientH, cItemID[i], false);
			}
		}
		m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateCreateFail, 0, 0, 0, 0);
		return;
	}

	// make the slates
	for(int i = 0; i < 4; i++) {
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[cItemID[i]] != 0) {
			ItemDepleteHandler(iClientH, cItemID[i], false);
		}
	}

	pItem = new CItem;

	int i = m_pGame->iDice(1, 1000);

	if (i < 50) { // Hp slate
		iSlateType = 1;
		cSlateColour = 32;
	}
	else if (i < 250) { // Bezerk slate
		iSlateType = 2;
		cSlateColour = 3;
	}
	else if (i < 750) { // Exp slate
		iSlateType = 4;
		cSlateColour = 7;
	}
	else if (i < 950) { // Mana slate
		iSlateType = 3;
		cSlateColour = 37;
	}
	else if (i < 1001) { // Hp slate
		iSlateType = 1;
		cSlateColour = 32;
	}

	// Notify client
	m_pGame->SendNotifyMsg(0, iClientH, Notify::SlateCreateSuccess, iSlateType, 0, 0, 0);

	// Create slates
	if (_bInitItemAttr(pItem, 867) == false) {
		delete pItem;
		return;
	}
	else {
		pItem->SetTouchEffectType(TouchEffectType::ID);
		pItem->m_sTouchEffectValue1 = static_cast<short>(m_pGame->iDice(1, 100000));
		pItem->m_sTouchEffectValue2 = static_cast<short>(m_pGame->iDice(1, 100000));
		pItem->m_sTouchEffectValue3 = (short)GameClock::GetTimeMS();

		_bItemLog(ItemLogAction::Get, iClientH, -1, pItem);

		pItem->m_sItemSpecEffectValue2 = iSlateType;
		pItem->m_cItemColor = cSlateColour;
		if (_bAddClientItemList(iClientH, pItem, &iEraseReq)) {
			iRet = SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return;
			}
		}
		else {
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, pItem);
			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute);
			iRet = SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

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
	return;
}

void ItemManager::SetSlateFlag(int iClientH, short sType, bool bFlag)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;

	if (sType == SlateClearNotify) {
		m_pGame->m_pClientList[iClientH]->m_status.bSlateInvincible = false;
		m_pGame->m_pClientList[iClientH]->m_status.bSlateMana = false;
		m_pGame->m_pClientList[iClientH]->m_status.bSlateExp = false;
		return;
	}

	if (bFlag) {
		if (sType == 1) { // Invincible slate
			m_pGame->m_pClientList[iClientH]->m_status.bSlateInvincible = true;
		}
		else if (sType == 3) { // Mana slate
			m_pGame->m_pClientList[iClientH]->m_status.bSlateMana = true;
		}
		else if (sType == 4) { // Exp slate
			m_pGame->m_pClientList[iClientH]->m_status.bSlateExp = true;
		}
	}
	else {
		if (m_pGame->m_pClientList[iClientH]->m_status.bSlateInvincible) {
			m_pGame->m_pClientList[iClientH]->m_status.bSlateInvincible = false;
		}
		else if (m_pGame->m_pClientList[iClientH]->m_status.bSlateMana) {
			m_pGame->m_pClientList[iClientH]->m_status.bSlateMana = false;
		}
		else if (m_pGame->m_pClientList[iClientH]->m_status.bSlateExp) {
			m_pGame->m_pClientList[iClientH]->m_status.bSlateExp = false;
		}
	}

	m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
}

void ItemManager::_ClearExchangeStatus(int iToH)
{
	if ((iToH <= 0) || (iToH >= MaxClients)) return;
	if (m_pGame->m_pClientList[iToH] == 0) return;

	if (m_pGame->m_pClientList[iToH]->m_cExchangeName)
		m_pGame->SendNotifyMsg(0, iToH, Notify::CancelExchangeItem, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0);

	// m_pGame->m_pClientList[iToH]->m_cExchangeName    = false;
	m_pGame->m_pClientList[iToH]->m_dwInitCCTime = false;
	m_pGame->m_pClientList[iToH]->m_iAlterItemDropIndex = 0;
	//m_pGame->m_pClientList[iToH]->m_cExchangeItemIndex = -1;
	m_pGame->m_pClientList[iToH]->m_iExchangeH = 0;

	m_pGame->m_pClientList[iToH]->m_bIsExchangeMode = false;

	std::memset(m_pGame->m_pClientList[iToH]->m_cExchangeName, 0, sizeof(m_pGame->m_pClientList[iToH]->m_cExchangeName));

}

void ItemManager::CancelExchangeItem(int iClientH)
{
	int iExH;

	iExH = m_pGame->m_pClientList[iClientH]->m_iExchangeH;
	_ClearExchangeStatus(iExH);
	_ClearExchangeStatus(iClientH);
}

bool ItemManager::bCheckIsItemUpgradeSuccess(int iClientH, int iItemIndex, int iSomH, bool bBonus)
{
	int iValue, iProb, iResult;

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[iSomH] == 0) return false;

	iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x0F0000000) >> 28;

	switch (iValue) {
	case 0: iProb = 30; break;  // +1 :90%     +1~+2
	case 1: iProb = 25; break;  // +2 :80%      +3
	case 2: iProb = 20; break;  // +3 :48%      +4 
	case 3: iProb = 15; break;  // +4 :24%      +5
	case 4: iProb = 10; break;  // +5 :9.6%     +6
	case 5: iProb = 10; break;  // +6 :2.8%     +7
	case 6: iProb = 8; break;  // +7 :0.57%    +8
	case 7: iProb = 8; break;  // +8 :0.05%    +9
	case 8: iProb = 5; break;  // +9 :0.004%   +10
	case 9: iProb = 3; break;  // +10:0.00016%
	default: iProb = 1; break;
	}

	if (((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00000001) != 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 > 100)) {
		if (iProb > 20)
			iProb += (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 / 10);
		else if (iProb > 7)
			iProb += (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 / 20);
		else
			iProb += (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2 / 40);
	}
	if (bBonus) iProb *= 2;

	iProb *= 100;
	iResult = m_pGame->iDice(1, 10000);

	if (iProb >= iResult) {
		_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
		return true;
	}

	_bItemLog(ItemLogAction::UpgradeFail, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);

	return false;
}

void ItemManager::ReloadItemConfigs()
{
	sqlite3* configDb = nullptr;
	std::string configDbPath;
	bool configDbCreated = false;
	if (!EnsureGameConfigDatabase(&configDb, configDbPath, &configDbCreated) || configDbCreated)
	{
		hb::logger::log("Item config reload failed: gameconfigs.db unavailable");
		return;
	}

	for(int i = 0; i < MaxItemTypes; i++)
	{
		if (m_pGame->m_pItemConfigList[i] != 0)
		{
			delete m_pGame->m_pItemConfigList[i];
			m_pGame->m_pItemConfigList[i] = 0;
		}
	}

	if (!LoadItemConfigs(configDb, m_pGame->m_pItemConfigList, MaxItemTypes))
	{
		hb::logger::log("Item config reload failed");
		CloseGameConfigDatabase(configDb);
		return;
	}

	CloseGameConfigDatabase(configDb);
	m_pGame->ComputeConfigHashes();
	hb::logger::log("Item configs reloaded successfully");
}

void ItemManager::RequestItemUpgradeHandler(int iClientH, int iItemIndex)
{
	int iItemX, iItemY, iSoM, iSoX, iSomH, iSoxH, iValue; // v2.172
	uint32_t dwTemp, dwSWEType;
	double dV1, dV2, dV3;
	short sItemUpgrade = 2;

	//hbest
	int bugint = 0;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if ((iItemIndex < 0) || (iItemIndex >= hb::shared::limits::MaxItems)) return;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] == 0) return;

	iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
	if (iValue >= 15 || iValue < 0) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 1, 0, 0, 0);
		return;
	}

	switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cCategory) {
	case 46: // Pendants are category 46
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->GetItemType() != ItemType::Equip)
		{
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return; // Pendants are type Equip
		}
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cEquipPos < 11)
		{
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return; // Pendants are left finger or more
		}
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->GetItemEffectType() != ItemEffectType::AddEffect)
		{
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return; // Pendants are EffectType AddEffect
		}
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemEffectValue1) {
		default: // Other items are not upgradable
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return; // Pendants are EffectType 14

		case 16: // AngelicPandent(STR)
		case 17: // AngelicPandent(DEX)
		case 18: // AngelicPandent(INT)
		case 19: // AngelicPandent(MAG)
			if (m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft <= 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}
			if (iValue >= 10)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}
			switch (iValue) {
			case 0:	sItemUpgrade = 10; break;
			case 1: sItemUpgrade = 11; break;
			case 2: sItemUpgrade = 13; break;
			case 3: sItemUpgrade = 16; break;
			case 4: sItemUpgrade = 20; break;
			case 5: sItemUpgrade = 25; break;
			case 6: sItemUpgrade = 31; break;
			case 7: sItemUpgrade = 38; break;
			case 8: sItemUpgrade = 46; break;
			case 9: sItemUpgrade = 55; break;
			default:
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
				break;
			}
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum1)
				|| (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum2)
				|| (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum3))
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
				return;
			}
			if ((m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft - sItemUpgrade) < 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}
			int iDicePTA = m_pGame->iDice(1, 100);
			if (iDicePTA <= 70)
			{
				m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft -= sItemUpgrade;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizonItemUpgradeLeft, m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft, 0, 0, 0);
				iValue++;
				if (iValue > 10) iValue = 10;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
			}
			else
			{
				m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft--;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizonItemUpgradeLeft, m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft, 0, 0, 0);
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
			}
			return;
			break;
		}
		break;

	case 1: // weapons upgrade
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
		case 703:
		case 709: // DarkKnightFlameberge 
		case 718: // DarkKnightGreatSword
		case 727: // DarkKnightFlamebergW
		case 736:
		case 737: // DarkKnightAxe
		case 745: // DarkKnightHammer
			if (m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft <= 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}

			sItemUpgrade = (iValue * (iValue + 6) / 8) + 2;

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum1) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum2) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum3))
			{
				if (iValue != 0) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
					return;
				}
			}

			if ((m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft - sItemUpgrade) < 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}

			m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft -= sItemUpgrade;

			m_pGame->SendNotifyMsg(0, iClientH, Notify::GizonItemUpgradeLeft, m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft, 0, 0, 0);

			if ((iValue == 0) && m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 703)
			{
				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 709) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;

			}
			else if ((iValue == 0) && ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 709) || (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 709)))
			{

				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 709) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);

				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;
			}
			else if ((iValue == 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 745))
			{

				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 745) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);

				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;
			}
			else if ((iValue == 0) && (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 737))
			{

				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 737) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);

				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;
			}
			else
			{
				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
			}
			break;

		default:

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) != 0) {
				dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) >> 20;
				if (dwSWEType == 9) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
					return;
				}
			}
			iSoX = iSoM = 0;
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
					switch (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum) {
					case 656: iSoX++; iSoxH = i; break;
					case 657: iSoM++; iSomH = i; break;
					}
				}
			if (iSoX > 0) {
				if (bCheckIsItemUpgradeSuccess(iClientH, iItemIndex, iSoxH) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28; // v2.172
					if (iValue >= 1) ItemDepleteHandler(iClientH, iItemIndex, false);
					ItemDepleteHandler(iClientH, iSoxH, false);
					return;
				}

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00000001) != 0) {
					iValue++;
					if (iValue > 10)
						iValue = 10;
					else {
						dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
						dwTemp = dwTemp & 0x0FFFFFFF;
						m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
						ItemDepleteHandler(iClientH, iSoxH, false);
					}
				}
				else {
					iValue++;
					if (iValue > 7)
						iValue = 7;
					else {
						dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
						dwTemp = dwTemp & 0x0FFFFFFF;
						m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
						ItemDepleteHandler(iClientH, iSoxH, false);
					}
				}
			}

			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
			break;
		}
		break;

	case 3:
		m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
		break;

	case 5:
		if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) != 0) {
			dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) >> 20;
			if (dwSWEType == 8) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
				return;
			}
		}
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
		case 620:
		case 623:
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return;
		default: break;
		}

		iSoX = iSoM = 0;
		for(int i = 0; i < hb::shared::limits::MaxItems; i++)
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
				switch (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum) {
				case 656: iSoX++; iSoxH = i; break;
				case 657: iSoM++; iSomH = i; break;
				}
			}

		if (iSoM > 0) {
			if (bCheckIsItemUpgradeSuccess(iClientH, iItemIndex, iSomH, true) == false) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
				iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28; // v2.172
				if (iValue >= 1) ItemDepleteHandler(iClientH, iItemIndex, false);
				ItemDepleteHandler(iClientH, iSomH, false);
				return;
			}

			iValue++;
			if (iValue > 10)
				iValue = 10;
			else {
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00000001) != 0) {
					// +20%
					dV1 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;
					dV2 = 0.2f * dV1;
					dV3 = dV1 + dV2;
				}
				else {
					// +15%
					dV1 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;
					dV2 = 0.15f * dV1;
					dV3 = dV1 + dV2;
				}
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 = (short)dV3;
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 < 0)
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1;
				ItemDepleteHandler(iClientH, iSomH, false);
			}
		}
		m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2);
		break;

	case 6: // armors upgrade
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
		case 621:
		case 622:

		case 700:
		case 701:
		case 702:
		case 704:
		case 706:
		case 707:
		case 708:
		case 710:
		case 711:
		case 712:
		case 713:
		case 724:
		case 725:
		case 726:
		case 728:
		case 729:
		case 730:
		case 731:
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
			return;

		default:
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) != 0) {
				dwSWEType = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00F00000) >> 20;
				if (dwSWEType == 8) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
					return;
				}
			}
			iSoX = iSoM = 0;
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
					switch (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum) {
					case 656: iSoX++; iSoxH = i; break;
					case 657: iSoM++; iSomH = i; break;
					}
				}
			if (iSoM > 0) {
				if (bCheckIsItemUpgradeSuccess(iClientH, iItemIndex, iSomH, true) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
					if (iValue >= 1) ItemDepleteHandler(iClientH, iItemIndex, false);
					ItemDepleteHandler(iClientH, iSomH, false);
					return;
				}
				iValue++;
				if (iValue > 10)
					iValue = 10;
				else {
					dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
					dwTemp = dwTemp & 0x0FFFFFFF;
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

					if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0x00000001) != 0) {
						dV1 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;
						dV2 = 0.2f * dV1;
						dV3 = dV1 + dV2;
					}
					else {
						dV1 = (double)m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;
						dV2 = 0.15f * dV1;
						dV3 = dV1 + dV2;
					}
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 = (short)dV3;
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 < 0)
						m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1 = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan;

					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wMaxLifeSpan = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1;
					ItemDepleteHandler(iClientH, iSomH, false);
				}
			}
			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue1, 0, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2);
			break;
		}
		break;

	case 8: // wands upgrade 
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
		case 291: // MagicWand(LLF)

		case 714:
		case 732:
		case 738:
		case 746:

			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum1) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum2) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum3))
			{
				if (iValue != 0) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
					return;
				}
			}

			if (m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft <= 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}
			sItemUpgrade = (iValue * (iValue + 6) / 8) + 2;

			if ((m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft - sItemUpgrade) < 0)
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}

			m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft -= sItemUpgrade;
			m_pGame->SendNotifyMsg(0, iClientH, Notify::GizonItemUpgradeLeft, m_pGame->m_pClientList[iClientH]->m_iGizonItemUpgradeLeft, 0, 0, 0);

			if (iValue == 0) {
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;
			}

			if ((iValue == 11) && ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 714) || (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 738)))
			{
				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;

				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 738) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;

			}
			else if ((iValue == 15) && (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 738))
			{
				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;

				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 746) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;

			}
			else if ((iValue == 15) && (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum == 746))
			{
				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;

				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 892) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					return;
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;

			}
			else
			{
				iValue += 1;
				if (iValue > 15) iValue = 15;
				dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
				dwTemp = dwTemp & 0x0FFFFFFF;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;
			}

		default:
			iSoX = iSoM = 0;
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
					switch (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum) {
					case 656: iSoX++; iSoxH = i; break;
					case 657: iSoM++; iSomH = i; break;
					}
				}
			if (iSoX > 0) {
				if (bCheckIsItemUpgradeSuccess(iClientH, iItemIndex, iSoxH) == false) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
					iValue = (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28; // v2.172
					if (iValue >= 1) ItemDepleteHandler(iClientH, iItemIndex, false);
					ItemDepleteHandler(iClientH, iSoxH, false);
					return;
				}

				iValue++;
				if (iValue > 7)
					iValue = 7;
				else {
					dwTemp = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute;
					dwTemp = dwTemp & 0x0FFFFFFF;
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute = dwTemp | (iValue << 28);
					ItemDepleteHandler(iClientH, iSoxH, false);
				}
			}

			m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);

			break;
		}
		break;

		//hbest hero cape upgrade
	case 13:
		switch (m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum) {
		case 400:
		case 401:
			iSoX = iSoM = 0;
			for(int i = 0; i < hb::shared::limits::MaxItems; i++)
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {
					switch (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_sIDnum) {
					case 656: iSoX++; iSoxH = i; break;
					case 657: iSoM++; iSomH = i; break;
					}
				}

			if (iSoM < 1) {
				return;
			}

			bugint = m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum;
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum1) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum2) ||
				(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 != m_pGame->m_pClientList[iClientH]->m_sCharIDnum3))
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 2, 0, 0, 0);
				return;
			}

			if ((m_pGame->m_pClientList[iClientH]->m_iContribution < 50) || (m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount < 50))
			{
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemUpgradeFail, 3, 0, 0, 0);
				return;
			}

			m_pGame->m_pClientList[iClientH]->m_iContribution -= 50;
			m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount -= 50;
			m_pGame->SendNotifyMsg(0, iClientH, Notify::EnemyKills, m_pGame->m_pClientList[iClientH]->m_iEnemyKillCount, 0, 0, 0);

			if (iValue == 0)
			{
				iItemX = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x;
				iItemY = m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y;

				delete m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex];
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = 0;

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex] = new CItem;

				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].x = iItemX;
				m_pGame->m_pClientList[iClientH]->m_ItemPosList[iItemIndex].y = iItemY;

				if (bugint == 400) {
					if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 427) == false) {
						m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
						return;
					}
				}
				else {
					if (_bInitItemAttr(m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex], 428) == false) {
						m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
						return;
					}
				}

				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->SetTouchEffectType(TouchEffectType::UniqueOwner);
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue1 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum1;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue2 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum2;
				m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sTouchEffectValue3 = m_pGame->m_pClientList[iClientH]->m_sCharIDnum3;

				ItemDepleteHandler(iClientH, iSomH, false);

				m_pGame->SendNotifyMsg(0, iClientH, Notify::GizoneItemChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemType,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_wCurLifeSpan, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cName,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSprite,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sSpriteFrame,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_cItemColor,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sItemSpecEffectValue2,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute,
					m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_sIDnum);
				_bItemLog(ItemLogAction::UpgradeSuccess, iClientH, (int)-1, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]);
				break;

			}

		default: break;
		}
		break;

	default:
		m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemAttributeChange, iItemIndex, m_pGame->m_pClientList[iClientH]->m_pItemList[iItemIndex]->m_dwAttribute, 0, 0);
		break;
	}
}

char ItemManager::_cCheckHeroItemEquipped(int iClientH)
{
	short sHeroLeggings, sHeroHauberk, sHeroArmor, sHeroHelm;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;

	sHeroHelm = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Head)];
	sHeroArmor = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Body)];
	sHeroHauberk = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Arms)];
	sHeroLeggings = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::Pants)];

	if ((sHeroHelm < 0) || (sHeroLeggings < 0) || (sHeroArmor < 0) || (sHeroHauberk < 0)) return 0;

	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk] == 0) return 0;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 403) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 411) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 419) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 423)) return 1;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 407) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 415) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 419) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 423)) return 2;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 404) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 412) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 420) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 424)) return 1;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 408) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 416) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 420) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 424)) return 2;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 405) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 413) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 421) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 425)) return 1;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 409) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 417) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 421) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 425)) return 2;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 406) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 414) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 422) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 426)) return 1;

	if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHelm]->m_sIDnum == 410) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroArmor]->m_sIDnum == 418) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroHauberk]->m_sIDnum == 422) &&
		(m_pGame->m_pClientList[iClientH]->m_pItemList[sHeroLeggings]->m_sIDnum == 426)) return 2;

	return 0;
}

bool ItemManager::bPlantSeedBag(int iMapIndex, int dX, int dY, int iItemEffectValue1, int iItemEffectValue2, int iClientH)
{
	int iNamingValue, tX, tY;
	short sOwnerH;
	char cOwnerType, cNpcName[hb::shared::limits::NpcNameLen], cName[hb::shared::limits::NpcNameLen], cNpcWaypointIndex[11];
	int bRet;

	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_iTotalAgriculture >= 200) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::NoMoreAgriculture, 0, 0, 0, 0);
		return false;
	}

	if (iItemEffectValue2 > m_pGame->m_pClientList[iClientH]->m_cSkillMastery[2]) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::AgricultureSkillLimit, 0, 0, 0, 0);
		return false;
	}

	iNamingValue = m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->iGetEmptyNamingValue();

	if (iNamingValue == -1) {
	}
	else {
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
		if (sOwnerH != 0 && sOwnerH == hb::shared::owner_class::Npc && m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit == 5) {
			m_pGame->SendNotifyMsg(0, iClientH, Notify::AgricultureNoArea, 0, 0, 0, 0);
			return false;
		}
		else {
			if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetIsFarm(dX, dY) == false) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::AgricultureNoArea, 0, 0, 0, 0);
				return false;
			}

			int iNpcConfigId = m_pGame->GetNpcConfigIdByName("Crops");
			std::memset(cName, 0, sizeof(cName));
			std::snprintf(cName, sizeof(cName), "XX%d", iNamingValue);
			cName[0] = '_';
			cName[1] = iMapIndex + 65;

			std::memset(cNpcWaypointIndex, 0, sizeof(cNpcWaypointIndex));
			tX = dX;
			tY = dY;

			bRet = m_pGame->bCreateNewNpc(iNpcConfigId, cName, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, 0, 0, MoveType::Random, &tX, &tY, cNpcWaypointIndex, 0, 0, 0, false, true);
			if (bRet == false) {
				m_pGame->m_pMapList[iMapIndex]->SetNamingValueEmpty(iNamingValue);
			}
			else {
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY);
				if (m_pGame->m_pNpcList[sOwnerH] == 0) return 0;
				m_pGame->m_pNpcList[sOwnerH]->m_cCropType = iItemEffectValue1;
				switch (iItemEffectValue1) {
				case 1: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 2: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 3: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 4: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 5: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 6: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 7: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 8: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 9: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 10: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 11: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 12: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				case 13: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = iItemEffectValue2; break;
				default: m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill = 100; break;
				}
				m_pGame->m_pNpcList[sOwnerH]->m_appearance.iSpecialFrame = 1;
				m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventLog, MsgType::Confirm, 0, 0, 0);
				hb::logger::log("Agriculture: skill={} type={} plant={} at ({},{}) total={}", m_pGame->m_pNpcList[sOwnerH]->m_cCropSkill, m_pGame->m_pNpcList[sOwnerH]->m_cCropType, cNpcName, tX, tY, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_iTotalAgriculture);
				return true;
			}
		}
	}
	return false;
}

void ItemManager::RequestRepairAllItemsHandler(int iClientH)
{
	int price;
	double d1, d2, d3;
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	m_pGame->m_pClientList[iClientH]->totalItemRepair = 0;

	for(int i = 0; i < hb::shared::limits::MaxItems; i++) {
		if (m_pGame->m_pClientList[iClientH]->m_pItemList[i] != 0) {

			if (((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cCategory >= 1) && (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cCategory <= 12)) ||
				((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cCategory >= 43) && (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_cCategory <= 50)))
			{
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wCurLifeSpan == m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wMaxLifeSpan)
					continue;
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wCurLifeSpan <= 0)
					price = (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wPrice / 2);
				else
				{
					d1 = (double)(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wCurLifeSpan);
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wMaxLifeSpan != 0)
						d2 = (double)(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wMaxLifeSpan);
					else
						d2 = (double)1.0f;
					d3 = (double)((d1 / d2) * 0.5f);
					d2 = (double)(m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wPrice);
					d3 = (d3 * d2);
					price = ((m_pGame->m_pClientList[iClientH]->m_pItemList[i]->m_wPrice / 2) - (short)(d3));
				}
				m_pGame->m_pClientList[iClientH]->m_stRepairAll[m_pGame->m_pClientList[iClientH]->totalItemRepair].index = i;
				m_pGame->m_pClientList[iClientH]->m_stRepairAll[m_pGame->m_pClientList[iClientH]->totalItemRepair].price = price;
				m_pGame->m_pClientList[iClientH]->totalItemRepair++;
			}
		}
	}
	m_pGame->SendNotifyMsg(0, iClientH, Notify::RepairAllPrices, 0, 0, 0, 0);
}

void ItemManager::RequestRepairAllItemsDeleteHandler(int iClientH, int index)
{
	
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	for(int i = index; i < m_pGame->m_pClientList[iClientH]->totalItemRepair; i++) {
		m_pGame->m_pClientList[iClientH]->m_stRepairAll[i] = m_pGame->m_pClientList[iClientH]->m_stRepairAll[i + 1];
	}
	m_pGame->m_pClientList[iClientH]->totalItemRepair--;
	m_pGame->SendNotifyMsg(0, iClientH, Notify::RepairAllPrices, 0, 0, 0, 0);
}

void ItemManager::RequestRepairAllItemsConfirmHandler(int iClientH)
{
	int      iRet, totalPrice = 0;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if (m_pGame->m_pClientList[iClientH]->m_pIsProcessingAllowed == false) return;

	for(int i = 0; i < m_pGame->m_pClientList[iClientH]->totalItemRepair; i++) {
		totalPrice += m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].price;
	}

	if (dwGetItemCountByID(iClientH, hb::shared::item::ItemId::Gold) < (uint32_t)totalPrice)
	{
		{
			hb::net::PacketNotifyNotEnoughGold pkt{};
			pkt.header.msg_id = MsgId::Notify;
			pkt.header.msg_type = Notify::NotEnoughGold;
			pkt.item_index = 0;
			iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
		}
		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			m_pGame->DeleteClient(iClientH, true, true);
			break;
		}

	}
	else
	{
		for(int i = 0; i < m_pGame->m_pClientList[iClientH]->totalItemRepair; i++)
		{
			if (m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].index] != 0) {
				m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].index]->m_wCurLifeSpan = m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].index]->m_wMaxLifeSpan;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::ItemRepaired, m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].index, m_pGame->m_pClientList[iClientH]->m_pItemList[m_pGame->m_pClientList[iClientH]->m_stRepairAll[i].index]->m_wCurLifeSpan, 0, 0);
			}
		}
		m_pGame->iCalcTotalWeight(SetItemCountByID(iClientH, hb::shared::item::ItemId::Gold, dwGetItemCountByID(iClientH, hb::shared::item::ItemId::Gold) - totalPrice));
	}
}
