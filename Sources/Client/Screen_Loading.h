// Screen_Loading.h: Loading Screen Interface
//
// Resource loading screen that progressively loads game assets.
// Delegates to CGame's resource loading methods.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Screen_Loading : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_Loading)

    explicit Screen_Loading(CGame* pGame);
    ~Screen_Loading() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;
};
