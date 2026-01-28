// Screen_Splash.cpp: Splash Screen Implementation
//
// Displays a splash screen before loading begins.
// After 5 seconds, transitions to the loading screen.
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Splash.h"
#include "Screen_Loading.h"
#include "Game.h"
#include "GameModeManager.h"

Screen_Splash::Screen_Splash(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Splash::on_initialize()
{
    GameModeManager::SetCurrentMode(GameMode::Splash);

    m_pGame->m_pSprite[DEF_SPRID_SPLASH_SCREEN] = SpriteLib::Sprites::Create("New-Dialog", 3, false);
}

void Screen_Splash::on_uninitialize()
{
    m_pGame->m_pSprite.remove(DEF_SPRID_SPLASH_SCREEN);
}

void Screen_Splash::on_update()
{
    // Transition to loading screen after duration
    // GameModeManager ignores duplicate set_screen calls for same type
    if (get_elapsed_ms() >= SPLASH_DURATION_MS)
    {
        set_screen<Screen_Loading>();
    }
}

void Screen_Splash::on_render()
{
    m_pGame->m_pSprite[DEF_SPRID_SPLASH_SCREEN]->Draw(0 + SCREENX, 0 + SCREENY, 0);

    // Credits - centered at bottom
    constexpr int lineHeight = 14;
    constexpr int bottomMargin = 5;
    constexpr int charWidth = 7;

    // Helper lambda to center text (7 = charWidth)
    auto centerX = [](const char* text) { return SCREENX + (640 - (int)strlen(text) * 7) / 2; };

    // Simulate bold by drawing text twice with 1-pixel offset
    auto putBold = [this](int x, int y, const char* text, COLORREF color) {
        m_pGame->PutString(x, y, text, color);
        m_pGame->PutString(x + 1, y, text, color);
    };

    // Credit data: name (bold), description
    struct Credit { const char* name; const char* desc; };
    Credit credits[] = {
        { "ShadowEvil", " - Primary Code Contributor" },
        { "Giaco", " - Secondary Code Contributor" },
        { "Casal0x", " - Code Contributor" },
        { "Pilgrim", " - Reasoning and Code Contributor" },
        { "Centuu", " - HelbreathServer starting base" }
    };

    const char* centuu_url = "https://github.com/centuu/HelbreathServer";
    constexpr int numCredits = 5;
    constexpr int totalLines = numCredits + 1;  // +1 for URL
    int baseY = SCREENY + 480 - bottomMargin - (totalLines * lineHeight);

    for (int i = 0; i < numCredits; i++)
    {
        // Calculate full line for centering
        char fullLine[128];
        snprintf(fullLine, sizeof(fullLine), "%s%s", credits[i].name, credits[i].desc);

        int x = centerX(fullLine);
        int y = baseY + (i * lineHeight);

        // Draw bold name
        putBold(x, y, credits[i].name, RGB(255, 255, 255));

        // Draw description after name
        int nameWidth = (int)strlen(credits[i].name) * charWidth;
        m_pGame->PutString(x + nameWidth, y, credits[i].desc, RGB(255, 255, 255));
    }

    // Draw Centuu's GitHub URL on last line (light blue)
    int urlY = baseY + (numCredits * lineHeight);
    m_pGame->PutString(centerX(centuu_url), urlY, centuu_url, RGB(150, 200, 255));

    m_pGame->DrawVersion();
}
