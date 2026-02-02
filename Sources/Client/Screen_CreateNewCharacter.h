// Screen_CreateNewCharacter.h: Create New Character Screen Class
//
// Handles character creation UI including appearance customization and stat allocation.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>

class Screen_CreateNewCharacter : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_CreateNewCharacter)

    explicit Screen_CreateNewCharacter(CGame* pGame);
    ~Screen_CreateNewCharacter() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    // Screen-specific state (migrated from file-scope statics)
    int m_iNewCharPoint;
    char m_cNewCharName[12];
    char m_cNewCharPrevFocus;
    uint32_t m_dwNewCharMTime;
    short m_sNewCharMsX;
    short m_sNewCharMsY;
    bool m_bNewCharFlag;

    // Focus state (local to screen)
    int m_cCurFocus;
    int m_cMaxFocus;

    // Offset for centering 640x480 content in 800x600 base resolution
    static constexpr short OX = 80;
    static constexpr short OY = 60;
};
