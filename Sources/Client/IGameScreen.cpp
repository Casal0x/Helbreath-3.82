// IGameScreen.cpp: Implementation of IGameScreen helper methods
//
//////////////////////////////////////////////////////////////////////

#include "IGameScreen.h"
#include "Game.h"
#include "GameModeManager.h"

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
    m_pGame->PutString(iX, iY, const_cast<char*>(pString), color);
}

void IGameScreen::PutString2(int iX, int iY, const char* pString, short sR, short sG, short sB)
{
    m_pGame->PutString2(iX, iY, const_cast<char*>(pString), sR, sG, sB);
}

void IGameScreen::PutAlignedString(int iX1, int iX2, int iY, const char* pString,
                                    short sR, short sG, short sB)
{
    m_pGame->PutAlignedString(iX1, iX2, iY, const_cast<char*>(pString), sR, sG, sB);
}

void IGameScreen::PutString_SprFont(int iX, int iY, const char* pStr, short sR, short sG, short sB)
{
    m_pGame->PutString_SprFont(iX, iY, const_cast<char*>(pStr), sR, sG, sB);
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
