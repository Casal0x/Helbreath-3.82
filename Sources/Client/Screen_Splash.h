// Screen_Splash.h: Splash Screen Interface
//
// Displays a splash screen before loading. Transitions to loading
// screen after 5 seconds.
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
    static constexpr uint32_t SPLASH_DURATION_MS = 5000;  // 5 seconds
};
