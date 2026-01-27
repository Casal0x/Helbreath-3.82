// Overlay_QueryForceLogin.h: "Character on Use" confirmation overlay
//
// Asks if player wants to force disconnect an existing session.
// Yes/No buttons with ESC to cancel.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Overlay_QueryForceLogin : public IGameScreen
{
public:
    SCREEN_TYPE(Overlay_QueryForceLogin)

    explicit Overlay_QueryForceLogin(CGame* pGame);
    ~Overlay_QueryForceLogin() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    uint32_t m_dwStartTime = 0;
    uint32_t m_dwAnimTime = 0;
};
