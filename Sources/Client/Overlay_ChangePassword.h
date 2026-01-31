// Overlay_ChangePassword.h: Password change overlay
//
// Displays password change form with input fields for old/new passwords.
// Shown over SelectCharacter screen.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include "NetConstants.h"

class Overlay_ChangePassword : public IGameScreen
{
public:
    SCREEN_TYPE(Overlay_ChangePassword)

    explicit Overlay_ChangePassword(CGame* pGame);
    ~Overlay_ChangePassword() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    void UpdateFocusedInput();
    void HandleSubmit();
    bool ValidateInputs();

    // Input buffers
    char m_cAccountName[DEF_ACCOUNT_NAME];
    char m_cOldPassword[DEF_ACCOUNT_PASS];
    char m_cNewPassword[DEF_ACCOUNT_PASS];
    char m_cConfirmPassword[DEF_ACCOUNT_PASS];

    // UI state
    int m_iCurFocus;      // 1=name, 2=old pass, 3=new pass, 4=confirm, 5=OK, 6=Cancel
    int m_iPrevFocus;
    int m_iMaxFocus;

    // Timing
    uint32_t m_dwStartTime = 0;
    uint32_t m_dwAnimTime = 0;
};
