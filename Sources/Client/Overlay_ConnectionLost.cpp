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
using namespace hb::client::sprite_id;

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
    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    DrawNewDialogBox(InterfaceNdGame4, dlgX, dlgY, 2);
    hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 64, dlgY + 55, "Connection Lost!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
    PutString(dlgX + 60, dlgY + 85, UPDATE_SCREEN_ON_CONNECTION_LOST, GameColors::UIBlack);
    DrawVersion();
}
