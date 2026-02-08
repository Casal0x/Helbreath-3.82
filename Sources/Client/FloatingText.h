#pragma once

#include <string>
#include <cstdint>
#include "FloatingTextTypes.h"

class CFloatingText {
public:
	enum class Category : uint8_t { Chat, Damage, Notify };

	// Chat constructor
	CFloatingText(ChatTextType eType, const char* pMsg, uint32_t dwTime)
		: m_eCategory(Category::Chat)
		, m_eChatType(eType)
		, m_eDamageType{}
		, m_eNotifyType{}
		, m_szText(pMsg ? pMsg : "")
		, m_sX(0), m_sY(0)
		, m_dwTime(dwTime)
		, m_iObjectID(-1)
	{
	}

	// Damage constructor
	CFloatingText(DamageTextType eType, const char* pMsg, uint32_t dwTime)
		: m_eCategory(Category::Damage)
		, m_eChatType{}
		, m_eDamageType(eType)
		, m_eNotifyType{}
		, m_szText(pMsg ? pMsg : "")
		, m_sX(0), m_sY(0)
		, m_dwTime(dwTime)
		, m_iObjectID(-1)
	{
	}

	// Notify constructor
	CFloatingText(NotifyTextType eType, const char* pMsg, uint32_t dwTime)
		: m_eCategory(Category::Notify)
		, m_eChatType{}
		, m_eDamageType{}
		, m_eNotifyType(eType)
		, m_szText(pMsg ? pMsg : "")
		, m_sX(0), m_sY(0)
		, m_dwTime(dwTime)
		, m_iObjectID(-1)
	{
	}

	const AnimParams& GetParams() const
	{
		switch (m_eCategory) {
		case Category::Chat:   return FloatingTextParams::Chat[static_cast<int>(m_eChatType)];
		case Category::Damage: return FloatingTextParams::Damage[static_cast<int>(m_eDamageType)];
		case Category::Notify: return FloatingTextParams::Notify[static_cast<int>(m_eNotifyType)];
		}
		return FloatingTextParams::Chat[0]; // fallback
	}

	bool UsesSpriteFont() const { return GetParams().bUseSpriteFont; }

	Category       m_eCategory;
	ChatTextType   m_eChatType;
	DamageTextType m_eDamageType;
	NotifyTextType m_eNotifyType;

	std::string    m_szText;
	short          m_sX, m_sY;
	uint32_t       m_dwTime;
	int            m_iObjectID;
};
