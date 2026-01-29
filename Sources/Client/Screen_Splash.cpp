// Screen_Splash.cpp: Splash Screen Implementation
//
// Displays a splash screen before loading begins.
// Shows contributors one at a time with fade effects over 10 seconds.
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Splash.h"
#include "Screen_Loading.h"
#include "Game.h"
#include "GameModeManager.h"
#include "ITextRenderer.h"
#include <algorithm>
#include "GameFonts.h"
#include "TextLibExt.h"

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
    if (get_elapsed_ms() >= SPLASH_DURATION_MS)
    {
        set_screen<Screen_Loading>();
    }
}

float Screen_Splash::GetContributorAlpha(uint32_t elapsedMs, int contributorIndex) const
{
    uint32_t startTime = contributorIndex * TIME_PER_CONTRIBUTOR_MS;
    uint32_t endTime = startTime + TIME_PER_CONTRIBUTOR_MS;
    bool isLastContributor = (contributorIndex == NUM_CONTRIBUTORS - 1);

    // Before this contributor's time
    if (elapsedMs < startTime)
        return 0.0f;

    // After this contributor's time (except last one stays visible)
    if (elapsedMs >= endTime && !isLastContributor)
        return 0.0f;

    uint32_t timeInSlot = elapsedMs - startTime;

    // Fade in during first FADE_DURATION_MS
    if (timeInSlot < FADE_DURATION_MS)
        return static_cast<float>(timeInSlot) / FADE_DURATION_MS;

    // Last contributor doesn't fade out
    if (isLastContributor)
        return 1.0f;

    // Fade out during last FADE_DURATION_MS
    uint32_t timeUntilEnd = endTime - elapsedMs;
    if (timeUntilEnd < FADE_DURATION_MS)
        return static_cast<float>(timeUntilEnd) / FADE_DURATION_MS;

    // Fully visible in the middle
    return 1.0f;
}

void Screen_Splash::on_render()
{
    m_pGame->m_pSprite[DEF_SPRID_SPLASH_SCREEN]->Draw(0 + SCREENX, 0 + SCREENY, 0);

    // Credits - centered at bottom
    constexpr int lineHeight = 16;
    constexpr int bottomMargin = 20;

    // Helper to measure text width properly
    auto measureWidth = [](const char* text) {
        return TextLib::GetTextRenderer()->MeasureText(text).width;
    };

    // Helper to center text on screen
    auto centerX = [&measureWidth](const char* text) {
        return SCREENX + (640 - measureWidth(text)) / 2;
    };

    // Simulate bold by drawing text twice with 1-pixel offset
    auto putBold = [this](int x, int y, const char* text, COLORREF color) {
        TextLib::DrawText(GameFont::Default, x, y, text, TextLib::TextStyle::FromColorRef(color));
        TextLib::DrawText(GameFont::Default, x + 1, y, text, TextLib::TextStyle::FromColorRef(color));
    };

    // Credit data: name (bold), description, optional URL
    struct Credit { const char* name; const char* desc; const char* url; };
    Credit credits[NUM_CONTRIBUTORS] = {
        { "Centuu", " - HelbreathServer starting base", "https://github.com/centuu/HelbreathServer" },
        { "Pilgrim", " - Reasoning and Code Contributor", nullptr },
        { "Casal0x", " - Code Contributor", nullptr },
        { "Giaco", " - Secondary Code Contributor", nullptr },
        { "ShadowEvil", " - Primary Code Contributor", nullptr }
    };

    uint32_t elapsedMs = get_elapsed_ms();

    // Find current contributor to display
    for (int i = 0; i < NUM_CONTRIBUTORS; i++)
    {
        float alpha = GetContributorAlpha(elapsedMs, i);
        if (alpha <= 0.0f)
            continue;

        // Interpolate color based on alpha (fade to/from black background)
        uint8_t brightness = static_cast<uint8_t>(255 * alpha);
        COLORREF textColor = RGB(brightness, brightness, brightness);

        // Calculate full line width for centering
        char fullLine[128];
        snprintf(fullLine, sizeof(fullLine), "%s%s", credits[i].name, credits[i].desc);
        int fullWidth = measureWidth(fullLine);

        // Center the full line, then position name at start
        int x = SCREENX + (640 - fullWidth) / 2;
        int y = SCREENY + 480 - bottomMargin - lineHeight * 2;  // Room for URL below

        // Draw bold name
        putBold(x, y, credits[i].name, textColor);

        // Draw description immediately after name (measure name width properly)
        int nameWidth = measureWidth(credits[i].name);
        TextLib::DrawText(GameFont::Default, x + nameWidth, y, credits[i].desc, TextLib::TextStyle::FromColorRef(textColor));

        // Draw URL if present (light blue, also faded)
        if (credits[i].url)
        {
            uint8_t urlR = static_cast<uint8_t>(150 * alpha);
            uint8_t urlG = static_cast<uint8_t>(200 * alpha);
            uint8_t urlB = static_cast<uint8_t>(255 * alpha);
            int urlY = y + lineHeight;
            TextLib::DrawText(GameFont::Default, centerX(credits[i].url), urlY, credits[i].url, TextLib::TextStyle::FromColorRef(RGB(urlR, urlG, urlB)));
        }
    }

    m_pGame->DrawVersion();
}
