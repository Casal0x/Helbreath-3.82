// Overlay_VersionNotMatch.cpp: Version mismatch error overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_VersionNotMatch.h"
#include "Game.h"
#include "GameModeManager.h"
#include "RendererFactory.h"
#include "lan_eng.h"
#include "IInput.h"
#include "ASIOSocket.h"

extern class ASIOSocket* G_pCalcSocket;

Overlay_VersionNotMatch::Overlay_VersionNotMatch(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_VersionNotMatch::on_initialize()
{
    m_iFrameCount = 0;

    // Close sockets
    if (G_pCalcSocket != nullptr)
    {
        delete G_pCalcSocket;
        G_pCalcSocket = nullptr;
    }
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
    if (Input::IsKeyPressed(VK_ESCAPE) || Input::IsKeyPressed(VK_RETURN))
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        Window::Close();
        return;
    }

    // Mouse click also closes
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        Window::Close();
        return;
    }
}

void Overlay_VersionNotMatch::on_render()
{
    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_QUIT, 0, 0, 0, true);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX, dlgY, 2);
    PutAlignedString(dlgX + 6, dlgX + 312, dlgY + 35, UPDATE_SCREEN_ON_VERSION_NO_MATCH1);
    PutAlignedString(dlgX + 6, dlgX + 312, dlgY + 55, UPDATE_SCREEN_ON_VERSION_NO_MATCH2);
    DrawVersion();
}
