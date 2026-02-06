#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace hb::item;

namespace NetworkMessageHandlers {
	void HandleItemPurchased(CGame* pGame, char* pData)
	{
		int i, j;
		uint32_t dwCount;
		char  cName[DEF_ITEMNAME], cEquipPos, cGenderLimit;
		ItemType cItemType;
		bool  bIsEquipped;
		short sSprite, sSpriteFrame, sLevelLimit;
		uint16_t wCost, wWeight, wCurLifeSpan, wMaxLifeSpan;
		char  cTxt[120], cItemColor;

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemPurchased>(
			pData, sizeof(hb::net::PacketNotifyItemPurchased));
		if (!pkt) return;

		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->name, sizeof(pkt->name));
		dwCount = pkt->count;
		cItemType = static_cast<ItemType>(pkt->item_type);
		cEquipPos = static_cast<char>(pkt->equip_pos);
		bIsEquipped = (pkt->is_equipped != 0);
		sLevelLimit = static_cast<short>(pkt->level_limit);
		cGenderLimit = static_cast<char>(pkt->gender_limit);
		wCurLifeSpan = pkt->cur_lifespan;
		wMaxLifeSpan = pkt->max_lifespan;
		wWeight = pkt->weight;
		sSprite = static_cast<short>(pkt->sprite);
		sSpriteFrame = static_cast<short>(pkt->sprite_frame);
		cItemColor = static_cast<char>(pkt->item_color);
		wCost = pkt->cost;
		std::memset(cTxt, 0, sizeof(cTxt));
		char cStr1[64], cStr2[64], cStr3[64];
		std::snprintf(cStr1, sizeof(cStr1), "%s", cName);
		cStr2[0] = 0;
		cStr3[0] = 0;
		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMPURCHASED, cStr1, wCost);
		pGame->AddEventList(cTxt, 10);

		short sItemID = pkt->item_id;

		if ((cItemType == ItemType::Consume) || (cItemType == ItemType::Arrow))
		{
			for (i = 0; i < DEF_MAXITEMS; i++)
				if ((pGame->m_pItemList[i] != 0) && (pGame->m_pItemList[i]->m_sIDnum == sItemID))
				{
					pGame->m_pItemList[i]->m_dwCount += dwCount;
					return;
				}
		}

		short nX, nY;
		for (i = 0; i < DEF_MAXITEMS; i++)
		{
			if ((pGame->m_pItemList[i] != 0) && (pGame->m_pItemList[i]->m_sIDnum == sItemID))
			{
				nX = pGame->m_pItemList[i]->m_sX;
				nY = pGame->m_pItemList[i]->m_sY;
				break;
			}
			else
			{
				nX = 40;
				nY = 30;
			}
		}

		for (i = 0; i < DEF_MAXITEMS; i++)
			if (pGame->m_pItemList[i] == 0)
			{
				pGame->m_pItemList[i] = std::make_unique<CItem>();
				pGame->m_pItemList[i]->m_sIDnum = sItemID;
				pGame->m_pItemList[i]->m_dwCount = dwCount;
				pGame->m_pItemList[i]->m_sX = nX;
				pGame->m_pItemList[i]->m_sY = nY;
				pGame->bSendCommand(MSGID_REQUEST_SETITEMPOS, 0, i, nX, nY, 0, 0);
				pGame->m_bIsItemDisabled[i] = false;
				pGame->m_bIsItemEquipped[i] = false;
				pGame->m_pItemList[i]->m_wCurLifeSpan = wCurLifeSpan;
				pGame->m_pItemList[i]->m_cItemColor = cItemColor;
				pGame->m_pItemList[i]->m_dwAttribute = 0;

				for (j = 0; j < DEF_MAXITEMS; j++)
					if (pGame->m_cItemOrder[j] == -1) {
						pGame->m_cItemOrder[j] = i;
						return;
					}

				return;
			}
	}

	void HandleItemObtained(CGame* pGame, char* pData)
	{
		int i, j;
		uint32_t dwCount, dwAttribute;
		char  cName[DEF_ITEMNAME], cEquipPos;
		ItemType cItemType;
		bool  bIsEquipped;
		short sSprite, sSpriteFrame, sLevelLimit, sSpecialEV2;
		char  cTxt[120], cGenderLimit, cItemColor;
		uint16_t wWeight, wCurLifeSpan, wMaxLifeSpan;

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemObtained>(
			pData, sizeof(hb::net::PacketNotifyItemObtained));
		if (!pkt) return;

		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->name, sizeof(pkt->name));
		dwCount = pkt->count;
		cItemType = static_cast<ItemType>(pkt->item_type);
		cEquipPos = static_cast<char>(pkt->equip_pos);
		bIsEquipped = (pkt->is_equipped != 0);
		sLevelLimit = static_cast<short>(pkt->level_limit);
		cGenderLimit = static_cast<char>(pkt->gender_limit);
		wCurLifeSpan = pkt->cur_lifespan;
		wMaxLifeSpan = pkt->max_lifespan;
		wWeight = pkt->weight;
		sSprite = static_cast<short>(pkt->sprite);
		sSpriteFrame = static_cast<short>(pkt->sprite_frame);
		cItemColor = static_cast<char>(pkt->item_color);
		sSpecialEV2 = static_cast<short>(pkt->spec_value2);
		dwAttribute = pkt->attribute;

		char cStr1[64], cStr2[64], cStr3[64];
		std::snprintf(cStr1, sizeof(cStr1), "%s", cName);
		cStr2[0] = 0;
		cStr3[0] = 0;

		std::memset(cTxt, 0, sizeof(cTxt));
		if (dwCount == 1) std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMOBTAINED2, cStr1);
		else std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMOBTAINED1, dwCount, cStr1);

		pGame->AddEventList(cTxt, 10);
		pGame->PlayGameSound('E', 20, 0);

		pGame->m_pMapData->bSetItem(pGame->m_pPlayer->m_sPlayerX, pGame->m_pPlayer->m_sPlayerY, 0, 0, 0, false);

		short sItemID = pkt->item_id;

		if ((cItemType == ItemType::Consume) || (cItemType == ItemType::Arrow))
		{
			for (i = 0; i < DEF_MAXITEMS; i++)
				if ((pGame->m_pItemList[i] != 0) && (pGame->m_pItemList[i]->m_sIDnum == sItemID))
				{
					pGame->m_pItemList[i]->m_dwCount += dwCount;
					pGame->m_bIsItemDisabled[i] = false;
					return;
				}
		}

		short nX, nY;
		for (i = 0; i < DEF_MAXITEMS; i++)
		{
			if ((pGame->m_pItemList[i] != 0) && (pGame->m_pItemList[i]->m_sIDnum == sItemID))
			{
				nX = pGame->m_pItemList[i]->m_sX;
				nY = pGame->m_pItemList[i]->m_sY;
				break;
			}
			else
			{
				nX = 40;
				nY = 30;
			}
		}

		for (i = 0; i < DEF_MAXITEMS; i++)
			if (pGame->m_pItemList[i] == 0)
			{
				pGame->m_pItemList[i] = std::make_unique<CItem>();
				pGame->m_pItemList[i]->m_sIDnum = sItemID;
				pGame->m_pItemList[i]->m_dwCount = dwCount;
				pGame->m_pItemList[i]->m_sX = nX;
				pGame->m_pItemList[i]->m_sY = nY;
				pGame->bSendCommand(MSGID_REQUEST_SETITEMPOS, 0, i, nX, nY, 0, 0);
				pGame->m_bIsItemDisabled[i] = false;
				pGame->m_bIsItemEquipped[i] = false;
				pGame->m_pItemList[i]->m_wCurLifeSpan = wCurLifeSpan;
				pGame->m_pItemList[i]->m_cItemColor = cItemColor;
				pGame->m_pItemList[i]->m_sItemSpecEffectValue2 = sSpecialEV2;
				pGame->m_pItemList[i]->m_dwAttribute = dwAttribute;

				pGame->_bCheckBuildItemStatus();

				for (j = 0; j < DEF_MAXITEMS; j++)
					if (pGame->m_cItemOrder[j] == -1) {
						pGame->m_cItemOrder[j] = i;
						return;
					}
				return;
			}
	}

	void HandleItemLifeSpanEnd(CGame* pGame, char* pData)
	{
		short sEquipPos, sItemIndex;
		char cTxt[120];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemLifeSpanEnd>(
			pData, sizeof(hb::net::PacketNotifyItemLifeSpanEnd));
		if (!pkt) return;
		sEquipPos = static_cast<short>(pkt->equip_pos);
		sItemIndex = static_cast<short>(pkt->item_index);

		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[sItemIndex].get(), cStr1, cStr2, cStr3);
		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMLIFE_SPANEND1, cStr1);
		pGame->AddEventList(cTxt, 10);
		pGame->m_sItemEquipmentStatus[pGame->m_pItemList[sItemIndex]->m_cEquipPos] = -1;
		pGame->m_bIsItemEquipped[sItemIndex] = false;
		pGame->m_pItemList[sItemIndex]->m_wCurLifeSpan = 0;

		pGame->PlayGameSound('E', 10, 0);
	}

	void HandleItemReleased(CGame* pGame, char* pData)
	{
		short sEquipPos, sItemIndex;
		char cTxt[120];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemReleased>(
			pData, sizeof(hb::net::PacketNotifyItemReleased));
		if (!pkt) return;
		sEquipPos = static_cast<short>(pkt->equip_pos);
		sItemIndex = static_cast<short>(pkt->item_index);

		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[sItemIndex].get(), cStr1, cStr2, cStr3);
		std::snprintf(cTxt, sizeof(cTxt), ITEM_EQUIPMENT_RELEASED, cStr1);
		pGame->AddEventList(cTxt, 10);
		pGame->m_bIsItemEquipped[sItemIndex] = false;
		pGame->m_sItemEquipmentStatus[pGame->m_pItemList[sItemIndex]->m_cEquipPos] = -1;

		{
			short sID = pGame->m_pItemList[sItemIndex]->m_sIDnum;
			if (sID == hb::item::ItemId::AngelicPandentSTR || sID == hb::item::ItemId::AngelicPandentDEX ||
				sID == hb::item::ItemId::AngelicPandentINT || sID == hb::item::ItemId::AngelicPandentMAG)
				pGame->PlayGameSound('E', 53, 0);
			else
				pGame->PlayGameSound('E', 29, 0);
		}
	}

	void HandleSetItemCount(CGame* pGame, char* pData)
	{
		short  sItemIndex;
		uint32_t dwCount;
		bool   bIsItemUseResponse;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifySetItemCount>(
			pData, sizeof(hb::net::PacketNotifySetItemCount));
		if (!pkt) return;
		sItemIndex = static_cast<short>(pkt->item_index);
		dwCount = pkt->count;
		bIsItemUseResponse = (pkt->notify != 0);
		if (pGame->m_pItemList[sItemIndex] != 0)
		{
			pGame->m_pItemList[sItemIndex]->m_dwCount = dwCount;
			if (bIsItemUseResponse == true) pGame->m_bIsItemDisabled[sItemIndex] = false;
		}
	}

	void HandleItemDepleted_EraseItem(CGame* pGame, char* pData)
	{
		short  sItemIndex;
		bool   bIsUseItemResult;
		char   cTxt[120];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemDepletedEraseItem>(
			pData, sizeof(hb::net::PacketNotifyItemDepletedEraseItem));
		if (!pkt) return;
		sItemIndex = static_cast<short>(pkt->item_index);
		bIsUseItemResult = (pkt->use_result != 0);

		std::memset(cTxt, 0, sizeof(cTxt));

		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[sItemIndex].get(), cStr1, cStr2, cStr3);

		CItem* pCfg = pGame->GetItemConfig(pGame->m_pItemList[sItemIndex]->m_sIDnum);

		if (pGame->m_bIsItemEquipped[sItemIndex] == true) {
			std::snprintf(cTxt, sizeof(cTxt), ITEM_EQUIPMENT_RELEASED, cStr1);
			pGame->AddEventList(cTxt, 10);

			if (pCfg) pGame->m_sItemEquipmentStatus[pCfg->m_cEquipPos] = -1;
			pGame->m_bIsItemEquipped[sItemIndex] = false;
		}

		std::memset(cTxt, 0, sizeof(cTxt));
		if (pCfg && ((pCfg->GetItemType() == ItemType::Consume) ||
			(pCfg->GetItemType() == ItemType::Arrow))) {
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMDEPlETED_ERASEITEM2, cStr1);
		}
		else if (pCfg) {
			if (pCfg->GetItemType() == ItemType::UseDeplete) {
				if (bIsUseItemResult == true) {
					std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMDEPlETED_ERASEITEM3, cStr1);
				}
			}
			else if (pCfg->GetItemType() == ItemType::Eat) {
				if (bIsUseItemResult == true) {
					std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMDEPlETED_ERASEITEM4, cStr1);
					if ((pGame->m_pPlayer->m_sPlayerType >= 1) && (pGame->m_pPlayer->m_sPlayerType <= 3))
						pGame->PlayGameSound('C', 19, 0);
					if ((pGame->m_pPlayer->m_sPlayerType >= 4) && (pGame->m_pPlayer->m_sPlayerType <= 6))
						pGame->PlayGameSound('C', 20, 0);
				}
			}
			else if (pCfg->GetItemType() == ItemType::UseDepleteDest) {
				if (bIsUseItemResult == true) {
					std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMDEPlETED_ERASEITEM3, cStr1);
				}
			}
			else {
				if (bIsUseItemResult == true) {
					std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMDEPlETED_ERASEITEM6, cStr1);
					pGame->PlayGameSound('E', 10, 0);
				}
			}
		}
		pGame->AddEventList(cTxt, 10);

		if (bIsUseItemResult == true) pGame->m_bItemUsingStatus = false;
		pGame->EraseItem((char)sItemIndex);
		pGame->_bCheckBuildItemStatus();
	}

	void HandleDropItemFin_EraseItem(CGame* pGame, char* pData)
	{
		int iAmount;
		short  sItemIndex;
		char   cTxt[120];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyDropItemFinEraseItem>(
			pData, sizeof(hb::net::PacketNotifyDropItemFinEraseItem));
		if (!pkt) return;
		sItemIndex = static_cast<short>(pkt->item_index);
		iAmount = static_cast<int>(pkt->amount);

		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[sItemIndex].get(), cStr1, cStr2, cStr3);

		std::memset(cTxt, 0, sizeof(cTxt));
		if (pGame->m_bIsItemEquipped[sItemIndex] == true)
		{
			std::snprintf(cTxt, sizeof(cTxt), ITEM_EQUIPMENT_RELEASED, cStr1);
			pGame->AddEventList(cTxt, 10);
			pGame->m_sItemEquipmentStatus[pGame->m_pItemList[sItemIndex]->m_cEquipPos] = -1;
			pGame->m_bIsItemEquipped[sItemIndex] = false;
		}
		if (pGame->m_pPlayer->m_iHP > 0)
		{
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_THROW_ITEM2, cStr1);
		}
		else
		{
			if (iAmount < 2)
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_DROPITEMFIN_ERASEITEM3, cStr1);
			else
			{
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_DROPITEMFIN_ERASEITEM5, cStr1);
			}
		}
		pGame->AddEventList(cTxt, 10);
		pGame->EraseItem((char)sItemIndex);
		pGame->_bCheckBuildItemStatus();
	}

	void HandleGiveItemFin_EraseItem(CGame* pGame, char* pData)
	{
		int iAmount;
		short  sItemIndex;
		char cName[DEF_ITEMNAME], cTxt[250];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyGiveItemFinEraseItem>(
			pData, sizeof(hb::net::PacketNotifyGiveItemFinEraseItem));
		if (!pkt) return;
		sItemIndex = static_cast<short>(pkt->item_index);
		iAmount = static_cast<int>(pkt->amount);

		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->name, sizeof(pkt->name));

		char cStr1[64], cStr2[64], cStr3[64];
		CItem* pCfg = pGame->GetItemConfig(pGame->m_pItemList[sItemIndex]->m_sIDnum);
		std::snprintf(cStr1, sizeof(cStr1), "%s", pCfg ? pCfg->m_cName : "Unknown");
		cStr2[0] = 0;
		cStr3[0] = 0;

		if (pGame->m_bIsItemEquipped[sItemIndex] == true) {
			std::snprintf(cTxt, sizeof(cTxt), ITEM_EQUIPMENT_RELEASED, cStr1);
			pGame->AddEventList(cTxt, 10);

			if (pCfg) pGame->m_sItemEquipmentStatus[pCfg->m_cEquipPos] = -1;
			pGame->m_bIsItemEquipped[sItemIndex] = false;
		}
		if (cName[0] == 0) std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM2, iAmount, cStr1);
		else {
			if (strcmp(cName, "Howard") == 0)
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM3, iAmount, cStr1);
			else if (strcmp(cName, "William") == 0)
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM4, iAmount, cStr1);
			else if (strcmp(cName, "Kennedy") == 0)
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM5, iAmount, cStr1);
			else if (strcmp(cName, "Tom") == 0)
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM7, iAmount, cStr1);
			else std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_ERASEITEM8, iAmount, cStr1, cName);
		}
		pGame->AddEventList(cTxt, 10);
		pGame->EraseItem((char)sItemIndex);
		pGame->_bCheckBuildItemStatus();
	}

	void HandleItemRepaired(CGame* pGame, char* pData)
	{
		char cTxt[120];
		DWORD dwItemID, dwLife;

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemRepaired>(
			pData, sizeof(hb::net::PacketNotifyItemRepaired));
		if (!pkt) return;
		dwItemID = pkt->item_id;
		dwLife = pkt->life;

		pGame->m_pItemList[dwItemID]->m_wCurLifeSpan = (WORD)dwLife;
		pGame->m_bIsItemDisabled[dwItemID] = false;
		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[dwItemID].get(), cStr1, cStr2, cStr3);

		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMREPAIRED1, cStr1);

		pGame->AddEventList(cTxt, 10);
	}

	void HandleRepairItemPrice(CGame* pGame, char* pData)
	{
		char cName[DEF_ITEMNAME];
		DWORD wV1, wV2, wV3, wV4;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyRepairItemPrice>(
			pData, sizeof(hb::net::PacketNotifyRepairItemPrice));
		if (!pkt) return;
		wV1 = pkt->v1;
		wV2 = pkt->v2;
		wV3 = pkt->v3;
		wV4 = pkt->v4;
		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->item_name, sizeof(pkt->item_name));
		pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::SellOrRepair, 2, wV1, wV2);
		pGame->m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sV3 = wV3;
	}

	void HandleRepairAllPrices(CGame* pGame, char* pData)
	{
		int i;

		pGame->totalPrice = 0;
		const auto* header = hb::net::PacketCast<hb::net::PacketNotifyRepairAllPricesHeader>(
			pData, sizeof(hb::net::PacketNotifyRepairAllPricesHeader));
		if (!header) return;
		const auto* entries = reinterpret_cast<const hb::net::PacketNotifyRepairAllPricesEntry*>(
			pData + sizeof(hb::net::PacketNotifyRepairAllPricesHeader));
		pGame->totalItemRepair = header->total;

		for (i = 0; i < pGame->totalItemRepair; i++)
		{
			pGame->m_stRepairAll[i].index = entries[i].index;
			pGame->m_stRepairAll[i].price = entries[i].price;

			pGame->totalPrice += pGame->m_stRepairAll[i].price;
		}
		if (pGame->totalItemRepair == 0)
			pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::RepairAll, 1, 0, 0);
		else
			pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::RepairAll, 0, 0, 0);
	}

	void HandleSellItemPrice(CGame* pGame, char* pData)
	{
		char cName[DEF_ITEMNAME];
		DWORD wV1, wV2, wV3, wV4;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifySellItemPrice>(
			pData, sizeof(hb::net::PacketNotifySellItemPrice));
		if (!pkt) return;
		wV1 = pkt->v1;
		wV2 = pkt->v2;
		wV3 = pkt->v3;
		wV4 = pkt->v4;
		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->item_name, sizeof(pkt->item_name));
		pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::SellOrRepair, 1, wV1, wV2);
		pGame->m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sV3 = wV3;
		pGame->m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sV4 = wV4;
	}

	void HandleCannotRepairItem(CGame* pGame, char* pData)
	{
		char cTxt[120], cStr1[64], cStr2[64], cStr3[64];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCannotRepairItem>(
			pData, sizeof(hb::net::PacketNotifyCannotRepairItem));
		if (!pkt) return;
		const auto wV1 = pkt->item_index;
		const auto wV2 = pkt->reason;
		std::memset(cStr1, 0, sizeof(cStr1));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));
		pGame->GetItemName(pGame->m_pItemList[wV1].get(), cStr1, cStr2, cStr3);

		switch (wV2) {
		case 1:
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_REPAIR_ITEM1, cStr1);
			pGame->AddEventList(cTxt, 10);
			break;
		case 2:
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_REPAIR_ITEM2, cStr1);
			pGame->AddEventList(cTxt, 10);
			break;
		}
		pGame->m_bIsItemDisabled[wV1] = false;
	}

	void HandleCannotSellItem(CGame* pGame, char* pData)
	{
		char cTxt[120], cStr1[64], cStr2[64], cStr3[64];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCannotSellItem>(
			pData, sizeof(hb::net::PacketNotifyCannotSellItem));
		if (!pkt) return;
		const auto wV1 = pkt->item_index;
		const auto wV2 = pkt->reason;

		std::memset(cStr1, 0, sizeof(cStr1));
		std::memset(cStr2, 0, sizeof(cStr2));
		std::memset(cStr3, 0, sizeof(cStr3));
		pGame->GetItemName(pGame->m_pItemList[wV1].get(), cStr1, cStr2, cStr3);

		switch (wV2) {
		case 1:
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_SELL_ITEM1, cStr1);
			pGame->AddEventList(cTxt, 10);
			break;

		case 2:
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_SELL_ITEM2, cStr1);
			pGame->AddEventList(cTxt, 10);
			break;

		case 3:
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_SELL_ITEM3, cStr1);
			pGame->AddEventList(cTxt, 10);
			pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM4, 10);
			break;

		case 4:
			pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM5, 10);
			pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM6, 10);
			break;
		}
		pGame->m_bIsItemDisabled[wV1] = false;
	}

	void HandleCannotGiveItem(CGame* pGame, char* pData)
	{
		char cName[DEF_ITEMNAME], cTxt[256];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCannotGiveItem>(
			pData, sizeof(hb::net::PacketNotifyCannotGiveItem));
		if (!pkt) return;
		const auto wItemIndex = pkt->item_index;
		const auto iAmount = static_cast<int>(pkt->amount);
		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->name, sizeof(pkt->name));

		char cStr1[64], cStr2[64], cStr3[64];
		pGame->GetItemName(pGame->m_pItemList[wItemIndex].get(), cStr1, cStr2, cStr3);
		if (iAmount == 1) std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_GIVE_ITEM2, cStr1, cName);
		else std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_GIVE_ITEM1, iAmount, cStr1, cName);

		pGame->AddEventList(cTxt, 10);
	}

	void HandleItemColorChange(CGame* pGame, char* pData)
	{
		short sItemIndex, sItemColor;
		char cTxt[120];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemColorChange>(
			pData, sizeof(hb::net::PacketNotifyItemColorChange));
		if (!pkt) return;
		sItemIndex = static_cast<short>(pkt->item_index);
		sItemColor = static_cast<short>(pkt->item_color);

		if (pGame->m_pItemList[sItemIndex] != 0) {
			char cStr1[64], cStr2[64], cStr3[64];
			pGame->GetItemName(pGame->m_pItemList[sItemIndex].get(), cStr1, cStr2, cStr3);
			if (sItemColor != -1) {
				pGame->m_pItemList[sItemIndex]->m_cItemColor = (char)sItemColor;
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMCOLOR_CHANGE1, cStr1);
				pGame->AddEventList(cTxt, 10);
			}
			else {
				std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_ITEMCOLOR_CHANGE2, cStr1);
				pGame->AddEventList(cTxt, 10);
			}
		}
	}

	void HandleDropItemFin_CountChanged(CGame* pGame, char* pData)
	{
		char cTxt[256];

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyDropItemFinCountChanged>(
			pData, sizeof(hb::net::PacketNotifyDropItemFinCountChanged));
		if (!pkt) return;
		const auto wItemIndex = pkt->item_index;
		const auto iAmount = static_cast<int>(pkt->amount);

		char cStr1[64], cStr2[64], cStr3[64];
		CItem* pCfg = pGame->GetItemConfig(pGame->m_pItemList[wItemIndex]->m_sIDnum);
		std::snprintf(cStr1, sizeof(cStr1), "%s", pCfg ? pCfg->m_cName : "Unknown");
		cStr2[0] = 0;
		cStr3[0] = 0;
		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_THROW_ITEM1, iAmount, cStr1);

		pGame->AddEventList(cTxt, 10);
	}

	void HandleGiveItemFin_CountChanged(CGame* pGame, char* pData)
	{
		char cName[DEF_ITEMNAME], cTxt[256];
		WORD wItemIndex;
		int iAmount;

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyGiveItemFinCountChanged>(
			pData, sizeof(hb::net::PacketNotifyGiveItemFinCountChanged));
		if (!pkt) return;
		wItemIndex = pkt->item_index;
		iAmount = static_cast<int>(pkt->amount);

		std::memset(cName, 0, sizeof(cName));
		memcpy(cName, pkt->name, sizeof(pkt->name));

		char cStr1[64], cStr2[64], cStr3[64];
		CItem* pCfg = pGame->GetItemConfig(pGame->m_pItemList[wItemIndex]->m_sIDnum);
		std::snprintf(cStr1, sizeof(cStr1), "%s", pCfg ? pCfg->m_cName : "Unknown");
		cStr2[0] = 0;
		cStr3[0] = 0;
		if (iAmount == 1) std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_COUNTCHANGED1, cStr1, cName);
		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_GIVEITEMFIN_COUNTCHANGED2, iAmount, cStr1, cName);
		pGame->AddEventList(cTxt, 10);
	}

	void HandleSetExchangeItem(CGame* pGame, char* pData)
	{
		short sDir, sSprite, sSpriteFrame, sCurLife, sMaxLife, sPerformance, sItemID;
		int iAmount, i;
		char cColor, cItemName[DEF_ITEMNAME], cCharName[12];
		DWORD dwAttribute;
		std::memset(cItemName, 0, sizeof(cItemName));
		std::memset(cCharName, 0, sizeof(cCharName));

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyExchangeItem>(
			pData, sizeof(hb::net::PacketNotifyExchangeItem));
		if (!pkt) return;
		sDir = static_cast<short>(pkt->dir);
		sSprite = pkt->sprite;
		sSpriteFrame = pkt->sprite_frame;
		iAmount = pkt->amount;
		cColor = static_cast<char>(pkt->color);
		sCurLife = pkt->cur_life;
		sMaxLife = pkt->max_life;
		sPerformance = pkt->performance;
		memcpy(cItemName, pkt->item_name, sizeof(pkt->item_name));
		memcpy(cCharName, pkt->char_name, 10);
		dwAttribute = pkt->attribute;
		sItemID = pkt->item_id;

		if (sDir >= 1000)  // Set the item I want to exchange
		{
			i = 0;
			while (pGame->m_stDialogBoxExchangeInfo[i].sV1 != -1)
			{
				i++;
				if (i >= 4) return; // Error situation
			}
		}
		else // Set the item he proposes me.
		{
			i = 4;
			while (pGame->m_stDialogBoxExchangeInfo[i].sV1 != -1)
			{
				i++;
				if (i >= 8) return; // Error situation
			}
		}
		pGame->m_stDialogBoxExchangeInfo[i].sV1 = sSprite;
		pGame->m_stDialogBoxExchangeInfo[i].sV2 = sSpriteFrame;
		pGame->m_stDialogBoxExchangeInfo[i].sV3 = iAmount;
		pGame->m_stDialogBoxExchangeInfo[i].sV4 = cColor;
		pGame->m_stDialogBoxExchangeInfo[i].sV5 = (int)sCurLife;
		pGame->m_stDialogBoxExchangeInfo[i].sV6 = (int)sMaxLife;
		pGame->m_stDialogBoxExchangeInfo[i].sV7 = (int)sPerformance;
		memcpy(pGame->m_stDialogBoxExchangeInfo[i].cStr1, cItemName, DEF_ITEMNAME - 1);
		memcpy(pGame->m_stDialogBoxExchangeInfo[i].cStr2, cCharName, 10);
		pGame->m_stDialogBoxExchangeInfo[i].dwV1 = dwAttribute;
		pGame->m_stDialogBoxExchangeInfo[i].sItemID = sItemID;
	}

	void HandleOpenExchangeWindow(CGame* pGame, char* pData)
	{
		short sDir, sSprite, sSpriteFrame, sCurLife, sMaxLife, sPerformance, sItemID;
		int iAmount;
		char cColor, cItemName[DEF_ITEMNAME], cCharName[12];
		DWORD dwAttribute;
		std::memset(cItemName, 0, sizeof(cItemName));
		std::memset(cCharName, 0, sizeof(cCharName));

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyExchangeItem>(
			pData, sizeof(hb::net::PacketNotifyExchangeItem));
		if (!pkt) return;
		sDir = static_cast<short>(pkt->dir);
		sSprite = pkt->sprite;
		sSpriteFrame = pkt->sprite_frame;
		iAmount = pkt->amount;
		cColor = static_cast<char>(pkt->color);
		sCurLife = pkt->cur_life;
		sMaxLife = pkt->max_life;
		sPerformance = pkt->performance;
		memcpy(cItemName, pkt->item_name, sizeof(pkt->item_name));
		memcpy(cCharName, pkt->char_name, 10);
		dwAttribute = pkt->attribute;
		sItemID = pkt->item_id;

		pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::Exchange, 1, 0, 0, 0);
		// Initialize all exchange slots
		for (int j = 0; j < 8; j++)
		{
			std::memset(pGame->m_stDialogBoxExchangeInfo[j].cStr1, 0, sizeof(pGame->m_stDialogBoxExchangeInfo[j].cStr1));
			std::memset(pGame->m_stDialogBoxExchangeInfo[j].cStr2, 0, sizeof(pGame->m_stDialogBoxExchangeInfo[j].cStr2));
			pGame->m_stDialogBoxExchangeInfo[j].sV1 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV2 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV3 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV4 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV5 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV6 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sV7 = -1;
			pGame->m_stDialogBoxExchangeInfo[j].sItemID = -1;
			pGame->m_stDialogBoxExchangeInfo[j].dwV1 = 0;
		}
		int i;
		if (sDir >= 1000)  // Set the item I want to exchange
		{
			i = 0;
			while (pGame->m_stDialogBoxExchangeInfo[i].sV1 != -1)
			{
				i++;
				if (i >= 4) return; // Error situation
			}
			if ((sDir > 1000) && (i == 0))
			{
				pGame->m_bIsItemDisabled[sDir - 1000] = true;
			}
		}
		else // Set the item he proposes me.
		{
			i = 4;
			while (pGame->m_stDialogBoxExchangeInfo[i].sV1 != -1)
			{
				i++;
				if (i >= 8) return; // Error situation
			}
		}
		pGame->m_stDialogBoxExchangeInfo[i].sV1 = sSprite;
		pGame->m_stDialogBoxExchangeInfo[i].sV2 = sSpriteFrame;
		pGame->m_stDialogBoxExchangeInfo[i].sV3 = iAmount;
		pGame->m_stDialogBoxExchangeInfo[i].sV4 = cColor;
		pGame->m_stDialogBoxExchangeInfo[i].sV5 = (int)sCurLife;
		pGame->m_stDialogBoxExchangeInfo[i].sV6 = (int)sMaxLife;
		pGame->m_stDialogBoxExchangeInfo[i].sV7 = (int)sPerformance;
		memcpy(pGame->m_stDialogBoxExchangeInfo[i].cStr1, cItemName, DEF_ITEMNAME - 1);
		memcpy(pGame->m_stDialogBoxExchangeInfo[i].cStr2, cCharName, 10);
		pGame->m_stDialogBoxExchangeInfo[i].dwV1 = dwAttribute;
		pGame->m_stDialogBoxExchangeInfo[i].sItemID = sItemID;
	}

	void HandleCurLifeSpan(CGame* pGame, char* pData)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCurLifeSpan>(
			pData, sizeof(hb::net::PacketNotifyCurLifeSpan));

		if (!pkt)
			return;

		int iItemIndex = pkt->item_index;

		if (pGame->m_pItemList[iItemIndex] != nullptr)
			return;

		pGame->m_pItemList[iItemIndex]->m_wCurLifeSpan = static_cast<WORD>(pkt->cur_lifespan);
	}

	void HandleNotEnoughGold(CGame* pGame, char* pData)
	{
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
		pGame->AddEventList(NOTIFY_MSG_HANDLER67, 10);
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyNotEnoughGold>(
			pData, sizeof(hb::net::PacketNotifyNotEnoughGold));
		if (!pkt) return;
		if (pkt->item_index >= 0) {
			pGame->m_bIsItemDisabled[pkt->item_index] = false;
		}
	}

	void HandleCannotCarryMoreItem(CGame* pGame, char* pData)
	{
		pGame->AddEventList(NOTIFY_MSG_HANDLER65, 10);
		pGame->AddEventList(NOTIFY_MSG_HANDLER66, 10);
		// Bank dialog Box
		pGame->m_dialogBoxManager.Info(DialogBoxId::Bank).cMode = 0;
	}

	void HandleItemAttributeChange(CGame* pGame, char* pData)
	{
		short sV1;
		DWORD dwTemp;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemAttributeChange>(
			pData, sizeof(hb::net::PacketNotifyItemAttributeChange));
		if (!pkt) return;
		sV1 = static_cast<short>(pkt->item_index);
		dwTemp = pGame->m_pItemList[sV1]->m_dwAttribute;
		pGame->m_pItemList[sV1]->m_dwAttribute = pkt->attribute;
		if (pkt->spec_value1 != 0)
			pGame->m_pItemList[sV1]->m_sItemSpecEffectValue1 = static_cast<short>(pkt->spec_value1);
		if (pkt->spec_value2 != 0)
			pGame->m_pItemList[sV1]->m_sItemSpecEffectValue2 = static_cast<short>(pkt->spec_value2);
		
		if (dwTemp == pGame->m_pItemList[sV1]->m_dwAttribute)
		{
			if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == true)
			{
				pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 4;// Failed
			}
			pGame->PlayGameSound('E', 24, 5);
		}
		else
		{
			if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == true)
			{
				pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 3; // Success
			}
			pGame->PlayGameSound('E', 23, 5);
			switch (pGame->m_pPlayer->m_sPlayerType) {
			case 1:
			case 2:
			case 3:
				pGame->PlayGameSound('C', 21, 0);
				break;
			case 4:
			case 5:
			case 6:
				pGame->PlayGameSound('C', 22, 0);
				break;
			}
		}
	}

	void HandleItemUpgradeFail(CGame* pGame, char* pData)
	{
		short sV1;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemUpgradeFail>(
			pData, sizeof(hb::net::PacketNotifyItemUpgradeFail));
		if (!pkt) return;
		sV1 = static_cast<short>(pkt->reason);
		if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == false) return;
		pGame->PlayGameSound('E', 24, 5);
		switch (sV1) {
		case 1:
			pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 8; // Failed
			break;
		case 2:
			pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 9; // Failed
			break;
		case 3:
			pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 10; // Failed
			break;
		}
	}

	void HandleGizonItemUpgradeLeft(CGame* pGame, char* pData)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyGizonItemUpgradeLeft>(
			pData, sizeof(hb::net::PacketNotifyGizonItemUpgradeLeft));
		if (!pkt) return;
		pGame->m_iGizonItemUpgradeLeft = pkt->left;
		switch (pkt->reason) {
		case 1:
			pGame->AddEventList(NOTIFY_MSG_HANDLER_GIZONITEMUPGRADELEFT1, 10);
			break;
		}
	}

	void HandleGizonItemChange(CGame* pGame, char* pData)
	{
		short sV1;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyGizonItemChange>(
			pData, sizeof(hb::net::PacketNotifyGizonItemChange));
		if (!pkt) return;
		sV1 = static_cast<short>(pkt->item_index);
		if (pkt->item_id > 0) pGame->m_pItemList[sV1]->m_sIDnum = pkt->item_id;
		pGame->m_pItemList[sV1]->m_wCurLifeSpan = pkt->cur_lifespan;
		pGame->m_pItemList[sV1]->m_cItemColor = pkt->item_color;
		pGame->m_pItemList[sV1]->m_sItemSpecEffectValue2 = pkt->spec_value2;
		pGame->m_pItemList[sV1]->m_dwAttribute = pkt->attribute;
		
		if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == true)
		{
			pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 3; // success
		}
		pGame->PlayGameSound('E', 23, 5);
		switch (pGame->m_pPlayer->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			pGame->PlayGameSound('C', 21, 0);
			break;
		case 4:
		case 5:
		case 6:
			pGame->PlayGameSound('C', 22, 0);
			break;
		}
	}

	void HandleItemPosList(CGame* pGame, char* pData)
	{
		int i;
		short sX, sY;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemPosList>(
			pData, sizeof(hb::net::PacketNotifyItemPosList));
		if (!pkt) return;
		for (i = 0; i < DEF_MAXITEMS; i++) {
			sX = pkt->positions[i * 2];
			sY = pkt->positions[i * 2 + 1];
			if (pGame->m_pItemList[i] != 0) {
				if (sY < -10) sY = -10;
				if (sX < 0)   sX = 0;
				if (sX > 170) sX = 170;
				if (sY > 95)  sY = 95;

				pGame->m_pItemList[i]->m_sX = sX;
				pGame->m_pItemList[i]->m_sY = sY;
			}
		}
	}

	void HandleItemSold(CGame* pGame, char* pData)
	{
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
	}
}

