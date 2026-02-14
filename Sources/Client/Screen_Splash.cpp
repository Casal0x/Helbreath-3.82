// Screen_Splash.cpp: Splash Screen Implementation
//
// Displays a splash screen before loading begins.
// Shows contributors one at a time with fade effects over 12 seconds.
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Splash.h"
#include "Screen_Loading.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "GameFonts.h"
#include "TextLibExt.h"
using namespace hb::client::sprite_id;

Screen_Splash::Screen_Splash(CGame* game)
    : IGameScreen(game)
{
}

void Screen_Splash::on_initialize()
{
    GameModeManager::set_current_mode(GameMode::Splash);

    m_game->m_sprite[SplashScreen] = hb::shared::sprite::Sprites::create("new-dialog", 3, false);

    m_credits = {{
        { "Centuu - HelbreathServer starting base", "https://github.com/centuu/HelbreathServer" },
        { "Pilgrim - Reasoning and Code Contributor", {} },
        { "Casal0x - Code Contributor", {} },
        { "Giaco - Secondary Code Contributor", {} },
        { "ShadowEvil - Primary Code Contributor", {} }
    }};
}

void Screen_Splash::on_uninitialize()
{
    m_game->m_sprite.remove(SplashScreen);
}

void Screen_Splash::on_update()
{
    if (get_elapsed_ms() >= SPLASH_DURATION_MS)
    {
        set_screen<Screen_Loading>();
    }
}

float Screen_Splash::get_contributor_alpha(uint32_t elapsedMs, int contributorIndex) const
{
    uint32_t startTime = contributorIndex * TIME_PER_CONTRIBUTOR_MS;
    uint32_t endTime = startTime + TIME_PER_CONTRIBUTOR_MS;
    bool last_contributor = (contributorIndex == NUM_CONTRIBUTORS - 1);

    // Before this contributor's time
    if (elapsedMs < startTime)
        return 0.0f;

    // After this contributor's time (except last one stays visible)
    if (elapsedMs >= endTime && !last_contributor)
        return 0.0f;

    uint32_t timeInSlot = elapsedMs - startTime;

    // Fade in during first FADE_DURATION_MS
    if (timeInSlot < FADE_DURATION_MS)
        return static_cast<float>(timeInSlot) / FADE_DURATION_MS;

    // Last contributor doesn't fade out
    if (last_contributor)
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
    m_game->m_sprite[SplashScreen]->draw(0, 0, 0);

    constexpr int lineHeight = 16;
    constexpr int bottomMargin = 20;
    const int creditY = LOGICAL_HEIGHT() - bottomMargin - lineHeight * 2;
    const uint32_t elapsedMs = get_elapsed_ms();

    for (int i = 0; i < NUM_CONTRIBUTORS; i++)
    {
        float alpha = get_contributor_alpha(elapsedMs, i);
        if (alpha <= 0.0f)
            continue;

        // Scale RGB by alpha to simulate fade (TTF fonts don't support alpha)
        auto fade = [alpha](const hb::shared::render::Color& c) {
            return hb::shared::text::TextStyle::with_drop_shadow(hb::shared::render::Color(
                static_cast<uint8_t>(c.r * alpha),
                static_cast<uint8_t>(c.g * alpha),
                static_cast<uint8_t>(c.b * alpha)));
        };

        const auto& credit = m_credits[i];
        hb::shared::text::draw_text_aligned(GameFont::Default, 0, creditY, LOGICAL_WIDTH(), lineHeight,
            credit.displayLine.c_str(), fade(GameColors::UIWhite), hb::shared::text::Align::TopCenter);

        if (!credit.url.empty())
        {
            hb::shared::text::draw_text_aligned(GameFont::Default, 0, creditY + lineHeight, LOGICAL_WIDTH(), lineHeight,
                credit.url.c_str(), fade(GameColors::UIFactionChat), hb::shared::text::Align::TopCenter);
        }
    }

    m_game->draw_version();
}
