#include "ShopManager.h"
#include "Game.h"
#include "Packet/PacketShop.h"
#include "Packet/PacketHelpers.h"

using namespace hb::shared::net;

ShopManager::ShopManager() = default;
ShopManager::~ShopManager() = default;

ShopManager& ShopManager::Get()
{
	static ShopManager instance;
	return instance;
}

void ShopManager::SetGame(CGame* pGame)
{
	m_game = pGame;
}

void ShopManager::ClearItems()
{
	for (auto& item : m_item_list) item.reset();
}

bool ShopManager::HasItems() const
{
	return m_item_list[0] != nullptr;
}

void ShopManager::RequestShopMenu(char cType)
{
	// Request shop contents from server using NPC type
	SendRequest(static_cast<int16_t>(cType));
}

void ShopManager::SendRequest(int16_t npcType)
{
	// Clear existing shop items
	for (int i = 0; i < game_limits::max_sell_list; i++) {
		if (m_item_list[i] != nullptr) {
			m_item_list[i].reset();
			m_item_list[i].reset();
		}
	}

	// Build and send shop request packet
	char cData[sizeof(hb::net::PacketShopRequest)]{};

	auto* req = reinterpret_cast<hb::net::PacketShopRequest*>(cData);
	req->header.msg_id = MSGID_REQUEST_SHOP_CONTENTS;
	req->header.msg_type = MsgType::Confirm;
	req->npcType = npcType;

	m_game->m_pGSock->iSendMsg(cData, sizeof(hb::net::PacketShopRequest));
}

void ShopManager::HandleResponse(char* pData)
{
	const auto* resp = hb::net::PacketCast<hb::net::PacketShopResponseHeader>(
		pData, sizeof(hb::net::PacketShopResponseHeader));
	if (!resp) {
		return;
	}

	uint16_t itemCount = resp->itemCount;

	if (itemCount > game_limits::max_menu_items) {
		itemCount = game_limits::max_menu_items;
	}

	// Clear existing shop items
	for (int i = 0; i < game_limits::max_menu_items; i++) {
		if (m_item_list[i] != nullptr) {
			m_item_list[i].reset();
			m_item_list[i].reset();
		}
	}

	// Get item IDs from packet (they follow the header)
	const int16_t* itemIds = reinterpret_cast<const int16_t*>(pData + sizeof(hb::net::PacketShopResponseHeader));

	// Populate shop list from item configs
	int shopIndex = 0;
	int skippedCount = 0;
	int notFoundCount = 0;
	for (uint16_t i = 0; i < itemCount && shopIndex < game_limits::max_menu_items; i++) {
		int16_t itemId = itemIds[i];
		if (itemId <= 0 || itemId >= 5000) {
			skippedCount++;
			continue;
		}
		if (m_game->m_pItemConfigList[itemId] == nullptr) {
			notFoundCount++;
			skippedCount++;
			continue;
		}

		// Create new item for shop based on config
		m_item_list[shopIndex] = std::make_unique<CItem>();
		CItem* pItem = m_item_list[shopIndex].get();
		CItem* pConfig = m_game->m_pItemConfigList[itemId].get();

		// Copy item data from config
		pItem->m_sIDnum = itemId;
		std::memcpy(pItem->m_cName, pConfig->m_cName, sizeof(pItem->m_cName));
		pItem->m_cItemType = pConfig->m_cItemType;
		pItem->m_cEquipPos = pConfig->m_cEquipPos;
		pItem->m_sSprite = pConfig->m_sSprite;
		pItem->m_sSpriteFrame = pConfig->m_sSpriteFrame;
		pItem->m_wPrice = pConfig->m_wPrice;
		pItem->m_wWeight = pConfig->m_wWeight;
		pItem->m_sItemEffectValue1 = pConfig->m_sItemEffectValue1;
		pItem->m_sItemEffectValue2 = pConfig->m_sItemEffectValue2;
		pItem->m_sItemEffectValue3 = pConfig->m_sItemEffectValue3;
		pItem->m_sItemEffectValue4 = pConfig->m_sItemEffectValue4;
		pItem->m_sItemEffectValue5 = pConfig->m_sItemEffectValue5;
		pItem->m_sItemEffectValue6 = pConfig->m_sItemEffectValue6;
		pItem->m_wMaxLifeSpan = pConfig->m_wMaxLifeSpan;
		pItem->m_sLevelLimit = pConfig->m_sLevelLimit;
		pItem->m_cGenderLimit = pConfig->m_cGenderLimit;
		pItem->m_sSpecialEffect = pConfig->m_sSpecialEffect;
		pItem->m_cSpeed = pConfig->m_cSpeed;

		shopIndex++;
	}

	printf("[SHOP] Populated: %d items added, %d skipped (%d not found in config list)\n", shopIndex, skippedCount, notFoundCount);

	// Only show shop dialog if we have items and there was a pending request
	if (shopIndex > 0 && m_pending_shop_type != 0) {
		// Enable the SaleMenu dialog - this will call EnableDialogBox which sets up the dialog
		m_game->m_dialogBoxManager.EnableDialogBox(DialogBoxId::SaleMenu, m_pending_shop_type, 0, 0, nullptr);
		m_pending_shop_type = 0;  // Clear pending request
	} else if (m_pending_shop_type != 0) {
		// No items available - show message to user
		m_game->AddEventList("This shop has no items available.", 10);
		m_pending_shop_type = 0;
	}
}

