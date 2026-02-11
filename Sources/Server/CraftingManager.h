// CraftingManager.h: Manages potion brewing and crafting recipe processing.
// Extracted from CGame (Phase B2).

#pragma once

#include "CommonTypes.h"
#include "Portion.h"
#include "Game.h"  // For hb::server::config constants

class CraftingManager
{
public:
	CraftingManager();
	~CraftingManager();

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Lifecycle
	void InitArrays();
	void CleanupArrays();

	// Crafting handlers (moved from CGame)
	void ReqCreatePortionHandler(int iClientH, char* pData);
	void ReqCreateCraftingHandler(int iClientH, char* pData);

	// Config arrays (public for database loading)
	CPortion* m_pPortionConfigList[hb::server::config::MaxPortionTypes]{};
	CPortion* m_pCraftingConfigList[hb::server::config::MaxPortionTypes]{};

private:
	CGame* m_pGame = nullptr;
};
