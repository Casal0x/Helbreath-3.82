#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <string>

class CItem;

struct ItemNameInfo
{
	std::string name;
	std::string effect;
	std::string extra;
	bool is_special = false;
};

class ItemNameFormatter
{
public:
	static ItemNameFormatter& Get();

	// Inject item config list (address must remain stable)
	void SetItemConfigs(const std::array<std::unique_ptr<CItem>, 5000>& configs);

	// Format item name, returning name + attribute strings + special flag
	ItemNameInfo Format(CItem* pItem);
	ItemNameInfo Format(short sItemId, uint32_t dwAttribute);

private:
	ItemNameFormatter() = default;

	CItem* GetConfig(int iItemID) const;

	const std::array<std::unique_ptr<CItem>, 5000>* m_item_configs = nullptr;
};
