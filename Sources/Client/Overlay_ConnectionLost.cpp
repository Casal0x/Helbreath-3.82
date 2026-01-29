// Overlay_ConnectionLost.cpp: Connection lost notification overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_ConnectionLost.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "AudioManager.h"
#include "TextLibExt.h"
#include "GameFonts.h"

Overlay_ConnectionLost::Overlay_ConnectionLost(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_ConnectionLost::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_iFrameCount = 0;

    // Stop sounds
    AudioManager::Get().StopSound(SoundType::Effect, 38);
    AudioManager::Get().StopMusic();
}

void Overlay_ConnectionLost::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_ConnectionLost::on_update()
{
    m_iFrameCount++;
    if (m_iFrameCount > 100) m_iFrameCount = 100;

    // Auto-transition to MainMenu after 5 seconds
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;
    if (dwElapsed > 5000)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
    }
}

void Overlay_ConnectionLost::on_render()
{
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);
    TextLib::DrawText(GameFont::Bitmap1, 172 + 54 + SCREENX, 180 + SCREENY, "Connection Lost!", TextLib::TextStyle::WithHighlight(58, 0, 0));
    PutString(172 + 50 + SCREENX, 180 + 30 + SCREENY, UPDATE_SCREEN_ON_CONNECTION_LOST, RGB(0, 0, 0));
    DrawVersion();
}
