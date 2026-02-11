#include "DialogBox_Shop.h"
#include "Game.h"
#include "IInput.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"

using namespace hb::shared::net;
using namespace hb::shared::item;
using namespace hb::client::sprite_id;

DialogBox_Shop::DialogBox_Shop(CGame* pGame)
    : IDialogBox(DialogBoxId::SaleMenu, pGame)
{
    SetDefaultRect(70 , 50 , 258, 339);
}

void DialogBox_Shop::OnDraw(short msX, short msY, short msZ, char cLB)
{
    short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sX;
    short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sY;

    m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 2);
    m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 11);

    switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode) {
    case 0:
        DrawItemList(sX, sY, msX, msY, msZ, cLB);
        break;
    default:
        DrawItemDetails(sX, sY, msX, msY, msZ);
        break;
    }
}

void DialogBox_Shop::DrawItemList(short sX, short sY, short msX, short msY, short msZ, char cLB)
{
    int iTotalLines = 0;
    int iPointerLoc;
    double d1, d2, d3;
    char cTemp[255], cStr2[255], cStr3[255];
    int iCost, iDiscountCost, iDiscountRatio;
    double dTmp1, dTmp2, dTmp3;

    for (int i = 0; i < game_limits::max_menu_items; i++)
        if (m_pGame->m_pItemForSaleList[i] != 0) iTotalLines++;

    if (iTotalLines > 13) {
        d1 = (double)m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView;
        d2 = (double)(iTotalLines - 13);
        d3 = (274.0f * d1) / d2;
        iPointerLoc = (int)(d3);
        m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 3);
        m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX + 242, sY + iPointerLoc + 35, 7);
    }
    else iPointerLoc = 0;

    if (cLB != 0 && iTotalLines > 13) {
        if ((m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::SaleMenu)) {
            if ((msX >= sX + 235) && (msX <= sX + 260) && (msY >= sY + 10) && (msY <= sY + 330)) {
                d1 = (double)(msY - (sY + 35));
                d2 = (double)(iTotalLines - 13);
                d3 = (d1 * d2) / 274.0f;
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView = (int)(d3 + 0.5);
            }
        }
    }
    else m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).bIsScrollSelected = false;

    if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::SaleMenu && msZ != 0) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView - msZ / 60;

    }

    if (iTotalLines > 13 && m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView > iTotalLines - 13)
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView = iTotalLines - 13;
    if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView < 0 || iTotalLines < 13)
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView = 0;

    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 22, sY + 45, (sX + 165) - (sX + 22), 15, DRAW_DIALOGBOX_SHOP1, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter); // "ITEM"
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 23, sY + 45, (sX + 166) - (sX + 23), 15, DRAW_DIALOGBOX_SHOP1, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 153, sY + 45, (sX + 250) - (sX + 153), 15, DRAW_DIALOGBOX_SHOP3, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 154, sY + 45, (sX + 251) - (sX + 154), 15, DRAW_DIALOGBOX_SHOP3, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);

    // Draw item names
    for (int i = 0; i < 13; i++)
        if (((i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView) < game_limits::max_menu_items) &&
            (m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView] != 0)) {
            std::memset(cTemp, 0, sizeof(cTemp));
            m_pGame->GetItemName(m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView].get(), cTemp, cStr2, cStr3);
            if ((msX >= sX + 20) && (msX <= sX + 220) && (msY >= sY + i * 18 + 65) && (msY <= sY + i * 18 + 79)) {
                hb::shared::text::DrawTextAligned(GameFont::Default, sX + 10, sY + i * 18 + 65, (sX + 190) - (sX + 10), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
            }
            else hb::shared::text::DrawTextAligned(GameFont::Default, sX + 10, sY + i * 18 + 65, (sX + 190) - (sX + 10), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
        }

    // Draw prices
    for (int i = 0; i < 13; i++)
        if (((i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView) < game_limits::max_menu_items) &&
            (m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView] != 0)) {
            iDiscountRatio = ((m_pGame->m_pPlayer->m_iCharisma - 10) / 4);
            dTmp1 = (double)iDiscountRatio;
            dTmp2 = dTmp1 / 100.0f;
            dTmp1 = (double)m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView]->m_wPrice;
            dTmp3 = dTmp1 * dTmp2;
            iDiscountCost = (int)dTmp3;
            iCost = (int)(m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView]->m_wPrice * ((100 + m_pGame->m_cDiscount) / 100.));
            iCost = iCost - iDiscountCost;

            if (iCost < static_cast<int>(m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView]->m_wPrice / 2))
                iCost = static_cast<int>(m_pGame->m_pItemForSaleList[i + m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView]->m_wPrice / 2) - 1;

            std::memset(cTemp, 0, sizeof(cTemp));
            std::snprintf(cTemp, sizeof(cTemp), "%6d", iCost);
            if ((msX >= sX + 20) && (msX <= sX + 220) && (msY >= sY + i * 18 + 65) && (msY <= sY + i * 18 + 79))
                hb::shared::text::DrawTextAligned(GameFont::Default, sX + 148, sY + i * 18 + 65, (sX + 260) - (sX + 148), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
            else hb::shared::text::DrawTextAligned(GameFont::Default, sX + 148, sY + i * 18 + 65, (sX + 260) - (sX + 148), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
        }
}

int DialogBox_Shop::CalculateDiscountedPrice(int iItemIndex)
{
    int iDiscountRatio = ((m_pGame->m_pPlayer->m_iCharisma - 10) / 4);
    double dTmp1 = (double)iDiscountRatio;
    double dTmp2 = dTmp1 / 100.0f;
    dTmp1 = (double)m_pGame->m_pItemForSaleList[iItemIndex]->m_wPrice;
    double dTmp3 = dTmp1 * dTmp2;
    int iDiscountCost = (int)dTmp3;
    int iCost = (int)(m_pGame->m_pItemForSaleList[iItemIndex]->m_wPrice * ((100 + m_pGame->m_cDiscount) / 100.));
    iCost = iCost - iDiscountCost;

    if (iCost < static_cast<int>(m_pGame->m_pItemForSaleList[iItemIndex]->m_wPrice / 2))
        iCost = static_cast<int>(m_pGame->m_pItemForSaleList[iItemIndex]->m_wPrice / 2) - 1;

    return iCost;
}

void DialogBox_Shop::DrawItemDetails(short sX, short sY, short msX, short msY, short msZ)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    int iItemIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode - 1;
    char cTemp[255], cStr2[255], cStr3[255];
    bool bFlagStatLow = false;
    bool bFlagRedShown = false;

    m_pGame->m_pSprite[ItemPackPivotPoint + m_pGame->m_pItemForSaleList[iItemIndex]->m_sSprite]->Draw(sX + 62 + 30 - 35, sY + 84 + 30 - 10, m_pGame->m_pItemForSaleList[iItemIndex]->m_sSpriteFrame);

    std::memset(cTemp, 0, sizeof(cTemp));
    m_pGame->GetItemName(m_pGame->m_pItemForSaleList[iItemIndex].get(), cTemp, cStr2, cStr3);

    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 50, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
    hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 50, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);

    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP3); //"PRICE"
    hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 78 + 30 - 10, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 78 + 30 - 10, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP6); // "Weight"
    hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 93 + 30 - 10, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 93 + 30 - 10, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    int iCost = CalculateDiscountedPrice(iItemIndex);
    std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP7, iCost); //": %d Gold"
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 98, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    int iWeight = m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100;
    std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP8, iWeight); //": %d Stone"
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 113, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    switch (m_pGame->m_pItemForSaleList[iItemIndex]->GetEquipPos()) {
    case EquipPos::RightHand:
    case EquipPos::TwoHand:
        DrawWeaponStats(sX, sY, iItemIndex, bFlagRedShown);
        break;

    case EquipPos::LeftHand:
        DrawShieldStats(sX, sY, iItemIndex, bFlagRedShown);
        break;

    case EquipPos::Head:
    case EquipPos::Body:
    case EquipPos::Leggings:
    case EquipPos::Arms:
    case EquipPos::Pants:
        DrawArmorStats(sX, sY, iItemIndex, bFlagStatLow, bFlagRedShown);
        break;

    case EquipPos::None:
        break;
    }

    DrawLevelRequirement(sX, sY, iItemIndex, bFlagRedShown);
    DrawQuantitySelector(sX, sY, msX, msY, msZ);

    // Draw buttons
    if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 31);
    else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 30);

    if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
        m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
    else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
}

void DialogBox_Shop::DrawWeaponStats(short sX, short sY, int iItemIndex, bool& bFlagRedShown)
{
    char cTemp[255];
    int iTemp;

    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP9);  // Damage
    hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP10); //"Speed(Min.~Max.)"
    hb::shared::text::DrawText(GameFont::Default, sX + 40, sY + 175, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 41, sY + 175, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    if (m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue3 != 0) {
        std::snprintf(cTemp, sizeof(cTemp), ": %dD%d+%d (S-M)", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue1,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue2,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue3);
    }
    else {
        std::snprintf(cTemp, sizeof(cTemp), ": %dD%d (S-M)", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue1,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue2);
    }
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    if (m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue6 != 0) {
        std::snprintf(cTemp, sizeof(cTemp), ": %dD%d+%d (L)", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue4,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue6);
    }
    else {
        std::snprintf(cTemp, sizeof(cTemp), ": %dD%d (L)", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue4,
            m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
    }
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 160, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    iTemp = m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100;
    if (m_pGame->m_pItemForSaleList[iItemIndex]->m_cSpeed == 0) std::snprintf(cTemp, sizeof(cTemp), ": 0(10~10)");
    else std::snprintf(cTemp, sizeof(cTemp), ": %d(%d ~ %d)", m_pGame->m_pItemForSaleList[iItemIndex]->m_cSpeed, iTemp, m_pGame->m_pItemForSaleList[iItemIndex]->m_cSpeed * 13);
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 175, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    if ((m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100) > m_pGame->m_pPlayer->m_iStr) {
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP11, (m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100));
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 258, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter); // *Your STR should be at least %d to use this item."
        bFlagRedShown = true;
    }
}

void DialogBox_Shop::DrawShieldStats(short sX, short sY, int iItemIndex, bool& bFlagRedShown)
{
    char cTemp[255];

    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP12); // "Defence"
    hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    std::snprintf(cTemp, sizeof(cTemp), ": +%d%%", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue1);
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    if ((m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100) > m_pGame->m_pPlayer->m_iStr) {
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP11, (m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100));
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter); // "*Your STR should be at least %d to use this item."
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 258, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        bFlagRedShown = true;
    }
}

void DialogBox_Shop::DrawArmorStats(short sX, short sY, int iItemIndex, bool& bFlagStatLow, bool& bFlagRedShown)
{
    char cTemp[255];

    std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP12); // "Defence"
    hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    std::snprintf(cTemp, sizeof(cTemp), ": +%d%%", m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue1);
    hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 145, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
    bFlagStatLow = false;

    switch (m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue4) {
    case 10://"Available for above Str %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP15, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iStr >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    case 11: // "Available for above Dex %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP16, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iDex >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    case 12: // "Available for above Vit %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP17, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iVit >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    case 13: // "Available for above Int %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP18, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iInt >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    case 14: // "Available for above Mag %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP19, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iMag >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    case 15: // "Available for above Chr %d"
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP20, m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5);
        if (m_pGame->m_pPlayer->m_iCharisma >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sItemEffectValue5) {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
        }
        else {
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 160, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 160, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
            bFlagStatLow = true;
        }
        break;
    default:
        break;
    }

    if ((m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100) > m_pGame->m_pPlayer->m_iStr) {
        std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP11, (m_pGame->m_pItemForSaleList[iItemIndex]->m_wWeight / 100));
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 288, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 288, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter); // "*Your STR should be at least %d to use this item."
        bFlagRedShown = true;
    }
    else if (bFlagStatLow == true) {
        std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP21); // "(Warning!) Your stat is too low for this item."
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 258, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        bFlagRedShown = true;
    }
    else if ((strstr(m_pGame->m_pItemForSaleList[iItemIndex]->m_cName, "(M)") != 0)
        && (m_pGame->m_pPlayer->m_sPlayerType > 3)) {
        std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP22); // "(Warning!) only for male."
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 258, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        bFlagRedShown = true;
    }
    else if ((strstr(m_pGame->m_pItemForSaleList[iItemIndex]->m_cName, "(W)") != 0)
        && (m_pGame->m_pPlayer->m_sPlayerType <= 3)) {
        std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP23); // "(Warning!) only for female."
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        hb::shared::text::DrawTextAligned(GameFont::Default, sX + 26, sY + 258, (sX + 241) - (sX + 26), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
        bFlagRedShown = true;
    }
}

void DialogBox_Shop::DrawLevelRequirement(short sX, short sY, int iItemIndex, bool& bFlagRedShown)
{
    char cTemp[255];

    if (m_pGame->m_pItemForSaleList[iItemIndex]->m_sLevelLimit != 0) {
        std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP24); // "Level"
        if (m_pGame->m_pPlayer->m_iLevel >= m_pGame->m_pItemForSaleList[iItemIndex]->m_sLevelLimit) {
            hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
            hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
            std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP25, m_pGame->m_pItemForSaleList[iItemIndex]->m_sLevelLimit);
            hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));// ": above %d"
        }
        else {
            hb::shared::text::DrawText(GameFont::Default, sX + 90, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
            hb::shared::text::DrawText(GameFont::Default, sX + 91, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
            std::snprintf(cTemp, sizeof(cTemp), DRAW_DIALOGBOX_SHOP25, m_pGame->m_pItemForSaleList[iItemIndex]->m_sLevelLimit);
            hb::shared::text::DrawText(GameFont::Default, sX + 140, sY + 190, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed));// ": above %d"
            if (bFlagRedShown == false) {
                std::snprintf(cTemp, sizeof(cTemp), "%s", DRAW_DIALOGBOX_SHOP26); // "(Warning!) Your level is too low for this item."
                hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25, sY + 258, (sX + 240) - (sX + 25), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
                hb::shared::text::DrawTextAligned(GameFont::Default, sX + 25 + 1, sY + 258, (sX + 240 + 1) - (sX + 25 + 1), 15, cTemp, hb::shared::text::TextStyle::Color(GameColors::UIWarningRed), hb::shared::text::Align::TopCenter);
                bFlagRedShown = true;
            }
        }
    }
}

void DialogBox_Shop::DrawQuantitySelector(short sX, short sY, short msX, short msY, short msZ)
{
    uint32_t dwTime = m_pGame->m_dwCurTime;
    char cTemp[255];

    m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + 156, sY + 219, 19);
    m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + 170, sY + 219, 19);
    hb::shared::text::DrawText(GameFont::Default, sX + 123 - 35, sY + 237 - 10, DRAW_DIALOGBOX_SHOP27, hb::shared::text::TextStyle::Color(GameColors::UILabel)); // "Quantity:"
    hb::shared::text::DrawText(GameFont::Default, sX + 124 - 35, sY + 237 - 10, DRAW_DIALOGBOX_SHOP27, hb::shared::text::TextStyle::Color(GameColors::UILabel));

    if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::SaleMenu && msZ != 0) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 + msZ / 60;

    }

    if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 > (50 - m_pGame->_iGetTotalItemNum()))
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = (50 - m_pGame->_iGetTotalItemNum());
    if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 < 1)
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;

    if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 >= 10) {
        std::memset(cTemp, 0, sizeof(cTemp));
        std::snprintf(cTemp, sizeof(cTemp), "%d", m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3);
        cTemp[1] = 0;
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 186, sY - 10 + 237, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 187, sY - 10 + 237, cTemp, hb::shared::text::TextStyle::Color(GameColors::UILabel));
        std::memset(cTemp, 0, sizeof(cTemp));
        std::snprintf(cTemp, sizeof(cTemp), "%d", m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3);
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 200, sY - 10 + 237, (cTemp + 1), hb::shared::text::TextStyle::Color(GameColors::UILabel));
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 201, sY - 10 + 237, (cTemp + 1), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    }
    else {
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 186, sY - 10 + 237, "0", hb::shared::text::TextStyle::Color(GameColors::UILabel));
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 187, sY - 10 + 237, "0", hb::shared::text::TextStyle::Color(GameColors::UILabel));
        std::memset(cTemp, 0, sizeof(cTemp));
        std::snprintf(cTemp, sizeof(cTemp), "%d", m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3);
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 200, sY - 10 + 237, (cTemp), hb::shared::text::TextStyle::Color(GameColors::UILabel));
        hb::shared::text::DrawText(GameFont::Default, sX - 35 + 201, sY - 10 + 237, (cTemp), hb::shared::text::TextStyle::Color(GameColors::UILabel));
    }
    m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + 156, sY + 244, 20);
    m_pGame->m_pSprite[InterfaceNdGame2]->Draw(sX + 170, sY + 244, 20);
}

bool DialogBox_Shop::OnClick(short msX, short msY)
{
    short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sX;
    short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sY;

    switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode) {
    case 0:
        return OnClickItemList(sX, sY, msX, msY);
    default:
        return OnClickItemDetails(sX, sY, msX, msY);
    }
    return false;
}

bool DialogBox_Shop::OnClickItemList(short sX, short sY, short msX, short msY)
{
    for (int i = 0; i < 13; i++)
        if ((msX >= sX + 20) && (msX <= sX + 220) && (msY >= sY + i * 18 + 65) && (msY <= sY + i * 18 + 79)) {
            if (m_pGame->_iGetTotalItemNum() >= 50) {
                m_pGame->AddEventList(DLGBOX_CLICK_SHOP1, 10);//"You cannot buy anything because your bag is full."
                return true;
            }

            m_pGame->PlayGameSound('E', 14, 5);
            if (m_pGame->m_pItemForSaleList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView + i] != 0)
                m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode = m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView + i + 1;
            return true;
        }
    return false;
}

bool DialogBox_Shop::OnClickItemDetails(short sX, short sY, short msX, short msY)
{
    char cTemp[21];

    // +10 quantity button
    if ((msX >= sX + 145) && (msX <= sX + 162) && (msY >= sY + 209) && (msY <= sY + 230)) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 += 10;
        if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 >= (50 - m_pGame->_iGetTotalItemNum()))
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = (50 - m_pGame->_iGetTotalItemNum());
        return true;
    }

    // -10 quantity button
    if ((msX >= sX + 145) && (msX <= sX + 162) && (msY >= sY + 234) && (msY <= sY + 251)) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 -= 10;
        if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 <= 1)
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;
        return true;
    }

    // +1 quantity button
    if ((msX >= sX + 163) && (msX <= sX + 180) && (msY >= sY + 209) && (msY <= sY + 230)) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3++;
        if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 >= (50 - m_pGame->_iGetTotalItemNum()))
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = (50 - m_pGame->_iGetTotalItemNum());
        return true;
    }

    // -1 quantity button
    if ((msX >= sX + 163) && (msX <= sX + 180) && (msY >= sY + 234) && (msY <= sY + 251)) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3--;
        if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 <= 1)
            m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;
        return true;
    }

    // Purchase button
    if ((msX >= sX + 30) && (msX <= sX + 30 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
        if ((50 - m_pGame->_iGetTotalItemNum()) < m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3) {
            m_pGame->AddEventList(DLGBOX_CLICK_SHOP1, 10);//"ou cannot buy anything because your bag is full."
        }
        else {
            std::memset(cTemp, 0, sizeof(cTemp));
            CItem* pShopItem = m_pGame->m_pItemForSaleList[m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode - 1].get();
            std::snprintf(cTemp, sizeof(cTemp), "%s", pShopItem->m_cName);
            // Send item ID in iV2 for reliable item lookup on server
            int iItemId = pShopItem->m_sIDnum;
            m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::ReqPurchaseItem, 0, m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3, iItemId, 0, cTemp);
        }
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode = 0;
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;
        m_pGame->PlayGameSound('E', 14, 5);
        return true;
    }

    // Cancel button
    if ((msX >= sX + 154) && (msX <= sX + 154 + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode = 0;
        m_pGame->m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;
        m_pGame->PlayGameSound('E', 14, 5);
        return true;
    }

    return false;
}

PressResult DialogBox_Shop::OnPress(short msX, short msY)
{
    short sX = Info().sX;
    short sY = Info().sY;

    // Only claim scroll in item list mode (cMode == 0)
    if (Info().cMode == 0)
    {
        if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 20) && (msY <= sY + 330))
        {
            return PressResult::ScrollClaimed;
        }
    }

    return PressResult::Normal;
}

