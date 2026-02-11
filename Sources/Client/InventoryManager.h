#pragma once

class CGame;

class InventoryManager
{
public:
	static InventoryManager& Get();

	void SetGame(CGame* pGame);

	// Item ordering
	void SetItemOrder(char cWhere, char cItemID);

	// Weight/count queries
	int CalcTotalWeight();
	int GetTotalItemCount();
	int GetBankItemCount();

	// Item operations
	void EraseItem(char cItemID);
	bool CheckItemOperationEnabled(char cItemID);

	// Equipment
	void UnequipSlot(char cEquipPos);
	void EquipItem(char cItemID);

private:
	InventoryManager() = default;
	CGame* m_game = nullptr;
};
