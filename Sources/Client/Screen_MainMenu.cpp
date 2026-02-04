// Screen_MainMenu.cpp: Main Menu Screen implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_MainMenu.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
Screen_MainMenu::Screen_MainMenu(CGame* pGame)
    : IGameScreen(pGame), m_cCurFocus(1), m_cMaxFocus(3)
{
}

void Screen_MainMenu::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::MainMenu);

    // Note: Sprite removal logic (m_pSprite.remove) is better handled by resource management,
    // but preserving legacy behavior for now.
    // DEF_SPRID_INTERFACE_ND_LOADING removal was in original code.
    // m_pGame->m_pSprite.remove(DEF_SPRID_INTERFACE_ND_LOADING); // Keeping strict to original if needed? 
    // Actually, let's keep it safe.
    m_pGame->m_pSprite.remove(DEF_SPRID_INTERFACE_ND_LOADING);
    
    m_pGame->EndInputString();

    m_cCurFocus = 1;
    m_cMaxFocus = 3;
    m_pGame->m_cArrowPressed = 0;
}

void Screen_MainMenu::on_uninitialize()
{
    // Nothing specific to clean up
}

void Screen_MainMenu::on_update()
{
    // Poll mouse input
    uint32_t dwTime = GameClock::GetTimeMS();
    m_pGame->m_dwCurTime = dwTime;

    // Update focus based on mouse position
    if ((Input::GetMouseX() >= 465) && (Input::GetMouseY() >= 238) && (Input::GetMouseX() <= 465+164) && (Input::GetMouseY() <= 238+22)) m_cCurFocus = 1;
    if ((Input::GetMouseX() >= 465) && (Input::GetMouseY() >= 276) && (Input::GetMouseX() <= 465+164) && (Input::GetMouseY() <= 276+22)) m_cCurFocus = 2;
    if ((Input::GetMouseX() >= 465) && (Input::GetMouseY() >= 315) && (Input::GetMouseX() <= 465+164) && (Input::GetMouseY() <= 315 +22)) m_cCurFocus = 3;

    if (m_pGame->m_cArrowPressed != 0) {
        switch (m_pGame->m_cArrowPressed) {
        case 1:
            m_cCurFocus--;
            if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
            break;
        case 3:
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
            break;
        }
        m_pGame->m_cArrowPressed = 0;
    }

    // Handle Tab key
    if (Input::IsKeyPressed(VK_TAB))
    {
        if (Input::IsShiftDown())
        {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus--;
            if (m_cCurFocus <= 0) m_cCurFocus = m_cMaxFocus;
        }
        else
        {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus++;
            if (m_cCurFocus > m_cMaxFocus) m_cCurFocus = 1;
        }
    }

    if (Input::IsKeyPressed(VK_RETURN) == true) {
        switch (m_cCurFocus) {
        case 1:
            m_pGame->PlaySound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::Login);
            return;
        case 2:
            m_pGame->PlaySound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::CreateNewAccount);
            return;
        case 3:
            m_pGame->PlaySound('E', 14, 5);
            m_pGame->ChangeGameMode(GameMode::Quit);
            return;
        }
    }

    // Mouse click detection
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Game button
        if (Input::IsMouseInRect(465, 238, 164, 22)) {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus = 1;
            m_pGame->ChangeGameMode(GameMode::Login);
            return;
        }
        // Account button
        else if (Input::IsMouseInRect(465, 276, 164, 22)) {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus = 2;
            m_pGame->ChangeGameMode(GameMode::CreateNewAccount);
            return;
        }
        // Quit button
        else if (Input::IsMouseInRect(465, 315, 164, 22)) {
            m_pGame->PlaySound('E', 14, 5);
            m_cCurFocus = 3;
            m_pGame->ChangeGameMode(GameMode::Quit);
            return;
        }
    }
}

void Screen_MainMenu::on_render()
{
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_MAINMENU, 0, 0, 0, true);

    switch (m_cCurFocus) {
    case 1:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(465, 238, 1);
        break;
    case 2:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(465, 276, 2);
        break;
    case 3:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(465, 315, 3);
        break;
    }

    m_pGame->DrawVersion();
}
