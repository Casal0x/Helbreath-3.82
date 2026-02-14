#include "TeleportManager.h"
#include "Game.h"
#include "Packet/PacketResponse.h"
#include "Packet/PacketHelpers.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::client::net;

teleport_manager::teleport_manager() = default;
teleport_manager::~teleport_manager() = default;

teleport_manager& teleport_manager::get()
{
	static teleport_manager instance;
	return instance;
}

void teleport_manager::set_game(CGame* game)
{
	m_game = game;
}

void teleport_manager::reset()
{
	m_is_requested = false;
	m_map_count = 0;
	m_list = {};
	m_loc_x = -1;
	m_loc_y = -1;
}

void teleport_manager::handle_teleport_list(char* data)
{
	int i;
#ifdef _DEBUG
	m_game->add_event_list("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		data, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		data + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_map_count = header->count;
	if (m_map_count < 0) m_map_count = 0;
	if (m_map_count > static_cast<int>(m_list.size())) m_map_count = static_cast<int>(m_list.size());
	for (i = 0; i < m_map_count; i++)
	{
		m_list[i].index = entries[i].index;
		m_list[i].mapname.assign(entries[i].map_name, strnlen(entries[i].map_name, 10));
		m_list[i].x = entries[i].x;
		m_list[i].y = entries[i].y;
		m_list[i].cost = entries[i].cost;
	}
}

void teleport_manager::handle_charged_teleport(char* data)
{
	short reject_reason = 0;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseChargedTeleport>(
		data, sizeof(hb::net::PacketResponseChargedTeleport));
	if (!pkt) return;
	reject_reason = pkt->reason;

#ifdef _DEBUG
	m_game->add_event_list("charged teleport ?", 10);
#endif

	switch (reject_reason) {
	case 1:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT1, 10);
		break;
	case 2:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT2, 10);
		break;
	case 3:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT3, 10);
		break;
	case 4:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT4, 10);
		break;
	case 5:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT5, 10);
		break;
	case 6:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT6, 10);
		break;
	default:
		m_game->add_event_list(RESPONSE_CHARGED_TELEPORT7, 10);
	}
}

void teleport_manager::handle_heldenian_teleport_list(char* data)
{
	int i;
#ifdef _DEBUG
	m_game->add_event_list("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		data, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		data + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_map_count = header->count;
	if (m_map_count < 0) m_map_count = 0;
	if (m_map_count > static_cast<int>(m_list.size())) m_map_count = static_cast<int>(m_list.size());
	for (i = 0; i < m_map_count; i++)
	{
		m_list[i].index = entries[i].index;
		m_list[i].mapname.assign(entries[i].map_name, strnlen(entries[i].map_name, 10));
		m_list[i].x = entries[i].x;
		m_list[i].y = entries[i].y;
		m_list[i].cost = entries[i].cost;
	}
}

