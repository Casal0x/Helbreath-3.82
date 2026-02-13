#include "SkillManager.h"
#include "Game.h"
#include "Item.h"
#include "ItemManager.h"
#include "MagicManager.h"
#include "EntityManager.h"
#include "DynamicObjectManager.h"
#include "CombatManager.h"
#include "FishingManager.h"
#include "Packet/SharedPackets.h"
#include "Skill.h"
#include "GameConfigSqliteStore.h"
#include "Log.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::net;
using namespace hb::server::config;
using namespace hb::shared::item;
namespace sock = hb::shared::net::socket;
using namespace hb::server::skill;

extern char G_cTxt[512];
extern char G_cData50000[50000];

static char _tmp_cCorpseX[] = { 0,  1, 1, 1, 0, -1, -1, -1, 0, 0, 0, 0 };
static char _tmp_cCorpseY[] = { -1, -1, 0, 1, 1,  1,  0, -1, 0, 0, 0 };

bool SkillManager::bSendClientSkillConfigs(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) {
		return false;
	}

	constexpr size_t maxPacketSize = 7000;
	constexpr size_t headerSize = sizeof(hb::net::PacketSkillConfigHeader);
	constexpr size_t entrySize = sizeof(hb::net::PacketSkillConfigEntry);
	constexpr size_t maxEntriesPerPacket = (maxPacketSize - headerSize) / entrySize;

	// Count total skills
	int totalSkills = 0;
	for(int i = 0; i < hb::shared::limits::MaxSkillType; i++) {
		if (m_pGame->m_pSkillConfigList[i] != 0) {
			totalSkills++;
		}
	}

	// Send skills in packets
	int skillsSent = 0;
	int packetIndex = 0;

	while (skillsSent < totalSkills) {
		std::memset(G_cData50000, 0, sizeof(G_cData50000));

		auto* pktHeader = reinterpret_cast<hb::net::PacketSkillConfigHeader*>(G_cData50000);
		pktHeader->header.msg_id = MsgId::SkillConfigContents;
		pktHeader->header.msg_type = MsgType::Confirm;
		pktHeader->totalSkills = static_cast<uint16_t>(totalSkills);
		pktHeader->packetIndex = static_cast<uint16_t>(packetIndex);

		auto* entries = reinterpret_cast<hb::net::PacketSkillConfigEntry*>(G_cData50000 + headerSize);

		uint16_t entriesInPacket = 0;
		int skipped = 0;

		for(int i = 0; i < hb::shared::limits::MaxSkillType && entriesInPacket < maxEntriesPerPacket; i++) {
			if (m_pGame->m_pSkillConfigList[i] == 0) {
				continue;
			}

			if (skipped < skillsSent) {
				skipped++;
				continue;
			}

			const CSkill* skill = m_pGame->m_pSkillConfigList[i];
			auto& entry = entries[entriesInPacket];

			entry.skillId = static_cast<int16_t>(i);
			std::memset(entry.name, 0, sizeof(entry.name));
			std::snprintf(entry.name, sizeof(entry.name), "%s", skill->m_cName);
			entry.isUseable = skill->m_bIsUseable ? 1 : 0;
			entry.useMethod = skill->m_cUseMethod;

			entriesInPacket++;
		}

		pktHeader->skillCount = entriesInPacket;
		size_t packetSize = headerSize + (entriesInPacket * entrySize);

		int iRet = m_pGame->m_pClientList[iClientH]->m_pXSock->iSendMsg(G_cData50000, static_cast<int>(packetSize));
		switch (iRet) {
		case sock::Event::QueueFull:
		case sock::Event::SocketError:
		case sock::Event::CriticalError:
		case sock::Event::SocketClosed:
			hb::logger::log("Failed to send skill configs: Client({}) Packet({})", iClientH, packetIndex);
			m_pGame->DeleteClient(iClientH, true, true);
			delete m_pGame->m_pClientList[iClientH];
			m_pGame->m_pClientList[iClientH] = 0;
			return false;
		}

		skillsSent += entriesInPacket;
		packetIndex++;
	}

	return true;
}

void SkillManager::TrainSkillResponse(bool bSuccess, int iClientH, int iSkillNum, int iSkillLevel)
{

	int   iRet;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((iSkillNum < 0) || (iSkillNum > 100)) return;
	if ((iSkillLevel < 0) || (iSkillLevel > 100)) return;

	if (bSuccess) {
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[iSkillNum] != 0) return;

		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[iSkillNum] = iSkillLevel;
		bCheckTotalSkillMasteryPoints(iClientH, iSkillNum);

		{

			hb::net::PacketNotifySkillTrainSuccess pkt{};
			pkt.header.msg_id = MsgId::Notify;
			pkt.header.msg_type = Notify::SkillTrainSuccess;
			pkt.skill_num = static_cast<uint8_t>(iSkillNum);
			pkt.skill_level = static_cast<uint8_t>(iSkillLevel);
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

	}

}

int SkillManager::_iCalcSkillSSNpoint(int iLevel)
{
	int iRet;

	if (iLevel < 1) return 1;

	if (iLevel <= 50)
		iRet = iLevel;
	else if (iLevel > 50) {
		iRet = (iLevel * 2);
	}

	return iRet;
}

void SkillManager::CalculateSSN_SkillIndex(int iClientH, short sSkillIndex, int iValue)
{
	int   iOldSSN, iSSNpoint, iWeaponIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;
	if ((sSkillIndex < 0) || (sSkillIndex >= hb::shared::limits::MaxSkillType)) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsKilled) return;

	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] == 0) return;

	iOldSSN = m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex];
	m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] += iValue;

	iSSNpoint = m_pGame->m_iSkillSSNpoint[m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] + 1];

	// SkillSSN   Skill .
	if ((m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] < 100) &&
		(m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] > iSSNpoint)) {

		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]++;
		// Skill .
		switch (sSkillIndex) {
		case 0:
		case 5:
		case 13:
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iStr + m_pGame->m_pClientList[iClientH]->m_iAngelicStr) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 3:
			// Level*2 .
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > (m_pGame->m_pClientList[iClientH]->m_iLevel * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 4:
		case 18: // Crafting
		case 21:
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iMag + m_pGame->m_pClientList[iClientH]->m_iAngelicMag) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 1:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iDex + m_pGame->m_pClientList[iClientH]->m_iAngelicDex) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 2:
		case 12:
		case 14:
		case 15:
		case 19:
		case 20: // Enchanting
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > ((m_pGame->m_pClientList[iClientH]->m_iInt + m_pGame->m_pClientList[iClientH]->m_iAngelicInt) * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		case 23:
			if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex] > (m_pGame->m_pClientList[iClientH]->m_iVit * 2)) {
				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex]--;
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = iOldSSN;
			}
			else m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;

		default:
			m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] = 0;
			break;
		}

		if (m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] == 0) {
			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1) {
				iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sSkillIndex) {
					m_pGame->m_pClientList[iClientH]->m_iHitRatio++;
				}
			}

			if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1) {
				iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
				if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sSkillIndex) {
					// Mace    .  1 .
					m_pGame->m_pClientList[iClientH]->m_iHitRatio++;
				}
			}
		}

		if (m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sSkillIndex] == 0) {
			// SKill  700     1 .
			bCheckTotalSkillMasteryPoints(iClientH, sSkillIndex);

			// Skill    .
			m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, sSkillIndex, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sSkillIndex], 0, 0);
		}
	}
}

bool SkillManager::bCheckTotalSkillMasteryPoints(int iClientH, int iSkill)
{
	
	int iRemainPoint, iTotalPoints, iWeaponIndex, iDownSkillSSN, iDownPoint;
	short sDownSkillIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;

	iTotalPoints = 0;
	for(int i = 0; i < hb::shared::limits::MaxSkillType; i++)
		iTotalPoints += m_pGame->m_pClientList[iClientH]->m_cSkillMastery[i];

	iRemainPoint = iTotalPoints - MaxSkillPoints;

	if (iRemainPoint > 0) {
		// .      SSN    .
		while (iRemainPoint > 0) {

			sDownSkillIndex = -1; // v1.4
			if (m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex != -1) {
				switch (m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex) {
				case 3:

				default:
					// 20    0  .
					if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex] > 0) {
						sDownSkillIndex = m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex;
					}
					else {
						iDownSkillSSN = 99999999;
						for(int i = 0; i < hb::shared::limits::MaxSkillType; i++)
							if ((m_pGame->m_pClientList[iClientH]->m_cSkillMastery[i] >= 21) && (i != iSkill) &&
								(m_pGame->m_pClientList[iClientH]->m_iSkillSSN[i] <= iDownSkillSSN)) {
								// V1.22     20    .
								iDownSkillSSN = m_pGame->m_pClientList[iClientH]->m_iSkillSSN[i];
								sDownSkillIndex = i;
							}
					}
					break;
				}
			}
			// 1      SSN   sDownSkillIndex

			if (sDownSkillIndex != -1) {

				if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sDownSkillIndex] <= 20) // v1.4
					iDownPoint = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sDownSkillIndex];
				else iDownPoint = 1;

				m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sDownSkillIndex] -= iDownPoint; // v1.4
				m_pGame->m_pClientList[iClientH]->m_iSkillSSN[sDownSkillIndex] = m_pGame->m_iSkillSSNpoint[m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sDownSkillIndex] + 1] - 1;
				iRemainPoint -= iDownPoint; // v1.4

				if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)] != -1) {
					iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::TwoHand)];
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sDownSkillIndex) {
						m_pGame->m_pClientList[iClientH]->m_iHitRatio -= iDownPoint; // v1.4
						if (m_pGame->m_pClientList[iClientH]->m_iHitRatio < 0) m_pGame->m_pClientList[iClientH]->m_iHitRatio = 0;
					}
				}

				if (m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)] != -1) {
					iWeaponIndex = m_pGame->m_pClientList[iClientH]->m_sItemEquipmentStatus[ToInt(EquipPos::RightHand)];
					if (m_pGame->m_pClientList[iClientH]->m_pItemList[iWeaponIndex]->m_sRelatedSkill == sDownSkillIndex) {
						m_pGame->m_pClientList[iClientH]->m_iHitRatio -= iDownPoint; // v1.4
						if (m_pGame->m_pClientList[iClientH]->m_iHitRatio < 0) m_pGame->m_pClientList[iClientH]->m_iHitRatio = 0;
					}
				}
				m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, sDownSkillIndex, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[sDownSkillIndex], 0, 0);
			}
			else {
				return false;
			}
		}
		return true;
	}

	return false;
}

void SkillManager::ClearSkillUsingStatus(int iClientH)
{
	
	short tX, fX, tY, fY;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	if (m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[19]) {
		tX = m_pGame->m_pClientList[iClientH]->m_sX;
		tY = m_pGame->m_pClientList[iClientH]->m_sY;
		if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetMoveable(tX, tY, 0) == false) {
			fX = m_pGame->m_pClientList[iClientH]->m_sX + _tmp_cCorpseX[m_pGame->m_pClientList[iClientH]->m_cDir];
			fY = m_pGame->m_pClientList[iClientH]->m_sY + _tmp_cCorpseY[m_pGame->m_pClientList[iClientH]->m_cDir];
			if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetMoveable(fX, fY, 0) == false) {
				m_pGame->m_pClientList[iClientH]->m_cDir = static_cast<char>(m_pGame->iDice(1, 8));
				fX = m_pGame->m_pClientList[iClientH]->m_sX + _tmp_cCorpseX[m_pGame->m_pClientList[iClientH]->m_cDir];
				fY = m_pGame->m_pClientList[iClientH]->m_sY + _tmp_cCorpseY[m_pGame->m_pClientList[iClientH]->m_cDir];
				if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->bGetMoveable(fX, fY, 0) == false) {
					return;
				}
			}
			m_pGame->SendNotifyMsg(0, iClientH, Notify::DamageMove, m_pGame->m_pClientList[iClientH]->m_cDir, 0, 0, 0);
		}
	}
	for(int i = 0; i < hb::shared::limits::MaxSkillType; i++) {
		m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[i] = false;
		m_pGame->m_pClientList[iClientH]->m_iSkillUsingTimeID[i] = 0;
	}

	if (m_pGame->m_pClientList[iClientH]->m_iAllocatedFish != 0) {
		m_pGame->m_pFishingManager->ReleaseFishEngagement(iClientH);
		m_pGame->SendNotifyMsg(0, iClientH, Notify::FishCanceled, 0, 0, 0, 0);
	}

}

void SkillManager::UseSkillHandler(int iClientH, int iV1, int iV2, int iV3)
{
	char  cOwnerType;
	short sAttackerWeapon, sOwnerH;
	int   iResult, iPlayerSkillLevel;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bIsInitComplete == false) return;

	if ((iV1 < 0) || (iV1 >= hb::shared::limits::MaxSkillType)) return;
	if (m_pGame->m_pSkillConfigList[iV1] == 0) return;
	if (m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[iV1]) return;

	/*
	if (iV1 != 19) {
		m_pGame->m_pClientList[iClientH]->m_iAbuseCount++;
		if ((m_pGame->m_pClientList[iClientH]->m_iAbuseCount % 30) == 0) {
			std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) ÇØÅ· ¿ëÀÇÀÚ(%s) Skill(%d) Tries(%d)",m_pGame->m_pClientList[iClientH]->m_cCharName,
																	   iV1, m_pGame->m_pClientList[iClientH]->m_iAbuseCount);
			PutLogFileList(G_cTxt);
		}
	}
	*/

	iPlayerSkillLevel = m_pGame->m_pClientList[iClientH]->m_cSkillMastery[iV1];
	iResult = m_pGame->iDice(1, 100);

	if (iResult > iPlayerSkillLevel) {
		m_pGame->SendNotifyMsg(0, iClientH, Notify::SkillUsingEnd, 0, 0, 0, 0);
		return;
	}

	switch (m_pGame->m_pSkillConfigList[iV1]->m_sType) {
	case EffectType::Pretend:
		switch (m_pGame->m_pSkillConfigList[iV1]->m_sValue1) {
		case 1:

			if (m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_bIsFightZone) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::SkillUsingEnd, 0, 0, 0, 0);
				return;
			}

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
			if (sOwnerH != 0) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::SkillUsingEnd, 0, 0, 0, 0);
				return;
			}

			iResult = 0;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY - 1);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY + 1);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX - 1, m_pGame->m_pClientList[iClientH]->m_sY);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX + 1, m_pGame->m_pClientList[iClientH]->m_sY);
			iResult += sOwnerH;

			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX - 1, m_pGame->m_pClientList[iClientH]->m_sY - 1);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX + 1, m_pGame->m_pClientList[iClientH]->m_sY - 1);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX - 1, m_pGame->m_pClientList[iClientH]->m_sY + 1);
			iResult += sOwnerH;
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, m_pGame->m_pClientList[iClientH]->m_sX + 1, m_pGame->m_pClientList[iClientH]->m_sY + 1);
			iResult += sOwnerH;

			if (iResult != 0) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::SkillUsingEnd, 0, 0, 0, 0);
				return;
			}

			CalculateSSN_SkillIndex(iClientH, iV1, 1);

			sAttackerWeapon = 1;
			m_pGame->SendEventToNearClient_TypeA(iClientH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::Dying, 0, sAttackerWeapon, 0);
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->ClearOwner(14, iClientH, hb::shared::owner_class::Player, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
			m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->SetDeadOwner(iClientH, hb::shared::owner_class::Player, m_pGame->m_pClientList[iClientH]->m_sX, m_pGame->m_pClientList[iClientH]->m_sY);
			break;
		}
		break;

	}

	m_pGame->m_pClientList[iClientH]->m_bSkillUsingStatus[iV1] = true;
}

void SkillManager::SetDownSkillIndexHandler(int iClientH, int iSkillIndex)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if ((iSkillIndex < 0) || (iSkillIndex >= hb::shared::limits::MaxSkillType)) return;

	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[iSkillIndex] > 0)
		m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex = iSkillIndex;

	m_pGame->SendNotifyMsg(0, iClientH, Notify::DownSkillIndexSet, m_pGame->m_pClientList[iClientH]->m_iDownSkillIndex, 0, 0, 0);
}

void SkillManager::_TamingHandler(int iClientH, int iSkillNum, char cMapIndex, int dX, int dY)
{
	int iSkillLevel, iRange, iTamingLevel, iResult;
	short sOwnerH;
	char  cOwnerType;

	if (m_pGame->m_pClientList[iClientH] == 0) return;
	if (m_pGame->m_pMapList[cMapIndex] == 0) return;

	iSkillLevel = (int)m_pGame->m_pClientList[iClientH]->m_cSkillMastery[iSkillNum];
	iRange = iSkillLevel / 12;

	for(int iX = dX - iRange; iX <= dX + iRange; iX++)
		for(int iY = dY - iRange; iY <= dY + iRange; iY++) {
			sOwnerH = 0;
			if ((iX > 0) && (iY > 0) && (iX < m_pGame->m_pMapList[cMapIndex]->m_sSizeX) && (iY < m_pGame->m_pMapList[cMapIndex]->m_sSizeY))
				m_pGame->m_pMapList[cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, iX, iY);

			if (sOwnerH != 0) {
				switch (cOwnerType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[sOwnerH] == 0) break;
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[sOwnerH] == 0) break;
					iTamingLevel = 10;
					switch (m_pGame->m_pNpcList[sOwnerH]->m_sType) {
					case 10:
					case 16: iTamingLevel = 1; break;
					case 22: iTamingLevel = 2; break;
					case 17:
					case 14: iTamingLevel = 3; break;
					case 18: iTamingLevel = 4; break;
					case 11: iTamingLevel = 5; break;
					case 23:
					case 12: iTamingLevel = 6; break;
					case 28: iTamingLevel = 7; break;
					case 13:
					case 27: iTamingLevel = 8; break;
					case 29: iTamingLevel = 9; break;
					case 33: iTamingLevel = 9; break;
					case 30: iTamingLevel = 9; break;
					case 31:
					case 32: iTamingLevel = 10; break;
					}

					iResult = (iSkillLevel / 10);

					if (iResult < iTamingLevel) break;

					break;
				}
			}
		}
}

void SkillManager::SkillCheck(int sTargetH) {
	//magic
	while ((m_pGame->m_pClientList[sTargetH]->m_iMag * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[4]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[4]--;
	}
	//hand attack
	while ((m_pGame->m_pClientList[sTargetH]->m_iStr * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[5]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[5]--;
	}
	//hammer
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[14]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[14]--;
	}
	//shield
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[11]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[11]--;
	}
	//axe
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[10]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[10]--;
	}
	//fencing
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[9]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[9]--;
	}
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[8]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[8]--;
	}
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[7]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[7]--;
	}
	//archery
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[6]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[6]--;
	}
	//staff
	while ((m_pGame->m_pClientList[sTargetH]->m_iMag * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[21]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[21]--;
	}
	//alc
	while ((m_pGame->m_pClientList[sTargetH]->m_iInt * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[12]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[12]--;
	}
	//manu
	while ((m_pGame->m_pClientList[sTargetH]->m_iStr * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[13]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[13]--;
	}
	while ((m_pGame->m_pClientList[sTargetH]->m_iVit * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[23]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[23]--;
	}
	while ((m_pGame->m_pClientList[sTargetH]->m_iInt * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[19]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[19]--;
	}
	//farming
	while ((m_pGame->m_pClientList[sTargetH]->m_iInt * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[2]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[2]--;
	}
	//fishing
	while ((m_pGame->m_pClientList[sTargetH]->m_iDex * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[1]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[1]--;
	}
	//mining
	while ((m_pGame->m_pClientList[sTargetH]->m_iStr * 2) < m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[0]) {
		m_pGame->m_pClientList[sTargetH]->m_cSkillMastery[0]--;
	}
}

void SkillManager::ReloadSkillConfigs()
{
	sqlite3* configDb = nullptr;
	std::string configDbPath;
	bool configDbCreated = false;
	if (!EnsureGameConfigDatabase(&configDb, configDbPath, &configDbCreated) || configDbCreated)
	{
		hb::logger::log("Skill config reload failed: gameconfigs.db unavailable");
		return;
	}

	for(int i = 0; i < hb::shared::limits::MaxSkillType; i++)
	{
		if (m_pGame->m_pSkillConfigList[i] != 0)
		{
			delete m_pGame->m_pSkillConfigList[i];
			m_pGame->m_pSkillConfigList[i] = 0;
		}
	}

	if (!LoadSkillConfigs(configDb, m_pGame))
	{
		hb::logger::log("Skill config reload failed");
		CloseGameConfigDatabase(configDb);
		return;
	}

	CloseGameConfigDatabase(configDb);
	m_pGame->ComputeConfigHashes();
	hb::logger::log("Skill configs reloaded successfully");
}

void SkillManager::SetSkillAll(int iClientH, char* pData, size_t dwMsgSize)
//SetSkillAll Acidx Command,  Added July 04, 2005 INDEPENDENCE BABY Fuck YEA
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	//Magic
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] = m_pGame->m_pClientList[iClientH]->m_iMag * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iMag > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 4, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[4], 0, 0);

	}
	//LongSword
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;

		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 8, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[8], 0, 0);

	}
	//Hammer
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 14, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[14], 0, 0);

	}
	//Axes
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 10, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[10], 0, 0);

	}
	//hand attack
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] = m_pGame->m_pClientList[iClientH]->m_iStr * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iStr > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 5, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[5], 0, 0);

	}
	//ShortSword
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 7, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[7], 0, 0);

	}
	//archery
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 6, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[6], 0, 0);

	}
	//Fencing
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 9, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[9], 0, 0);

	}
	//Staff Attack
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] = m_pGame->m_pClientList[iClientH]->m_iInt * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iInt > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 21, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[21], 0, 0);

	}
	//shield
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11] = m_pGame->m_pClientList[iClientH]->m_iDex * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iDex > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 11, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[11], 0, 0);

	}
	//mining
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[0] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[0] = 100;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 0, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[0], 0, 0);

	}
	//fishing
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[1] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[1] = 100;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 1, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[1], 0, 0);

	}
	//farming
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[2] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[2] = 100;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 2, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[2], 0, 0);

	}
	//alchemy
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[12] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[12] = 100;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 12, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[12], 0, 0);

	}
	//manufacturing
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13] = 100;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 13, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[13], 0, 0);

	}
	//poison resistance
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[23] < 20)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[23] = 20;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 23, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[23], 0, 0);

	}
	//pretend corpse
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19] < 100)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19] = m_pGame->m_pClientList[iClientH]->m_iInt * 2;
		if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19] > 100)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19] = 100;
		}
		if (m_pGame->m_pClientList[iClientH]->m_iInt > 50)
		{
			m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19] = 100;
		}
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 19, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[19], 0, 0);

	}
	//magic resistance
	if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[3] < 20)
	{
		// now we add skills
		m_pGame->m_pClientList[iClientH]->m_cSkillMastery[3] = 20;
		//Send a notify to update the client
		m_pGame->SendNotifyMsg(0, iClientH, Notify::Skill, 3, m_pGame->m_pClientList[iClientH]->m_cSkillMastery[3], 0, 0);

	}
}
