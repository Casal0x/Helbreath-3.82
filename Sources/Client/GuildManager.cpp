// GuildManager.cpp: Handles client-side guild network messages.
// Extracted from NetworkMessages_Guild.cpp (Phase B4).

#include "GuildManager.h"
#include "Game.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

void GuildManager::UpdateLocationFlags(CGame* pGame, const char* cLocation)
{
	if (memcmp(cLocation, "aresden", 7) == 0)
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(cLocation, "arehunter", 9) == 0)
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = true;
	}
	else if (memcmp(cLocation, "elvine", 6) == 0)
	{
		pGame->m_pPlayer->m_bAresden = false;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(cLocation, "elvhunter", 9) == 0)
	{
		pGame->m_pPlayer->m_bAresden = false;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = true;
	}
	else
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = false;
		pGame->m_pPlayer->m_bHunter = true;
	}
}

void GuildManager::HandleCreateNewGuildResponse(char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case MsgType::Confirm:
		m_pGame->m_pPlayer->m_iGuildRank = 0;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 3;
		break;
	case MsgType::Reject:
		m_pGame->m_pPlayer->m_iGuildRank = -1;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 4;
		break;
	}
}

void GuildManager::HandleDisbandGuildResponse(char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case MsgType::Confirm:
		std::memset(m_pGame->m_pPlayer->m_cGuildName, 0, sizeof(m_pGame->m_pPlayer->m_cGuildName));
		m_pGame->m_pPlayer->m_iGuildRank = -1;
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 7;
		break;
	case MsgType::Reject:
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 8;
		break;
	}
}

void GuildManager::HandleGuildDisbanded(char* pData)
{
	char cName[24], cLocation[12];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyGuildDisbanded>(
		pData, sizeof(hb::net::PacketNotifyGuildDisbanded));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	std::memset(cLocation, 0, sizeof(cLocation));
	memcpy(cName, pkt->guild_name, 20);
	memcpy(cLocation, pkt->location, 10);
	CMisc::ReplaceString(cName, '_', ' ');
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 7);
	std::memset(m_pGame->m_pPlayer->m_cGuildName, 0, sizeof(m_pGame->m_pPlayer->m_cGuildName));
	m_pGame->m_pPlayer->m_iGuildRank = -1;
	std::memset(m_pGame->m_cLocation, 0, sizeof(m_pGame->m_cLocation));
	memcpy(m_pGame->m_cLocation, cLocation, 10);
	UpdateLocationFlags(m_pGame, m_pGame->m_cLocation);
}

void GuildManager::HandleNewGuildsMan(char* pData)
{
	char cName[12], cTxt[120];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyNewGuildsMan>(
		pData, sizeof(hb::net::PacketNotifyNewGuildsMan));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, 10);
	std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_NEW_GUILDMAN1, cName);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->ClearGuildNameList();
}

void GuildManager::HandleDismissGuildsMan(char* pData)
{
	char cName[12], cTxt[120];

	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyDismissGuildsMan>(
		pData, sizeof(hb::net::PacketNotifyDismissGuildsMan));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, 10);

	if (memcmp(m_pGame->m_pPlayer->m_cPlayerName, cName, 10) != 0) {
		std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_DISMISS_GUILDMAN1, cName);
		m_pGame->AddEventList(cTxt, 10);
	}
	m_pGame->ClearGuildNameList();
}

void GuildManager::HandleCannotJoinMoreGuildsMan(char* pData)
{
	char cName[12], cTxt[120];

	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCannotJoinMoreGuildsMan>(
		pData, sizeof(hb::net::PacketNotifyCannotJoinMoreGuildsMan));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, 10);

	std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN1, cName);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->AddEventList(NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN2, 10);
}

void GuildManager::HandleJoinGuildApprove(char* pData)
{
	char cName[21];
	short sRank;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyJoinGuildApprove>(
		pData, sizeof(hb::net::PacketNotifyJoinGuildApprove));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->guild_name, 20);
	sRank = pkt->rank;
	std::memset(m_pGame->m_pPlayer->m_cGuildName, 0, sizeof(m_pGame->m_pPlayer->m_cGuildName));
	std::snprintf(m_pGame->m_pPlayer->m_cGuildName, sizeof(m_pGame->m_pPlayer->m_cGuildName), "%s", cName);
	m_pGame->m_pPlayer->m_iGuildRank = sRank;
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 3);
}

void GuildManager::HandleJoinGuildReject(char* pData)
{
	char cName[21];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyJoinGuildReject>(
		pData, sizeof(hb::net::PacketNotifyJoinGuildReject));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->guild_name, 20);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 4);
}

void GuildManager::HandleDismissGuildApprove(char* pData)
{
	char cName[24], cLocation[12];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyDismissGuildApprove>(
		pData, sizeof(hb::net::PacketNotifyDismissGuildApprove));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	std::memset(cLocation, 0, sizeof(cLocation));
	memcpy(cName, pkt->guild_name, 20);
	memcpy(cLocation, pkt->location, 10);
	std::memset(m_pGame->m_pPlayer->m_cGuildName, 0, sizeof(m_pGame->m_pPlayer->m_cGuildName));
	m_pGame->m_pPlayer->m_iGuildRank = -1;
	std::memset(m_pGame->m_cLocation, 0, sizeof(m_pGame->m_cLocation));
	memcpy(m_pGame->m_cLocation, cLocation, 10);
	UpdateLocationFlags(m_pGame, m_pGame->m_cLocation);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 5);
}

void GuildManager::HandleDismissGuildReject(char* pData)
{
	char cName[21];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyDismissGuildReject>(
		pData, sizeof(hb::net::PacketNotifyDismissGuildReject));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->guild_name, 20);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 6);
}

void GuildManager::HandleQueryJoinGuildPermission(char* pData)
{
	char cName[12];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQueryJoinGuildPermission>(
		pData, sizeof(hb::net::PacketNotifyQueryJoinGuildPermission));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, 10);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 1);
}

void GuildManager::HandleQueryDismissGuildPermission(char* pData)
{
	char cName[12];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQueryDismissGuildPermission>(
		pData, sizeof(hb::net::PacketNotifyQueryDismissGuildPermission));
	if (!pkt) return;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, 10);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuildOperation, 0, 0, 0);
	m_pGame->_PutGuildOperationList(cName, 2);
}

void GuildManager::HandleReqGuildNameAnswer(char* pData)
{
	short sV1, sV2;
	char cTemp[256];
	int i;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyReqGuildNameAnswer>(
		pData, sizeof(hb::net::PacketNotifyReqGuildNameAnswer));
	if (!pkt) return;
	sV1 = static_cast<short>(pkt->guild_rank);
	sV2 = static_cast<short>(pkt->index);
	std::memset(cTemp, 0, sizeof(cTemp));
	memcpy(cTemp, pkt->guild_name, sizeof(pkt->guild_name));

	std::memset(m_pGame->m_stGuildName[sV2].cGuildName, 0, sizeof(m_pGame->m_stGuildName[sV2].cGuildName));
	std::snprintf(m_pGame->m_stGuildName[sV2].cGuildName, sizeof(m_pGame->m_stGuildName[sV2].cGuildName), "%s", cTemp);
	m_pGame->m_stGuildName[sV2].iGuildRank = sV1;
	for (i = 0; i < 20; i++) if (m_pGame->m_stGuildName[sV2].cGuildName[i] == '_') m_pGame->m_stGuildName[sV2].cGuildName[i] = ' ';
}

void GuildManager::HandleNoGuildMasterLevel(char* pData)
{
	m_pGame->AddEventList(NOTIFY_MSG_HANDLER59, 10);
}

void GuildManager::HandleSuccessBanGuildMan(char* pData)
{
	m_pGame->AddEventList(NOTIFY_MSG_HANDLER60, 10);
}

void GuildManager::HandleCannotBanGuildMan(char* pData)
{
	m_pGame->AddEventList(NOTIFY_MSG_HANDLER61, 10);
}
