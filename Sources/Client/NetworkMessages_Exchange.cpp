#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <cstring>
#include <cstdio>

namespace NetworkMessageHandlers {
	void HandleExchangeItemComplete(CGame* game, char* data)
	{
		game->add_event_list(NOTIFYMSG_EXCHANGEITEM_COMPLETE1, 10);
		game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Exchange);
		//Snoopy: MultiTrade
		game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ConfirmExchange);
		game->play_game_sound('E', 23, 5);
	}

	void HandleCancelExchangeItem(CGame* game, char* data)
	{
		game->play_game_sound('E', 24, 5);
		game->add_event_list(NOTIFYMSG_CANCEL_EXCHANGEITEM1, 10);
		game->add_event_list(NOTIFYMSG_CANCEL_EXCHANGEITEM2, 10);

		// Explicitly clear item disabled flags before disabling dialogs.
		// disable_dialog_box(Exchange) also clears these, but if exchange info
		// was partially cleared by item removal, flags could get stuck.
		for (int i = 0; i < 4; i++)
		{
			int item_id = game->m_dialog_box_exchange_info[i].item_id;
			if (item_id >= 0 && item_id < hb::shared::limits::MaxItems)
				game->m_is_item_disabled[item_id] = false;
		}

		//Snoopy: MultiTrade
		game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::ConfirmExchange);
		game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Exchange);
	}
} // namespace NetworkMessageHandlers
