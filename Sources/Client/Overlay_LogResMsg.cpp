// Overlay_LogResMsg.cpp: Login/Account result message overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_LogResMsg.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "AudioManager.h"
#include "TextLibExt.h"
#include "GameFonts.h"
using namespace hb::client::sprite_id;


namespace MouseButton = hb::shared::input::MouseButton;

Overlay_LogResMsg::Overlay_LogResMsg(CGame* pGame)
    : IGameScreen(pGame)
    , m_cReturnDest('1')
    , m_cMsgCode('1')
{
}

void Overlay_LogResMsg::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;

    // Read parameters from CGame (set before ChangeGameMode call)
    // m_cMsg[0] = return destination, m_cMsg[1] = message code
    m_cReturnDest = m_pGame->m_cMsg[0];
    m_cMsgCode = m_pGame->m_cMsg[1];

    // Stop any playing sound
    AudioManager::Get().StopSound(SoundType::Effect, 38);
}

void Overlay_LogResMsg::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_LogResMsg::HandleDismiss()
{
    // Note: clear_overlay() is not needed here as ChangeGameMode will either:
    // - Call set_screen<T>() which clears overlays automatically
    // - Call set_overlay<T>() which clears existing overlays automatically

    // Transition based on return destination
    switch (m_cReturnDest)
    {
    case '0':
        m_pGame->ChangeGameMode(GameMode::CreateNewAccount);
        break;
    case '1':
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        break;
    case '2':
        m_pGame->ChangeGameMode(GameMode::CreateNewCharacter);
        break;
    case '3':
    case '4':
        m_pGame->ChangeGameMode(GameMode::SelectCharacter);
        break;
    case '5':
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        break;
    case '6':
        // Context-dependent based on message code
        switch (m_cMsgCode)
        {
        case 'B':
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            break;
        case 'C':
        case 'M':
            m_pGame->ChangeGameMode(GameMode::ChangePassword);
            break;
        default:
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            break;
        }
        break;
    case '7':
    case '8':
    default:
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        break;
    }
}

void Overlay_LogResMsg::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();

    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    // ESC or Enter dismisses the message
    if (hb::shared::input::IsKeyPressed(KeyCode::Escape) || hb::shared::input::IsKeyPressed(KeyCode::Enter))
    {
        HandleDismiss();
        return;
    }

    // Check for OK button click
    if (hb::shared::input::IsMouseButtonPressed(MouseButton::Left))
    {
        if (hb::shared::input::IsMouseInRect(dlgX + 208, dlgY + 119, ui_layout::btn_size_x, ui_layout::btn_size_y))
        {
            HandleDismiss();
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

void Overlay_LogResMsg::RenderMessage(int dlgX, int dlgY)
{
    char cTxt[128];

    switch (m_cMsgCode)
    {
    case '1':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Password is not correct!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG5);
        break;

    case '2':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Not existing account!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG6);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 90, UPDATE_SCREEN_ON_LOG_MSG7);
        break;

    case '3':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG8);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG9);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG10);
        break;

    case '4':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New account created.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG11);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG12);
        break;

    case '5':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new account!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG13);
        break;

    case '6':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new account!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing account name.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG14);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG15);
        break;

    case '7':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New character created.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG16);
        break;

    case '8':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new character!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG17);
        break;

    case '9':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new character!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing character name.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG18);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG19);
        break;

    case 'A':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Character deleted.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG20);
        break;

    case 'B':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Password changed.", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG21);
        break;

    case 'C':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not change password!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG22);
        break;

    case 'D':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG23);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG24);
        break;

    case 'E':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG25);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG26);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG27);
        break;

    case 'F':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG28);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG29);
        break;

    case 'G':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG30);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG31);
        break;

    case 'H':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Connection Rejected!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        if (m_pGame->m_iBlockYear == 0)
        {
            PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG32);
            PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG33);
        }
        else
        {
            PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG34);
            std::snprintf(cTxt, sizeof(cTxt), UPDATE_SCREEN_ON_LOG_MSG35,
                     m_pGame->m_iBlockYear, m_pGame->m_iBlockMonth, m_pGame->m_iBlockDay);
            PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, cTxt);
        }
        break;

    case 'I':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Not Enough Point!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Not enough points to play.");
        break;

    case 'J':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "World Server Full", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Please ! Try Other World Server");
        break;

    case 'M':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Your password expired", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Please! Change password");
        break;

    case 'U':
        hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Keycode input Success!", hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Keycode Registration successed.");
        break;

    case 'X':
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG38);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG39);
        break;

    case 'Y':
        PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG40);
        PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG41);
        break;

    case 'Z':
        PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG42);
        PutAlignedString(dlgX + 16, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG41);
        break;
    }
}

void Overlay_LogResMsg::on_render()
{
    int msX = hb::shared::input::GetMouseX();
    int msY = hb::shared::input::GetMouseY();

    int dlgX, dlgY;
    GetCenteredDialogPos(InterfaceNdGame4, 2, dlgX, dlgY);

    // Draw dialog box
    DrawNewDialogBox(InterfaceNdGame4, dlgX, dlgY, 2);

    // Draw OK button with hover effect
    bool bHover = (msX >= dlgX + 208) && (msX <= dlgX + 208 + ui_layout::btn_size_x) &&
                  (msY >= dlgY + 119) && (msY <= dlgY + 119 + ui_layout::btn_size_y);
    DrawNewDialogBox(InterfaceNdButton, dlgX + 208, dlgY + 119, bHover ? 1 : 0);

    // Render the appropriate message
    RenderMessage(dlgX, dlgY);
}
