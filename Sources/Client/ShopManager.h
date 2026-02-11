#pragma once

#include <array>
#include <memory>
#include "GameConstants.h"

class CItem;
class CGame;

class ShopManager
{
public:
	static ShopManager& Get();
	void SetGame(CGame* pGame);

	void RequestShopMenu(char cType);
	void HandleResponse(char* pData);

	void ClearItems();
	bool HasItems() const;

	auto& GetItemList() { return m_item_list; }
	int16_t GetPendingShopType() const { return m_pending_shop_type; }
	void SetPendingShopType(int16_t type) { m_pending_shop_type = type; }

private:
	ShopManager();
	~ShopManager();

	void SendRequest(int16_t npcType);

	CGame* m_game = nullptr;
	std::array<std::unique_ptr<CItem>, game_limits::max_menu_items> m_item_list;
	int16_t m_pending_shop_type = 0;
};
