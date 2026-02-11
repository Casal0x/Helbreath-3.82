// DelayEventManager.cpp: Manages timed delay events (magic expiration, meteors, etc.).
// Extracted from Game.cpp (Phase B5).

#include "DelayEventManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "WarManager.h"
#include "ItemManager.h"
#include "DelayEvent.h"
#include "Packet/SharedPackets.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::config;
namespace sdelay = hb::server::delay_event;

void DelayEventManager::InitArrays()
{
	for (int i = 0; i < MaxDelayEvents; i++)
		m_pDelayEventList[i] = 0;
}

void DelayEventManager::CleanupArrays()
{
	for (int i = 0; i < MaxDelayEvents; i++)
		if (m_pDelayEventList[i] != 0) delete m_pDelayEventList[i];
}

bool DelayEventManager::bRegisterDelayEvent(int iDelayType, int iEffectType, uint32_t dwLastTime, int iTargetH, char cTargetType, char cMapIndex, int dX, int dY, int iV1, int iV2, int iV3)
{

	for(int i = 0; i < MaxDelayEvents; i++)
		if (m_pDelayEventList[i] == 0) {
			m_pDelayEventList[i] = new class CDelayEvent;
			m_pDelayEventList[i]->m_iDelayType = iDelayType;
			m_pDelayEventList[i]->m_iEffectType = iEffectType;
			m_pDelayEventList[i]->m_cMapIndex = cMapIndex;
			m_pDelayEventList[i]->m_dX = dX;
			m_pDelayEventList[i]->m_dY = dY;
			m_pDelayEventList[i]->m_iTargetH = iTargetH;
			m_pDelayEventList[i]->m_cTargetType = cTargetType;
			m_pDelayEventList[i]->m_iV1 = iV1;
			m_pDelayEventList[i]->m_iV2 = iV2;
			m_pDelayEventList[i]->m_iV3 = iV3;
			m_pDelayEventList[i]->m_dwTriggerTime = dwLastTime;
			return true;
		}
	return false;
}

bool DelayEventManager::bRemoveFromDelayEventList(int iH, char cType, int iEffectType)
{

	for(int i = 0; i < MaxDelayEvents; i++)
		if (m_pDelayEventList[i] != 0) {

			if (iEffectType == 0) {
				// Effect
				if ((m_pDelayEventList[i]->m_iTargetH == iH) && (m_pDelayEventList[i]->m_cTargetType == cType)) {
					delete m_pDelayEventList[i];
					m_pDelayEventList[i] = 0;
				}
			}
			else {
				// Effect .
				if ((m_pDelayEventList[i]->m_iTargetH == iH) && (m_pDelayEventList[i]->m_cTargetType == cType) &&
					(m_pDelayEventList[i]->m_iEffectType == iEffectType)) {
					delete m_pDelayEventList[i];
					m_pDelayEventList[i] = 0;
				}
			}
		}

	return true;
}

void DelayEventManager::DelayEventProcessor()
{
	int iSkillNum, iResult;
	uint32_t dwTime = GameClock::GetTimeMS();
	int iTemp;

	for(int i = 0; i < MaxDelayEvents; i++)
		if ((m_pDelayEventList[i] != 0) && (m_pDelayEventList[i]->m_dwTriggerTime < dwTime)) {

			switch (m_pDelayEventList[i]->m_iDelayType) {

			case sdelay::Type::AncientTablet:
				if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_status.bSlateInvincible) {
					iTemp = 1;
				}
				else if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_status.bSlateMana) {
					iTemp = 3;
				}
				else if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_status.bSlateExp) {
					iTemp = 4;
				}

				m_pGame->SendNotifyMsg(0, m_pDelayEventList[i]->m_iTargetH, Notify::SlateStatus, iTemp, 0, 0, 0);
				m_pGame->m_pItemManager->SetSlateFlag(m_pDelayEventList[i]->m_iTargetH, iTemp, false);
				break;

			case sdelay::Type::CalcMeteorStrikeEffect:
				m_pGame->m_pWarManager->CalcMeteorStrikeEffectHandler(m_pDelayEventList[i]->m_cMapIndex);
				break;

			case sdelay::Type::DoMeteorStrikeDamage:
				m_pGame->m_pWarManager->DoMeteorStrikeDamageHandler(m_pDelayEventList[i]->m_cMapIndex);
				break;

			case sdelay::Type::MeteorStrike:
				m_pGame->m_pWarManager->MeteorStrikeHandler(m_pDelayEventList[i]->m_cMapIndex);
				break;

			case sdelay::Type::UseItemSkill:
				switch (m_pDelayEventList[i]->m_cTargetType) {
				case hb::shared::owner_class::Player:
					iSkillNum = m_pDelayEventList[i]->m_iEffectType;

					if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH] == 0) break;
					if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_bSkillUsingStatus[iSkillNum] == false) break;
					// ID   v1.12
					if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_iSkillUsingTimeID[iSkillNum] != m_pDelayEventList[i]->m_iV2) break;

					m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_bSkillUsingStatus[iSkillNum] = false;
					m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_iSkillUsingTimeID[iSkillNum] = 0;

					// Skill    .
					iResult = m_pGame->m_pItemManager->iCalculateUseSkillItemEffect(m_pDelayEventList[i]->m_iTargetH, m_pDelayEventList[i]->m_cTargetType,
						m_pDelayEventList[i]->m_iV1, iSkillNum, m_pDelayEventList[i]->m_cMapIndex, m_pDelayEventList[i]->m_dX, m_pDelayEventList[i]->m_dY);

					m_pGame->SendNotifyMsg(0, m_pDelayEventList[i]->m_iTargetH, Notify::SkillUsingEnd, iResult, 0, 0, 0);
					break;
				}
				break;

			case sdelay::Type::DamageObject:
				break;

			case sdelay::Type::MagicRelease:
				// Removes the aura after time
				switch (m_pDelayEventList[i]->m_cTargetType) {
				case hb::shared::owner_class::Player:
					if (m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH] == 0) break;

					m_pGame->SendNotifyMsg(0, m_pDelayEventList[i]->m_iTargetH, Notify::MagicEffectOff,
						m_pDelayEventList[i]->m_iEffectType, m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_cMagicEffectStatus[m_pDelayEventList[i]->m_iEffectType], 0, 0);

					m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_cMagicEffectStatus[m_pDelayEventList[i]->m_iEffectType] = 0;

					// Inbitition casting
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Inhibition)
						m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_bInhibition = false;

					// Invisibility
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Invisibility)
						m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);

					// Berserk
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Berserk)
						m_pGame->m_pStatusEffectManager->SetBerserkFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);

					// Haste
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Haste)
						m_pGame->m_pStatusEffectManager->SetHasteFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);

					// Confusion
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Confuse)
						switch (m_pDelayEventList[i]->m_iV1) {
						case 3: m_pGame->m_pStatusEffectManager->SetIllusionFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false); break;
						case 4: m_pGame->m_pStatusEffectManager->SetIllusionMovementFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false); break;
						}

					// Protection Magic
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Protect) {
						switch (m_pDelayEventList[i]->m_iV1) {
						case 1:
							m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);
							break;
						case 2:
						case 5:
							m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);
							break;
						case 3:
						case 4:
							m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);
							break;
						}
					}


					// polymorph
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Polymorph) {
						m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_sType = m_pGame->m_pClientList[m_pDelayEventList[i]->m_iTargetH]->m_sOriginalType;
						m_pGame->SendEventToNearClient_TypeA(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
					}

					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Ice)
						m_pGame->m_pStatusEffectManager->SetIceFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Player, false);
					break;

				case hb::shared::owner_class::Npc:
					if (m_pGame->m_pNpcList[m_pDelayEventList[i]->m_iTargetH] == 0) break;

					m_pGame->m_pNpcList[m_pDelayEventList[i]->m_iTargetH]->m_cMagicEffectStatus[m_pDelayEventList[i]->m_iEffectType] = 0;

					// Invisibility
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Invisibility)
						m_pGame->m_pStatusEffectManager->SetInvisibilityFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);

					// Berserk
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Berserk)
						m_pGame->m_pStatusEffectManager->SetBerserkFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);

					// polymorph
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Polymorph) {
						m_pGame->m_pNpcList[m_pDelayEventList[i]->m_iTargetH]->m_sType = m_pGame->m_pNpcList[m_pDelayEventList[i]->m_iTargetH]->m_sOriginalType;
						m_pGame->SendEventToNearClient_TypeA(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::NullAction, 0, 0, 0);
					}

					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Ice)
						m_pGame->m_pStatusEffectManager->SetIceFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);

					// Illusion
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Confuse)
						m_pGame->m_pStatusEffectManager->SetIllusionFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);

					// Protection Magic
					if (m_pDelayEventList[i]->m_iEffectType == hb::shared::magic::Protect) {
						switch (m_pDelayEventList[i]->m_iV1) {
						case 1:
							m_pGame->m_pStatusEffectManager->SetProtectionFromArrowFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);
							break;
						case 2:
						case 5:
							m_pGame->m_pStatusEffectManager->SetMagicProtectionFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);
							break;
						case 3:
						case 4:
							m_pGame->m_pStatusEffectManager->SetDefenseShieldFlag(m_pDelayEventList[i]->m_iTargetH, hb::shared::owner_class::Npc, false);
							break;
						}
					}

					break;
				}
				break;
			}

			delete m_pDelayEventList[i];
			m_pDelayEventList[i] = 0;
		}
}

void DelayEventManager::DelayEventProcess()
{

}
