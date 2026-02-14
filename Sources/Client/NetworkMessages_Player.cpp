#include "Game.h"
#include "ChatManager.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <cstdio>
#include <cstring>
#include <string_view>
#include <cmath>
#include <format>
#include <string>

namespace NetworkMessageHandlers {
	void HandleCharisma(CGame* game, char* data)
	{
		int  prev_char;
		std::string txt;

		prev_char = game->m_player->m_charisma;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCharisma>(
			data, sizeof(hb::net::PacketNotifyCharisma));
		if (!pkt) return;
		game->m_player->m_charisma = static_cast<int>(pkt->charisma);

		if (game->m_player->m_charisma > prev_char)
		{
			txt = std::format(NOTIFYMSG_CHARISMA_UP, game->m_player->m_charisma - prev_char);
			game->add_event_list(txt.c_str(), 10);
			game->play_game_sound('E', 21, 0);
		}
		else
		{
			txt = std::format(NOTIFYMSG_CHARISMA_DOWN, prev_char - game->m_player->m_charisma);
			game->add_event_list(txt.c_str(), 10);
		}
	}

	void HandleHunger(CGame* game, char* data)
	{
		char h_lv;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyHunger>(
			data, sizeof(hb::net::PacketNotifyHunger));
		if (!pkt) return;
		game->m_player->m_hunger_status = pkt->hunger;

		h_lv = game->m_player->m_hunger_status;
		if ((h_lv <= 40) && (h_lv > 30)) game->add_event_list(NOTIFYMSG_HUNGER1, 10);
		if ((h_lv <= 25) && (h_lv > 20)) game->add_event_list(NOTIFYMSG_HUNGER2, 10);
		if ((h_lv <= 20) && (h_lv > 15)) game->add_event_list(NOTIFYMSG_HUNGER3, 10);
		if ((h_lv <= 15) && (h_lv > 10)) game->add_event_list(NOTIFYMSG_HUNGER4, 10);
		if ((h_lv <= 10) && (h_lv >= 0)) game->add_event_list(NOTIFYMSG_HUNGER5, 10);
	}

	void HandlePlayerProfile(CGame* game, char* data)
	{
		std::string temp;
		int i;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPlayerProfile>(
			data, sizeof(hb::net::PacketNotifyPlayerProfile));
		if (!pkt) return;
		temp = pkt->text;
		for (i = 0; i < static_cast<int>(temp.size()); i++)
			if (temp[i] == '_') temp[i] = ' ';
		game->add_event_list(temp.c_str(), 10);
	}

	void HandlePlayerStatus(CGame* game, bool on_game, char* data)
	{
		char name[12]{}, map_name[12]{};
		std::string txt;
		uint16_t dx = 1, dy = 1;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPlayerStatus>(
			data, sizeof(hb::net::PacketNotifyPlayerStatus));
		if (!pkt) return;
		memcpy(name, pkt->name, sizeof(pkt->name));
		memcpy(map_name, pkt->map_name, sizeof(pkt->map_name));
		dx = pkt->x;
		dy = pkt->y;
		if (on_game == true) {
			if (map_name[0] == 0)
				txt = std::format(NOTIFYMSG_PLAYER_STATUS1, name);
			else txt = std::format(NOTIFYMSG_PLAYER_STATUS2, name, map_name, dx, dy);
		}
		else txt = std::format(NOTIFYMSG_PLAYER_STATUS3, name);
		game->add_event_list(txt.c_str(), 10);
	}

	void HandleWhisperMode(CGame* game, bool active, char* data)
	{
		char name[12]{};
		std::string txt;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyWhisperMode>(
			data, sizeof(hb::net::PacketNotifyWhisperMode));
		if (!pkt) return;
		memcpy(name, pkt->name, sizeof(pkt->name));
		if (active == true)
		{
			txt = std::format(NOTIFYMSG_WHISPERMODE1, name);
			ChatManager::get().add_whisper_target(name);
		}
		else txt = NOTIFYMSG_WHISPERMODE2;

		game->add_event_list(txt.c_str(), 10);
	}

	void HandlePlayerShutUp(CGame* game, char* data)
	{
		char name[12]{};
		std::string txt;
		uint16_t time;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyPlayerShutUp>(
			data, sizeof(hb::net::PacketNotifyPlayerShutUp));
		if (!pkt) return;
		time = pkt->time;
		memcpy(name, pkt->name, sizeof(pkt->name));
		if (game->m_player->m_player_name == std::string_view(name, strnlen(name, hb::shared::limits::CharNameLen)))
			txt = std::format(NOTIFYMSG_PLAYER_SHUTUP1, time);
		else txt = std::format(NOTIFYMSG_PLAYER_SHUTUP2, name, time);

		game->add_event_list(txt.c_str(), 10);
	}

	void HandleRatingPlayer(CGame* game, char* data)
	{
		char name[12]{};
		std::string txt;
		uint16_t value;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyRatingPlayer>(
			data, sizeof(hb::net::PacketNotifyRatingPlayer));
		if (!pkt) return;
		value = pkt->result;
		memcpy(name, pkt->name, sizeof(pkt->name));
		if (game->m_player->m_player_name == std::string_view(name, strnlen(name, hb::shared::limits::CharNameLen)))
		{
			if (value == 1)
			{
				txt = NOTIFYMSG_RATING_PLAYER1;
				game->play_game_sound('E', 23, 0);
			}
		}
		else
		{
			if (value == 1)
				txt = std::format(NOTIFYMSG_RATING_PLAYER2, name);
			else txt = std::format(NOTIFYMSG_RATING_PLAYER3, name);
		}
		if (!txt.empty()) game->add_event_list(txt.c_str(), 10);
	}

	void HandleCannotRating(CGame* game, char* data)
	{
		std::string txt;

		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyCannotRating>(
			data, sizeof(hb::net::PacketNotifyCannotRating));
		if (!pkt) return;
		const auto time = pkt->time_left;

		if (time == 0) txt = NOTIFYMSG_CANNOT_RATING1;
		else txt = std::format(NOTIFYMSG_CANNOT_RATING2, time * 3);
		game->add_event_list(txt.c_str(), 10);
	}
}
