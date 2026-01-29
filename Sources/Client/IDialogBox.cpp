#include "IDialogBox.h"
#include "Game.h"
#include "GameFonts.h"
#include "TextLibExt.h"

IDialogBox::IDialogBox(DialogBoxId::Type id, CGame* pGame)
	: m_pGame(pGame)
	, m_id(id)
{
}

DialogBoxInfo& IDialogBox::Info()
{
	return m_pGame->m_dialogBoxManager.Info(m_id);
}

const DialogBoxInfo& IDialogBox::Info() const
{
	return m_pGame->m_dialogBoxManager.Info(m_id);
}

bool IDialogBox::IsEnabled() const
{
	return m_pGame->m_dialogBoxManager.IsEnabled(m_id);
}

void IDialogBox::DrawNewDialogBox(char cType, int sX, int sY, int iFrame, bool bIsNoColorKey, bool bIsTrans)
{
	m_pGame->DrawNewDialogBox(cType, sX, sY, iFrame, bIsNoColorKey, bIsTrans);
}

void IDialogBox::PutString(int iX, int iY, const char* pString, uint32_t color)
{
	TextLib::DrawText(GameFont::Default, iX, iY, pString, TextLib::TextStyle::FromColorRef(color));
}

void IDialogBox::PutString2(int iX, int iY, const char* pString, short sR, short sG, short sB)
{
	TextLib::DrawText(GameFont::Default, iX, iY, pString, TextLib::TextStyle::WithShadow(sR, sG, sB));
}

void IDialogBox::PutAlignedString(int iX1, int iX2, int iY, const char* pString, short sR, short sG, short sB)
{
	TextLib::DrawTextAligned(GameFont::Default, iX1, iY, iX2 - iX1, 15, pString,
	                         TextLib::TextStyle::Color(sR, sG, sB), TextLib::Align::TopCenter);
}

void IDialogBox::PlaySoundEffect(char cType, int iNum, int iDist, long lPan)
{
	m_pGame->PlaySound(cType, iNum, iDist, lPan);
}

void IDialogBox::AddEventList(char* pTxt, char cColor, bool bDupAllow)
{
	m_pGame->AddEventList(pTxt, cColor, bDupAllow);
}

bool IDialogBox::bSendCommand(uint32_t dwMsgID, uint16_t wCommand, char cDir, int iV1, int iV2, int iV3, char* pString, int iV4)
{
	return m_pGame->bSendCommand(dwMsgID, wCommand, cDir, iV1, iV2, iV3, pString, iV4);
}

void IDialogBox::SetDefaultRect(short sX, short sY, short sSizeX, short sSizeY)
{
	auto& info = Info();
	info.sX = sX;
	info.sY = sY;
	info.sSizeX = sSizeX;
	info.sSizeY = sSizeY;
}

void IDialogBox::EnableDialogBox(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString)
{
	m_pGame->m_dialogBoxManager.EnableDialogBox(id, cType, sV1, sV2, pString);
}

void IDialogBox::DisableDialogBox(DialogBoxId::Type id)
{
	m_pGame->m_dialogBoxManager.DisableDialogBox(id);
}

void IDialogBox::DisableThisDialog()
{
	m_pGame->m_dialogBoxManager.DisableDialogBox(m_id);
}

void IDialogBox::SetCanCloseOnRightClick(bool bCanClose)
{
	Info().bCanCloseOnRightClick = bCanClose;
}

IDialogBox* IDialogBox::GetDialogBox(DialogBoxId::Type id)
{
	return m_pGame->m_dialogBoxManager.GetDialogBox(id);
}

DialogBoxInfo& IDialogBox::InfoOf(DialogBoxId::Type id)
{
	return m_pGame->m_dialogBoxManager.Info(id);
}
