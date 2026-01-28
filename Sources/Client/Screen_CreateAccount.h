#pragma once

#include "IGameScreen.h"
#include <string>

class Screen_CreateAccount : public IGameScreen {
public:
    Screen_CreateAccount(class CGame* pGame);
    virtual ~Screen_CreateAccount();

    virtual void on_initialize() override;
    virtual void on_uninitialize() override;
    virtual void on_update() override;
    virtual void on_render() override;

    static constexpr const char* screen_type_id = "Screen_CreateAccount";
    virtual const char* get_type_id() const override { return screen_type_id; }

private:
    void _submit_create_account();

    // Input field buffers (migrated from static variables in Game.cpp)
    char m_cNewAcctName[12];
    char m_cNewAcctPassword[12];
    char m_cNewAcctConfirm[12];
    char m_cNewAcctQuiz[44];
    char m_cNewAcctTempQuiz[44];
    char m_cNewAcctAnswer[20];
    
    char m_cNewAcctPrevFocus;
    short m_sNewAcctMsX;
    short m_sNewAcctMsY;
    char m_cNewAcctPrevLB;

    int m_cCurFocus;
    int m_cMaxFocus;
};
