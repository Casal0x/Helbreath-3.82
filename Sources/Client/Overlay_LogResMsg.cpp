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
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    // ESC or Enter dismisses the message
    if (Input::IsKeyPressed(VK_ESCAPE) || Input::IsKeyPressed(VK_RETURN))
    {
        HandleDismiss();
        return;
    }

    // Check for OK button click
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (Input::IsMouseInRect(dlgX + 208, dlgY + 119, DEF_BTNSZX, DEF_BTNSZY))
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
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Password is not correct!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG5);
        break;

    case '2':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 80, dlgY + 40, "Not existing account!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG6);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 90, UPDATE_SCREEN_ON_LOG_MSG7);
        break;

    case '3':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG8);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG9);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG10);
        break;

    case '4':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New account created.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG11);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG12);
        break;

    case '5':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new account!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG13);
        break;

    case '6':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new account!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing account name.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG14);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG15);
        break;

    case '7':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "New character created.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG16);
        break;

    case '8':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 68, dlgY + 40, "Can not create new character!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG17);
        break;

    case '9':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not create new character!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 34, dlgY + 55, "Already existing character name.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 80, UPDATE_SCREEN_ON_LOG_MSG18);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 95, UPDATE_SCREEN_ON_LOG_MSG19);
        break;

    case 'A':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Character deleted.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG20);
        break;

    case 'B':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 91, dlgY + 40, "Password changed.", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG21);
        break;

    case 'C':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 46, dlgY + 40, "Can not change password!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG22);
        break;

    case 'D':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG23);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG24);
        break;

    case 'E':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG25);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG26);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 100, UPDATE_SCREEN_ON_LOG_MSG27);
        break;

    case 'F':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG28);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG29);
        break;

    case 'G':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 54, dlgY + 40, "Can not connect to game server!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 70, UPDATE_SCREEN_ON_LOG_MSG30);
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, UPDATE_SCREEN_ON_LOG_MSG31);
        break;

    case 'H':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Connection Rejected!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
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
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Not Enough Point!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Not enough points to play.");
        break;

    case 'J':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "World Server Full", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Please ! Try Other World Server");
        break;

    case 'M':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Your password expired", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
        PutAlignedString(dlgX + 36, dlgX + 291, dlgY + 85, "Please! Change password");
        break;

    case 'U':
        TextLib::DrawText(GameFont::Bitmap1, dlgX + 78, dlgY + 40, "Keycode input Success!", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed));
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
    int msX = Input::GetMouseX();
    int msY = Input::GetMouseY();

    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX, dlgY, 2);

    // Draw OK button with hover effect
    bool bHover = (msX >= dlgX + 208) && (msX <= dlgX + 208 + DEF_BTNSZX) &&
                  (msY >= dlgY + 119) && (msY <= dlgY + 119 + DEF_BTNSZY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, dlgX + 208, dlgY + 119, bHover ? 1 : 0);

    // Render the appropriate message
    RenderMessage(dlgX, dlgY);
}
