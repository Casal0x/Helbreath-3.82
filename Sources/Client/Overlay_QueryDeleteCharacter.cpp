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


using namespace hb::shared::net;
using namespace hb::client::sprite_id;
namespace MouseButton = hb::shared::input::MouseButton;

Overlay_QueryDeleteCharacter::Overlay_QueryDeleteCharacter(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_QueryDeleteCharacter::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;

    // Play warning sound
    PlayGameSound('E', 25, 0);
}

void Overlay_QueryDeleteCharacter::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_QueryDeleteCharacter::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    // ESC cancels - base screen (SelectCharacter) will be revealed
    if (hb::shared::input::IsKeyPressed(KeyCode::Escape))
    {
        clear_overlay();
        return;
    }

    // Mouse click detection
    if (hb::shared::input::IsMouseButtonPressed(MouseButton::Left))
    {
        PlayGameSound('E', 14, 5);

        // Yes button - confirm deletion
        if (hb::shared::input::IsMouseInRect(dlgX + 38, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
        {
            // Create login socket and initiate delete request
            m_pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(m_pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
            m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr.c_str(), m_pGame->m_iLogServerPort + (rand() % 1));
            m_pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

            m_pGame->m_dwConnectMode = MsgId::RequestDeleteCharacter;
            std::snprintf(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg), "%s", "33");

            // set_overlay will clear this overlay automatically
            m_pGame->ChangeGameMode(GameMode::Connecting);
            return;
        }

        // No button - cancel, base screen (SelectCharacter) will be revealed
        if (hb::shared::input::IsMouseInRect(dlgX + 208, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
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
    int msX = hb::shared::input::GetMouseX();
    int msY = hb::shared::input::GetMouseY();
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;

    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    // Double shadow effect after initial animation period (600ms)
    if (dwElapsed >= 600)
    {
        m_pGame->m_Renderer->DrawRectFilled(0, 0, LOGICAL_MAX_X(), LOGICAL_MAX_Y(), hb::shared::render::Color::Black(128));
    }

    // Draw dialog box
    DrawNewDialogBox(InterfaceNdGame4, dlgX, dlgY, 2);

    // Title
    hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 96, dlgY + 35, "Delete Character", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));

    // Character name display
    PutString(dlgX + 53, dlgY + 70, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER1, GameColors::UIBlack);
    PutString(dlgX + 173, dlgY + 74, "__________", GameColors::UIBlack);

    // Get character name from the selected character slot
    if (m_pGame->m_wEnterGameType > 0 && m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1] != nullptr)
    {
        PutString(dlgX + 173, dlgY + 70, m_pGame->m_pCharList[m_pGame->m_wEnterGameType - 1]->m_cName.c_str(), GameColors::UILabel);
    }

    // Confirmation text
    PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_QUERY_DELETE_CHARACTER2);

    // Yes button with hover effect
    bool bYesHover = (msX >= dlgX + 38) && (msX <= dlgX + 38 + ui_layout::btn_size_x) &&
                     (msY >= dlgY + 119) && (msY <= dlgY + 119 + ui_layout::btn_size_y);
    DrawNewDialogBox(InterfaceNdButton, dlgX + 38, dlgY + 119, bYesHover ? 19 : 18);

    // No button with hover effect
    bool bNoHover = (msX >= dlgX + 208) && (msX <= dlgX + 208 + ui_layout::btn_size_x) &&
                    (msY >= dlgY + 119) && (msY <= dlgY + 119 + ui_layout::btn_size_y);
    DrawNewDialogBox(InterfaceNdButton, dlgX + 208, dlgY + 119, bNoHover ? 3 : 2);

    DrawVersion();
}
