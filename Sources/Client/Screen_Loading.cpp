// Screen_Loading.cpp: Loading Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Loading.h"
#include "Game.h"
#include "GameModeManager.h"

Screen_Loading::Screen_Loading(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Loading::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::Loading);
}

void Screen_Loading::on_uninitialize()
{
    // Nothing to clean up
}

void Screen_Loading::on_update()
{
    // Delegate to CGame's resource loading logic
    // Pass false to indicate we'll handle drawing separately
    m_pGame->UpdateScreen_OnLoading(false);
}

void Screen_Loading::on_render()
{
    // Delegate to CGame's loading progress display
    m_pGame->DrawScreen_OnLoadingProgress();
}
