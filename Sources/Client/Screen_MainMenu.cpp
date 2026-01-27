// Screen_MainMenu.cpp: Main Menu Screen implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_MainMenu.h"
#include "Game.h"
#include "GameModeManager.h"
#include "IInput.h"
#include "GlobalDef.h"
#include "XSocket.h"

extern class XSocket* G_pCalcSocket;

Screen_MainMenu::Screen_MainMenu(CGame* pGame)
    : IGameScreen(pGame), m_cCurFocus(1), m_cMaxFocus(3)
{
}

void Screen_MainMenu::on_initialize()
{
    // Set legacy mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetLegacyMode(GameMode::MainMenu);

    // Initialization logic from CGame::UpdateScreen_MainMenu
    if (G_pCalcSocket != 0)
    {
        delete G_pCalcSocket;
        G_pCalcSocket = 0;
    }
    
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
    // Update focus based on mouse position
    if ((Input::GetMouseX() >= 384 + SCREENX) && (Input::GetMouseY() >= 177 + SCREENY) && (Input::GetMouseX() <= 548 + SCREENX) && (Input::GetMouseY() <= 198 + SCREENY)) m_cCurFocus = 1;
    if ((Input::GetMouseX() >= 384 + SCREENX) && (Input::GetMouseY() >= 215 + SCREENY) && (Input::GetMouseX() <= 548 + SCREENX) && (Input::GetMouseY() <= 236 + SCREENY)) m_cCurFocus = 2;
    if ((Input::GetMouseX() >= 384 + SCREENX) && (Input::GetMouseY() >= 254 + SCREENY) && (Input::GetMouseX() <= 548 + SCREENX) && (Input::GetMouseY() <= 275 + SCREENY)) m_cCurFocus = 3;

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

    if (Input::IsKeyPressed(VK_RETURN) == true) {
        m_pGame->PlaySound('E', 14, 5);
        switch (m_cCurFocus) {
        case 1:
            m_pGame->ChangeGameMode(GameMode::Login);
            return;
        case 2:
            m_pGame->ClearContents_OnSelectCharacter();
            m_pGame->ChangeGameMode(GameMode::CreateNewAccount);
            return;
        case 3:
            m_pGame->ChangeGameMode(GameMode::Quit);
            return;
        }
    }

    // Mouse click detection
    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_pGame->PlaySound('E', 14, 5);
        // Game button
        if (Input::IsMouseInRect(384 + SCREENX, 177 + SCREENY, 548 + SCREENX, 198 + SCREENY)) {
            m_cCurFocus = 1;
            m_pGame->ChangeGameMode(GameMode::Login);
        }
        // Account button
        else if (Input::IsMouseInRect(384 + SCREENX, 215 + SCREENY, 548 + SCREENX, 236 + SCREENY)) {
            m_cCurFocus = 2;
            m_pGame->ClearContents_OnSelectCharacter();
            m_pGame->ChangeGameMode(GameMode::CreateNewAccount);
            return;
        }
        // Quit button
        else if (Input::IsMouseInRect(384 + SCREENX, 254 + SCREENY, 548 + SCREENX, 275 + SCREENY)) {
            m_cCurFocus = 3;
            m_pGame->ChangeGameMode(GameMode::Quit);
            return;
        }
    }
}

void Screen_MainMenu::on_render()
{
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_MAINMENU, 0 + SCREENX, 0 + SCREENY, 0, true);

    switch (m_cCurFocus) {
    case 1:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(384 + SCREENX, 177 + SCREENY, 1);
        break;
    case 2:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(384 + SCREENX, 215 + SCREENY, 2);
        break;
    case 3:
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU]->Draw(384 + SCREENX, 254 + SCREENY, 3);
        break;
    }

    m_pGame->DrawVersion();
}
