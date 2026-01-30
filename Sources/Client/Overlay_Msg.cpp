// Overlay_Msg.cpp: Simple message display overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_Msg.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "GameFonts.h"
#include "TextLibExt.h"

Overlay_Msg::Overlay_Msg(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_Msg::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
}

void Overlay_Msg::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_Msg::on_update()
{
    // Auto-transition to MainMenu after 1.5 seconds
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;
    if (dwElapsed > 1500)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
    }
}

void Overlay_Msg::on_render()
{
    TextLib::DrawText(GameFont::Default, 10, 10, m_pGame->m_cMsg,
                      TextLib::TextStyle::WithShadow(GameColors::ErrorPink.r, GameColors::ErrorPink.g, GameColors::ErrorPink.b));
    DrawVersion();
}
