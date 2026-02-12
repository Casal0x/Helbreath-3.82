#include "FloatingTextManager.h"
#include "MapData.h"
#include "IRenderer.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "ConfigManager.h"
#include "ActionID.h"
#include "AudioManager.h"
#include "GameGeometry.h"
#include <cstdio>
#include <cstring>
#include <format>
#include <string>


using namespace hb::shared::action;

// Multibyte character kind detection (replicates CGame::GetCharKind)
static int GetCharKind(const char* str, int index)
{
	int kind = 1;
	do
	{
		if (kind == 2) kind = 3;
		else
		{
			if ((unsigned char)*str < 128) kind = 1;
			else kind = 2;
		}
		str++;
		index--;
	} while (index >= 0);
	return kind;
}
static constexpr int CharKind_HAN1 = 2; // CharKind_HAN1 equivalent

// ---------------------------------------------------------------
// Slot management
// ---------------------------------------------------------------

int CFloatingTextManager::FindFreeSlot() const
{
	for (int i = 1; i < MaxMessages; i++)
		if (!m_messages[i])
			return i;
	return 0;
}

int CFloatingTextManager::BindToTile(int iIndex, int iObjectID, CMapData* pMapData, short sX, short sY)
{
	m_messages[iIndex]->m_iObjectID = iObjectID;
	if (!pMapData->bSetChatMsgOwner(static_cast<uint16_t>(iObjectID), sX, sY, iIndex))
	{
		m_messages[iIndex].reset();
		return 0;
	}
	return iIndex;
}

// ---------------------------------------------------------------
// Add methods
// ---------------------------------------------------------------

int CFloatingTextManager::AddChatText(const char* pMsg, uint32_t dwTime, int iObjectID,
                                      CMapData* pMapData, short sX, short sY)
{
	int i = FindFreeSlot();
	if (i == 0) return 0;
	m_messages[i] = std::make_unique<CFloatingText>(ChatTextType::PlayerChat, pMsg, dwTime);
	return BindToTile(i, iObjectID, pMapData, sX, sY);
}

int CFloatingTextManager::AddDamageText(DamageTextType eType, const char* pMsg, uint32_t dwTime,
                                        int iObjectID, CMapData* pMapData)
{
	int i = FindFreeSlot();
	if (i == 0) return 0;
	m_messages[i] = std::make_unique<CFloatingText>(eType, pMsg, dwTime);
	return BindToTile(i, iObjectID, pMapData, -10, -10);
}

int CFloatingTextManager::AddNotifyText(NotifyTextType eType, const char* pMsg, uint32_t dwTime,
                                        int iObjectID, CMapData* pMapData)
{
	int i = FindFreeSlot();
	if (i == 0) return 0;
	m_messages[i] = std::make_unique<CFloatingText>(eType, pMsg, dwTime);
	return BindToTile(i, iObjectID, pMapData, -10, -10);
}

int CFloatingTextManager::AddDamageFromValue(short sDamage, bool bLastHit, uint32_t dwTime,
                                             int iObjectID, CMapData* pMapData)
{
	std::string cTxt;

	if (sDamage == Sentinel::DamageImmune)
	{
		cTxt = "* Immune *";
		AudioManager::Get().PlayGameSound(SoundType::Character, 17, 0, 0);
		return AddDamageText(DamageTextType::Medium, cTxt.c_str(), dwTime, iObjectID, pMapData);
	}
	if (sDamage == Sentinel::MagicFailed)
	{
		cTxt = "* Failed! *";
		AudioManager::Get().PlayGameSound(SoundType::Character, 17, 0, 0);
		return AddDamageText(DamageTextType::Medium, cTxt.c_str(), dwTime, iObjectID, pMapData);
	}
	if (sDamage > 128)
	{
		cTxt = "Critical!";
		return AddDamageText(DamageTextType::Large, cTxt.c_str(), dwTime, iObjectID, pMapData);
	}
	if (sDamage > 0)
	{
		if (bLastHit && sDamage >= 12)
			cTxt = std::format("-{}Pts!", sDamage);
		else
			cTxt = std::format("-{}Pts", sDamage);

		DamageTextType eType;
		if (sDamage < 12)       eType = DamageTextType::Small;
		else if (sDamage < 40)  eType = DamageTextType::Medium;
		else                    eType = DamageTextType::Large;
		return AddDamageText(eType, cTxt.c_str(), dwTime, iObjectID, pMapData);
	}
	return 0;
}

// ---------------------------------------------------------------
// Removal / cleanup
// ---------------------------------------------------------------

void CFloatingTextManager::RemoveByObjectID(int iObjectID)
{
	for (int i = 1; i < MaxMessages; i++)
		if (m_messages[i] && m_messages[i]->m_iObjectID == iObjectID)
			m_messages[i].reset();
}

void CFloatingTextManager::ReleaseExpired(uint32_t dwTime)
{
	for (int i = 1; i < MaxMessages; i++)
	{
		if (!m_messages[i]) continue;
		const auto& params = m_messages[i]->GetParams();
		uint32_t dwTotal = params.dwLifetimeMs + params.dwShowDelayMs;
		if ((dwTime - m_messages[i]->m_dwTime) > dwTotal)
			m_messages[i].reset();
	}
}

void CFloatingTextManager::Clear(int iIndex)
{
	if (iIndex >= 0 && iIndex < MaxMessages)
		m_messages[iIndex].reset();
}

void CFloatingTextManager::ClearAll()
{
	for (int i = 0; i < MaxMessages; i++)
		m_messages[i].reset();
}

// ---------------------------------------------------------------
// Access / position updates
// ---------------------------------------------------------------

CFloatingText* CFloatingTextManager::Get(int iIndex)
{
	if (iIndex >= 0 && iIndex < MaxMessages)
		return m_messages[iIndex].get();
	return nullptr;
}

void CFloatingTextManager::UpdatePosition(int iIndex, short sX, short sY)
{
	if (iIndex >= 0 && iIndex < MaxMessages && m_messages[iIndex])
	{
		m_messages[iIndex]->m_sX = sX;
		m_messages[iIndex]->m_sY = sY;
	}
}

bool CFloatingTextManager::IsValid(int iIndex, int iObjectID) const
{
	if (iIndex < 0 || iIndex >= MaxMessages) return false;
	return m_messages[iIndex] && m_messages[iIndex]->m_iObjectID == iObjectID;
}

bool CFloatingTextManager::IsOccupied(int iIndex) const
{
	if (iIndex < 0 || iIndex >= MaxMessages) return false;
	return m_messages[iIndex] != nullptr;
}

// ---------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------

void CFloatingTextManager::DrawAll(short sMinX, short sMinY, short sMaxX, short sMaxY,
                                   uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer)
{
	for (int i = 0; i < MaxMessages; i++)
	{
		if (!m_messages[i]) continue;
		if (m_messages[i]->m_sX < sMinX || m_messages[i]->m_sX > sMaxX ||
		    m_messages[i]->m_sY < sMinY || m_messages[i]->m_sY > sMaxY)
			continue;
		DrawMessage(*m_messages[i], m_messages[i]->m_sX, m_messages[i]->m_sY, dwCurTime, pRenderer);
	}
}

void CFloatingTextManager::DrawSingle(int iIndex, short sX, short sY,
                                      uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer)
{
	if (iIndex < 0 || iIndex >= MaxMessages || !m_messages[iIndex]) return;
	DrawMessage(*m_messages[iIndex], sX, sY, dwCurTime, pRenderer);
}

void CFloatingTextManager::DrawMessage(const CFloatingText& msg, short sX, short sY,
                                       uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer)
{
	const auto& params = msg.GetParams();

	// Compute elapsed time
	uint32_t dwElapsed = dwCurTime - msg.m_dwTime;
	if (dwElapsed < params.dwShowDelayMs) return;
	uint32_t dwVisible = dwElapsed - params.dwShowDelayMs;

	// Compute rise offset via linear interpolation
	int iRise;
	if (params.iRiseDurationMs <= 0 || static_cast<int>(dwVisible) >= params.iRiseDurationMs)
		iRise = params.iRisePixels;
	else
		iRise = static_cast<int>((dwVisible * params.iRisePixels) / params.iRiseDurationMs);

	// Detail-level transparency
	bool bIsTrans = (ConfigManager::Get().GetDetailLevel() != 0);

	if (params.bUseSpriteFont)
	{
		// Sprite font path: split text into 20-char lines for multi-line display
		// Note: Text encoding is EUC-KR (legacy Korean). GetCharKind detects multibyte boundaries.
		const char* pText = msg.m_szText.c_str();
		char cMsgA[22]{}, cMsgB[22]{}, cMsgC[22]{};
		const char* cp = pText;
		size_t cpLen;
		int iLines = 0;

		cpLen = std::strlen(cp);
		if (cpLen != 0) {
			std::memcpy(cMsgA, cp, std::min<size_t>(20, cpLen));
			if (cpLen > 20) {
				int iRet = GetCharKind(cMsgA, 19);
				if (iRet == CharKind_HAN1) {
					cMsgA[20] = cp[20];
					cp++;
				}
			}
			cp += 20;
			iLines = 1;
		}

		cpLen = std::strlen(cp);
		if (cpLen != 0) {
			std::memcpy(cMsgB, cp, std::min<size_t>(20, cpLen));
			if (cpLen > 20) {
				int iRet = GetCharKind(cMsgB, 19);
				if (iRet == CharKind_HAN1) {
					cMsgB[20] = cp[20];
					cp++;
				}
			}
			cp += 20;
			iLines = 2;
		}

		cpLen = std::strlen(cp);
		if (cpLen != 0) {
			std::memcpy(cMsgC, cp, std::min<size_t>(20, cpLen));
			if (cpLen > 20) {
				int iRet = GetCharKind(cMsgC, 19);
				if (iRet == CharKind_HAN1) {
					cMsgC[20] = cp[20];
					cp++;
				}
			}
			cp += 20;
			iLines = 3;
		}

		// Compute text pixel width for centering
		int iSize = 0;
		for (int i = 0; i < 20; i++)
			if (cMsgA[i] != 0)
			{
				if (static_cast<unsigned char>(cMsgA[i]) >= 128) { iSize += 5; i++; }
				else iSize += 4;
			}

		int iFontID = GameFont::SprFont3_0 + params.iFontOffset;
		int iBaseY = sY - params.iStartOffsetY - iRise;

		// MagicCastName uses full-string width and red shadow style
		if (msg.m_eCategory == CFloatingText::Category::Notify &&
		    msg.m_eNotifyType == NotifyTextType::MagicCastName)
		{
			int iSize2 = 0;
			for (size_t i = 0; i < msg.m_szText.size(); i++)
				if (msg.m_szText[i] != 0)
				{
					if (static_cast<unsigned char>(msg.m_szText[i]) >= 128) { iSize2 += 5; i++; }
					else iSize2 += 4;
				}
			hb::shared::text::DrawText(iFontID, sX - iSize2, iBaseY, msg.m_szText.c_str(),
			                  hb::shared::text::TextStyle::WithTwoPointShadow(GameColors::Red4x).WithAdditive());
		}
		else
		{
			// Damage/LevelUp/EnemyKill: yellow sprite font with multi-line support
			auto style = bIsTrans
				? hb::shared::text::TextStyle::Color(GameColors::Yellow2x).WithAlpha(0.7f).WithAdditive()
				: hb::shared::text::TextStyle::WithTwoPointShadow(GameColors::Yellow2x).WithAdditive();

			switch (iLines) {
			case 1:
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY, cMsgA, style);
				break;
			case 2:
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY - 16, cMsgA, style);
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY, cMsgB, style);
				break;
			case 3:
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY - 32, cMsgA, style);
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY - 16, cMsgB, style);
				hb::shared::text::DrawText(iFontID, sX - iSize, iBaseY, cMsgC, style);
				break;
			}
		}
	}
	else
	{
		// hb::shared::render::Renderer text path (chat, skill change) — word-wrap handled by TextLib
		const char* pText = msg.m_szText.c_str();
		hb::shared::render::Color rgb = params.color;

		int iTextHeight = hb::shared::text::MeasureWrappedTextHeight(GameFont::Default, pText, 160);
		int iBoxHeight = params.iStartOffsetY + iTextHeight;
		int iBoxY = sY - iBoxHeight - iRise;

		// Shadow (+1,0) and (0,+1) offsets in black, then foreground
		auto black = hb::shared::text::TextStyle::Color(hb::shared::render::Color::Black());
		hb::shared::text::DrawTextWrapped(GameFont::Default, sX - 80 + 1, iBoxY, 160, iBoxHeight, pText, black, hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextWrapped(GameFont::Default, sX - 80, iBoxY + 1, 160, iBoxHeight, pText, black, hb::shared::text::Align::TopCenter);
		hb::shared::text::DrawTextWrapped(GameFont::Default, sX - 80, iBoxY, 160, iBoxHeight, pText, hb::shared::text::TextStyle::Color(rgb), hb::shared::text::Align::TopCenter);
	}
}
