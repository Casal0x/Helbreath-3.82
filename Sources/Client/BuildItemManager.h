#pragma once

#include <array>
#include <memory>
#include <string>
#include "NetConstants.h"

class CBuildItem;
class CGame;

class BuildItemManager
{
public:
	static BuildItemManager& Get();
	void SetGame(CGame* pGame);

	bool LoadRecipes();
	bool UpdateAvailableRecipes();
	bool ValidateCurrentRecipe();

	auto& GetDisplayList() { return m_display_recipes; }

private:
	BuildItemManager();
	~BuildItemManager();

	bool ParseRecipeFile(const std::string& buffer);

	CGame* m_game = nullptr;
	std::array<std::unique_ptr<CBuildItem>, hb::shared::limits::MaxBuildItems> m_recipes;
	std::array<std::unique_ptr<CBuildItem>, hb::shared::limits::MaxBuildItems> m_display_recipes;
};
