#include "DialogBox_ItemUpgrade.h"
#include "CursorTarget.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

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

    m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 0);
    m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 5); // Item Upgrade Text

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
        if (sID == hb::shared::item::ItemId::AngelicPandentSTR || sID == hb::shared::item::ItemId::AngelicPandentDEX ||
            sID == hb::shared::item::ItemId::AngelicPandentINT || sID == hb::shared::item::ItemId::AngelicPandentMAG)
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

    char cItemColor = m_pGame->m_pItemList[iItemIndex]->m_cItemColor;
    CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[iItemIndex]->m_sIDnum);
    if (pCfg && ((pCfg->GetEquipPos() == EquipPos::LeftHand)
        || (pCfg->GetEquipPos() == EquipPos::RightHand)
        || (pCfg->GetEquipPos() == EquipPos::TwoHand)))
    {
        m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
    }
    else if (pCfg)
    {
        m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
    }

    auto itemInfo = ItemNameFormatter::Get().Format(m_pGame->m_pItemList[iItemIndex].get());
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, itemInfo.name.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, itemInfo.effect.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, itemInfo.extra.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
}

void DialogBox_ItemUpgrade::DrawMode1_GizonUpgrade(int sX, int sY, int msX, int msY)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;
    std::string cTxt;

    m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE1, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE2, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE3, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    cTxt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE11, m_pGame->m_iGizonItemUpgradeLeft);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
        int iValue = CalculateUpgradeCost(iItemIndex);

        cTxt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE12, iValue);
        if (m_pGame->m_iGizonItemUpgradeLeft < iValue)
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        else
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 115, (sX + 248) - (sX + 24), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

        DrawItemPreview(sX, sY, iItemIndex);

        if (m_pGame->m_iGizonItemUpgradeLeft < iValue)
            m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
        else
        {
            if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
                m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 47);
            else
                m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
        }
    }
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    // Cancel button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode2_InProgress(int sX, int sY)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE5, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE6, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
        char cItemColor = m_pGame->m_pItemList[iItemIndex]->m_cItemColor;
        CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[iItemIndex]->m_sIDnum);

        if (pCfg && ((pCfg->GetEquipPos() == EquipPos::LeftHand)
            || (pCfg->GetEquipPos() == EquipPos::RightHand)
            || (pCfg->GetEquipPos() == EquipPos::TwoHand)))
        {
            m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Weapons[cItemColor].r, GameColors::Weapons[cItemColor].g, GameColors::Weapons[cItemColor].b));
        }
        else if (pCfg)
        {
            m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Tint(GameColors::Items[cItemColor].r, GameColors::Items[cItemColor].g, GameColors::Items[cItemColor].b));
        }

        // Flickering effect
        if (pCfg && (rand() % 5) == 0)
            m_pGame->m_pSprite[ItemPackPivotPoint + pCfg->m_sSprite]->Draw(sX + 134, sY + 182, pCfg->m_sSpriteFrame, hb::shared::sprite::DrawParams::Alpha(0.25f));

        auto itemInfo2 = ItemNameFormatter::Get().Format(m_pGame->m_pItemList[iItemIndex].get());
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 230 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.name.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 245 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.effect.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 260 + 20, (sX + 248) - (sX + 24), 15, itemInfo2.extra.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    // Send upgrade command after 4 seconds
    if (((dwTime - m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1) / 1000 > 4)
        && (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 != 0))
    {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = 0;
        m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::UpgradeItem, 0, iItemIndex, 0, 0, 0);
    }
}

void DialogBox_ItemUpgrade::DrawMode3_Success(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE7, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 45 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE8, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);
    }

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode4_Failed(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 55 + 30 + 282 - 117 - 170, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE9, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // Check if item was destroyed
    if ((iItemIndex != -1) && (m_pGame->m_pItemList[iItemIndex] == 0))
    {
        m_pGame->PlayGameSound('E', 24, 0, 0);
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 7;
        return;
    }

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);
    }

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode5_SelectUpgradeType(int sX, int sY, int msX, int msY)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE13, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // Normal item upgrade option
    if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 100) && (msY < sY + 115))
    {
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE16, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE17, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE18, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE19, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 210, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE20, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE21, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 255, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE26, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 270, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE27, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 100, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE14, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

    // Majestic item upgrade option
    if ((msX > sX + 24) && (msX < sX + 248) && (msY > sY + 120) && (msY < sY + 135))
    {
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 150, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE22, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 165, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE23, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 180, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE24, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 195, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE25, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 225, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE28, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 240, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE29, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }
    else
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 120, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE15, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

    // Cancel button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode6_StoneUpgrade(int sX, int sY, int msX, int msY)
{
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;
    int iSoX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV2;
    int iSoM = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV3;
    std::string cTxt;

    m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 30, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE31, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 45, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE32, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 60, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE33, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    if (iSoX == 0)
    {
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE41, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
    }
    else
    {
        cTxt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE34, iSoX);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 80, (sX + 248) - (sX + 24), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    if (iSoM == 0)
    {
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE42, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
    }
    else
    {
        cTxt = std::format(DRAW_DIALOGBOX_ITEMUPGRADE35, iSoM);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 95, (sX + 248) - (sX + 24), 15, cTxt.c_str(), hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    }

    m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);

    if (iItemIndex != -1)
    {
        m_pGame->DrawNewDialogBox(InterfaceNdGame3, sX, sY, 3);
        DrawItemPreview(sX, sY, iItemIndex);

        if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
            m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 47);
        else
            m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 46);
    }

    // Cancel button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_ItemUpgrade::DrawMode7_ItemLost(int sX, int sY, int msX, int msY)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE36, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 145, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE37, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode8_MaxUpgrade(int sX, int sY, int msX, int msY)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE38, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode9_CannotUpgrade(int sX, int sY, int msX, int msY)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE39, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_ItemUpgrade::DrawMode10_NoPoints(int sX, int sY, int msX, int msY)
{
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 20 + 130, (sX + 248) - (sX + 24), 15, DRAW_DIALOGBOX_ITEMUPGRADE40, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // OK button
    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
    else
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

bool DialogBox_ItemUpgrade::OnClick(short msX, short msY)
{
    short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sX;
    short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sY;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1;

    switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode) {
    case 1: // Gizon upgrade
        if ((iItemIndex != -1) && (msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x)
            && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
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
        if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
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
        if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
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
            for (int i = 0; i < hb::shared::limits::MaxItems; i++)
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
        if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemUpgrade);
            return true;
        }
        break;

    case 6: // Stone upgrade
        if ((iItemIndex != -1) && (msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x)
            && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        {
            m_pGame->PlayGameSound('E', 14, 5);
            m_pGame->PlayGameSound('E', 44, 0);
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 2;
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = m_pGame->m_dwCurTime;
            return true;
        }
        if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x)
            && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
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
	char cItemID = static_cast<char>(CursorTarget::GetSelectedID());
	if (m_pGame->m_bIsItemDisabled[cItemID]) return false;
	if (m_pGame->m_pPlayer->m_Controller.GetCommand() < 0) return false;
	CItem* pCfg = m_pGame->GetItemConfig(m_pGame->m_pItemList[cItemID]->m_sIDnum);
	if (!pCfg || pCfg->GetEquipPos() == EquipPos::None) return false;

	switch (Info().cMode) {
	case 1:
		if (Info().sV1 >= 0 && Info().sV1 < hb::shared::limits::MaxItems)
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
		Info().sV1 = cItemID;
		m_pGame->m_bIsItemDisabled[cItemID] = true;
		m_pGame->PlayGameSound('E', 29, 0);
		break;

	case 6:
		if (Info().sV1 >= 0 && Info().sV1 < hb::shared::limits::MaxItems)
			m_pGame->m_bIsItemDisabled[Info().sV1] = false;
		Info().sV1 = cItemID;
		m_pGame->m_bIsItemDisabled[cItemID] = true;
		m_pGame->PlayGameSound('E', 29, 0);
		break;
	}

	return true;
}
