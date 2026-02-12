#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <cstdio>
#include <cstring>
#include <format>
#include <string>

namespace NetworkMessageHandlers {

void HandleApocGateStart(CGame* pGame, char* pData)
{
	pGame->SetTopMsg("The portal to the Apocalypse is opened.", 10);
}

void HandleApocGateEnd(CGame* pGame, char* pData)
{
	pGame->SetTopMsg("The portal to the Apocalypse is closed.", 10);
}

void HandleApocGateOpen(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyApocGateOpen>(
		pData, sizeof(hb::net::PacketNotifyApocGateOpen));
	if (!pkt) return;
	pGame->m_iGatePositX = pkt->gate_x;
	pGame->m_iGatePositY = pkt->gate_y;
	pGame->m_cGateMapName.assign(pkt->map_name, strnlen(pkt->map_name, sizeof(pkt->map_name)));
}

void HandleApocGateClose(CGame* pGame, char* pData)
{
	pGame->m_iGatePositX = pGame->m_iGatePositY = -1;
}

void HandleApocForceRecall(CGame* pGame, char* pData)
{
	pGame->AddEventList("You are recalled by force, because the Apocalypse is started.", 10);
}

void HandleAbaddonKilled(CGame* pGame, char* pData)
{
	std::string cTxt;
	char cKiller[21]{};
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyAbaddonKilled>(
		pData, sizeof(hb::net::PacketNotifyAbaddonKilled));
	if (!pkt) return;
	memcpy(cKiller, pkt->killer_name, sizeof(pkt->killer_name));
	
	cTxt = std::format("Abaddon is destroyed by {}", cKiller);
	pGame->AddEventList(cTxt.c_str(), 10);
}

} // namespace NetworkMessageHandlers
