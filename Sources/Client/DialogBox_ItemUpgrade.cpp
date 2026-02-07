#include "DialogBox_ItemUpgrade.h"
#include "CursorTarget.h"
#include "Game.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

using namespace hb::item;

DialogBox_ItemUpgrade::DialogBox_ItemUpgrade(CGame* pGame)
	: IDialogBox(DialogBoxId::ItemUpgrade, pGame)
{
	SetDefaultRect(60 , 50 , 258, 339);
}

void DialogBox_ItemUpgrade::OnDraw(short msX, short msY, short msZ, char cLB)
{
    if (!m_pGame->EnsureItemConfigsLoaded()) return;
    int sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sX;
    int sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sY;

    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 0);
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 5); // Item Upgrade Text

    switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode) {
    case 1:
        DrawMode1_GizonUpgrade(sX, sY, msX, msY);
        break;
    case 2:
        DrawMode2_InProgress(sX, sY);
        break;
    case 3:
        DrawMode3_Success(sX, sY, msX, msY);
        break;
    case 4:
        DrawMode4_Failed(sX, sY, msX, msY);
        break;
    case 5:
        DrawMode5_SelectUpgradeType(sX, sY, msX, msY);
        break;
    case 6:
        DrawMode6_StoneUpgrade(sX, sY, msX, msY);
        break;
    case 7:
        DrawMode7_ItemLost(sX, sY, msX, msY);
        break;
    case 8:
        DrawMode8_MaxUpgrade(sX, sY, msX, msY);
        break;
    case 9:
        DrawMode9_CannotUpgrade(sX, sY, msX, msY);
        break;
    case 10:
        DrawMode10_NoPoints(sX, sY, msX, msY);
        break;
    }
}

int DialogBox_ItemUpgrade::CalculateUpgradeCost(int iItemIndex)
{
    int iValue = (m_pGame->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
    iValue = iValue * (iValue + 6) / 8 + 2;

    // Special handling for Angelic Pendants
    CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[iItemIndex]->m_sIDnum);
    if (pCfg && (pCfg->m_cEquipPos >= 11)
        && (pCfg->GetItemType() == ItemType::Equip))
    {
        short sID = m_pGame->m_pItemList[iItemIndex]->m_sIDnum;
        if (sID == hb::item::ItemId::AngelicPandentSTR || sID == hb::item::ItemId::AngelicPandentDEX ||
            sID == hb::item::ItemId::AngelicPandentINT || sID == hb::item::ItemId::AngelicPandentMAG)
        {
            iValue = (m_pGame->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
            switch (iValue) {
            case 0: iValue = 10; break;
            case 1: iValue = 11; break;
            case 2: iValue = 13; break;
            case 3: iValue = 16; break;
            case 4: iValue = 20; break;
            case 5: iValue = 25; break;
            case 6: iValue = 31; break;
            case 7: iValue = 38; break;
            case 8: iValue = 46; break;
            case 9: iValue = 55; break;
            }
        }
    }
    return iValue;
}

void DialogBox_ItemUpgrade::DrawItemPreview(int sX, int sY, int iItemIndex)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    char cStr1[120], cStr2[120], cStr3[120];

    char cItemColor = m_pGame->m_pItemList[iItemIndex]->m_cItemColor;
    CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[iItemIndex]->m_sIDnum);
    if (pCfg && ((pCfg->GetEquipPos() == EquipPos::LeftHand)
        || (pCfg->GetEquipPos() == EquipPos::RightHand)
        || (pCfg->GetEquipPos() == EquipPos::TwoHand)))
    {
        m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
    }
    else if (pCfg)
    {
        m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
    }

    std::memset(cStr1, 0, sizeof(cStr1));
    std::memset(cStr2, 0, sizeof(cStr2));
    std::memset(cStr3, 0, sizeof(cStr3));
    m_pGame->GetItemName(m_pGame->m_pItemList[iItemIndex].get(), cStr1, cStr2, cStr3);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, cStr1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, cStr2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, cStr3, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
}

void DialogBox_ItemUpgrade::DrawMode1_GizonUpgrade(int sX, int sY, int msX, int msY)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;
    char cTxt[256];

    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE3, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);

    std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_ITEMUPGRADE11, m_pGame->m_iGizonItemUpgradeLeft);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, cTxt, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
        int iValue = CalculateUpgradeCost(iItemIndex);

        std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_ITEMUPGRADE12, iValue);
        if (m_pGame->m_iGizonItemUpgradeLeft < iValue)
            TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, cTxt, TextLib::TextStyle::Color(GameColors::UIWarningRed), TextLib::Align::TopCenter);
        else
            TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, cTxt, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

        DrawItemPreview(sX, sY, iItemIndex);

        if (m_pGame->m_iGizonItemUpgradeLeft < iValue)
            m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);
        else
        {
            if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
                m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 47);
            else
                m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);
        }
    }
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);

    // Cancel button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 17);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 16);
}

void DialogBox_ItemUpgrade::DrawMode2_InProgress(int sX, int sY)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE5, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE6, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
        char cItemColor = m_pGame->m_pItemList[iItemIndex]->m_cItemColor;
        CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[iItemIndex]->m_sIDnum);

        if (pCfg && ((pCfg->GetEquipPos() == EquipPos::LeftHand)
            || (pCfg->GetEquipPos() == EquipPos::RightHand)
            || (pCfg->GetEquipPos() == EquipPos::TwoHand)))
        {
            m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
        }
        else if (pCfg)
        {
            m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
        }

        // Flickering effect
        if (pCfg && (rand() % 5) == 0)
            m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, SpriteLib::DrawParams::Alpha(0.25f));

        char cStr1[120], cStr2[120], cStr3[120];
        std::memset(cStr1, 0, sizeof(cStr1));
        std::memset(cStr2, 0, sizeof(cStr2));
        std::memset(cStr3, 0, sizeof(cStr3));
        m_pGame->GetItemName(m_pGame->m_pItemList[iItemIndex].get(), cStr1, cStr2, cStr3);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, cStr1, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, cStr2, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, cStr3, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }

    // Send upgrade command after 4 seconds
    if (((dwTime - m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1) / 1000 > 4)
        && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 != 0))
    {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = 0;
        m_pGame->bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_UPGRADEITEM, 0, iItemIndex, 0, 0, 0);
    }
}

void DialogBox_ItemUpgrade::DrawMode3_Success(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE7, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE8, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);
    }

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

void DialogBox_ItemUpgrade::DrawMode4_Failed(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE9, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // Check if item was destroyed
    if ((iItemIndex != -1) && (m_pGame->m_pItemList[iItemIndex] == 0))
    {
        m_pGame->PlayGameSound('E', 24, 0, 0);
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 7;
        return;
    }

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);
    }

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

void DialogBox_ItemUpgrade::DrawMode5_SelectUpgradeType(int sX, int sY, int msX, int msY)
{
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE13, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // Normal item upgrade option
    if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 100) && (msY < sY + 115))
    {
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE16, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE17, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE18, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE19, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 210, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE20, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE21, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 255, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE26, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 270, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE27, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);

    // Majestic item upgrade option
    if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 120) && (msY < sY + 135))
    {
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE22, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE23, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE24, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE25, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE28, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 240, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE29, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }
    else
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);

    // Cancel button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 17);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 16);
}

void DialogBox_ItemUpgrade::DrawMode6_StoneUpgrade(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;
    int iSoX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV2;
    int iSoM = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV3;
    char cTxt[256];

    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE31, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE32, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE33, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    if (iSoX == 0)
    {
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE41, TextLib::TextStyle::Color(GameColors::UIWarningRed), TextLib::Align::TopCenter);
    }
    else
    {
        std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_ITEMUPGRADE34, iSoX);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, cTxt, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }

    if (iSoM == 0)
    {
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE42, TextLib::TextStyle::Color(GameColors::UIWarningRed), TextLib::Align::TopCenter);
    }
    else
    {
        std::snprintf(cTxt, sizeof(cTxt), DRAW_DIALOGBOX_ITEMUPGRADE35, iSoM);
        TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, cTxt, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    }

    m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);

        if ((msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
            m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 47);
        else
            m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_LBTNPOSX, sY + DEF_BTNPOSY, 46);
    }

    // Cancel button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 17);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 16);
}

void DialogBox_ItemUpgrade::DrawMode7_ItemLost(int sX, int sY, int msX, int msY)
{
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE36, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 145, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE37, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

void DialogBox_ItemUpgrade::DrawMode8_MaxUpgrade(int sX, int sY, int msX, int msY)
{
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE38, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

void DialogBox_ItemUpgrade::DrawMode9_CannotUpgrade(int sX, int sY, int msX, int msY)
{
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE39, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

void DialogBox_ItemUpgrade::DrawMode10_NoPoints(int sX, int sY, int msX, int msY)
{
    TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE40, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);

    // OK button
    if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
    else
        m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
}

bool DialogBox_ItemUpgrade::OnClick(short msX, short msY)
{
    short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sX;
    short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sY;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode) {
    case 1: // Gizon upgrade
        if ((iItemIndex != -1) && (msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX)
            && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            int iValue = (m_pGame->m_pItemList[iItemIndex]->m_dwAttribute & 0xF0000000) >> 28;
            iValue = iValue * (iValue + 6) / 8 + 2;
            if (m_pGame->m_iGizonItemUpgradeLeft < iValue) break;

            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->PlayGameSound('E', 44, 0);
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 2;
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = m_pGame->m_dwCurTime;
            return true;
        }
        if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX)
            && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 3:  // Success
    case 4:  // Failed
    case 7:  // Item lost
    case 8:  // Max upgrade
    case 9:  // Cannot upgrade
    case 10: // No points
    case 12: // Need stone
        if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX)
            && (msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 5: // Main menu - select upgrade type
        // Normal item upgrade (Stone)
        if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 100) && (msY < sY + 115))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            int iSoX = 0, iSoM = 0;
            for (int i = 0; i < DEF_MAXITEMS; i++)
                if (m_pGame->m_pItemList[i] != 0)
                {
                    if ((m_pGame->m_pItemList[i]->m_sSprite == 6) && (m_pGame->m_pItemList[i]->m_sSpriteFrame == 128)) iSoX++;
                    if ((m_pGame->m_pItemList[i]->m_sSprite == 6) && (m_pGame->m_pItemList[i]->m_sSpriteFrame == 129)) iSoM++;
                }

            if ((iSoX > 0) || (iSoM > 0))
            {
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 6;
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV2 = iSoX;
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV3 = iSoM;
            }
            else
                m_pGame->AddEventList(DRAW_DIALOGBOX_ITEMUPGRADE30, 10);
            return true;
        }
        // Majestic item upgrade (Gizon)
        if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 120) && (msY < sY + 135))
        {
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 1;
            m_pGame->PlayGameSound('E', 14, 5);
            return true;
        }
        // Cancel
        if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX)
            && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 6: // Stone upgrade
        if ((iItemIndex != -1) && (msX >= sX + DEF_LBTNPOSX) && (msX <= sX + DEF_LBTNPOSX + DEF_BTNSZX)
            && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->PlayGameSound('E', 44, 0);
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 2;
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = m_pGame->m_dwCurTime;
            return true;
        }
        if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX)
            && (msY >= sY + DEF_BTNPOSY) && (msY <= sY + DEF_BTNPOSY + DEF_BTNSZY))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;
    }

    return false;
}

bool DialogBox_ItemUpgrade::OnItemDrop(short msX, short msY)
{
	char cItemID = (char)CursorTarget::GetSelectedID();
	if (m_pGame->m_bIsItemDisabled[cItemID]) return false;
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;
	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
	if (!pCfg || pCfg->GetEquipPos() == EquipPos::None) return false;

	switch (Info().cMode) {
	case 1:
		m_pGame->m_bIsItemDisabled[Info().sV1] = false;
		Info().sV1 = cItemID;
		m_pGame->m_bIsItemDisabled[cItemID] = true;
		m_pGame->PlayGameSound('E', 29, 0);
		break;

	case 6:
		m_pGame->m_bIsItemDisabled[Info().sV1] = false;
		Info().sV1 = cItemID;
		m_pGame->m_bIsItemDisabled[cItemID] = true;
		m_pGame->PlayGameSound('E', 29, 0);
		break;
	}

	return true;
}
