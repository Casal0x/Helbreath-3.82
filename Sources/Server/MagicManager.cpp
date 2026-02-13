#include "MagicManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "Item.h"
#include "CombatManager.h"
#include "EntityManager.h"
#include "DynamicObjectManager.h"
#include "DelayEventManager.h"
#include "ItemManager.h"
#include "SkillManager.h"
#include "Packet/SharedPackets.h"
#include "ObjectIDRange.h"
#include "Skill.h"
#include "GameConfigSqliteStore.h"
#include "Log.h"
#include "ServerLogChannels.h"

using namespace hb::shared::net;

using hb::log_channel;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;
namespace dynamic_object = hb::shared::dynamic_object;
namespace smap = hb::server::map;
namespace sdelay = hb::server::delay_event;
using namespace hb::server::npc;
using namespace hb::server::skill;

extern char G_cTxt[512];
extern char G_cData50000[50000];

static int _tmp_iMCProb[] = { 0, 300, 250, 200, 150, 100, 80, 70, 60, 50, 40 };
static int _tmp_iMLevelPenalty[] = { 0,   5,   5,   8,   8,  10, 14, 28, 32, 36, 40 };

bool MagicManager::bSendClientMagicConfigs(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) {
		return false;
	}

	constexpr size_t maxPacketSize = 7000;
	constexpr size_t headerSize = sizeof(hb::net::PacketMagicConfigHeader);
	constexpr size_t entrySize = sizeof(hb::net::PacketMagicConfigEntry);
	constexpr size_t maxEntriesPerPacket = (maxPacketSize - headerSize) / entrySize;

	// Count total magics
	int totalMagics = 0;
	for(int i = 0; i < hb::shared::limits::MaxMagicType; i++) {
		if (m_pGame->m_pMagicConfigList[i] != 0) {
			totalMagics++;
		}
	}

	// Send magics in packets
	int magicsSent = 0;
	int packetIndex = 0;

	while (magicsSent < totalMagics) {
		std::memset(G_cData50000, 0, sizeof(G_cData50000));

		auto* pktHeader = reinterpret_cast<hb::net::PacketMagicConfigHeader*>(G_cData50000);
		pktHeader->header.msg_id = MsgId::MagicConfigContents;
		pktHeader->header.msg_type = MsgType::Confirm;
		pktHeader->totalMagics = static_cast<uint16_t>(totalMagics);
		pktHeader->packetIndex = static_cast<uint16_t>(packetIndex);

		auto* entries = reinterpret_cast<hb::net::PacketMagicConfigEntry*>(G_cData50000 + headerSize);

		uint16_t entriesInPacket = 0;
		int skipped = 0;

		for(int i = 0; i < hb::shared::limits::MaxMagicType && entriesInPacket < maxEntriesPerPacket; i++) {
			if (m_pGame->m_pMagicConfigList[i] == 0) {
				continue;
			}

			if (skipped < magicsSent) {
				skipped++;
				continue;
			}

			const CMagic* magic = m_pGame->m_pMagicConfigList[i];
			auto& entry = entries[entriesInPacket];

			entry.magicId = static_cast<int16_t>(i);
			std::memset(entry.name, 0, sizeof(entry.name));
			std::snprintf(entry.name, sizeof(entry.name), "%s", magic->m_cName);
			entry.manaCost = magic->m_sValue1;
			entry.intLimit = magic->m_sIntLimit;
			entry.goldCost = magic->m_iGoldCost;
			entry.isVisible = (magic->m_iGoldCost >= 0) ? 1 : 0;
			entry.magicType = magic->m_sType;
			entry.aoeRadiusX = magic->m_sValue2;
			entry.aoeRadiusY = magic->m_sValue3;
			entry.dynamicPattern = magic->m_sValue11;
			entry.dynamicRadius = magic->m_sValue12;

			entriesInPacket++;
		}

		pktHeader->magicCount = entriesInPacket;
		size_t packetSize = headerSize + (entriesInPacket * entrySize);

		int iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(G_cData50000, static_cast<int>(packetSize));
		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			hb::logger::log("Failed to send magic configs: Client({}) Packet({})", iClientH, packetIndex);
			m_pGame->DeleteClient(iClientH, true, true);
			delete m_pGame->m_pClientList[iClientH];
			m_pGame->m_pClientList[iClientH] = 0;
			return false;
		}

		magicsSent += entriesInPacket;
		packetIndex++;
	}

	return true;
}

int MagicManager::iClientMotion_Magic_Handler(int iClientH, short sX, short sY, char cDir)
{
	int     iRet;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return 0;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return 0;

	if ((sX != m_pGame->m_pClientList[iClientH]->m_sX) || (sY != m_pGame->m_pClientList[iClientH]->m_sY)) return 2;

	int iStX, iStY;
	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex] != 0) {
		iStX = m_pGame->m_pClientList[iClientH]->m_sX / 20;
		iStY = m_pGame->m_pClientList[iClientH]->m_sY / 20;
		m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iPlayerActivity++;

		switch (m_pGame->m_pClientList[iClientH]->m_cSide) {
		case 0: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iNeutralActivity++; break;
		case 1: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iAresdenActivity++; break;
		case 2: m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_stTempSectorInfo[iStX][iStY].iElvineActivity++;  break;
		}
	}

	m_pGame->m_pSkillManager->ClearSkillUsingStatus(iClientH);

	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->ClearOwner(0, iClientH, hb::shared::owner_class::Player, sX, sY);
	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetOwner(iClientH, hb::shared::owner_class::Player, sX, sY);

	if (m_pGame->m_pClientList[iClientH]->m_status.bInvisibility) {
		m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(iClientH, hb::shared::owner_class::Player, false);
		m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(iClientH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
		m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
	}

	m_pGame->m_pClientList[iClientH]->m_cDir = cDir;

	{
		hb::net::PacketResponseMotionHeader pkt{};
		pkt.header.msg_id = MsgId::ResponseMotion;
		pkt.header.msg_type = Confirm::MotionConfirm;
		iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
	}
	switch (iRet) {
	case sock::Event::QueueFull:
	case sock::Event::SocketError:
	case sock::Event::CriticalError:
	case sock::Event::SocketClosed:
		m_pGame->DeleteClient(iClientH, true, true);
		return 0;
	}

	return 1;
}

void MagicManager::PlayerMagicHandler(int iClientH, int dX, int dY, short sType, bool bItemEffect, int iV1, uint16_t targetObjectID)
{
	short sX, sY, sOwnerH, sMagicCircle, rx, ry, sLevelMagic;
	char cDir, cOwnerType, cName[hb::shared::limits::CharNameLen], cItemName[hb::shared::limits::ItemNameLen], cNpcWaypoint[11], cName_Master[hb::shared::limits::CharNameLen], cNpcName[hb::shared::limits::NpcNameLen], cRemainItemColor, cScanMessage[256];
	double dV1, dV2, dV3, dV4;
	int iErr, iRet, iResult, iDiceRes, iNamingValue, iFollowersNum, iEraseReq, iWhetherBonus;
	int tX, tY, iManaCost, iMagicAttr;
	CItem* pItem;
	uint32_t dwTime;
	uint16_t wWeaponType;
	short sEqStatus;
	int iMapSide = 0;
	short sIDNum;
	uint32_t dwAttr;

	dwTime = GameClock::GetTimeMS();
	m_pGame->m_pClientList[iClientH]->m_bMagicConfirm = true;
	m_pGame->m_pClientList[iClientH]->m_bMagicPauseTime = false;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	// Auto-aim: If client clicked on an entity, use the target's current position
	// This compensates for latency - players with high ping can still hit moving targets
	// Security: Only allow if target is within reasonable distance of original click (prevents cross-map exploits)
	constexpr int MAX_AUTOAIM_DISTANCE = 10;  // Max tiles target can move and still be tracked
	if (targetObjectID != 0)
	{
		int targetMapIndex = m_pGame->m_pClientList[iClientH]->m_cMapIndex;
		int targetX = -1, targetY = -1;

		if (hb::shared::object_id::IsPlayerID(targetObjectID))
		{
			// Target is a player
			if ((targetObjectID > 0) && (targetObjectID < MaxClients) && m_pGame->m_pClientList[targetObjectID] != nullptr)
			{
				// Verify target is on the same map
				if (m_pGame->m_pClientList[targetObjectID]->m_cMapIndex == targetMapIndex)
				{
					targetX = m_pGame->m_pClientList[targetObjectID]->m_sX;
					targetY = m_pGame->m_pClientList[targetObjectID]->m_sY;
				}
			}
		}
		else
		{
			// Target is an NPC
			int npcIndex = hb::shared::object_id::ToNpcIndex(targetObjectID);
			if ((npcIndex > 0) && (npcIndex < MaxNpcs) && m_pGame->m_pNpcList[npcIndex] != nullptr)
			{
				// Verify target is on the same map
				if (m_pGame->m_pNpcList[npcIndex]->m_cMapIndex == targetMapIndex)
				{
					targetX = m_pGame->m_pNpcList[npcIndex]->m_sX;
					targetY = m_pGame->m_pNpcList[npcIndex]->m_sY;
				}
			}
		}

		// Only use auto-aim if target was near the original click position
		// This prevents exploits where hackers send fake objectIDs for targets across the map
		if (targetX >= 0 && targetY >= 0)
		{
			int distX = abs(targetX - dX);
			int distY = abs(targetY - dY);
			if (distX <= MAX_AUTOAIM_DISTANCE && distY <= MAX_AUTOAIM_DISTANCE)
			{
				dX = targetX;
				dY = targetY;
			}
			// If target moved too far, fall back to original tile coordinates (no tracking)
		}
	}

	if ((dX < 0) || (dX >= m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_sSizeX) ||
		(dY < 0) || (dY >= m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_sSizeY)) return;

	if (((dwTime - m_pGame->m_pClientList[iClientH]->m_dwRecentAttackTime) < 1000) && (bItemEffect == 0)) {
		try
		{
			hb::logger::warn<log_channel::security>("Fast cast detection: IP={} player={}, magic casting too fast", m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName);
			m_pGame->DeleteClient(iClientH, true, true);
		}
		catch (...)
		{
		}
		return;
	}
	m_pGame->m_pClientList[iClientH]->m_dwRecentAttackTime = dwTime;
	m_pGame->m_pClientList[iClientH]->m_dwLastActionTime = dwTime;

	if (m_pGame->m_pClientList[iClientH]->m_cMapIndex < 0) return;
	if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex] == 0) return;

	if ((sType < 0) || (sType >= 100))     return;
	if (m_pGame->m_pMagicConfigList[sType] == 0) return;

	if ((bItemEffect == false) && (m_pGame->m_pClientList[iClientH]->m_cMagicMastery[sType] != 1)) return;

	// Only block offensive magic in no-attack zones; allow friendly spells (healing, buffs, teleport, create, etc.)
	if ((m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsAttackEnabled == false)
		&& sType != 14)
	{

		switch (m_pGame->m_pMagicConfigList[sType]->m_sType)
		{
		case hb::shared::magic::DamageSpot:
		case hb::shared::magic::DamageArea:
		case hb::shared::magic::SpDownSpot:
		case hb::shared::magic::SpDownArea:
		case hb::shared::magic::HoldObject:
		case hb::shared::magic::Possession:
		case hb::shared::magic::Confuse:
		case hb::shared::magic::Poison:
		case hb::shared::magic::DamageLinear:
		case hb::shared::magic::DamageAreaNoSpot:
		case hb::shared::magic::Tremor:
		case hb::shared::magic::Ice:
		case hb::shared::magic::DamageAreaNoSpotSpDown:
		case hb::shared::magic::IceLinear:
		case hb::shared::magic::DamageAreaArmorBreak:
		case hb::shared::magic::DamageLinearSpDown:
		case hb::shared::magic::Inhibition:
			return;
		}
	}
	//if ((var_874 ) && (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsHeldenianMap ) && (m_pGame->m_pMagicConfigList[sType]->m_sType != 8)) return;

	if ((m_pGame->m_pClientList[iClientH]->m_status.bInhibitionCasting) && (bItemEffect != true)) {
		m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::MagicFailed, -1, 0);
		return;
	}

	if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1) {
		wWeaponType = m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType;
		if ((wWeaponType >= 34) && (wWeaponType <= 39)) {
		}
		else return;
	}

	if ((m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::LeftHand)] != -1) ||
		(m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1)) return;

	// Reject spell if the cast was interrupted by damage (sentinel value -1)
	if ((bItemEffect == false) && (m_pGame->m_pClientList[iClientH]->m_iSpellCount == -1)) {
		m_pGame->m_pClientList[iClientH]->m_iSpellCount = 0;
		return;
	}

	if ((m_pGame->m_pClientList[iClientH]->m_iSpellCount > 1) && (bItemEffect == false)) {
		try
		{
			hb::logger::warn<log_channel::security>("Spell hack: IP={} player={}, casting without precast", m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName);
			m_pGame->DeleteClient(iClientH, true, true);
		}
		catch (...)
		{
		}
		return;
	}

	if (m_pGame->m_pClientList[iClientH]->m_bInhibition) {
		m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::MagicFailed, -1, 0);
		return;
	}

	/*if (((m_pGame->m_pClientList[iClientH]->m_iUninteruptibleCheck - (m_pGame->iGetMaxHP(iClientH)/10)) > (m_pGame->m_pClientList[iClientH]->m_iHP)) && (m_pGame->m_pClientList[iClientH]->m_bMagicItem == false)) {
		m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::Magic, 0,
			0, 0, 0, 0, 0, 0);
		return;
	}*/

	if (m_pGame->m_pMagicConfigList[sType]->m_sType == 32) { // Invisiblity
		sEqStatus = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
		if ((sEqStatus != -1) && (m_pGame->m_pClientList[iClientH]->m_pItemList[sEqStatus] != 0)) {
			if ((m_pGame->m_pClientList[iClientH]->m_pItemList[sEqStatus]->m_sIDnum == 865) || (m_pGame->m_pClientList[iClientH]->m_pItemList[sEqStatus]->m_sIDnum == 866)) {
				bItemEffect = true;
			}
		}
	}

	sX = m_pGame->m_pClientList[iClientH]->m_sX;
	sY = m_pGame->m_pClientList[iClientH]->m_sY;

	sMagicCircle = (sType / 10) + 1;
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] == 0)
		dV1 = 1.0f;
	else dV1 = (double)m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4];

	if (bItemEffect) dV1 = (double)100.0f;
	dV2 = (double)(dV1 / 100.0f);
	dV3 = (double)_tmp_iMCProb[sMagicCircle];
	dV1 = dV2 * dV3;
	iResult = (int)dV1;

	if ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) > 50)
		iResult += ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) - 50) / 2;

	sLevelMagic = (m_pGame->m_pClientList[iClientH]->m_iLevel / 10);
	if (sMagicCircle != sLevelMagic) {
		if (sMagicCircle > sLevelMagic) {
			dV1 = (double)(m_pGame->m_pClientList[iClientH]->m_iLevel - sLevelMagic * 10);
			dV2 = (double)abs(sMagicCircle - sLevelMagic) * _tmp_iMLevelPenalty[sMagicCircle];
			dV3 = (double)abs(sMagicCircle - sLevelMagic) * 10;
			dV4 = (dV1 / dV3) * dV2;
			iResult -= abs(abs(sMagicCircle - sLevelMagic) * _tmp_iMLevelPenalty[sMagicCircle] - (int)dV4);
		}
		else {
			iResult += 5 * abs(sMagicCircle - sLevelMagic);
		}
	}

	switch (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cWhetherStatus) {
	case 0: break;
	case 1: iResult = iResult - (iResult / 24); break;
	case 2:	iResult = iResult - (iResult / 12); break;
	case 3: iResult = iResult - (iResult / 5);  break;
	}

	if (m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectType == 10) {
		dV1 = (double)iResult;
		dV2 = (double)(m_pGame->m_pClientList[iClientH]->m_iSpecialWeaponEffectValue * 3);
		dV3 = dV1 + dV2;
		iResult = (int)dV3;
	}

	if (iResult <= 0) iResult = 1;

	iWhetherBonus = iGetWhetherMagicBonusEffect(sType, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cWhetherStatus);

	iManaCost = m_pGame->m_pMagicConfigList[sType]->m_sValue1;
	if ((m_pGame->m_pClientList[iClientH]->m_bIsSafeAttackMode) &&
		(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsFightZone == false)) {
		iManaCost += (iManaCost / 2) - (iManaCost / 10);
	}

	if (m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio > 0) {
		dV1 = (double)m_pGame->m_pClientList[iClientH]->m_iManaSaveRatio;
		dV2 = (double)(dV1 / 100.0f);
		dV3 = (double)iManaCost;
		dV1 = dV2 * dV3;
		dV2 = dV3 - dV1;
		iManaCost = (int)dV2;

		if (iManaCost <= 0) iManaCost = 1;
	}

	wWeaponType = m_pGame->m_pClientList[iClientH]->m_appearance.iWeaponType;
	if (wWeaponType == 34) {
		iManaCost += 20;
	}

	if (iResult < 100) {
		iDiceRes = m_pGame->iDice(1, 100);
		if (iResult < iDiceRes) {
			m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::MagicFailed, -1, 0);
			return;
		}
	}

	if (((m_pGame->m_pClientList[iClientH]->m_iHungerStatus <= 10) || (m_pGame->m_pClientList[iClientH]->m_iSP <= 0)) && (m_pGame->iDice(1, 1000) <= 100)) {
		m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, Sentinel::MagicFailed, -1, 0);
		return;
	}

	if (m_pGame->m_pClientList[iClientH]->m_iMP < iManaCost) {
		return;
	}

	iResult = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4];
	if ((m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) > 50) iResult += ((m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) - 50);

	sLevelMagic = (m_pGame->m_pClientList[iClientH]->m_iLevel / 10);
	if (sMagicCircle != sLevelMagic) {
		if (sMagicCircle > sLevelMagic) {
			dV1 = (double)(m_pGame->m_pClientList[iClientH]->m_iLevel - sLevelMagic * 10);
			dV2 = (double)abs(sMagicCircle - sLevelMagic) * _tmp_iMLevelPenalty[sMagicCircle];
			dV3 = (double)abs(sMagicCircle - sLevelMagic) * 10;
			dV4 = (dV1 / dV3) * dV2;

			iResult -= abs(abs(sMagicCircle - sLevelMagic) * _tmp_iMLevelPenalty[sMagicCircle] - (int)dV4);
		}
		else {
			iResult += 5 * abs(sMagicCircle - sLevelMagic);
		}
	}

	iResult += m_pGame->m_pClientList[iClientH]->m_iAddAR;
	if (iResult <= 0) iResult = 1;

	if (sType >= 80) iResult += 10000;

	if (m_pGame->m_pMagicConfigList[sType]->m_sType == 28) {
		iResult += 10000;
	}

	if (m_pGame->m_pMagicConfigList[sType]->m_cCategory == 1) {
		if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->iGetAttribute(sX, sY, 0x00000005) != 0) return;
	}

	iMagicAttr = m_pGame->m_pMagicConfigList[sType]->m_iAttribute;
	if (m_pGame->m_pClientList[iClientH]->m_status.bInvisibility) {
		m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(iClientH, hb::shared::owner_class::Player, false);
		m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(iClientH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
		m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
	}

	m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
	if ((m_pGame->m_bIsCrusadeMode == false) && (cOwnerType == hb::shared::owner_class::Player)) {
		if ((m_pGame->m_pClientList[iClientH]->m_bIsPlayerCivil != true) && (m_pGame->m_pClientList[sOwnerH]->m_bIsPlayerCivil)) {
			if (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[sOwnerH]->m_cSide) return;
		}
		else if ((m_pGame->m_pClientList[iClientH]->m_bIsPlayerCivil) && (m_pGame->m_pClientList[sOwnerH]->m_bIsPlayerCivil == false)) {
			switch (m_pGame->m_pMagicConfigList[sType]->m_sType) {
			case 1:  // hb::shared::magic::DamageSpot
			case 4:  // hb::shared::magic::SpDownSpot 4
			case 8:  // hb::shared::magic::Teleport 8
			case 10: // hb::shared::magic::Create 10
			case 11: // hb::shared::magic::Protect 11
			case 12: // hb::shared::magic::HoldObject 12
			case 16: // hb::shared::magic::Confuse
			case 17: // hb::shared::magic::Poison
			case 32: // hb::shared::magic::Resurrection
			case hb::shared::magic::Haste:
				return;
			}
		}
	}

	if (m_pGame->m_pMagicConfigList[sType]->m_dwDelayTime == 0) {
		switch (m_pGame->m_pMagicConfigList[sType]->m_sType) {
		case hb::shared::magic::Haste:
			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1:
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (sOwnerH == iClientH) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Haste] != 0) goto MAGIC_NOEFFECT;
					m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Haste] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pStatusEffectManager->SetHasteFlag(sOwnerH, cOwnerType, true);
					break;

				case hb::shared::owner_class::Npc:
					goto MAGIC_NOEFFECT;
					break;
				}
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Haste, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				if (cOwnerType == hb::shared::owner_class::Player)
					m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Haste, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
				break;
			}
			break;
		case hb::shared::magic::DamageSpot:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) && (m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
			}
			break;

		case hb::shared::magic::HpUpSpot:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			m_pGame->m_pCombatManager->Effect_HpUp_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6);
			break;

		case hb::shared::magic::DamageArea:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) && (m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
			}

			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) && (m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					}
				}
			break;

		case hb::shared::magic::SpDownSpot:
			break;

		case hb::shared::magic::SpDownArea:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
				m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6);
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
				}
			break;

		case hb::shared::magic::Polymorph:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (1) { // m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Polymorph] != 0) goto MAGIC_NOEFFECT;
					m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Polymorph] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pClientList[sOwnerH]->m_sOriginalType = m_pGame->m_pClientList[sOwnerH]->m_sType;
					m_pGame->m_pClientList[sOwnerH]->m_sType = 18;
					m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Polymorph] != 0) goto MAGIC_NOEFFECT;
					m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Polymorph] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pNpcList[sOwnerH]->m_sOriginalType = m_pGame->m_pNpcList[sOwnerH]->m_sType;
					m_pGame->m_pNpcList[sOwnerH]->m_sType = 18;
					m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
					break;
				}

				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Polymorph, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				if (cOwnerType == hb::shared::owner_class::Player)
					m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Polymorph, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
			}
			break;

			// 05/20/2004 - Hypnotoad - Cancellation
		case hb::shared::magic::Cancellation:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) && (m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {

				// Removes Invisibility Flag 0x0010
				m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sOwnerH, cOwnerType, false);

				// Removes Illusion Flag 0x01000000
				m_pGame->m_pStatusEffectManager->SetIllusionFlag(sOwnerH, cOwnerType, false);

				// Removes Defense Shield Flag 0x02000000
				// Removes Great Defense Shield Flag 0x02000000
				m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(sOwnerH, cOwnerType, false);

				// Removes Absolute Magic Protection Flag 0x04000000	
				// Removes Protection From Magic Flag 0x04000000
				m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(sOwnerH, cOwnerType, false);

				// Removes Protection From Arrow Flag 0x08000000
				m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(sOwnerH, cOwnerType, false);

				// Removes Illusion Movement Flag 0x00200000
				m_pGame->m_pStatusEffectManager->SetIllusionMovementFlag(sOwnerH, cOwnerType, false);

				// Removes Berserk Flag 0x0020
				m_pGame->m_pStatusEffectManager->SetBerserkFlag(sOwnerH, cOwnerType, false);

				//Removes ice-added 
				m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, false);

				//Remove paralyse

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Ice);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::HoldObject, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Inhibition);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Inhibition, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Invisibility, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Berserk);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Berserk, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Protect);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Protect, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Confuse);
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Confuse, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				// Update Client
				m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
			}
			break;

		case hb::shared::magic::DamageAreaNoSpotSpDown:
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, false, iMagicAttr);
						m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
					}

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, false, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
						}
					}
				}
			break;

		case hb::shared::magic::DamageLinear:
			sX = m_pGame->m_pClientList[iClientH]->m_sX;
			sY = m_pGame->m_pClientList[iClientH]->m_sY;

			for(int i = 2; i < 10; i++) {
				iErr = 0;
				CMisc::GetPoint2(sX, sY, dX, dY, &tX, &tY, &iErr, i);

				// tx, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
				}

				// tx-1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
				}

				// tx+1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
				}

				// tx, ty-1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
				}

				// tx, ty+1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
				}

				if ((abs(tX - dX) <= 1) && (abs(tY - dY) <= 1)) break;
			}

			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					}
				}

			// dX, dY
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
				(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false
			}
			break;

		case hb::shared::magic::IceLinear:
			sX = m_pGame->m_pClientList[iClientH]->m_sX;
			sY = m_pGame->m_pClientList[iClientH]->m_sY;

			for(int i = 2; i < 10; i++) {
				iErr = 0;
				CMisc::GetPoint2(sX, sY, dX, dY, &tX, &tY, &iErr, i);

				// tx, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pClientList[sOwnerH]->m_iHP < 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}
				}

				// tx-1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}
				}

				// tx+1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}
				}

				// tx, ty-1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}
				}

				// tx, ty+1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}
				}

				if ((abs(tX - dX) <= 1) && (abs(tY - dY) <= 1)) break;
			}

			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}
					}

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
							switch (cOwnerType) {
							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
									if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
										m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
										m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
										m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
											sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
									}
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
									if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
										m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
										m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
										m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
											sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									}
								}
								break;
							}
						}
					}
				}

			// dX, dY
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
						if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
							m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
							m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
							m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
								sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
						}
					}
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
						if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
							m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
							m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
							m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
								sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
						}
					}
					break;
				}
			}

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
				(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
							}
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
							if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
								m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
								m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
							}
						}
						break;
					}
				}
			}
			break;

		case hb::shared::magic::Inhibition:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			switch (cOwnerType) {
			case hb::shared::owner_class::Player:
				if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
				if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Inhibition] != 0) goto MAGIC_NOEFFECT;
				if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Protect] == 5) goto MAGIC_NOEFFECT;
				if (m_pGame->m_pClientList[iClientH]->m_cSide == m_pGame->m_pClientList[sOwnerH]->m_cSide) goto MAGIC_NOEFFECT;
				if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;

				m_pGame->m_pClientList[sOwnerH]->m_bInhibition = true;
				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Inhibition, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
				break;
			}
			break;

		case hb::shared::magic::Tremor:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
				(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
			}

			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					}
				}
			break;

		case hb::shared::magic::DamageAreaNoSpot:
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					}
				}
			break;

		case hb::shared::magic::SpUpArea:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			m_pGame->m_pCombatManager->Effect_SpUp_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6);
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					m_pGame->m_pCombatManager->Effect_SpUp_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
				}
			break;

		case hb::shared::magic::DamageLinearSpDown:
			sX = m_pGame->m_pClientList[iClientH]->m_sX;
			sY = m_pGame->m_pClientList[iClientH]->m_sY;

			for(int i = 2; i < 10; i++) {
				iErr = 0;
				CMisc::GetPoint2(sX, sY, dX, dY, &tX, &tY, &iErr, i);

				// tx, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}
				}

				// tx-1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {

							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX - 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}
				}

				// tx+1, ty
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX + 1, tY);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}
				}

				// tx, ty-1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY - 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}
				}

				// tx, ty+1
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, sX, sY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;
					}
				}

				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, tX, tY + 1);
				if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
					(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}
				}

				if ((abs(tX - dX) <= 1) && (abs(tY - dY) <= 1)) break;
			}

			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
								m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
								m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
								m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
							}
							break;
						}
					}

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
							switch (cOwnerType) {
							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
									m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
									m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
									m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
									m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
									m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
									m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
								}
								break;
							}
						}
					}
				}

			// dX, dY
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
				m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
						m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
						m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
					}
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
						m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
						m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
					}
					break;
				}
			}

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
				(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
				if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
					m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr); // v1.41 false
					switch (cOwnerType) {
					case hb::shared::owner_class::Player:
						if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
						break;

					case hb::shared::owner_class::Npc:
						if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_SpDown_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);

						}
						break;
					}
				}
			}
			break;

		case hb::shared::magic::Teleport:
			// . sValue 4    .
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1:
				// . Recall.
				if ((cOwnerType == hb::shared::owner_class::Player) && (sOwnerH == iClientH)) {
					// Recall  .
					m_pGame->RequestTeleportHandler(iClientH, "1   ");
				}
				break;
			}
			break;

		case hb::shared::magic::Summon:

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsFightZone) return;

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			// Owner Master .
			if ((sOwnerH != 0) && (cOwnerType == hb::shared::owner_class::Player)) {
				// Master       .
				iFollowersNum = m_pGame->iGetFollowerNumber(sOwnerH, cOwnerType);

				// Casting  Magery/20     .
				if (iFollowersNum >= (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] / 20)) break;

				iNamingValue = m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->iGetEmptyNamingValue();
				if (iNamingValue == -1) {
					// NPC  .     .
				}
				else {
					// NPC .
					std::memset(cName, 0, sizeof(cName));
					std::snprintf(cName, sizeof(cName), "XX%d", iNamingValue);
					cName[0] = '_';
					cName[1] = m_pGame->m_pClientList[iClientH]->m_cMapIndex + 65;

					// Magery     .
					std::memset(cNpcName, 0, sizeof(cNpcName));

					switch (iV1) {
					case 0:
						iResult = m_pGame->iDice(1, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] / 10);

						if (iResult < m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] / 20)
							iResult = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] / 20;

						switch (iResult) {
						case 1: strcpy(cNpcName, "Slime"); break;
						case 2: strcpy(cNpcName, "Giant-Ant"); break;
						case 3: strcpy(cNpcName, "Amphis"); break;
						case 4: strcpy(cNpcName, "Orc"); break;
						case 5: strcpy(cNpcName, "Skeleton"); break;
						case 6:	strcpy(cNpcName, "Clay-Golem"); break;
						case 7:	strcpy(cNpcName, "Stone-Golem"); break;
						case 8: strcpy(cNpcName, "Orc-Mage"); break;
						case 9:	strcpy(cNpcName, "Hellbound"); break;
						case 10:strcpy(cNpcName, "Cyclops"); break;
						}
						break;

					case 1:	strcpy(cNpcName, "Orc"); break;
					case 2: strcpy(cNpcName, "Skeleton"); break;
					case 3: strcpy(cNpcName, "Clay-Golem"); break;
					case 4: strcpy(cNpcName, "Stone-Golem"); break;
					case 5: strcpy(cNpcName, "Hellbound"); break;
					case 6: strcpy(cNpcName, "Cyclops"); break;
					case 7: strcpy(cNpcName, "Troll"); break;
					case 8: strcpy(cNpcName, "Orge"); break;
					}

					int iNpcConfigId = m_pGame->GetNpcConfigIdByName(cNpcName);
					if (m_pGame->bCreateNewNpc(iNpcConfigId, cName, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, 0, 0, MoveType::Random, &dX, &dY, cNpcWaypoint, 0, 0, m_pGame->m_pClientList[iClientH]->m_cSide, false, true) == false) {
						// NameValue .
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetNamingValueEmpty(iNamingValue);
					}
					else {
						std::memset(cName_Master, 0, sizeof(cName_Master));
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							memcpy(cName_Master, m_pGame->m_pClientList[sOwnerH]->m_cCharName, hb::shared::limits::CharNameLen - 1);
							break;
						case hb::shared::owner_class::Npc:
							memcpy(cName_Master, m_pGame->m_pNpcList[sOwnerH]->m_cName, 5);
							break;
						}
						if (m_pGame->m_pEntityManager != 0) m_pGame->m_pEntityManager->bSetNpcFollowMode(cName, cName_Master, cOwnerType);
					}
				}
			}
			break;

		case hb::shared::magic::Create:

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetIsMoveAllowedTile(dX, dY) == false)
				goto MAGIC_NOEFFECT;

			pItem = new CItem;

			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1:
				// Food
				if (m_pGame->iDice(1, 2) == 1)
					std::snprintf(cItemName, sizeof(cItemName), "Meat");
				else std::snprintf(cItemName, sizeof(cItemName), "Baguette");
				break;
			}

			m_pGame->m_pItemManager->_bInitItemAttr(pItem, cItemName);

			pItem->SetTouchEffectType(TouchEffectType::ID);
			pItem->m_sTouchEffectValue1 = static_cast<short>(m_pGame->iDice(1, 100000));
			pItem->m_sTouchEffectValue2 = static_cast<short>(m_pGame->iDice(1, 100000));
			pItem->m_sTouchEffectValue3 = (short)GameClock::GetTimeMS();

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(dX, dY, pItem);

			m_pGame->m_pItemManager->_bItemLog(ItemLogAction::Drop, iClientH, (int)-1, pItem);

			m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::ItemDrop, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
				dX, dY, pItem->m_sIDnum, 0, pItem->m_cItemColor, pItem->m_dwAttribute); // v1.4 color
			break;

		case hb::shared::magic::Protect:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

			switch (cOwnerType) {
			case hb::shared::owner_class::Player:
				if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
				if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Protect] != 0) goto MAGIC_NOEFFECT;
				if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;

				m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Protect] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
				switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
				case 1:
					m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(sOwnerH, hb::shared::owner_class::Player, true);
					break;
				case 2:
				case 5:
					m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(sOwnerH, hb::shared::owner_class::Player, true);
					break;
				case 3:
				case 4:
					m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(sOwnerH, hb::shared::owner_class::Player, true);
					break;
				}
				break;

			case hb::shared::owner_class::Npc:
				if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
				if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Protect] != 0) goto MAGIC_NOEFFECT;
				// NPC    .
				if (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit != 0) goto MAGIC_NOEFFECT;
				m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Protect] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;

				switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
				case 1:
					m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(sOwnerH, hb::shared::owner_class::Npc, true);
					break;
				case 2:
				case 5:
					m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(sOwnerH, hb::shared::owner_class::Npc, true);
					break;
				case 3:
				case 4:
					m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(sOwnerH, hb::shared::owner_class::Npc, true);
					break;
				}
				break;
			}

			m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Protect, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
				sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

			if (cOwnerType == hb::shared::owner_class::Player)
				m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Protect, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
			break;

		case hb::shared::magic::Scan:
			std::memset(cScanMessage, 0, sizeof(cScanMessage));
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					std::snprintf(cScanMessage, sizeof(cScanMessage), " Player: %s HP:%d MP:%d.", m_pGame->m_pClientList[sOwnerH]->m_cCharName, m_pGame->m_pClientList[sOwnerH]->m_iHP, m_pGame->m_pClientList[sOwnerH]->m_iMP);
					m_pGame->ShowClientMsg(iClientH, cScanMessage);
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					std::snprintf(cScanMessage, sizeof(cScanMessage), " NPC: %s HP:%d MP:%d", m_pGame->m_pNpcList[sOwnerH]->m_cNpcName, m_pGame->m_pNpcList[sOwnerH]->m_iHP, m_pGame->m_pNpcList[sOwnerH]->m_iMana);
					m_pGame->ShowClientMsg(iClientH, cScanMessage);
					break;
				}
				m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::Magic, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
					m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, dX, dY, 10, (short)10);
			}
			break;

		case hb::shared::magic::HoldObject:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {

				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_iAddPR >= 500) goto MAGIC_NOEFFECT;
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;
					// 2002-09-10 #2 (No-Attack-Area)
					if (cOwnerType == hb::shared::owner_class::Player) {

						if (m_pGame->m_pMapList[m_pGame->m_pClientList[sOwnerH]->m_cMapIndex]->iGetAttribute(sX, sY, 0x00000006) != 0) goto MAGIC_NOEFFECT;
						if (m_pGame->m_pMapList[m_pGame->m_pClientList[sOwnerH]->m_cMapIndex]->iGetAttribute(dX, dY, 0x00000006) != 0) goto MAGIC_NOEFFECT;
					}

					// 2002-09-10 #3
					if (strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "middleland") != 0 &&
						m_pGame->m_bIsCrusadeMode == false &&
						m_pGame->m_pClientList[iClientH]->m_cSide == m_pGame->m_pClientList[sOwnerH]->m_cSide)
						goto MAGIC_NOEFFECT;

					m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicLevel >= 6) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) goto MAGIC_NOEFFECT;
					m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					break;
				}

				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::HoldObject, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				if (cOwnerType == hb::shared::owner_class::Player)
					m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::HoldObject, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
			}
			break;

		case hb::shared::magic::Invisibility:
			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1:
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if ((sOwnerH != iClientH) && ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "elvhunter", 9) == 0) || (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "arehunter", 9) == 0)) && ((memcmp(m_pGame->m_pClientList[sOwnerH]->m_cLocation, "elvhunter", 9) != 0) || (memcmp(m_pGame->m_pClientList[sOwnerH]->m_cLocation, "arehunter", 9) != 0))) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] != 0) goto MAGIC_NOEFFECT;
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;

					m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sOwnerH, cOwnerType, true);
					m_pGame->m_pCombatManager->RemoveFromTarget(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::Invisibility);
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] != 0) goto MAGIC_NOEFFECT;

					if (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit == 0) {
						// NPC     .
						m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
						m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sOwnerH, cOwnerType, true);
						// NPC    .
						m_pGame->m_pCombatManager->RemoveFromTarget(sOwnerH, hb::shared::owner_class::Npc, hb::shared::magic::Invisibility);
					}
					break;
				}

				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Invisibility, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				if (cOwnerType == hb::shared::owner_class::Player)
					m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Invisibility, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
				break;

			case 2:
				if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;
				if ((memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "elvhunter", 9) == 0) || (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "arehunter", 9) == 0)) goto MAGIC_NOEFFECT;

				// dX, dY  8  Invisibility  Object   .
				for(int ix = dX - 8; ix <= dX + 8; ix++)
					for(int iy = dY - 8; iy <= dY + 8; iy++) {
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (sOwnerH != 0) {
							switch (cOwnerType) {
							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] != 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
									m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sOwnerH, cOwnerType, false);
									m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, cOwnerType, hb::shared::magic::Invisibility);
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] != 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] = 0;
									m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(sOwnerH, cOwnerType, false);
									m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, cOwnerType, hb::shared::magic::Invisibility);
								}
								break;
							}
						}
					}
				break;
			}
			break;

		case hb::shared::magic::CreateDynamic:
			// Dynamic Object    .

			if (m_pGame->m_bIsCrusadeMode == false) {
				if (strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "aresden") == 0) return;
				if (strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "elvine") == 0) return;
				// v2.14
				if (strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "arefarm") == 0) return;
				if (strcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, "elvfarm") == 0) return;
			}

			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue10) {
			case dynamic_object::PCloudBegin:

			case dynamic_object::Fire:   // Fire .
			case dynamic_object::Spike:  // Spike

				switch (m_pGame->m_pMagicConfigList[sType]->m_sValue11) {
				case 1:
					// wall - type
					cDir = CMisc::cGetNextMoveDir(m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, dX, dY);
					switch (cDir) {
					case 1:	rx = 1; ry = 0;   break;
					case 2: rx = 1; ry = 1;   break;
					case 3: rx = 0; ry = 1;   break;
					case 4: rx = -1; ry = 1;  break;
					case 5: rx = 1; ry = 0;   break;
					case 6: rx = -1; ry = -1; break;
					case 7: rx = 0; ry = -1;  break;
					case 8: rx = 1; ry = -1;  break;
					}

					m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(iClientH, hb::shared::owner_class::PlayerIndirect, m_pGame->m_pMagicConfigList[sType]->m_sValue10, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
						dX, dY, m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000);

					m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, dX, dY);

					for(int i = 1; i <= m_pGame->m_pMagicConfigList[sType]->m_sValue12; i++) {
						m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(iClientH, hb::shared::owner_class::PlayerIndirect, m_pGame->m_pMagicConfigList[sType]->m_sValue10, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
							dX + i * rx, dY + i * ry, m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000);
						m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, dX + i * rx, dY + i * ry);

						m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(iClientH, hb::shared::owner_class::PlayerIndirect, m_pGame->m_pMagicConfigList[sType]->m_sValue10, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
							dX - i * rx, dY - i * ry, m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000);
						m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, dX - i * rx, dY - i * ry);
					}
					break;

				case 2:
					// Field - Type
					bool bFlag = false;
					int cx, cy;
					for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue12; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue12; ix++)
						for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue12; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue12; iy++) {
							m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(iClientH, hb::shared::owner_class::PlayerIndirect, m_pGame->m_pMagicConfigList[sType]->m_sValue10, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
								ix, iy, m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000, m_pGame->m_pMagicConfigList[sType]->m_sValue5);

							if (m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, ix, iy, true)) {
								bFlag = true;
								cx = ix;
								cy = iy;
							}
						}
					if (bFlag) m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, cx, cy);
					break;
				}
				break;

			case dynamic_object::IceStorm:
				// Ice-Storm Dynamic Object 
				m_pGame->m_pDynamicObjectManager->iAddDynamicObjectList(iClientH, hb::shared::owner_class::PlayerIndirect, m_pGame->m_pMagicConfigList[sType]->m_sValue10, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
					dX, dY, m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000,
					m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4]);
				break;

			default:
				break;
			}
			break;

		case hb::shared::magic::Possession:
			if (m_pGame->m_pClientList[iClientH]->m_cSide == 0) goto MAGIC_NOEFFECT;

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);
			if (sOwnerH != 0) break;

			pItem = m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->pGetItem(dX, dY, &sIDNum, &cRemainItemColor, &dwAttr);
			if (pItem != 0) {
				if (m_pGame->m_pItemManager->_bAddClientItemList(iClientH, pItem, &iEraseReq)) {

					m_pGame->m_pItemManager->_bItemLog(ItemLogAction::Get, iClientH, (int)-1, pItem);

					iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);

					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(iClientH, true, true);
						return;
					}
				}
				else
				{

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bSetItem(dX, dY, pItem);

					iRet = m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

					switch (iRet) {
					case sock::Event::QueueFull:
					case sock::Event::SocketError:
					case sock::Event::CriticalError:
					case sock::Event::SocketClosed:
						m_pGame->DeleteClient(iClientH, true, true);
						return;
					}
				}
			}
			break;

		case hb::shared::magic::Confuse:
			// if the caster side is the same as the targets side, no effect occurs
			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1: // confuse Language.
			case 2: // Confusion, Mass Confusion 	
				for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
					for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (cOwnerType == hb::shared::owner_class::Player) {
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) && (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[sOwnerH]->m_cSide)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] != 0) break; // Confuse  .
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;

								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Confuse, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Confuse, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
							}
						}
					}
				break;

			case 3: // Ilusion, Mass-Ilusion
				for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
					for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (cOwnerType == hb::shared::owner_class::Player) {
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) && (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[sOwnerH]->m_cSide)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] != 0) break; // Confuse  .
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;

								switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
								case 3:
									m_pGame->m_pStatusEffectManager->SetIllusionFlag(sOwnerH, hb::shared::owner_class::Player, true);
									break;
								}

								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Confuse, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Confuse, m_pGame->m_pMagicConfigList[sType]->m_sValue4, iClientH, 0);
							}
						}
					}
				break;

			case 4: // Ilusion Movement
				if (m_pGame->m_pClientList[iClientH]->m_cMagicEffectStatus[hb::shared::magic::Invisibility] != 0) break;
				for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
					for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
						m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (cOwnerType == hb::shared::owner_class::Player) {
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) && (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pClientList[sOwnerH]->m_cSide)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] != 0) break;
								m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Confuse] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
								switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
								case 4:
									m_pGame->m_pStatusEffectManager->SetIllusionMovementFlag(sOwnerH, hb::shared::owner_class::Player, true);
									break;
								}

								m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Confuse, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
									sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

								m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Confuse, m_pGame->m_pMagicConfigList[sType]->m_sValue4, iClientH, 0);
							}
						}
					}
			}
			break;

		case hb::shared::magic::Poison:
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

			if (m_pGame->m_pMagicConfigList[sType]->m_sValue4 == 1) {
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) goto MAGIC_NOEFFECT;

					m_pGame->m_pCombatManager->bAnalyzeCriminalAction(iClientH, dX, dY);

					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						if (m_pGame->m_pCombatManager->bCheckResistingPoisonSuccess(sOwnerH, cOwnerType) == false) {
							m_pGame->m_pClientList[sOwnerH]->m_bIsPoisoned = true;
							m_pGame->m_pClientList[sOwnerH]->m_iPoisonLevel = m_pGame->m_pMagicConfigList[sType]->m_sValue5;
							m_pGame->m_pClientList[sOwnerH]->m_dwPoisonTime = dwTime;
							// 05/06/2004 - Hypnotoad - poison aura appears when cast Poison
							m_pGame->m_pStatusEffectManager->SetPoisonFlag(sOwnerH, cOwnerType, true);
							m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Poison, m_pGame->m_pMagicConfigList[sType]->m_sValue5, 0, 0);

						}
					}
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						if (m_pGame->m_pCombatManager->bCheckResistingPoisonSuccess(sOwnerH, cOwnerType) == false) {

						}
					}
					break;
				}
			}
			else if (m_pGame->m_pMagicConfigList[sType]->m_sValue4 == 0) {
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;

					if (m_pGame->m_pClientList[sOwnerH]->m_bIsPoisoned) {
						m_pGame->m_pClientList[sOwnerH]->m_bIsPoisoned = false;
						// 05/06/2004 - Hypnotoad - poison aura removed when cure cast
						m_pGame->m_pStatusEffectManager->SetPoisonFlag(sOwnerH, cOwnerType, false);
						m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOff, hb::shared::magic::Poison, 0, 0, 0);
					}
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					break;
				}
			}
			break;

		case hb::shared::magic::Berserk:
			switch (m_pGame->m_pMagicConfigList[sType]->m_sValue4) {
			case 1:
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, dX, dY);

				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] != 0) goto MAGIC_NOEFFECT;
					m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pStatusEffectManager->SetBerserkFlag(sOwnerH, cOwnerType, true);
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] != 0) goto MAGIC_NOEFFECT;
					if (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit != 0) goto MAGIC_NOEFFECT;
					// 2002-09-11 #3
					if (m_pGame->m_pClientList[iClientH]->m_cSide != m_pGame->m_pNpcList[sOwnerH]->m_cSide) goto MAGIC_NOEFFECT;

					m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Berserk] = (char)m_pGame->m_pMagicConfigList[sType]->m_sValue4;
					m_pGame->m_pStatusEffectManager->SetBerserkFlag(sOwnerH, cOwnerType, true);
					break;
				}

				m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Berserk, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_dwLastTime * 1000),
					sOwnerH, cOwnerType, 0, 0, 0, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);

				if (cOwnerType == hb::shared::owner_class::Player)
					m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Berserk, m_pGame->m_pMagicConfigList[sType]->m_sValue4, 0, 0);
				break;
			}
			break;

		case hb::shared::magic::DamageAreaArmorBreak:
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {
					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
						m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
					}

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
							m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue7, m_pGame->m_pMagicConfigList[sType]->m_sValue8, m_pGame->m_pMagicConfigList[sType]->m_sValue9 + iWhetherBonus, false, iMagicAttr);
							m_pGame->m_pCombatManager->ArmorLifeDecrement(iClientH, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue10);
						}
					}
				}
			break;

			// Resurrection Magic. 
		case hb::shared::magic::Resurrection:
			// 10 Mins once
			if (m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime != 0) goto MAGIC_NOEFFECT;
			m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime = SpecialAbilityTimeSec / 2;
			// Get the ID of the dead Player/NPC on coords dX, dY. 
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
			switch (cOwnerType) {
				// For Player. 
			case hb::shared::owner_class::Player:
				// The Player has to exist. 
				if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
				// Resurrection is not for alive Players. 
				if (m_pGame->m_pClientList[sOwnerH]->m_bIsKilled == false) goto MAGIC_NOEFFECT;
				// Set Deadflag to Alive. 
				m_pGame->m_pClientList[sOwnerH]->m_bIsKilled = false;
				// Player's HP becomes half of the Max HP. 
				m_pGame->m_pClientList[sOwnerH]->m_iHP = ((m_pGame->m_pClientList[sOwnerH]->m_iLevel * 2) + (m_pGame->m_pClientList[sOwnerH]->m_iVit * 3) + ((m_pGame->m_pClientList[sOwnerH]->m_iStr + m_pGame->m_pClientList[sOwnerH]->m_iAngelicStr) / 2)) / 2;
				// Send new HP to Player. 
				m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);
				// Make Player stand up. (Currently, by a fake damage). 
				m_pGame->m_pMapList[m_pGame->m_pClientList[sOwnerH]->m_cMapIndex]->ClearDeadOwner(dX, dY);
				m_pGame->m_pMapList[m_pGame->m_pClientList[sOwnerH]->m_cMapIndex]->SetOwner(sOwnerH, hb::shared::owner_class::Player, dX, dY);
				m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Damage, 0, 0, 0);
				m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);
				break;
				// Resurrection is not for NPC's. 
			case hb::shared::owner_class::Npc:
				goto MAGIC_NOEFFECT;
				break;
			}
			break;

		case hb::shared::magic::Ice:
			for(int iy = dY - m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy <= dY + m_pGame->m_pMagicConfigList[sType]->m_sValue3; iy++)
				for(int ix = dX - m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix <= dX + m_pGame->m_pMagicConfigList[sType]->m_sValue2; ix++) {

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
					if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {
						//m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
						m_pGame->m_pCombatManager->Effect_Damage_Spot_DamageMove(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, dX, dY, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
						switch (cOwnerType) {
						case hb::shared::owner_class::Player:
							if (m_pGame->m_pClientList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
							break;

						case hb::shared::owner_class::Npc:
							if (m_pGame->m_pNpcList[sOwnerH] == 0) goto MAGIC_NOEFFECT;
							if ((m_pGame->m_pNpcList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
								}
							}
							break;
						}

					}

					m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
					if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
						(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
						if (m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false) {

							//m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							m_pGame->m_pCombatManager->Effect_Damage_Spot(iClientH, hb::shared::owner_class::Player, sOwnerH, cOwnerType, m_pGame->m_pMagicConfigList[sType]->m_sValue4, m_pGame->m_pMagicConfigList[sType]->m_sValue5, m_pGame->m_pMagicConfigList[sType]->m_sValue6 + iWhetherBonus, true, iMagicAttr);
							if ((m_pGame->m_pClientList[sOwnerH]->m_iHP > 0) && (m_pGame->m_pCombatManager->bCheckResistingIceSuccess(m_pGame->m_pClientList[iClientH]->m_cDir, sOwnerH, cOwnerType, iResult) == false)) {
								if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0) {
									m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
									m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
									m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (m_pGame->m_pMagicConfigList[sType]->m_sValue10 * 1000),
										sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);

									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
								}
							}
						}
					}
				}
			break;

		default:
			break;
		}
	}
	else {
		// Casting
		// Resurrection wand(MS.10) or Resurrection wand(MS.20)

		if (m_pGame->m_pMagicConfigList[sType]->m_sType == hb::shared::magic::Resurrection) {
			//Check if player has resurrection wand
			if (m_pGame->m_pClientList[iClientH] != 0 && m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime == 0 &&
				m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled == false) {
				m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, dX, dY);
				if (m_pGame->m_pClientList[sOwnerH] != 0) {
					// GM's can ressurect ne1, and players must be on same side to ressurect

					if (m_pGame->m_pClientList[sOwnerH]->m_cSide != m_pGame->m_pClientList[iClientH]->m_cSide) {
						return;
					}
					if (cOwnerType == hb::shared::owner_class::Player && m_pGame->m_pClientList[sOwnerH] != 0 &&
						m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
						m_pGame->m_pClientList[sOwnerH]->m_bIsBeingResurrected = true;
						m_pGame->SendNotifyMsg(0, sOwnerH, Notify::ResurrectPlayer, 0, 0, 0, 0);
						m_pGame->m_pClientList[iClientH]->m_bIsSpecialAbilityEnabled = true;
						m_pGame->m_pClientList[iClientH]->m_dwSpecialAbilityStartTime = dwTime;
						m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityLastSec = 0;
						m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityTime = m_pGame->m_pMagicConfigList[sType]->m_dwDelayTime;

						m_pGame->m_pClientList[iClientH]->m_appearance.iEffectType = 4;
						m_pGame->SendNotifyMsg(0, iClientH, Notify::SpecialAbilityStatus, 1, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityType, m_pGame->m_pClientList[iClientH]->m_iSpecialAbilityLastSec, 0);
						m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
					}
				}
			}
		}
	}

MAGIC_NOEFFECT:

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	//Mana Slate
	if (m_pGame->m_pClientList[iClientH]->m_status.bSlateMana) {
		iManaCost = 0;
	}

	// Mana  .
	m_pGame->m_pClientList[iClientH]->m_iMP -= iManaCost; // sValue1 Mana Cost
	if (m_pGame->m_pClientList[iClientH]->m_iMP < 0)
		m_pGame->m_pClientList[iClientH]->m_iMP = 0;

	m_pGame->m_pSkillManager->CalculateSSN_SkillIndex(iClientH, 4, 1);

	m_pGame->SendNotifyMsg(0, iClientH, Notify::Mp, 0, 0, 0, 0);

	// .  + 100
	m_pGame->SendEventToNearClient_TypeB(MsgId::EventCommon, CommonType::Magic, m_pGame->m_pClientList[iClientH]->m_cMapIndex,
		m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY, dX, dY, (sType + 100), m_pGame->m_pClientList[iClientH]->m_sType);

}

void MagicManager::RequestStudyMagicHandler(int iClientH, const char* pName, bool bIsPurchase)
{
	char cMagicName[31];
	uint32_t dwGoldCount;
	int iReqInt, iCost, iRet;
	bool bMagic = true;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	std::memset(cMagicName, 0, sizeof(cMagicName));
	memcpy(cMagicName, pName, 30);

	iRet = _iGetMagicNumber(cMagicName, &iReqInt, &iCost);
	if (iRet == -1) {

	}
	else {
		if (bIsPurchase) {
			if (m_pGame->m_pMagicConfigList[iRet]->m_iGoldCost < 0) bMagic = false;
			dwGoldCount = m_pGame->m_pItemManager->dwGetItemCountByID(iClientH, hb::shared::item::ItemId::Gold);
			if ((uint32_t)iCost > dwGoldCount)  bMagic = false;
		}
		//wizard remove
		//if (m_pGame->m_pClientList[iClientH]->m_bIsInsideWizardTower == false && bIsPurchase) return;
		if (m_pGame->m_pClientList[iClientH]->m_cMagicMastery[iRet] != 0) return;

		if ((iReqInt <= (m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt)) && (bMagic)) {

			if (bIsPurchase) m_pGame->m_pItemManager->SetItemCountByID(iClientH, hb::shared::item::ItemId::Gold, dwGoldCount - iCost);

			m_pGame->iCalcTotalWeight(iClientH);

			m_pGame->m_pClientList[iClientH]->m_cMagicMastery[iRet] = 1;

			{

				hb::net::PacketNotifyMagicStudySuccess pkt{};
				pkt.header.msg_id = MsgId::Notify;
				pkt.header.msg_type = Notify::MagicStudySuccess;
				pkt.magic_id = static_cast<uint8_t>(iRet);
				memcpy(pkt.magic_name, cMagicName, sizeof(pkt.magic_name));
				iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
			}

			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return;
			}
		}
		else {
			{

				hb::net::PacketNotifyMagicStudyFail pkt{};
				pkt.header.msg_id = MsgId::Notify;
				pkt.header.msg_type = Notify::MagicStudyFail;
				pkt.result = 1;
				pkt.magic_id = static_cast<uint8_t>(iRet);
				memcpy(pkt.magic_name, cMagicName, sizeof(pkt.magic_name));
				pkt.cost = static_cast<int32_t>(iCost);
				pkt.req_int = static_cast<int32_t>(iReqInt);
				iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(reinterpret_cast<char*>(&pkt), sizeof(pkt));
			}
			switch (iRet) {
			case sock::Event::QueueFull:
			case sock::Event::SocketError:
			case sock::Event::CriticalError:
			case sock::Event::SocketClosed:
				m_pGame->DeleteClient(iClientH, true, true);
				return;
			}
		}
	}
}

int MagicManager::_iGetMagicNumber(char* pMagicName, int* pReqInt, int* pCost)
{
	
	char cTmpName[31];

	std::memset(cTmpName, 0, sizeof(cTmpName));
	strcpy(cTmpName, pMagicName);

	for(int i = 0; i < hb::shared::limits::MaxMagicType; i++)
		if (m_pGame->m_pMagicConfigList[i] != 0) {
			if (memcmp(cTmpName, m_pGame->m_pMagicConfigList[i]->m_cName, 30) == 0) {
				*pReqInt = (int)m_pGame->m_pMagicConfigList[i]->m_sIntLimit;
				*pCost = (int)m_pGame->m_pMagicConfigList[i]->m_iGoldCost;

				return i;
			}
		}

	return -1;
}

bool MagicManager::bCheckMagicInt(int iClientH)
{

	for(int i = 0; i < hb::shared::limits::MaxMagicType; i++)
	{
		if (m_pGame->m_pMagicConfigList[i] != 0)
			if (m_pGame->m_pMagicConfigList[i]->m_sIntLimit > (m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt))
			{
				m_pGame->m_pClientList[iClientH]->m_cMagicMastery[i] = 0;
			}
	}

	return true;
}

int MagicManager::iGetWhetherMagicBonusEffect(short sType, char cWheatherStatus)
{
	int iWheatherBonus;

	iWheatherBonus = 0;
	switch (cWheatherStatus) {
	case 0: break;
	case 1:
	case 2:
	case 3:
		switch (sType) {
		case 10:
		case 37:
		case 43:
		case 51:
			iWheatherBonus = 1;
			break;

		case 20:
		case 30:
			iWheatherBonus = -1;
			break;
		}
		break;
	}
	return iWheatherBonus;
}

void MagicManager::GetMagicAbilityHandler(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] != 0) return;

	m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] = 20;
	m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 4, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4], 0, 0);
	m_pGame->m_pSkillManager->bCheckTotalSkillMasteryPoints(iClientH, 4);
}

bool MagicManager::bCheckClientMagicFrequency(int iClientH, uint32_t dwClientTime)
{
	uint32_t dwTimeGap;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;

	if (m_pGame->m_pClientList[iClientH]->m_dwMagicFreqTime == 0)
		m_pGame->m_pClientList[iClientH]->m_dwMagicFreqTime = dwClientTime;
	else {
		dwTimeGap = dwClientTime - m_pGame->m_pClientList[iClientH]->m_dwMagicFreqTime;
		m_pGame->m_pClientList[iClientH]->m_dwMagicFreqTime = dwClientTime;

		if ((dwTimeGap < 1500) && (m_pGame->m_pClientList[iClientH]->m_bMagicConfirm)) {
			try
			{
				hb::logger::warn<log_channel::security>("Speed cast: IP={} player={}, irregular casting rate", m_pGame->m_pClientList[iClientH]->m_cIPaddress, m_pGame->m_pClientList[iClientH]->m_cCharName);
				m_pGame->DeleteClient(iClientH, true, true);
			}
			catch (...)
			{
			}
			return false;
		}

		m_pGame->m_pClientList[iClientH]->m_iSpellCount--;
		if (m_pGame->m_pClientList[iClientH]->m_iSpellCount < 0)
			m_pGame->m_pClientList[iClientH]->m_iSpellCount = 0;
		m_pGame->m_pClientList[iClientH]->m_bMagicConfirm = false;
	}

	return false;
}

void MagicManager::ReloadMagicConfigs()
{
	sqlite3* configDb = nullptr;
	std::string configDbPath;
	bool configDbCreated = false;
	if (!EnsureGameConfigDatabase(&configDb, configDbPath, &configDbCreated) || configDbCreated)
	{
		hb::logger::log("Magic config reload failed: GameConfigs.db unavailable");
		return;
	}

	for(int i = 0; i < hb::shared::limits::MaxMagicType; i++)
	{
		if (m_pGame->m_pMagicConfigList[i] != 0)
		{
			delete m_pGame->m_pMagicConfigList[i];
			m_pGame->m_pMagicConfigList[i] = 0;
		}
	}

	if (!LoadMagicConfigs(configDb, m_pGame))
	{
		hb::logger::log("Magic config reload failed");
		CloseGameConfigDatabase(configDb);
		return;
	}

	CloseGameConfigDatabase(configDb);
	m_pGame->ComputeConfigHashes();
	hb::logger::log("Magic configs reloaded successfully");
}
