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

class item_name_formatter
{
public:
	static item_name_formatter& get();

	// Inject item config list (address must remain stable)
	void set_item_configs(const std::array<std::unique_ptr<CItem>, 5000>& configs);

	// format item name, returning name + attribute strings + special flag
	ItemNameInfo format(CItem* item);
	ItemNameInfo format(short item_id, uint32_t attribute);

private:
	item_name_formatter() = default;

	CItem* get_config(int item_id) const;

	const std::array<std::unique_ptr<CItem>, 5000>* m_item_configs = nullptr;
};
