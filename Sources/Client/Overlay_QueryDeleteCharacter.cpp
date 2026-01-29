// Overlay_QueryDeleteCharacter.cpp: "Delete Character" confirmation overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_QueryDeleteCharacter.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "XSocket.h"
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
        if (Input::IsMouseInRect(200 + SCREENX, 244 + SCREENY,
                                  200 + DEF_BTNSZX + SCREENX, 244 + DEF_BTNSZY + SCREENY))
        {
            // Create login socket and initiate delete request
            m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(30000);

            m_pGame->m_dwConnectMode = MSGID_REQUEST_DELETECHARACTER;
            std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
            strcpy(m_pGame->m_cMsg, "33");

            // set_overlay will clear this overlay automatically
            m_pGame->ChangeGameMode(GameMode::Connecting);
            return;
        }

        // No button - cancel, base screen (SelectCharacter) will be revealed
        if (Input::IsMouseInRect(370 + SCREENX, 244 + SCREENY,
                                  370 + DEF_BTNSZX + SCREENX, 244 + DEF_BTNSZY + SCREENY))
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

    // Double shadow effect after initial animation period (600ms)
    if (dwElapsed >= 600)
    {
        m_pGame->m_Renderer->DrawShadowBox(0, 0, LOGICAL_MAX_X, LOGICAL_MAX_Y);
    }

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);

    // Title
    TextLib::DrawText(GameFont::Bitmap1, 172 + 86 + SCREENX, 160 + SCREENY, "Delete Character", TextLib::TextStyle::WithHighlight(58, 0, 0));

    // Character name display
    PutString(215 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER1, RGB(5, 5, 5));
    PutString(335 + SCREENX, 199 + SCREENY, "__________", RGB(5, 5, 5));

    // Get character name from the selected character slot
    if (m_pGame->m_wEnterGameType > 0 && m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1] != nullptr)
    {
        PutString(335 + SCREENX, 195 + SCREENY,
                  m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1]->m_cName, RGB(25, 35, 25));
    }

    // Confirmation text
    PutAlignedString(178 + SCREENX, 453 + SCREENX, 220 + SCREENY, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER2);

    // Yes button with hover effect
    bool bYesHover = (msX >= 200 + SCREENX) && (msX <= 200 + DEF_BTNSZX + SCREENX) &&
                     (msY >= 244 + SCREENY) && (msY <= 244 + DEF_BTNSZY + SCREENY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 200 + SCREENX, 244 + SCREENY, bYesHover ? 19 : 18);

    // No button with hover effect
    bool bNoHover = (msX >= 370 + SCREENX) && (msX <= 370 + DEF_BTNSZX + SCREENX) &&
                    (msY >= 244 + SCREENY) && (msY <= 244 + DEF_BTNSZY + SCREENY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 370 + SCREENX, 244 + SCREENY, bNoHover ? 3 : 2);

    DrawVersion();
}
