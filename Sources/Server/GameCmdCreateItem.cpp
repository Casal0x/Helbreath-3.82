#include <windows.h>
#include "GameCmdCreateItem.h"
#include "Game.h"
#include "Item.h"
#include "Item/ItemEnums.h"
#include <cstring>
#include <cstdio>

bool GameCmdCreateItem::Execute(CGame* pGame, int iClientH, const char* pArgs)
{
	if (pGame->m_pClientList[iClientH] == nullptr)
		return true;

	int iItemID = 0, iAmount = 1;
	if (pArgs == nullptr || pArgs[0] == '\0' || sscanf(pArgs, "%d %d", &iItemID, &iAmount) < 1)
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Usage: /createitem <item_id> [amount]");
		return true;
	}

	if (iItemID < 0 || iItemID >= DEF_MAXITEMTYPES || pGame->m_pItemConfigList[iItemID] == nullptr)
	{
		pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Invalid item ID.");
		return true;
	}

	if (iAmount < 1) iAmount = 1;
	if (iAmount > 1000) iAmount = 1000;

	const char* pItemName = pGame->m_pItemConfigList[iItemID]->m_cName;
	bool bStackable = hb::item::IsStackableType(pGame->m_pItemConfigList[iItemID]->GetItemType());

	int iCreated = 0;

	if (bStackable)
	{
		CItem* pItem = new CItem();
		if (pGame->_bInitItemAttr(pItem, pItemName))
		{
			pItem->m_dwCount = iAmount;
			int iEraseReq = 0;
			if (pGame->_bAddClientItemList(iClientH, pItem, &iEraseReq))
			{
				pGame->SendItemNotifyMsg(iClientH, DEF_NOTIFY_ITEMOBTAINED, pItem, 0);
				iCreated = iAmount;
			}
			else
			{
				delete pItem;
				pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Inventory full.");
				return true;
			}
		}
		else
		{
			delete pItem;
			pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, "Failed to create item.");
			return true;
		}
	}
	else
	{
		for (int i = 0; i < iAmount; i++)
		{
			CItem* pItem = new CItem();
			if (pGame->_bInitItemAttr(pItem, pItemName))
			{
				int iEraseReq = 0;
				if (pGame->_bAddClientItemList(iClientH, pItem, &iEraseReq))
				{
					pGame->SendItemNotifyMsg(iClientH, DEF_NOTIFY_ITEMOBTAINED, pItem, 0);
					iCreated++;
				}
				else
				{
					delete pItem;
					break;
				}
			}
			else
			{
				delete pItem;
				break;
			}
		}
	}

	char buf[128];
	std::snprintf(buf, sizeof(buf), "Created %d x %s (ID: %d).", iCreated, pItemName, iItemID);
	pGame->SendNotifyMsg(0, iClientH, DEF_NOTIFY_NOTICEMSG, 0, 0, 0, buf);

	return true;
}
