#pragma once

class CGame;

class InventoryManager
{
public:
	static InventoryManager& Get();

	void SetGame(CGame* pGame);

	// Item ordering
	void SetItemOrder(int cWhere, int cItemID);

	// Weight/count queries
	int CalcTotalWeight();
	int GetTotalItemCount();
	int GetBankItemCount();

	// Item operations
	void EraseItem(int cItemID);
	bool CheckItemOperationEnabled(int cItemID);

	// Equipment
	void UnequipSlot(int cEquipPos);
	void EquipItem(int cItemID);

private:
	InventoryManager() = default;
	CGame* m_game = nullptr;
};
