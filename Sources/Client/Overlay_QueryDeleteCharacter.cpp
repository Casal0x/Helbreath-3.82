// Overlay_QueryDeleteCharacter.cpp: "Delete Character" confirmation overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_QueryDeleteCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "ASIOSocket.h"
#include "TextLibExt.h"
#include "GameFonts.h"

Overlay_QueryDeleteCharacter::Overlay_QueryDeleteCharacter(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_QueryDeleteCharacter::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;

    // Play warning sound
    PlaySound('E', 25, 0);
}

void Overlay_QueryDeleteCharacter::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_QueryDeleteCharacter::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    // ESC cancels - base screen (SelectCharacter) will be revealed
    if (Input::IsKeyPressed(VK_ESCAPE))
    {
        clear_overlay();
        return;
    }

    // Mouse click detection
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        PlaySound('E', 14, 5);

        // Yes button - confirm deletion
        if (Input::IsMouseInRect(dlgX + 38, dlgY + 119, DEF_BTNSZX, DEF_BTNSZY))
        {
            // Create login socket and initiate delete request
            m_pGame->m_pLSock = std::make_unique<ASIOSocket>(m_pGame->m_pIOPool->GetContext(), DEF_SOCKETBLOCKLIMIT);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(DEF_MSGBUFFERSIZE);

            m_pGame->m_dwConnectMode = MSGID_REQUEST_DELETECHARACTER;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            strcpy(m_pGame->m_cMsg, "33");

            // set_overlay will clear this overlay automatically
            m_pGame->ChangeGameMode(GameMode::Connecting);
            return;
        }

        // No button - cancel, base screen (SelectCharacter) will be revealed
        if (Input::IsMouseInRect(dlgX + 208, dlgY + 119, DEF_BTNSZX, DEF_BTNSZY))
        {
            clear_overlay();
            return;
        }
    }

    // Animation frame updates
    if ((dwTime - m_dwAnimTime) > 100)
    {
        m_pGame->m_cMenuFrame++;
        m_dwAnimTime = dwTime;
    }
    if (m_pGame->m_cMenuFrame >= 8)
    {
        m_pGame->m_cMenuDirCnt++;
        if (m_pGame->m_cMenuDirCnt > 8)
        {
            m_pGame->m_cMenuDir++;
            m_pGame->m_cMenuDirCnt = 1;
        }
        if (m_pGame->m_cMenuDir > 8) m_pGame->m_cMenuDir = 1;
        m_pGame->m_cMenuFrame = 0;
    }
}

void Overlay_QueryDeleteCharacter::on_render()
{
    int msX = Input::GetMouseX();
    int msY = Input::GetMouseY();
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;

    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    // Double shadow effect after initial animation period (600ms)
    if (dwElapsed >= 600)
    {
        m_pGame->m_Renderer->DrawShadowBox(0, 0, LOGICAL_MAX_X(), LOGICAL_MAX_Y());
    }

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX, dlgY, 2);

    // Title
    TextLib::DrawText(GameFont::Bitmap1, dlgX + 96, dlgY + 35, "Delete Character", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed.r, GameColors::UIDarkRed.g, GameColors::UIDarkRed.b));

    // Character name display
    PutString(dlgX + 53, dlgY + 70, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER1, GameColors::UIBlack.ToColorRef());
    PutString(dlgX + 173, dlgY + 74, "__________", GameColors::UIBlack.ToColorRef());

    // Get character name from the selected character slot
    if (m_pGame->m_wEnterGameType > 0 && m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1] != nullptr)
    {
        PutString(dlgX + 173, dlgY + 70,
                  m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1]->m_cName, GameColors::UILabel.ToColorRef());
    }

    // Confirmation text
    PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER2);

    // Yes button with hover effect
    bool bYesHover = (msX >= dlgX + 38) && (msX <= dlgX + 38 + DEF_BTNSZX) &&
                     (msY >= dlgY + 119) && (msY <= dlgY + 119 + DEF_BTNSZY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, dlgX + 38, dlgY + 119, bYesHover ? 19 : 18);

    // No button with hover effect
    bool bNoHover = (msX >= dlgX + 208) && (msX <= dlgX + 208 + DEF_BTNSZX) &&
                    (msY >= dlgY + 119) && (msY <= dlgY + 119 + DEF_BTNSZY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, dlgX + 208, dlgY + 119, bNoHover ? 3 : 2);

    DrawVersion();
}
