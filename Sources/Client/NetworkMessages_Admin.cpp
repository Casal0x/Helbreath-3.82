#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "DialogBoxIDs.h"
#include <cstdio>
#include <cstring>
#include <windows.h>

namespace NetworkMessageHandlers {

void HandleCrashHandler(CGame* pGame, char* pData)
{
	// 0x0BEF: Crash or closes the client? (Calls SE entry !)
	// I'm not sure at all of this function's result, so let's quit game...
	// Empty handler - just acknowledge the message
}

void HandleIpAccountInfo(CGame* pGame, char* pData)
{
	char cTemp[256];
	std::memset(cTemp, 0, sizeof(cTemp));
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyIpAccountInfo>(
		pData, sizeof(hb::net::PacketNotifyIpAccountInfo));
	if (!pkt) return;
	strcpy(cTemp, pkt->text);
	pGame->AddEventList(cTemp);
}

void HandleRewardGold(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyRewardGold>(
		pData, sizeof(hb::net::PacketNotifyRewardGold));
	if (!pkt) return;
	pGame->m_pPlayer->m_iRewardGold = pkt->gold;
}

void HandleServerShutdown(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyServerShutdown>(
		pData, sizeof(hb::net::PacketNotifyServerShutdown));
	if (!pkt) return;
	if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::Noticement) == false)
		pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::Noticement, pkt->mode, 0, 0);
	else pGame->m_dialogBoxManager.Info(DialogBoxId::Noticement).cMode = pkt->mode;
	pGame->PlaySound('E', 27, 0);
}

} // namespace NetworkMessageHandlers
