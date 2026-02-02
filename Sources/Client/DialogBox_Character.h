#pragma once
#include "IDialogBox.h"
#include "Item/ItemEnums.h"

struct EquipSlotLayout
{
	hb::item::EquipPos equipPos;
	int offsetX;
	int offsetY;
	bool useWeaponColors;
};

class DialogBox_Character : public IDialogBox
{
public:
	DialogBox_Character(CGame* pGame);
	~DialogBox_Character() override = default;

	void OnDraw(short msX, short msY, short msZ, char cLB) override;
	bool OnClick(short msX, short msY) override;
	bool OnDoubleClick(short msX, short msY) override;
	PressResult OnPress(short msX, short msY) override;
	bool OnItemDrop(short msX, short msY) override;

private:
	// Helper methods
	void DrawStat(int x1, int x2, int y, int baseStat, int angelicBonus);
	void DrawEquippedItem(hb::item::EquipPos equipPos, int drawX, int drawY,
		const char* cEquipPoiStatus, bool useWeaponColors, bool bHighlight, int spriteOffset = 0);
	void DrawHoverButton(int sX, int sY, int btnX, int btnY,
		short msX, short msY, int hoverFrame, int normalFrame);
	void DrawMaleCharacter(short sX, short sY, short msX, short msY,
		const char* cEquipPoiStatus, char& cCollison);
	void DrawFemaleCharacter(short sX, short sY, short msX, short msY,
		const char* cEquipPoiStatus, char& cCollison);

	// Shared helpers
	void BuildEquipStatusArray(char (&cEquipPoiStatus)[hb::item::DEF_MAXITEMEQUIPPOS]) const;
	char FindEquipItemAtPoint(short msX, short msY, short sX, short sY,
		const char* cEquipPoiStatus) const;
};
