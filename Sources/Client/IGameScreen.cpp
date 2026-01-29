// IGameScreen.cpp: Implementation of IGameScreen helper methods
//
//////////////////////////////////////////////////////////////////////

#include "IGameScreen.h"
#include "Game.h"
#include "GameModeManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"

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

void IGameScreen::PutString(int iX, int iY, const char* pString, uint32_t color)
{
    TextLib::DrawText(GameFont::Default, iX, iY, pString, TextLib::TextStyle::FromColorRef(color));
}

void IGameScreen::PutString2(int iX, int iY, const char* pString, short sR, short sG, short sB)
{
    TextLib::DrawText(GameFont::Default, iX, iY, pString, TextLib::TextStyle::WithShadow(sR, sG, sB));
}

void IGameScreen::PutAlignedString(int iX1, int iX2, int iY, const char* pString,
                                    short sR, short sG, short sB)
{
    TextLib::DrawTextAligned(GameFont::Default, iX1, iY, iX2 - iX1, 15, pString,
                             TextLib::TextStyle::Color(sR, sG, sB), TextLib::Align::TopCenter);
}

void IGameScreen::PutString_SprFont(int iX, int iY, const char* pStr, short sR, short sG, short sB)
{
    TextLib::DrawText(GameFont::Bitmap1, iX, iY, pStr, TextLib::TextStyle::WithHighlight(sR, sG, sB));
}

void IGameScreen::DrawVersion()
{
    m_pGame->DrawVersion();
}

// ============== Audio Helpers ==============

void IGameScreen::PlaySound(char cType, int iNum, int iDist, long lPan)
{
    m_pGame->PlaySound(cType, iNum, iDist, lPan);
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
