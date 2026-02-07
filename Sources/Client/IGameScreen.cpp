// IGameScreen.cpp: Implementation of IGameScreen helper methods
//
//////////////////////////////////////////////////////////////////////

#include "IGameScreen.h"
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

void IGameScreen::PutString(int iX, int iY, const char* pString, const Color& color)
{
    TextLib::DrawText(GameFont::Default, iX, iY, pString, TextLib::TextStyle::Color(color));
}

void IGameScreen::PutAlignedString(int iX1, int iX2, int iY, const char* pString,
                                    const Color& color)
{
    TextLib::DrawTextAligned(GameFont::Default, iX1, iY, iX2 - iX1, 15, pString,
                             TextLib::TextStyle::Color(color), TextLib::Align::TopCenter);
}

void IGameScreen::PutString_SprFont(int iX, int iY, const char* pStr, uint8_t r, uint8_t g, uint8_t b)
{
    TextLib::DrawText(GameFont::Bitmap1, iX, iY, pStr, TextLib::TextStyle::WithHighlight(Color(r, g, b)));
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
    m_pGame->AddEventList(const_cast<char*>(pTxt), cColor, bDupAllow);
}

// ============== Input String Helpers ==============

void IGameScreen::StartInputString(int sX, int sY, unsigned char iLen, char* pBuffer, bool bIsHide)
{
    m_pGame->StartInputString(sX, sY, iLen, pBuffer, bIsHide);
}

void IGameScreen::EndInputString()
{
    m_pGame->EndInputString();
}

void IGameScreen::ClearInputString()
{
    m_pGame->ClearInputString();
}

void IGameScreen::ShowReceivedString(bool bIsHide)
{
    m_pGame->ShowReceivedString(bIsHide);
}

// ============== Timing Helper ==============

uint32_t IGameScreen::get_elapsed_ms() const
{
    return GameClock::GetTimeMS() - GameModeManager::GetModeStartTime();
}
