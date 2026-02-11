#include "TeleportManager.h"
#include "Game.h"
#include "Packet/PacketResponse.h"
#include "Packet/PacketHelpers.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::client::net;

TeleportManager::TeleportManager() = default;
TeleportManager::~TeleportManager() = default;

TeleportManager& TeleportManager::Get()
{
	static TeleportManager instance;
	return instance;
}

void TeleportManager::SetGame(CGame* pGame)
{
	m_game = pGame;
}

void TeleportManager::Reset()
{
	m_is_requested = false;
	m_map_count = 0;
	m_list = {};
	m_loc_x = -1;
	m_loc_y = -1;
	std::memset(m_map_name, 0, sizeof(m_map_name));
}

void TeleportManager::HandleTeleportList(char* pData)
{
	int i;
#ifdef _DEBUG
	m_game->AddEventList("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		pData, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		pData + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_map_count = header->count;
	for (i = 0; i < m_map_count; i++)
	{
		m_list[i].iIndex = entries[i].index;
		std::memset(m_list[i].mapname, 0, sizeof(m_list[i].mapname));
		memcpy(m_list[i].mapname, entries[i].map_name, 10);
		m_list[i].iX = entries[i].x;
		m_list[i].iY = entries[i].y;
		m_list[i].iCost = entries[i].cost;
	}
}

void TeleportManager::HandleChargedTeleport(char* pData)
{
	short sRejectReason = 0;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseChargedTeleport>(
		pData, sizeof(hb::net::PacketResponseChargedTeleport));
	if (!pkt) return;
	sRejectReason = pkt->reason;

#ifdef _DEBUG
	m_game->AddEventList("charged teleport ?", 10);
#endif

	switch (sRejectReason) {
	case 1:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT1, 10);
		break;
	case 2:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT2, 10);
		break;
	case 3:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT3, 10);
		break;
	case 4:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT4, 10);
		break;
	case 5:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT5, 10);
		break;
	case 6:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT6, 10);
		break;
	default:
		m_game->AddEventList(RESPONSE_CHARGED_TELEPORT7, 10);
	}
}

void TeleportManager::HandleHeldenianTeleportList(char* pData)
{
	int i;
#ifdef _DEBUG
	m_game->AddEventList("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		pData, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		pData + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_map_count = header->count;
	for (i = 0; i < m_map_count; i++)
	{
		m_list[i].iIndex = entries[i].index;
		std::memset(m_list[i].mapname, 0, sizeof(m_list[i].mapname));
		memcpy(m_list[i].mapname, entries[i].map_name, 10);
		m_list[i].iX = entries[i].x;
		m_list[i].iY = entries[i].y;
		m_list[i].iCost = entries[i].cost;
	}
}

