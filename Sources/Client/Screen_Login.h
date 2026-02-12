// Screen_Login.h: Login Screen Class
//
// Handles user login interaction, credential entry, and server connection request.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>
#include <string>

class Screen_Login : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_Login)

    explicit Screen_Login(CGame* pGame);
    ~Screen_Login() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

    bool AttemptLogin();

private:
    // Helper method for rendering (migrated from CGame::_Draw_OnLogin)
    void DrawLoginWindow(int msX, int msY);

private:
    // Screen-specific input buffers (migrated from file-scope statics)
    std::string m_cLoginName;
    std::string m_cLoginPassword;
    
    // Logic state
    char m_cPrevFocus;
    char m_cCurFocus;
    char m_cMaxFocus;
};
