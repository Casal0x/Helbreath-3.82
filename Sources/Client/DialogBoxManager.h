#pragma once

#include "DialogBoxIDs.h"
#include "DialogBoxInfo.h"
#include <memory>

class CGame;
class IDialogBox;

class DialogBoxManager
{
public:
	explicit DialogBoxManager(CGame* game = nullptr);
	~DialogBoxManager() = default;  // unique_ptr handles cleanup automatically

	void Initialize(CGame* game);
	void InitDefaults();
	void InitializeDialogBoxes();
	void RegisterDialogBox(std::unique_ptr<IDialogBox> pDialogBox);
	IDialogBox* GetDialogBox(DialogBoxId::Type id) const;
	IDialogBox* GetDialogBox(int iBoxID) const;
	void UpdateDialogBoxs();
	void DrawDialogBoxs(short msX, short msY, short msZ, char cLB);
	void EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char* pString = nullptr);
	void EnableDialogBox(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString = nullptr);
	void DisableDialogBox(int iBoxID);
	void DisableDialogBox(DialogBoxId::Type id);
	void ToggleDialogBox(DialogBoxId::Type id, int cType = 0, int sV1 = 0, int sV2 = 0, char* pString = nullptr);
	int iGetTopDialogBoxIndex() const;

	void DrawAll(short msX, short msY, short msZ, char cLB);
	bool HandleClick(short msX, short msY);
	bool HandleDoubleClick(short msX, short msY);
	bool HandlePress(int iDlgID, short msX, short msY);
	bool HandleItemDrop(int iDlgID, short msX, short msY);
	bool HandleDraggingItemRelease(short msX, short msY);
	void Enable(DialogBoxId::Type id, int cType, int sV1, int sV2, char* pString = nullptr);
	void Disable(DialogBoxId::Type id);
	void Toggle(DialogBoxId::Type id, int cType = 0, int sV1 = 0, int sV2 = 0, char* pString = nullptr);
	int GetTopId() const;
	bool IsEnabled(DialogBoxId::Type id) const;
	bool IsEnabled(int iBoxID) const;
	void SetEnabled(DialogBoxId::Type id, bool enabled);
	void SetEnabled(int iBoxID, bool enabled);
	DialogBoxInfo& Info(DialogBoxId::Type id);
	const DialogBoxInfo& Info(DialogBoxId::Type id) const;
	DialogBoxInfo& Info(int iBoxID);
	const DialogBoxInfo& Info(int iBoxID) const;
	char OrderAt(int index) const;
	void SetOrderAt(int index, char value);

private:
	CGame* m_game;
	DialogBoxInfo m_info[61]{};
	char m_order[61]{};
	bool m_enabled[61]{};
	std::unique_ptr<IDialogBox> m_pDialogBoxes[61];
};
