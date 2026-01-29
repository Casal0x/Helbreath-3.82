#include "Screen_CreateAccount.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "XSocket.h"
#include "Misc.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

#ifdef DEF_MAKE_ACCOUNT

Screen_CreateAccount::Screen_CreateAccount(CGame* pGame)
    : IGameScreen(pGame), m_cCurFocus(1), m_cMaxFocus(9)
{
    std::memset(m_cNewAcctName, 0, sizeof(m_cNewAcctName));
    std::memset(m_cNewAcctPassword, 0, sizeof(m_cNewAcctPassword));
    std::memset(m_cNewAcctConfirm, 0, sizeof(m_cNewAcctConfirm));
    std::memset(m_cNewAcctQuiz, 0, sizeof(m_cNewAcctQuiz));
    std::memset(m_cNewAcctTempQuiz, 0, sizeof(m_cNewAcctTempQuiz));
    std::memset(m_cNewAcctAnswer, 0, sizeof(m_cNewAcctAnswer));
    
    m_cNewAcctPrevFocus = 1;
    m_sNewAcctMsX = 0;
    m_sNewAcctMsY = 0;
    m_cNewAcctPrevLB = 0;
}

Screen_CreateAccount::~Screen_CreateAccount()
{
}

void Screen_CreateAccount::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::CreateNewAccount);

    m_pGame->EndInputString();

    m_cNewAcctPrevFocus = 1;
    m_cNewAcctPrevLB = 0;
    m_cCurFocus = 1;
    m_cMaxFocus = 9;
    m_pGame->m_cArrowPressed = 0;

    std::memset(m_cNewAcctName, 0, sizeof(m_cNewAcctName));
    std::memset(m_cNewAcctPassword, 0, sizeof(m_cNewAcctPassword));
    std::memset(m_cNewAcctConfirm, 0, sizeof(m_cNewAcctConfirm));
    std::memset(m_pGame->m_cEmailAddr, 0, sizeof(m_pGame->m_cEmailAddr));
    std::memset(m_cNewAcctQuiz, 0, sizeof(m_cNewAcctQuiz));
    std::memset(m_cNewAcctAnswer, 0, sizeof(m_cNewAcctAnswer));

    m_pGame->StartInputString(427 + SCREENX, 84 + SCREENY, 11, m_cNewAcctName);
    m_pGame->ClearInputString();
}

void Screen_CreateAccount::on_uninitialize()
{
}

void Screen_CreateAccount::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwCurTime = dwTime;

    m_sNewAcctMsX = static_cast<short>(Input::GetMouseX());
    m_sNewAcctMsY = static_cast<short>(Input::GetMouseY());
    char cLB = Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0;

    // Handle arrow key navigation
    if (m_pGame->m_cArrowPressed != 0)
    {
        switch (m_pGame->m_cArrowPressed) {
        case 1: // Up
            m_cCurFocus--;
            if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
            break;
        case 3: // Down
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        }
        m_pGame->m_cArrowPressed = 0;
    }

    // Handle focus change - switch input field
    if (m_cNewAcctPrevFocus != m_cCurFocus)
    {
        m_pGame->EndInputString();
        switch (m_cCurFocus) {
        case 1: m_pGame->StartInputString(427 + SCREENX, 84 + SCREENY, 11, m_cNewAcctName); break;
        case 2: m_pGame->StartInputString(427 + SCREENX, 106 + SCREENY, 11, m_cNewAcctPassword, true); break;
        case 3: m_pGame->StartInputString(427 + SCREENX, 129 + SCREENY, 11, m_cNewAcctConfirm, true); break;
        case 4: m_pGame->StartInputString(311 + SCREENX, 48 + 190 - 25 + 2 + SCREENY, 49, m_pGame->m_cEmailAddr); break;
        case 5: m_pGame->StartInputString(311 + SCREENX, 48 + 226 - 25 + 4 + SCREENY, 43, m_cNewAcctQuiz); break;
        case 6: m_pGame->StartInputString(311 + SCREENX, 291 + SCREENY, 19, m_cNewAcctAnswer); break;
        }
        m_pGame->EndInputString();
        m_cNewAcctPrevFocus = m_cCurFocus;
    }

    // Direct mouse click focus selection (logic from UpdateScreen_CreateNewAccount)
    if (cLB != 0 && m_cNewAcctPrevLB == 0)
    {
        if (Input::IsMouseInRect(427 + SCREENX, 84 + SCREENY, 427 + 100 + SCREENX, 84 + 18 + SCREENY)) m_cCurFocus = 1;
        if (Input::IsMouseInRect(427 + SCREENX, 106 + SCREENY, 427 + 100 + SCREENX, 106 + 18 + SCREENY)) m_cCurFocus = 2;
        if (Input::IsMouseInRect(427 + SCREENX, 129 + SCREENY, 427 + 100 + SCREENX, 129 + 18 + SCREENY)) m_cCurFocus = 3;
        if (Input::IsMouseInRect(311 + SCREENX, 215 + SCREENY, 311 + 250 + SCREENX, 215 + 18 + SCREENY)) m_cCurFocus = 4;
        if (Input::IsMouseInRect(311 + SCREENX, 253 + SCREENY, 311 + 250 + SCREENX, 253 + 18 + SCREENY)) m_cCurFocus = 5;
        if (Input::IsMouseInRect(311 + SCREENX, 291 + SCREENY, 311 + 250 + SCREENX, 291 + 18 + SCREENY)) m_cCurFocus = 6;

        // Button 7: Create
        if (Input::IsMouseInRect(297 + SCREENX, 398 + SCREENY, 297 + 72 + SCREENX, 398 + 20 + SCREENY))
        {
            m_cCurFocus = 7;
            m_pGame->PlaySound('E', 14, 5);
            _submit_create_account();
        }
        // Button 8: Clear
        if (Input::IsMouseInRect(392 + SCREENX, 398 + SCREENY, 392 + 72 + SCREENX, 398 + 20 + SCREENY))
        {
            m_cCurFocus = 8;
            m_pGame->PlaySound('E', 14, 5);
            std::memset(m_cNewAcctName, 0, sizeof(m_cNewAcctName));
            std::memset(m_cNewAcctPassword, 0, sizeof(m_cNewAcctPassword));
            std::memset(m_cNewAcctConfirm, 0, sizeof(m_cNewAcctConfirm));
            std::memset(m_pGame->m_cEmailAddr, 0, sizeof(m_pGame->m_cEmailAddr));
            std::memset(m_cNewAcctQuiz, 0, sizeof(m_cNewAcctQuiz));
            std::memset(m_cNewAcctAnswer, 0, sizeof(m_cNewAcctAnswer));
            m_cCurFocus = 1;
            m_cNewAcctPrevFocus = 0; // Trigger reset
        }
        // Button 9: Cancel
        if (Input::IsMouseInRect(488 + SCREENX, 398 + SCREENY, 488 + 72 + SCREENX, 398 + 20 + SCREENY))
        {
            m_cCurFocus = 9;
            m_pGame->PlaySound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::MainMenu);
        }
    }
    m_cNewAcctPrevLB = cLB;

    if (Input::IsKeyPressed(VK_ESCAPE) == true)
    {
        m_pGame->ChangeGameMode(GameMode::MainMenu);
        return;
    }

    // Handle Tab key
    if (Input::IsKeyPressed(VK_TAB))
    {
        m_pGame->PlaySound('E', 14, 5);
        if (Input::IsShiftDown())
        {
            m_cCurFocus--;
            if (m_cCurFocus < 1) m_cCurFocus = 6;
        }
        else
        {
            m_cCurFocus++;
            if (m_cCurFocus > 6) m_cCurFocus = 1;
        }
    }

    // Handle Enter key
    if (Input::IsKeyPressed(VK_RETURN))
    {
        m_pGame->PlaySound('E', 14, 5);

        if (m_cCurFocus <= 6)
        {
            m_cCurFocus++;
            if (m_cCurFocus > 6) m_cCurFocus = 7; // Move to Create button
        }
        else if (m_cCurFocus == 7)
        {
            // Trigger Create action
            _submit_create_account();
        }
    }
}

void Screen_CreateAccount::_submit_create_account()
{
    // Compute validation
    char cTempQuiz[44];
    wsprintf(cTempQuiz, "%s", m_cNewAcctQuiz);
    CMisc::ReplaceString(cTempQuiz, ' ', '_');

    bool bReady = (strlen(m_cNewAcctName) > 0 && strlen(m_cNewAcctPassword) > 0 &&
        strlen(m_cNewAcctConfirm) > 0 && CMisc::bIsValidEmail(m_pGame->m_cEmailAddr) &&
        CMisc::bCheckValidName(m_cNewAcctName) && CMisc::bCheckValidName(m_cNewAcctPassword) &&
        std::memcmp(m_cNewAcctPassword, m_cNewAcctConfirm, 10) == 0 &&
        strlen(cTempQuiz) > 0 && strlen(m_cNewAcctAnswer) > 0 &&
        CMisc::bCheckValidName(cTempQuiz) && CMisc::bCheckValidName(m_cNewAcctAnswer));

    if (bReady)
    {
        m_pGame->m_cArrowPressed = 0;
        
        std::memset(m_pGame->m_pPlayer->m_cAccountName, 0, sizeof(m_pGame->m_pPlayer->m_cAccountName));
        std::strcpy(m_pGame->m_pPlayer->m_cAccountName, m_cNewAcctName);
        std::memset(m_pGame->m_pPlayer->m_cAccountPassword, 0, sizeof(m_pGame->m_pPlayer->m_cAccountPassword));
        std::strcpy(m_pGame->m_pPlayer->m_cAccountPassword, m_cNewAcctPassword);

        std::memset(m_pGame->m_cAccountQuiz, 0, sizeof(m_pGame->m_cAccountQuiz));
        std::strcpy(m_pGame->m_cAccountQuiz, m_cNewAcctQuiz);
        std::memset(m_pGame->m_cAccountAnswer, 0, sizeof(m_pGame->m_cAccountAnswer));
        std::strcpy(m_pGame->m_cAccountAnswer, m_cNewAcctAnswer);

        // Connection logic
        m_pGame->m_pLSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
        m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, m_pGame->m_iLogServerPort);
        m_pGame->m_pLSock->bInitBufferSize(30000);

        m_pGame->ChangeGameMode(GameMode::Connecting);
        m_pGame->m_dwConnectMode = MSGID_REQUEST_CREATENEWACCOUNT;
        std::memset(m_pGame->m_cMsg, 0, sizeof(m_pGame->m_cMsg));
        std::strcpy(m_pGame->m_cMsg, "01");
    }
}

void Screen_CreateAccount::on_render()
{
    int iFlag = 0;

    // Compute validation flags for display
    wsprintf(m_cNewAcctTempQuiz, "%s", m_cNewAcctQuiz);
    CMisc::ReplaceString(m_cNewAcctTempQuiz, ' ', '_');

    if (CMisc::bCheckValidName(m_cNewAcctAnswer) == false)        iFlag = 13;
    if (CMisc::bCheckValidName(m_cNewAcctTempQuiz) == false)     iFlag = 12;
    if (strlen(m_cNewAcctAnswer) == 0)                          iFlag = 11;
    if (strlen(m_cNewAcctTempQuiz) == 0)                        iFlag = 10;
    if (memcmp(m_cNewAcctPassword, m_cNewAcctConfirm, 10) != 0)  iFlag = 9;
    if (CMisc::bCheckValidName(m_cNewAcctPassword) == false)    iFlag = 7;
    if (CMisc::bCheckValidName(m_cNewAcctName) == false)        iFlag = 6;
    if (CMisc::bIsValidEmail(m_pGame->m_cEmailAddr) == false)   iFlag = 5;
    if (strlen(m_cNewAcctConfirm) == 0)                         iFlag = 3;
    if (strlen(m_cNewAcctPassword) == 0)                        iFlag = 2;
    if (strlen(m_cNewAcctName) == 0)                           iFlag = 1;

    // Draw background
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_NEWACCOUNT, 0 + SCREENX, 0 + SCREENY, 0, true);

    // Draw labels
    TextLib::DrawText(GameFont::Default, 377 + SCREENX, 84 + SCREENY, "Account:", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));
    TextLib::DrawText(GameFont::Default, 372 + SCREENX, 106 + SCREENY, "Password:", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));
    TextLib::DrawText(GameFont::Default, 372 + SCREENX, 129 + SCREENY, "(confirm)", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));
    TextLib::DrawText(GameFont::Default, 271 + SCREENX, 215 + SCREENY, "eMail:", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));
    TextLib::DrawText(GameFont::Default, 276 + SCREENX, 253 + SCREENY, "Quiz:", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));
    TextLib::DrawText(GameFont::Default, 266 + SCREENX, 291 + SCREENY, "Answer:", TextLib::TextStyle::FromColorRef(RGB(100, 100, 200)));

    // Show active input string
    if ((m_cCurFocus == 2) || (m_cCurFocus == 3))
        m_pGame->ShowReceivedString(true);
    else if ((m_cCurFocus == 1) || (m_cCurFocus == 4) || (m_cCurFocus == 5) || (m_cCurFocus == 6))
        m_pGame->ShowReceivedString();

    // Draw input field values
    if (m_cCurFocus != 1) {
        if (CMisc::bCheckValidName(m_cNewAcctName) != false)
            TextLib::DrawText(GameFont::Default, 427 + SCREENX, 84 + SCREENY, m_cNewAcctName, TextLib::TextStyle::WithShadow(100, 200, 100));
        else TextLib::DrawText(GameFont::Default, 427 + SCREENX, 84 + SCREENY, m_cNewAcctName, TextLib::TextStyle::WithShadow(200, 100, 100));
    }
    if (m_cCurFocus != 2) {
        std::string masked2(strlen(m_cNewAcctPassword), '*');
        if (CMisc::bCheckValidName(m_cNewAcctPassword) != false)
            TextLib::DrawText(GameFont::Default, 427 + SCREENX, 106 + SCREENY, masked2.c_str(), TextLib::TextStyle::WithShadow(100, 200, 100));
        else TextLib::DrawText(GameFont::Default, 427 + SCREENX, 106 + SCREENY, masked2.c_str(), TextLib::TextStyle::WithShadow(200, 100, 100));
    }
    if (m_cCurFocus != 3) {
        std::string masked3(strlen(m_cNewAcctConfirm), '*');
        if (memcmp(m_cNewAcctPassword, m_cNewAcctConfirm, 10) == 0)
            TextLib::DrawText(GameFont::Default, 427 + SCREENX, 129 + SCREENY, masked3.c_str(), TextLib::TextStyle::WithShadow(100, 200, 100));
        else TextLib::DrawText(GameFont::Default, 427 + SCREENX, 129 + SCREENY, masked3.c_str(), TextLib::TextStyle::WithShadow(200, 100, 100));
    }
    if (m_cCurFocus != 4) {
        if (CMisc::bIsValidEmail(m_pGame->m_cEmailAddr))
            TextLib::DrawText(GameFont::Default, 311 + SCREENX, 48 + 190 - 25 + 2 + SCREENY, m_pGame->m_cEmailAddr, TextLib::TextStyle::WithShadow(100, 200, 100));
        else TextLib::DrawText(GameFont::Default, 311 + SCREENX, 48 + 190 - 25 + 2 + SCREENY, m_pGame->m_cEmailAddr, TextLib::TextStyle::WithShadow(200, 100, 100));
    }
    if (m_cCurFocus != 5) {
        if (CMisc::bCheckValidName(m_cNewAcctTempQuiz) != false)
            TextLib::DrawText(GameFont::Default, 311 + SCREENX, 48 + 226 - 25 + 4 + SCREENY, m_cNewAcctQuiz, TextLib::TextStyle::WithShadow(100, 200, 100));
    }
    if (m_cCurFocus != 6) {
        if (CMisc::bCheckValidName(m_cNewAcctAnswer) != false)
            TextLib::DrawText(GameFont::Default, 311 + SCREENX, 291 + SCREENY, m_cNewAcctAnswer, TextLib::TextStyle::WithShadow(100, 200, 100));
    }

    // Draw help text based on focus
    switch (m_cCurFocus) {
    case 1:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT2, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 2:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT4, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 3:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT8, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 4:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT21, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT22, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 360 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT23, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 5:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT25, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT26, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 6:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT29, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 7:
        switch (iFlag) {
        case 0:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT33, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 1:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT35, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 2:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT38, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 3:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT42, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 5:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT50, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 6:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT52, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT53, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 7:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT56, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT57, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 9:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT63, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT64, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 360 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT65, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 10:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT67, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 11:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT69, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 12:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT73, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT74, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        case 13:
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT77, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 345 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT78, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
            break;
        }
        break;
    case 8:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT80, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    case 9:
        TextLib::DrawTextAligned(GameFont::Default, 290 + SCREENX, 330 + SCREENY, (575 + SCREENX) - (290 + SCREENX), 15, UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT81, TextLib::TextStyle::Color(0, 0, 0), TextLib::Align::TopCenter);
        break;
    }

    // Draw buttons - highlight on focus OR mouse hover
    // Button order left to right: Create, Clear, Cancel

    // Button 7: Create (at 297, 398 - size 72x20) - LEFT
    bool bHoverCreate = (m_sNewAcctMsX >= 297 + SCREENX && m_sNewAcctMsX <= 297 + 72 + SCREENX &&
        m_sNewAcctMsY >= 398 + SCREENY && m_sNewAcctMsY <= 398 + 20 + SCREENY);
    if ((iFlag == 0) && (m_cCurFocus == 7 || bHoverCreate))
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(199 + 98 + SCREENX, 398 + SCREENY, 25);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(199 + 98 + SCREENX, 398 + SCREENY, 24);

    // Button 8: Clear (at 392, 398 - size 72x20) - CENTER
    bool bHoverClear = (m_sNewAcctMsX >= 392 + SCREENX && m_sNewAcctMsX <= 392 + 72 + SCREENX &&
        m_sNewAcctMsY >= 398 + SCREENY && m_sNewAcctMsY <= 398 + 20 + SCREENY);
    if (m_cCurFocus == 8 || bHoverClear)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(294 + 98 + SCREENX, 398 + SCREENY, 27);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(294 + 98 + SCREENX, 398 + SCREENY, 26);

    // Button 9: Cancel (at 488, 398 - size 72x20) - RIGHT
    bool bHoverCancel = (m_sNewAcctMsX >= 488 + SCREENX && m_sNewAcctMsX <= 488 + 72 + SCREENX &&
        m_sNewAcctMsY >= 398 + SCREENY && m_sNewAcctMsY <= 398 + 20 + SCREENY);
    if (m_cCurFocus == 9 || bHoverCancel)
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(390 + 98 + SCREENX, 398 + SCREENY, 17);
    else m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON]->Draw(390 + 98 + SCREENX, 398 + SCREENY, 16);

    m_pGame->DrawVersion();
}

#endif // DEF_MAKE_ACCOUNT
