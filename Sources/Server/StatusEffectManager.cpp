#include "StatusEffectManager.h"
#include "Game.h"
#include "Item.h"
#include "ItemManager.h"
#include "Packet/SharedPackets.h"
#include "ObjectIDRange.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
namespace sock = hb::shared::net::socket;

extern char G_cTxt[512];

void StatusEffectManager::SetHeroFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bHero = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bHero = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bHero = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bHero = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetBerserkFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bBerserk = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bBerserk = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bBerserk = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bBerserk = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetHasteFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bHaste = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bHaste = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		break;
	}
}

void StatusEffectManager::SetPoisonFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bPoisoned = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bPoisoned = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bPoisoned = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bPoisoned = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetDefenseShieldFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bDefenseShield = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bDefenseShield = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bDefenseShield = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bDefenseShield = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetMagicProtectionFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bMagicProtection = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bMagicProtection = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bMagicProtection = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bMagicProtection = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetProtectionFromArrowFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bProtectionFromArrow = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bProtectionFromArrow = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bProtectionFromArrow = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bProtectionFromArrow = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetIllusionMovementFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bIllusionMovement = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bIllusionMovement = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetIllusionFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bIllusion = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bIllusion = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bIllusion = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bIllusion = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetIceFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bFrozen = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bFrozen = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bFrozen = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bFrozen = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetInvisibilityFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bInvisibility = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bInvisibility = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bInvisibility = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bInvisibility = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetInhibitionCastingFlag(short sOwnerH, char cOwnerType, bool bStatus)
{
	switch (cOwnerType) {
	case hb::shared::owner_class::Player:
		if (m_pGame->m_pClientList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pClientList[sOwnerH]->m_status.bInhibitionCasting = true;
		else m_pGame->m_pClientList[sOwnerH]->m_status.bInhibitionCasting = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;

	case hb::shared::owner_class::Npc:
		if (m_pGame->m_pNpcList[sOwnerH] == 0) return;
		if (bStatus)
			m_pGame->m_pNpcList[sOwnerH]->m_status.bInhibitionCasting = true;
		else m_pGame->m_pNpcList[sOwnerH]->m_status.bInhibitionCasting = false;
		m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
		break;
	}
}

void StatusEffectManager::SetAngelFlag(short sOwnerH, char cOwnerType, int iStatus, int iTemp)
{
	if (cOwnerType != hb::shared::owner_class::Player) return;
	if (m_pGame->m_pClientList[sOwnerH] == 0) return;
	switch (iStatus) {
	case 1: // STR Angel
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelSTR = true;
		break;
	case 2: // DEX Angel
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelDEX = true;
		break;
	case 3: // INT Angel
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelINT = true;
		break;
	case 4: // MAG Angel
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelMAG = true;
		break;
	default:
	case 0: // Remove all Angels
		m_pGame->m_pClientList[sOwnerH]->m_status.iAngelPercent = 0;
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelSTR = false;
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelDEX = false;
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelINT = false;
		m_pGame->m_pClientList[sOwnerH]->m_status.bAngelMAG = false;
		break;
	}
	if (iTemp > 4)
	{
		int iAngelicStars = (iTemp / 3) * (iTemp / 5);
		m_pGame->m_pClientList[sOwnerH]->m_status.iAngelPercent = static_cast<uint8_t>(iAngelicStars);
	}
	m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
}

void StatusEffectManager::_CheckFarmingAction(short sAttackerH, short sTargetH, bool bType)
{
	char cCropType;
	int iItemID;
	CItem* pItem;

	iItemID = 0;
	cCropType = 0;

	cCropType = m_pGame->m_pNpcList[sTargetH]->m_cCropType;
	switch (cCropType) {
	case 1: m_pGame->GetExp(sAttackerH, m_pGame->iDice(3, 10)); iItemID = 820; break; // WaterMelon
	case 2: m_pGame->GetExp(sAttackerH, m_pGame->iDice(3, 10)); iItemID = 821; break; // Pumpkin
	case 3: m_pGame->GetExp(sAttackerH, m_pGame->iDice(4, 10)); iItemID = 822; break; // Garlic
	case 4: m_pGame->GetExp(sAttackerH, m_pGame->iDice(4, 10)); iItemID = 823; break; // Barley
	case 5: m_pGame->GetExp(sAttackerH, m_pGame->iDice(5, 10)); iItemID = 824; break; // Carrot
	case 6: m_pGame->GetExp(sAttackerH, m_pGame->iDice(5, 10)); iItemID = 825; break; // Radish
	case 7: m_pGame->GetExp(sAttackerH, m_pGame->iDice(6, 10)); iItemID = 826; break; // Corn
	case 8: m_pGame->GetExp(sAttackerH, m_pGame->iDice(6, 10)); iItemID = 827; break; // ChineseBellflower
	case 9: m_pGame->GetExp(sAttackerH, m_pGame->iDice(7, 10)); iItemID = 828; break; // Melone
	case 10: m_pGame->GetExp(sAttackerH, m_pGame->iDice(7, 10)); iItemID = 829; break; // Tommato
	case 11: m_pGame->GetExp(sAttackerH, m_pGame->iDice(8, 10)); iItemID = 830; break; // Grapes
	case 12: m_pGame->GetExp(sAttackerH, m_pGame->iDice(8, 10)); iItemID = 831; break; // BlueGrapes
	case 13: m_pGame->GetExp(sAttackerH, m_pGame->iDice(9, 10)); iItemID = 832; break; // Mushroom
	default: m_pGame->GetExp(sAttackerH, m_pGame->iDice(10, 10)); iItemID = 721; break; // Ginseng

	}

	pItem = new CItem;
	if (m_pGame->m_pItemManager->_bInitItemAttr(pItem, iItemID) == false) {
		delete pItem;
	}
	if (bType == 0) {
		m_pGame->m_pMapList[m_pGame->m_pClientList[sAttackerH]->m_cMapIndex]->bSetItem(m_pGame->m_pClientList[sAttackerH]->m_sX, m_pGame->m_pClientList[sAttackerH]->m_sY, pItem);
		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[sAttackerH]->m_cMapIndex,
			m_pGame->m_pClientList[sAttackerH]->m_sX, m_pGame->m_pClientList[sAttackerH]->m_sY, pItem->m_sIDnum, 0,
			pItem->m_cItemColor, pItem->m_dwAttribute);
	}
	else if (bType == 1) {
		m_pGame->m_pMapList[m_pGame->m_pNpcList[sTargetH]->m_cMapIndex]->bSetItem(m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY, pItem);
		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pNpcList[sTargetH]->m_cMapIndex,
			m_pGame->m_pNpcList[sTargetH]->m_sX, m_pGame->m_pNpcList[sTargetH]->m_sY, pItem->m_sIDnum, 0,
			pItem->m_cItemColor, pItem->m_dwAttribute);
	}

}
