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

    // ESC or Enter dismisses the message
    if (Input::IsKeyPressed(VK_ESCAPE) || Input::IsKeyPressed(VK_RETURN))
    {
        HandleDismiss();
        return;
    }

    // Check for OK button click
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (Input::IsMouseInRect(370 + SCREENX, 244 + SCREENY,
                                  370 + SCREENX + DEF_BTNSZX, 244 + SCREENY + DEF_BTNSZY))
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

void Overlay_LogResMsg::RenderMessage()
{
    char cTxt[128];

    switch (m_cMsgCode)
    {
    case '1':
        PutString_SprFont(172 + 70 + SCREENX, 165 + SCREENY, "Password is not correct!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG5);
        break;

    case '2':
        PutString_SprFont(172 + 70 + SCREENX, 165 + SCREENY, "Not existing account!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG6);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 215 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG7);
        break;

    case '3':
        PutString_SprFont(172 + 10 + 34 + SCREENX, 165 + SCREENY, "Can not connect to game server!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG8);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG9);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 225 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG10);
        break;

    case '4':
        PutString_SprFont(172 + 58 + SCREENX, 165 + SCREENY, "New account created.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG11);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG12);
        break;

    case '5':
        PutString_SprFont(172 + 58 + SCREENX, 165 + SCREENY, "Can not create new account!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG13);
        break;

    case '6':
        PutString_SprFont(172 + 36 + SCREENX, 165 + SCREENY, "Can not create new account!", 58, 0, 0);
        PutString_SprFont(172 + 24 + SCREENX, 180 + SCREENY, "Already existing account name.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 205 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG14);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 220 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG15);
        break;

    case '7':
        PutString_SprFont(172 + 58 + SCREENX, 165 + SCREENY, "New character created.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG16);
        break;

    case '8':
        PutString_SprFont(172 + 58 + SCREENX, 165 + SCREENY, "Can not create new character!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG17);
        break;

    case '9':
        PutString_SprFont(172 + 36 + SCREENX, 165 + SCREENY, "Can not create new character!", 58, 0, 0);
        PutString_SprFont(172 + 24 + SCREENX, 180 + SCREENY, "Already existing character name.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 205 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG18);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 220 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG19);
        break;

    case 'A':
        PutString_SprFont(172 + 36 + 45 + SCREENX, 165 + SCREENY, "Character deleted.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG20);
        break;

    case 'B':
        PutString_SprFont(172 + 36 + 45 + SCREENX, 165 + SCREENY, "Password changed.", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG21);
        break;

    case 'C':
        PutString_SprFont(172 + 36 + SCREENX, 165 + SCREENY, "Can not change password!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG22);
        break;

    case 'D':
        PutString_SprFont(172 + 10 + 34 + SCREENX, 165 + SCREENY, "Can not connect to game server!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG23);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG24);
        break;

    case 'E':
        PutString_SprFont(172 + 10 + 34 + SCREENX, 165 + SCREENY, "Can not connect to game server!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG25);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG26);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 225 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG27);
        break;

    case 'F':
        PutString_SprFont(172 + 10 + 34 + SCREENX, 165 + SCREENY, "Can not connect to game server!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG28);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG29);
        break;

    case 'G':
        PutString_SprFont(172 + 10 + 34 + SCREENX, 165 + SCREENY, "Can not connect to game server!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG30);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG31);
        break;

    case 'H':
        PutString_SprFont(172 + 68 + SCREENX, 165 + SCREENY, "Connection Rejected!", 58, 0, 0);
        if (m_pGame->m_iBlockYear == 0)
        {
            PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG32);
            PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG33);
        }
        else
        {
            PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG34);
            wsprintf(cTxt, UPDATE_SCREEN_ON_LOG_MSG35,
                     m_pGame->m_iBlockYear, m_pGame->m_iBlockMonth, m_pGame->m_iBlockDay);
            PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, cTxt);
        }
        break;

    case 'I':
        PutString_SprFont(172 + 68 + SCREENX, 165 + SCREENY, "Not Enough Point!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, "Not enough points to play.");
        break;

    case 'J':
        PutString_SprFont(172 + 68 + SCREENX, 165 + SCREENY, "World Server Full", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, "Please ! Try Other World Server");
        break;

    case 'M':
        PutString_SprFont(172 + 68 + SCREENX, 165 + SCREENY, "Your password expired", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, "Please! Change password");
        break;

    case 'U':
        PutString_SprFont(172 + 68 + SCREENX, 165 + SCREENY, "Keycode input Success!", 58, 0, 0);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, "Keycode Registration successed.");
        break;

    case 'X':
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG38);
        PutAlignedString(198 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG39);
        break;

    case 'Y':
        PutAlignedString(178 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG40);
        PutAlignedString(178 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG41);
        break;

    case 'Z':
        PutAlignedString(178 + SCREENX, 453 + SCREENX, 195 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG42);
        PutAlignedString(178 + SCREENX, 453 + SCREENX, 210 + SCREENY, UPDATE_SCREEN_ON_LOG_MSG41);
        break;
    }
}

void Overlay_LogResMsg::on_render()
{
    int msX = Input::GetMouseX();
    int msY = Input::GetMouseY();

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);

    // Draw OK button with hover effect
    bool bHover = (msX >= 370 + SCREENX) && (msX <= 370 + DEF_BTNSZX + SCREENX) &&
                  (msY >= 244 + SCREENY) && (msY <= 244 + DEF_BTNSZY + SCREENY);
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, 370 + SCREENX, 244 + SCREENY, bHover ? 1 : 0);

    // Render the appropriate message
    RenderMessage();
}
