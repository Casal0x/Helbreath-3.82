#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <format>
#include <string>

using namespace hb::shared::net;
namespace NetworkMessageHandlers {

void HandleParty(CGame* game, char* data)
{
	int i;
	int v1, v2, v3, v4;
	char txt[120];

	const auto* basic = hb::net::PacketCast<hb::net::PacketNotifyPartyBasic>(
		data, sizeof(hb::net::PacketNotifyPartyBasic));
	
	// If it's a basic packet, extract values. If not, v1 will be 0 and handled or overwritten later.
	// But mostly this handler handles multiple packet types by casting differently based on type.
	// We should be careful. The original code unconditionally casts to PartyBasic first.
	// PacketNotifyPartyBasic is: header, type(16), v2(16), v3(16), v4(16).
	// This seems to be a common header for party messages.
	
	if (basic) {
		v1 = basic->type;
		v2 = basic->v2;
		v3 = basic->v3;
		v4 = basic->v4;
	} else {
		// Should not happen if dispatch logic is correct and packet size is sufficient
		return; 
	}

	switch (v1) {
	case 1: //
		switch (v2) {
		case 0:
			game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
			game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 9;
			break;

		case 1:
			game->m_party_status = 1;
			game->m_total_party_member = 0;
			game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
			game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 8;
			game->send_command(MsgId::CommandCommon, CommonType::RequestJoinParty, 0, 2, 0, 0, game->m_mc_name.c_str());
			break;
		}
		break;

	case 2: //
		game->m_party_status = 0;
		game->m_total_party_member = 0;
		game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
		game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 10;
		break;

	case 4:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPartyName>(
			data, sizeof(hb::net::PacketNotifyPartyName));
		if (!pkt) return;
		std::memset(txt, 0, sizeof(txt));
		memcpy(txt, pkt->name, sizeof(pkt->name));

		switch (v2) {
		case 0: //
			game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
			game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 9;
			break;

		case 1: //
			if (strcmp(txt, game->m_player->m_player_name.c_str()) == 0) {
				game->m_party_status = 2;
				game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
				game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 8;
			}
			else {
				std::string partyMsgBuf;
				partyMsgBuf = std::format(NOTIFY_MSG_HANDLER1, txt);
				game->add_event_list(partyMsgBuf.c_str(), 10);
			}

			game->m_total_party_member++;
			for (i = 0; i < hb::shared::limits::MaxPartyMembers; i++)
				if (game->m_party_member_name_list[i].name.size() == 0) {
					game->m_party_member_name_list[i].name.assign(txt, strnlen(txt, hb::shared::limits::CharNameLen));
					break; // Replaced goto with break
				}
			break;

		case 2: //
			break;
		}
	}
	break;

	case 5: //
		game->m_total_party_member = 0;

		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPartyList>(
				data, sizeof(hb::net::PacketNotifyPartyList));
			if (!pkt) return;
			const char* names = pkt->names;
			game->m_total_party_member = (std::min)(static_cast<int>(pkt->count), hb::shared::limits::MaxPartyMembers);
			for (i = 1; i <= pkt->count; i++) {
				game->m_party_member_name_list[i - 1].name.assign(names, strnlen(names, hb::shared::limits::CharNameLen));
				names += hb::shared::limits::CharNameLen;
			}
		}
		break;

	case 6:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPartyName>(
			data, sizeof(hb::net::PacketNotifyPartyName));
		if (!pkt) return;
		std::memset(txt, 0, sizeof(txt));
		memcpy(txt, pkt->name, sizeof(pkt->name));

		switch (v2) {
		case 0: //
			game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
			game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 7;
			break;

		case 1: //
			if (strcmp(txt, game->m_player->m_player_name.c_str()) == 0) {
				game->m_party_status = 0;
				game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
				game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 6;
			}
			else {
				std::string partyMsgBuf;
				partyMsgBuf = std::format(NOTIFY_MSG_HANDLER2, txt);
				game->add_event_list(partyMsgBuf.c_str(), 10);
			}
			for (i = 0; i < hb::shared::limits::MaxPartyMembers; i++)
				if (game->m_party_member_name_list[i].name == txt) {
					game->m_total_party_member--;
					break; // Replaced goto with break
				}
			break;
		}
	}
	break;

	case 7: //
		game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
		game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 9;
		break;

	case 8: //
		game->m_party_status = 0;
		game->m_total_party_member = 0;
		break;
	}
}

void HandleQueryJoinParty(CGame* game, char* data)
{
	game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Party, 0, 0, 0);
	game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 1;
	std::memset(game->m_dialog_box_manager.Info(DialogBoxId::Party).m_str, 0, sizeof(game->m_dialog_box_manager.Info(DialogBoxId::Party).m_str));
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQueryJoinParty>(
		data, sizeof(hb::net::PacketNotifyQueryJoinParty));
	if (!pkt) return;
	std::snprintf(game->m_dialog_box_manager.Info(DialogBoxId::Party).m_str, sizeof(game->m_dialog_box_manager.Info(DialogBoxId::Party).m_str), "%s", pkt->name);
}

void HandleResponseCreateNewParty(CGame* game, char* data)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyResponseCreateNewParty>(
		data, sizeof(hb::net::PacketNotifyResponseCreateNewParty));
	if (!pkt) return;
	if ((bool)pkt->result == true)
	{
		game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 2;
	}
	else
	{
		game->m_dialog_box_manager.Info(DialogBoxId::Party).m_mode = 3;
	}
}

} // namespace NetworkMessageHandlers
