// Overlay_ChangePassword.h: Password change overlay
//
// Displays password change form with input fields for old/new passwords.
// Shown over SelectCharacter screen.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include "NetConstants.h"
#include <string>

class Overlay_ChangePassword : public IGameScreen
{
public:
    SCREEN_TYPE(Overlay_ChangePassword)

    explicit Overlay_ChangePassword(CGame* game);
    ~Overlay_ChangePassword() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    void update_focused_input();
    void handle_submit();
    bool validate_inputs();

    // Input buffers
    std::string m_account_name;
    std::string m_cOldPassword;
    std::string m_cNewPassword;
    std::string m_cConfirmPassword;

    // UI state
    int m_iCurFocus;      // 1=name, 2=old pass, 3=new pass, 4=confirm, 5=OK, 6=Cancel
    int m_iPrevFocus;
    int m_iMaxFocus;

    // Timing
    uint32_t m_dwStartTime = 0;
    uint32_t m_dwAnimTime = 0;
};
