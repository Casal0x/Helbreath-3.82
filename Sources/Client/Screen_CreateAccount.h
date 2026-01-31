#pragma once

#include "IGameScreen.h"
#include "NetConstants.h"
#include <string>
#include <string_view>

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
    void _clear_fields();

    // Raw input buffers - written to directly by the engine's input system.
    // Use name(), password(), confirm(), email() for std::string_view access.
    char m_cNewAcctName[DEF_ACCOUNT_NAME];
    char m_cNewAcctPassword[DEF_ACCOUNT_PASS];
    char m_cNewAcctConfirm[DEF_ACCOUNT_PASS];
    char m_cEmail[DEF_ACCOUNT_EMAIL];

    std::string_view name()     const { return m_cNewAcctName; }
    std::string_view password() const { return m_cNewAcctPassword; }
    std::string_view confirm()  const { return m_cNewAcctConfirm; }
    std::string_view email()    const { return m_cEmail; }

    char m_cNewAcctPrevFocus;
    short m_sNewAcctMsX;
    short m_sNewAcctMsY;
    char m_cNewAcctPrevLB;

    int m_cCurFocus;
    int m_cMaxFocus;
};
