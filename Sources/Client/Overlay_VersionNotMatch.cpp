// Overlay_VersionNotMatch.cpp: Version mismatch error overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_VersionNotMatch.h"
#include "Game.h"
#include "GameModeManager.h"
#include "RendererFactory.h"
#include "lan_eng.h"
#include "IInput.h"
using namespace hb::client::sprite_id;


namespace MouseButton = hb::shared::input::MouseButton;

Overlay_VersionNotMatch::Overlay_VersionNotMatch(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_VersionNotMatch::on_initialize()
{
    m_iFrameCount = 0;

    // Close game socket
    if (m_pGame->m_pGSock != nullptr)
    {
        m_pGame->m_pGSock.reset();
    }
}

void Overlay_VersionNotMatch::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_VersionNotMatch::on_update()
{
    m_iFrameCount++;
    if (m_iFrameCount > 120) m_iFrameCount = 120;

    // Any key press closes the application
    if (hb::shared::input::is_key_pressed(KeyCode::Escape) || hb::shared::input::is_key_pressed(KeyCode::Enter))
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        hb::shared::render::Window::close();
        return;
    }

    // Mouse click also closes
    if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        hb::shared::render::Window::close();
        return;
    }
}

void Overlay_VersionNotMatch::on_render()
{
    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    DrawNewDialogBox(InterfaceNdQuit, 0, 0, 0, true);
    DrawNewDialogBox(InterfaceNdGame4, dlgX, dlgY, 2);
    PutAlignedString(dlgX + 6, dlgX + 312, dlgY + 35, UPDATE_SCREEN_ON_VERSION_NO_MATCH1);
    PutAlignedString(dlgX + 6, dlgX + 312, dlgY + 55, UPDATE_SCREEN_ON_VERSION_NO_MATCH2);
    DrawVersion();
}
