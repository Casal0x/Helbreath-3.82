// Overlay_VersionNotMatch.h: Version mismatch error overlay
//
// Displays version mismatch error. Any input closes the application.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Overlay_VersionNotMatch : public IGameScreen
{
public:
    SCREEN_TYPE(Overlay_VersionNotMatch)

    explicit Overlay_VersionNotMatch(CGame* game);
    ~Overlay_VersionNotMatch() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    int m_iFrameCount = 0;
};
