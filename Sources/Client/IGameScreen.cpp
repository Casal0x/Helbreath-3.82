// IGameScreen.cpp: Implementation of IGameScreen helper methods
//
//////////////////////////////////////////////////////////////////////

#include "IGameScreen.h"
#include "EventListManager.h"
#include "TextInputManager.h"
#include "Game.h"
#include "GameModeManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "GlobalDef.h"
#include "SpriteID.h"

IGameScreen::IGameScreen(CGame* pGame)
    : m_pGame(pGame)
{
}

// ============== Drawing Helpers ==============

void IGameScreen::DrawNewDialogBox(char cType, int sX, int sY, int iFrame,
                                    bool bIsNoColorKey, bool bIsTrans)
{
    m_pGame->DrawNewDialogBox(cType, sX, sY, iFrame, bIsNoColorKey, bIsTrans);
}

void IGameScreen::GetCenteredDialogPos(char cType, int iFrame, int& outX, int& outY)
{
    auto rect = m_pGame->m_pSprite[cType]->GetFrameRect(iFrame);
    outX = (LOGICAL_WIDTH() - rect.width) / 2;
    outY = (LOGICAL_HEIGHT() - rect.height) / 2;
}

void IGameScreen::PutString(int iX, int iY, const char* pString, const hb::shared::render::Color& color)
{
    hb::shared::text::DrawText(GameFont::Default, iX, iY, pString, hb::shared::text::TextStyle::Color(color));
}

void IGameScreen::PutAlignedString(int iX1, int iX2, int iY, const char* pString,
                                    const hb::shared::render::Color& color)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, iX1, iY, iX2 - iX1, 15, pString,
                             hb::shared::text::TextStyle::Color(color), hb::shared::text::Align::TopCenter);
}

void IGameScreen::PutString_SprFont(int iX, int iY, const char* pStr, uint8_t r, uint8_t g, uint8_t b)
{
    hb::shared::text::DrawText(GameFont::Bitmap1, iX, iY, pStr, hb::shared::text::TextStyle::WithHighlight(hb::shared::render::Color(r, g, b)));
}

void IGameScreen::DrawVersion()
{
    m_pGame->DrawVersion();
}

// ============== Audio Helpers ==============

void IGameScreen::PlayGameSound(char cType, int iNum, int iDist, long lPan)
{
    m_pGame->PlayGameSound(cType, iNum, iDist, lPan);
}

// ============== Event/Message Helpers ==============

void IGameScreen::AddEventList(const char* pTxt, char cColor, bool bDupAllow)
{
    EventListManager::Get().AddEvent(pTxt, cColor, bDupAllow);
}

// ============== Input String Helpers ==============

void IGameScreen::StartInputString(int sX, int sY, unsigned char iLen, char* pBuffer, bool bIsHide)
{
    TextInputManager::Get().StartInput(sX, sY, iLen, pBuffer, bIsHide);
}

void IGameScreen::EndInputString()
{
    TextInputManager::Get().EndInput();
}

void IGameScreen::ClearInputString()
{
    TextInputManager::Get().ClearInput();
}

void IGameScreen::ShowReceivedString(bool bIsHide)
{
    TextInputManager::Get().ShowInput(bIsHide);
}

// ============== Timing Helper ==============

uint32_t IGameScreen::get_elapsed_ms() const
{
    return GameClock::GetTimeMS() - GameModeManager::GetModeStartTime();
}
