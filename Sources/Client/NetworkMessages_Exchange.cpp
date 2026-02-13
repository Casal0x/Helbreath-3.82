#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <cstring>
#include <cstdio>

namespace NetworkMessageHandlers {
	void HandleExchangeItemComplete(CGame* pGame, char* pData)
	{
		pGame->AddEventList(NOTIFYMSG_EXCHANGEITEM_COMPLETE1, 10);
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Exchange);
		//Snoopy: MultiTrade
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ConfirmExchange);
		pGame->PlayGameSound('E', 23, 5);
	}

	void HandleCancelExchangeItem(CGame* pGame, char* pData)
	{
		pGame->PlayGameSound('E', 24, 5);
		pGame->AddEventList(NOTIFYMSG_CANCEL_EXCHANGEITEM1, 10);
		pGame->AddEventList(NOTIFYMSG_CANCEL_EXCHANGEITEM2, 10);

		// Explicitly clear item disabled flags before disabling dialogs.
		// DisableDialogBox(Exchange) also clears these, but if exchange info
		// was partially cleared by item removal, flags could get stuck.
		for (int i = 0; i < 4; i++)
		{
			int item_id = pGame->m_stDialogBoxExchangeInfo[i].sItemID;
			if (item_id >= 0 && item_id < hb::shared::limits::MaxItems)
				pGame->m_bIsItemDisabled[item_id] = false;
		}

		//Snoopy: MultiTrade
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ConfirmExchange);
		pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Exchange);
	}
} // namespace NetworkMessageHandlers
