#pragma once

#include <cstdint>
#include <memory>
#include <array>

class CItem;

class ItemNameFormatter
{
public:
	static ItemNameFormatter& Get();

	// Inject item config list (address must remain stable)
	void SetItemConfigs(const std::array<std::unique_ptr<CItem>, 5000>& configs);

	// Format item name into 3 output strings (pStr1=name, pStr2/pStr3=attributes)
	// Sets internal IsSpecial() flag based on item attributes
	void Format(CItem* pItem, char* pStr1, char* pStr2, char* pStr3);
	void Format(short sItemId, uint32_t dwAttribute, char* pStr1, char* pStr2, char* pStr3);

	// True if last Format() call found the item to be "special" (magical/enhanced)
	bool IsSpecial() const { return m_is_special; }

private:
	ItemNameFormatter() = default;

	CItem* GetConfig(int iItemID) const;

	const std::array<std::unique_ptr<CItem>, 5000>* m_item_configs = nullptr;
	bool m_is_special = false;
};
