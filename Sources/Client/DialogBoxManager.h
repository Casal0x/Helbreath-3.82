#pragma once

#include "DialogBoxIDs.h"
#include "DialogBoxInfo.h"
#include "IDialogBox.h"
#include <cstdint>
#include <memory>

class CGame;

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
	PressResult HandlePress(int iDlgID, short msX, short msY);
	bool HandleItemDrop(int iDlgID, short msX, short msY);
	bool HandleDraggingItemRelease(short msX, short msY);

	// Mouse down handling - replaces _iCheckDlgBoxFocus from Game.cpp
	// Returns: 1 = dialog hit, 0 = no dialog hit, -1 = scroll region claimed
	int HandleMouseDown(short msX, short msY);

	// Right-click to close dialogs
	// Returns: true if a dialog was under the mouse (and potentially closed)
	bool HandleRightClick(short msX, short msY, uint32_t dwTime);
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
	uint8_t OrderAt(int index) const;
	void SetOrderAt(int index, uint8_t value);

private:
	CGame* m_game;
	DialogBoxInfo m_info[61]{};
	uint8_t m_order[61]{};
	bool m_enabled[61]{};
	std::unique_ptr<IDialogBox> m_pDialogBoxes[61];
	uint32_t m_dwDialogCloseTime = 0;
};
