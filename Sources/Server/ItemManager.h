#pragma once

#include <cstdint>
#include <map>

struct DropTable;
class CGame;
class CItem;
struct ShopData;

class ItemManager
{
public:
	ItemManager() = default;
	~ItemManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Item config / init
	bool bSendClientItemConfigs(int iClientH);
	const DropTable* GetDropTable(int id) const;
	void _ClearItemConfigList();
	bool _bInitItemAttr(CItem* pItem, const char* pItemName);
	bool _bInitItemAttr(CItem* pItem, int iItemID);
	void ReloadItemConfigs();

	// Item attribute generation
	void _AdjustRareItemValue(CItem* pItem);
	bool GenerateItemAttributes(CItem* pItem);
	int RollAttributeValue();

	// Inventory management
	bool bAddItem(int iClientH, CItem* pItem, char cMode);
	bool _bAddClientItemList(int iClientH, CItem* pItem, int* pDelReq);
	int _bAddClientBulkItemList(int iClientH, const char* pItemName, int iAmount);
	void ReleaseItemHandler(int iClientH, short sItemIndex, bool bNotice);
	int SetItemCount(int iClientH, int iItemIndex, uint32_t dwCount);
	int SetItemCountByID(int iClientH, short sItemID, uint32_t dwCount);
	uint32_t dwGetItemCountByID(int iClientH, short sItemID);
	int _iGetItemSpaceLeft(int iClientH);
	void _SetItemPos(int iClientH, char* pData);
	int iGetItemWeight(CItem* pItem, int iCount);
	bool bCopyItemContents(CItem* pOriginal, CItem* pCopy);
	bool _bCheckItemReceiveCondition(int iClientH, CItem* pItem);
	int SendItemNotifyMsg(int iClientH, uint16_t wMsgType, CItem* pItem, int iV1);

	// Item use / effects
	void UseItemHandler(int iClientH, short sItemIndex, short dX, short dY, short sDestItemID);
	void ItemDepleteHandler(int iClientH, short sItemIndex, bool bIsUseItemResult);
	bool _bDepleteDestTypeItemUseEffect(int iClientH, int dX, int dY, short sItemIndex, short sDestItemID);
	int iCalculateUseSkillItemEffect(int iOwnerH, char cOwnerType, char cOwnerSkill, int iSkillNum, char cMapIndex, int dX, int dY);
	bool bPlantSeedBag(int iMapIndex, int dX, int dY, int iItemEffectValue1, int iItemEffectValue2, int iClientH);

	// Equipment
	bool bEquipItemHandler(int iClientH, short sItemIndex, bool bNotify = true);
	void CalcTotalItemEffect(int iClientH, int iEquipItemID, bool bNotify = true);
	void CheckUniqueItemEquipment(int iClientH);
	bool bCheckAndConvertPlusWeaponItem(int iClientH, int iItemIndex);
	char _cCheckHeroItemEquipped(int iClientH);
	int _iGetArrowItemIndex(int iClientH);
	void CalculateSSN_ItemIndex(int iClientH, short sWeaponIndex, int iValue);

	// Drop / pickup
	void DropItemHandler(int iClientH, short sItemIndex, int iAmount, const char* pItemName, bool bByPlayer = true);
	int iClientMotion_GetItem_Handler(int iClientH, short sX, short sY, char cDir);

	// Give / exchange
	void GiveItemHandler(int iClientH, short sItemIndex, int iAmount, short dX, short dY, uint16_t wObjectID, const char* pItemName);
	void ExchangeItemHandler(int iClientH, short sItemIndex, int iAmount, short dX, short dY, uint16_t wObjectID, const char* pItemName);
	void SetExchangeItem(int iClientH, int iItemIndex, int iAmount);
	void ConfirmExchangeItem(int iClientH);
	void CancelExchangeItem(int iClientH);
	void _ClearExchangeStatus(int iToH);

	// Bank
	bool bSetItemToBankItem(int iClientH, CItem* pItem);
	bool bSetItemToBankItem(int iClientH, short sItemIndex);
	void RequestRetrieveItemHandler(int iClientH, char* pData);

	// Shop / purchase / sell
	void RequestPurchaseItemHandler(int iClientH, const char* pItemName, int iNum, int iItemId = 0);
	void RequestSellItemListHandler(int iClientH, char* pData);
	void ReqSellItemHandler(int iClientH, char cItemID, char cSellToWhom, int iNum, const char* pItemName);
	void ReqSellItemConfirmHandler(int iClientH, char cItemID, int iNum, const char* pString);

	// Repair
	void ReqRepairItemHandler(int iClientH, char cItemID, char cRepairWhom, const char* pString);
	void ReqRepairItemCofirmHandler(int iClientH, char cItemID, const char* pString);
	void RequestRepairAllItemsHandler(int iClientH);
	void RequestRepairAllItemsDeleteHandler(int iClientH, int index);
	void RequestRepairAllItemsConfirmHandler(int iClientH);

	// Crafting
	void BuildItemHandler(int iClientH, char* pData);

	// Upgrade
	bool bCheckIsItemUpgradeSuccess(int iClientH, int iItemIndex, int iSomH, bool bBonus = false);
	void RequestItemUpgradeHandler(int iClientH, int iItemIndex);

	// Hero / special
	void GetHeroMantleHandler(int iClientH, int iItemID, const char* pString);

	// Slate
	void ReqCreateSlateHandler(int iClientH, char* pData);
	void SetSlateFlag(int iClientH, short sType, bool bFlag);

	// Logging
	bool _bItemLog(int iAction, int iClientH, char* cName, CItem* pItem);
	bool _bItemLog(int iAction, int iGiveH, int iRecvH, CItem* pItem, bool bForceItemLog = false);
	bool _bCheckGoodItem(CItem* pItem);

private:
	CGame* m_pGame = nullptr;
};
