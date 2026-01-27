// Screen_Login.h: Login Screen Class
//
// Handles user login interaction, credential entry, and server connection request.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>

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

private:
    // Helper method for rendering (migrated from CGame::_Draw_OnLogin)
    void DrawLoginWindow(char* pAccount, char* pPassword, int msX, int msY, int iFrame);

private:
    // Screen-specific input buffers (migrated from file-scope statics)
    char m_cLoginName[12];
    char m_cLoginPassword[12];
    
    // Logic state
    char m_cPrevFocus;
    char m_cCurFocus;
    char m_cMaxFocus;
};
