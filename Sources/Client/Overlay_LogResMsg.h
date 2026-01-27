// Overlay_LogResMsg.h: Login/Account result message overlay
//
// Displays various login, account creation, and character operation results.
// Has an OK button that transitions based on context.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Overlay_LogResMsg : public IGameScreen
{
public:
    SCREEN_TYPE(Overlay_LogResMsg)

    // returnDest: Where to go when dismissed
    //   '0' = CreateNewAccount, '1' = MainMenu, '2' = CreateNewCharacter
    //   '3'/'4' = SelectCharacter, '5' = MainMenu, '6' = context-dependent
    //   '7'/'8' = MainMenu
    // msgCode: Message type to display ('1'-'9', 'A'-'M', 'U', 'X'-'Z')
    Overlay_LogResMsg(CGame* pGame, char returnDest, char msgCode);
    ~Overlay_LogResMsg() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    void HandleDismiss();
    void RenderMessage();

    char m_cReturnDest;
    char m_cMsgCode;
    uint32_t m_dwStartTime = 0;
    uint32_t m_dwAnimTime = 0;
};
