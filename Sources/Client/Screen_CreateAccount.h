#pragma once

#include "IGameScreen.h"
#include "NetConstants.h"
#include <string>

class Screen_CreateAccount : public IGameScreen {
public:
    SCREEN_TYPE(Screen_CreateAccount)

    Screen_CreateAccount(class CGame* pGame);
    virtual ~Screen_CreateAccount();

    virtual void on_initialize() override;
    virtual void on_uninitialize() override;
    virtual void on_update() override;
    virtual void on_render() override;
    virtual bool on_net_response(uint16_t wResponseType, char* pData) override;

private:
    void _submit_create_account();

    // Input field buffers
    char m_cNewAcctName[DEF_ACCOUNT_NAME];
    char m_cNewAcctPassword[DEF_ACCOUNT_PASS];
    char m_cNewAcctConfirm[DEF_ACCOUNT_PASS];
    char m_cEmail[DEF_ACCOUNT_EMAIL];

    char m_cNewAcctPrevFocus;
    short m_sNewAcctMsX;
    short m_sNewAcctMsY;
    char m_cNewAcctPrevLB;

    int m_cCurFocus;
    int m_cMaxFocus;
};
