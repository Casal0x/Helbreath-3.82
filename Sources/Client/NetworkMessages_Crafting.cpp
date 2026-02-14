#include "Game.h"
#include "CraftingManager.h"

namespace NetworkMessageHandlers {

void handle_crafting_success(CGame* game, char* data)
{
	game->m_crafting_manager.handle_crafting_success(data);
}

void handle_crafting_fail(CGame* game, char* data)
{
	game->m_crafting_manager.handle_crafting_fail(data);
}

void handle_build_item_success(CGame* game, char* data)
{
	game->m_crafting_manager.handle_build_item_success(data);
}

void handle_build_item_fail(CGame* game, char* data)
{
	game->m_crafting_manager.handle_build_item_fail(data);
}

void handle_portion_success(CGame* game, char* data)
{
	game->m_crafting_manager.handle_portion_success(data);
}

void handle_portion_fail(CGame* game, char* data)
{
	game->m_crafting_manager.handle_portion_fail(data);
}

void handle_low_portion_skill(CGame* game, char* data)
{
	game->m_crafting_manager.handle_low_portion_skill(data);
}

void handle_no_matching_portion(CGame* game, char* data)
{
	game->m_crafting_manager.handle_no_matching_portion(data);
}

} // namespace NetworkMessageHandlers
