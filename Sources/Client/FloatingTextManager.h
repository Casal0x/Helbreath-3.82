#pragma once

#include <array>
#include <memory>
#include <cstdint>
#include "FloatingText.h"
#include "GameConstants.h"

class CMapData;
namespace hb::shared::render { class IRenderer; }

class CFloatingTextManager {
public:
	// Add messages - find slot, bind to tile, return index or 0
	int AddChatText(const char* pMsg, uint32_t dwTime, int iObjectID,
	                CMapData* pMapData, short sX, short sY);
	int AddDamageText(DamageTextType eType, const char* pMsg, uint32_t dwTime,
	                  int iObjectID, CMapData* pMapData);
	int AddNotifyText(NotifyTextType eType, const char* pMsg, uint32_t dwTime,
	                  int iObjectID, CMapData* pMapData);

	// Damage factory (replaces CreateDamageMsg logic)
	int AddDamageFromValue(short sDamage, bool bLastHit, uint32_t dwTime,
	                       int iObjectID, CMapData* pMapData);

	void RemoveByObjectID(int iObjectID);
	void ReleaseExpired(uint32_t dwTime);
	void Clear(int iIndex);
	void ClearAll();

	// Rendering
	void DrawAll(short sMinX, short sMinY, short sMaxX, short sMaxY,
	             uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer);
	void DrawSingle(int iIndex, short sX, short sY,
	                uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer);

	// Position updates from entity renderers
	CFloatingText* Get(int iIndex);
	void UpdatePosition(int iIndex, short sX, short sY);

	// Check if slot is occupied and matches objectID
	bool IsValid(int iIndex, int iObjectID) const;
	bool IsOccupied(int iIndex) const;

private:
	static constexpr int MaxMessages = game_limits::max_chat_msgs;

	int FindFreeSlot() const;
	int BindToTile(int iIndex, int iObjectID, CMapData* pMapData, short sX, short sY);
	void DrawMessage(const CFloatingText& msg, short sX, short sY,
	                 uint32_t dwCurTime, hb::shared::render::IRenderer* pRenderer);

	std::array<std::unique_ptr<CFloatingText>, MaxMessages> m_messages;
};
