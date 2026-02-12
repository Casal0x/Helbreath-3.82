#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <cstdio>
#include <cstring>
#include "DialogBoxIDs.h"
#include <format>
#include <string>

namespace NetworkMessageHandlers {
	void HandleSpawnEvent(CGame* pGame, char* pData)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifySpawnEvent>(
			pData, sizeof(hb::net::PacketNotifySpawnEvent));
		if (!pkt) return;
		pGame->m_sMonsterID = pkt->monster_id;
		pGame->m_sEventX = pkt->x;
		pGame->m_sEventY = pkt->y;
		pGame->m_dwMonsterEventTime = pGame->m_dwCurTime;
	}

	void HandleMonsterCount(CGame* pGame, char* pData)
	{
		std::string cTxt;
		int iCount;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyMonsterCount>(
			pData, sizeof(hb::net::PacketNotifyMonsterCount));
		if (!pkt) return;
		iCount = pkt->count;
		cTxt = std::format("Rest Monster :{}", iCount);
		pGame->AddEventList(cTxt.c_str(), 10);
	}

	void HandleResurrectPlayer(CGame* pGame, char* pData)
	{
		pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::Resurrect, 0, 0, 0);
	}
} // namespace NetworkMessageHandlers
