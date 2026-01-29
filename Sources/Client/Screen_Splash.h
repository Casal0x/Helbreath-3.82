// Screen_Splash.h: Splash Screen Interface
//
// Displays a splash screen before loading. Transitions to loading
// screen after 10 seconds. Shows contributors one at a time with fade effects.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>

class Screen_Splash : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_Splash)

    explicit Screen_Splash(CGame* pGame);
    ~Screen_Splash() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    static constexpr uint32_t SPLASH_DURATION_MS = 12000;  // 12 seconds total
    static constexpr int NUM_CONTRIBUTORS = 5;
    static constexpr uint32_t TIME_PER_CONTRIBUTOR_MS = 2000;  // 2 seconds each
    static constexpr uint32_t FADE_DURATION_MS = 400;  // 0.4 second fade in/out

    // Calculate fade alpha (0.0 to 1.0) for current contributor
    // Last contributor doesn't fade out - stays visible for remaining time
    float GetContributorAlpha(uint32_t elapsedMs, int contributorIndex) const;
};
