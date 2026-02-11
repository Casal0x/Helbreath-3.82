#include "DynamicObjectManager.h"
#include "Game.h"
#include "StatusEffectManager.h"
#include "CombatManager.h"
#include "DynamicObject.h"
#include "DelayEventManager.h"
#include "FishingManager.h"
#include "EntityManager.h"
#include "Packet/SharedPackets.h"
#include "Entity/DynamicObjectID.h"

using namespace hb::shared::net;
using namespace hb::shared::action;
using namespace hb::server::config;
namespace sdelay = hb::server::delay_event;
namespace dynamic_object = hb::shared::dynamic_object;

extern char G_cTxt[512];
extern void PutLogList(char* cStr);


void DynamicObjectManager::InitArrays()
{
	for (int i = 0; i < MaxDynamicObjects; i++)
		m_pDynamicObjectList[i] = 0;
}

void DynamicObjectManager::CleanupArrays()
{
	for (int i = 0; i < MaxDynamicObjects; i++)
		if (m_pDynamicObjectList[i] != 0) delete m_pDynamicObjectList[i];
}

int DynamicObjectManager::iAddDynamicObjectList(short sOwner, char cOwnerType, short sType, char cMapIndex, short sX, short sY, uint32_t dwLastTime, int iV1)
{

	short sPreType;
	uint32_t dwTime, dwRegisterTime;

	m_pGame->m_pMapList[cMapIndex]->bGetDynamicObject(sX, sY, &sPreType, &dwRegisterTime);
	if (sPreType != 0) return 0;

	switch (sType) {
	case dynamic_object::Fire3:
	case dynamic_object::Fire:
		if (m_pGame->m_pMapList[cMapIndex]->bGetIsMoveAllowedTile(sX, sY) == false)
			return 0;
		if (dwLastTime != 0) {
			switch (m_pGame->m_pMapList[cMapIndex]->m_cWhetherStatus) {
			case 1:	dwLastTime = dwLastTime - (dwLastTime / 2);       break;
			case 2:	dwLastTime = (dwLastTime / 2) - (dwLastTime / 3); break;
			case 3:	dwLastTime = (dwLastTime / 3) - (dwLastTime / 4); break;
			}

			if (dwLastTime == 0) dwLastTime = 1000;
		}
		break;

	case dynamic_object::FishObject:
	case dynamic_object::Fish:
		if (m_pGame->m_pMapList[cMapIndex]->bGetIsWater(sX, sY) == false)
			return 0;
		break;

	case dynamic_object::AresdenFlag1:
	case dynamic_object::ElvineFlag1:
	case dynamic_object::Mineral1:
	case dynamic_object::Mineral2:
		if (m_pGame->m_pMapList[cMapIndex]->bGetMoveable(sX, sY) == false)
			return 0;
		m_pGame->m_pMapList[cMapIndex]->SetTempMoveAllowedFlag(sX, sY, false);
		break;

	}

	for(int i = 1; i < MaxDynamicObjects; i++)
		if (m_pDynamicObjectList[i] == 0) {
			dwTime = GameClock::GetTimeMS();

			if (dwLastTime != 0)
				dwLastTime += (m_pGame->iDice(1, 4) * 1000);

			m_pDynamicObjectList[i] = new class CDynamicObject(sOwner, cOwnerType, sType, cMapIndex, sX, sY, dwTime, dwLastTime, iV1);
			m_pGame->m_pMapList[cMapIndex]->SetDynamicObject(i, sType, sX, sY, dwTime);
			m_pGame->SendEventToNearClient_TypeB(MsgId::DynamicObject, MsgType::Confirm, cMapIndex, sX, sY, sType, i, 0, (short)0);

			return i;
		}
	return 0;
}

void DynamicObjectManager::CheckDynamicObjectList()
{

	uint32_t dwTime = GameClock::GetTimeMS(), dwRegisterTime;
	short sType;

	for(int i = 1; i < MaxDynamicObjects; i++) {
		if ((m_pDynamicObjectList[i] != 0) && (m_pDynamicObjectList[i]->m_dwLastTime != 0)) {

			switch (m_pDynamicObjectList[i]->m_sType) {
			case dynamic_object::Fire3:
			case dynamic_object::Fire:
				switch (m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->m_cWhetherStatus) {
				case 0: break;
				case 1:
				case 2:
				case 3:
					// ( /10)*    .
					m_pDynamicObjectList[i]->m_dwLastTime = m_pDynamicObjectList[i]->m_dwLastTime -
						(m_pDynamicObjectList[i]->m_dwLastTime / 10) * m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->m_cWhetherStatus;
					break;
				}
				break;
			}
		}
	}

	// .  NULL    .
	for(int i = 1; i < MaxDynamicObjects; i++) {
		if ((m_pDynamicObjectList[i] != 0) && (m_pDynamicObjectList[i]->m_dwLastTime != 0) &&
			((dwTime - m_pDynamicObjectList[i]->m_dwRegisterTime) >= m_pDynamicObjectList[i]->m_dwLastTime)) {

			m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->bGetDynamicObject(m_pDynamicObjectList[i]->m_sX, m_pDynamicObjectList[i]->m_sY, &sType, &dwRegisterTime);

			if (dwRegisterTime == m_pDynamicObjectList[i]->m_dwRegisterTime) {
				m_pGame->SendEventToNearClient_TypeB(MsgId::DynamicObject, MsgType::Reject, m_pDynamicObjectList[i]->m_cMapIndex, m_pDynamicObjectList[i]->m_sX, m_pDynamicObjectList[i]->m_sY, m_pDynamicObjectList[i]->m_sType, i, 0, (short)0);
				m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->SetDynamicObject(0, 0, m_pDynamicObjectList[i]->m_sX, m_pDynamicObjectList[i]->m_sY, dwTime);
			}

			switch (sType) {
			case dynamic_object::FishObject:
			case dynamic_object::Fish:
				m_pGame->m_pFishingManager->bDeleteFish(m_pDynamicObjectList[i]->m_sOwner, 2);
				break;
			}

			delete m_pDynamicObjectList[i];
			m_pDynamicObjectList[i] = 0;
		}
	}
}

void DynamicObjectManager::DynamicObjectEffectProcessor()
{
	int iIndex;
	short sOwnerH, sType;
	int iDamage;
	char  cOwnerType;
	uint32_t dwTime = GameClock::GetTimeMS(), dwRegisterTime;

	for(int i = 0; i < MaxDynamicObjects; i++)
		if (m_pDynamicObjectList[i] != 0) {
			switch (m_pDynamicObjectList[i]->m_sType) {

			case dynamic_object::PCloudBegin:
				for(int ix = m_pDynamicObjectList[i]->m_sX - 1; ix <= m_pDynamicObjectList[i]->m_sX + 1; ix++)
					for(int iy = m_pDynamicObjectList[i]->m_sY - 1; iy <= m_pDynamicObjectList[i]->m_sY + 1; iy++) {

						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (sOwnerH != 0) {
							// Poison Damage .
							switch (cOwnerType) {
							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) break;
								if (m_pGame->m_pClientList[sOwnerH]->m_bIsKilled) break;
								//if ((m_pGame->m_pClientList[sOwnerH]->m_bIsNeutral ) && !m_pGame->m_pClientList[sOwnerH]->m_appearance.bIsWalking) break;

								if (m_pDynamicObjectList[i]->m_iV1 < 20)
									iDamage = m_pGame->iDice(1, 6);
								else iDamage = m_pGame->iDice(1, 8);

								// New 17/05/2004 Changed
								m_pGame->m_pClientList[sOwnerH]->m_iHP -= iDamage;

								if (m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
									m_pGame->m_pCombatManager->ClientKilledHandler(sOwnerH, sOwnerH, cOwnerType, iDamage);
								}
								else {
									if (iDamage > 0) {
										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);

										if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
											// 1: Hold-Person
											// 2: Paralize
											m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);

											m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
											m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
										}
									}

									if ((m_pGame->m_pCombatManager->bCheckResistingMagicSuccess(1, sOwnerH, hb::shared::owner_class::Player, 100) == false) &&
										(m_pGame->m_pClientList[sOwnerH]->m_bIsPoisoned == false)) {

										m_pGame->m_pClientList[sOwnerH]->m_bIsPoisoned = true;
										m_pGame->m_pClientList[sOwnerH]->m_iPoisonLevel = m_pDynamicObjectList[i]->m_iV1;
										m_pGame->m_pClientList[sOwnerH]->m_dwPoisonTime = dwTime;
										m_pGame->m_pStatusEffectManager->SetPoisonFlag(sOwnerH, cOwnerType, true);// poison aura appears from dynamic objects
										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Poison, m_pGame->m_pClientList[sOwnerH]->m_iPoisonLevel, 0, 0);
									}
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) break;

								if (m_pDynamicObjectList[i]->m_iV1 < 20)
									iDamage = m_pGame->iDice(1, 6);
								else iDamage = m_pGame->iDice(1, 8);

								switch (m_pGame->m_pNpcList[sOwnerH]->m_sType) {
								case 40: // ESG
								case 41: // GMG
								case 67: // McGaffin
								case 68: // Perry
								case 69: // Devlin
									iDamage = 0;
									break;
								}

								// HP . Action Limit  .
								switch (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit) {
								case 0:
								case 3:
								case 5:
									m_pGame->m_pNpcList[sOwnerH]->m_iHP -= iDamage;
									break;
								}
								//if (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit == 0)
								//	m_pGame->m_pNpcList[sOwnerH]->m_iHP -= iDamage;

								if (m_pGame->m_pNpcList[sOwnerH]->m_iHP <= 0) {
									// NPC .
									m_pGame->m_pEntityManager->OnEntityKilled(sOwnerH, sOwnerH, cOwnerType, 0);
								}
								else {
									// Damage    .
									if (m_pGame->iDice(1, 3) == 2)
										m_pGame->m_pNpcList[sOwnerH]->m_dwTime = dwTime;

									if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
										// Hold    .
										m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
									}

									// NPC   .
									m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);
								}
								break;
							}
						}
					}
				break;

			case dynamic_object::IceStorm:
				for(int ix = m_pDynamicObjectList[i]->m_sX - 2; ix <= m_pDynamicObjectList[i]->m_sX + 2; ix++)
					for(int iy = m_pDynamicObjectList[i]->m_sY - 2; iy <= m_pDynamicObjectList[i]->m_sY + 2; iy++) {

						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (sOwnerH != 0) {
							switch (cOwnerType) {
							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) break;
								if (m_pGame->m_pClientList[sOwnerH]->m_bIsKilled) break;

								iDamage = m_pGame->iDice(3, 3) + 5;

								m_pGame->m_pClientList[sOwnerH]->m_iHP -= iDamage;

								if (m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
									m_pGame->m_pCombatManager->ClientKilledHandler(sOwnerH, sOwnerH, cOwnerType, iDamage);
								}
								else {
									if (iDamage > 0) {

										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);

										if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] == 1) {

											m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);

											m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
											m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
										}
									}

									if ((m_pGame->m_pCombatManager->bCheckResistingIceSuccess(1, sOwnerH, hb::shared::owner_class::Player, m_pDynamicObjectList[i]->m_iV1) == false) &&
										(m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0)) {

										m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
										m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
										m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (20 * 1000),
											sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);

										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOn, hb::shared::magic::Ice, 1, 0, 0);
									}
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) break;

								iDamage = m_pGame->iDice(3, 3) + 5;

								switch (m_pGame->m_pNpcList[sOwnerH]->m_sType) {
								case 40: // ESG
								case 41: // GMG
								case 67: // McGaffin
								case 68: // Perry
								case 69: // Devlin
									iDamage = 0;
									break;
								}

								switch (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit) {
								case 0:
								case 3:
								case 5:
									m_pGame->m_pNpcList[sOwnerH]->m_iHP -= iDamage;
									break;
								}

								if (m_pGame->m_pNpcList[sOwnerH]->m_iHP <= 0) {
									// NPC .
									m_pGame->m_pEntityManager->OnEntityKilled(sOwnerH, sOwnerH, cOwnerType, 0);
								}
								else {
									// Damage    .
									if (m_pGame->iDice(1, 3) == 2)
										m_pGame->m_pNpcList[sOwnerH]->m_dwTime = dwTime;

									if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
										// Hold    .
										m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
									}

									// NPC   .
									m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);

									if ((m_pGame->m_pCombatManager->bCheckResistingIceSuccess(1, sOwnerH, hb::shared::owner_class::Npc, m_pDynamicObjectList[i]->m_iV1) == false) &&
										(m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] == 0)) {

										m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::Ice] = 1;
										m_pGame->m_pStatusEffectManager->SetIceFlag(sOwnerH, cOwnerType, true);
										m_pGame->m_pDelayEventManager->bRegisterDelayEvent(sdelay::Type::MagicRelease, hb::shared::magic::Ice, dwTime + (20 * 1000),
											sOwnerH, cOwnerType, 0, 0, 0, 1, 0, 0);
									}
								}
								break;
							}
						}

						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
						if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
							(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
							iDamage = m_pGame->iDice(3, 2);
							m_pGame->m_pClientList[sOwnerH]->m_iHP -= iDamage;

							if (m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
								m_pGame->m_pCombatManager->ClientKilledHandler(sOwnerH, sOwnerH, cOwnerType, iDamage);
							}
							else {
								if (iDamage > 0) {
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);
								}
							}
						}

						// Fire Object   .
						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->bGetDynamicObject(ix, iy, &sType, &dwRegisterTime, &iIndex);
						if (((sType == dynamic_object::Fire) || (sType == dynamic_object::Fire3)) && (m_pDynamicObjectList[iIndex] != 0))
							m_pDynamicObjectList[iIndex]->m_dwLastTime = m_pDynamicObjectList[iIndex]->m_dwLastTime - (m_pDynamicObjectList[iIndex]->m_dwLastTime / 10);
					}
				break;

			case dynamic_object::Fire3:
			case dynamic_object::Fire:
				// Fire-Wall
				if (m_pDynamicObjectList[i]->m_iCount == 1) {
					m_pGame->m_pCombatManager->CheckFireBluring(m_pDynamicObjectList[i]->m_cMapIndex, m_pDynamicObjectList[i]->m_sX, m_pDynamicObjectList[i]->m_sY);
				}
				m_pDynamicObjectList[i]->m_iCount++;
				if (m_pDynamicObjectList[i]->m_iCount > 10) m_pDynamicObjectList[i]->m_iCount = 10;


				for(int ix = m_pDynamicObjectList[i]->m_sX - 1; ix <= m_pDynamicObjectList[i]->m_sX + 1; ix++)
					for(int iy = m_pDynamicObjectList[i]->m_sY - 1; iy <= m_pDynamicObjectList[i]->m_sY + 1; iy++) {

						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->GetOwner(&sOwnerH, &cOwnerType, ix, iy);
						if (sOwnerH != 0) {
							// Fire Damage .
							switch (cOwnerType) {

							case hb::shared::owner_class::Player:
								if (m_pGame->m_pClientList[sOwnerH] == 0) break;
								if (m_pGame->m_pClientList[sOwnerH]->m_bIsKilled) break;
								//if ((m_pGame->m_pClientList[sOwnerH]->m_bIsNeutral ) && !m_pGame->m_pClientList[sOwnerH]->m_appearance.bIsWalking) break;

								iDamage = m_pGame->iDice(1, 6);
								// New 17/05/2004
								m_pGame->m_pClientList[sOwnerH]->m_iHP -= iDamage;

								if (m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
									m_pGame->m_pCombatManager->ClientKilledHandler(sOwnerH, sOwnerH, cOwnerType, iDamage);
								}
								else {
									if (iDamage > 0) {
										m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);

										if (m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
											// Hold-Person    . Fire Field   .
											// 1: Hold-Person
											// 2: Paralize
											m_pGame->SendNotifyMsg(0, sOwnerH, Notify::MagicEffectOff, hb::shared::magic::HoldObject, m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject], 0, 0);

											m_pGame->m_pClientList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
											m_pGame->m_pDelayEventManager->bRemoveFromDelayEventList(sOwnerH, hb::shared::owner_class::Player, hb::shared::magic::HoldObject);
										}
									}
								}
								break;

							case hb::shared::owner_class::Npc:
								if (m_pGame->m_pNpcList[sOwnerH] == 0) break;

								iDamage = m_pGame->iDice(1, 6);

								switch (m_pGame->m_pNpcList[sOwnerH]->m_sType) {
								case 40: // ESG
								case 41: // GMG
								case 67: // McGaffin
								case 68: // Perry
								case 69: // Devlin
									iDamage = 0;
									break;
								}

								// HP . Action Limit  .
								switch (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit) {
								case 0:
								case 3:
								case 5:
									m_pGame->m_pNpcList[sOwnerH]->m_iHP -= iDamage;
									break;
								}
								//if (m_pGame->m_pNpcList[sOwnerH]->m_cActionLimit == 0)
								//	m_pGame->m_pNpcList[sOwnerH]->m_iHP -= iDamage;

								if (m_pGame->m_pNpcList[sOwnerH]->m_iHP <= 0) {
									// NPC .
									m_pGame->m_pEntityManager->OnEntityKilled(sOwnerH, sOwnerH, cOwnerType, 0);
								}
								else {
									// Damage    .
									if (m_pGame->iDice(1, 3) == 2)
										m_pGame->m_pNpcList[sOwnerH]->m_dwTime = dwTime;

									if (m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] != 0) {
										// Hold    .
										m_pGame->m_pNpcList[sOwnerH]->m_cMagicEffectStatus[hb::shared::magic::HoldObject] = 0;
									}

									// NPC   .
									m_pGame->SendEventToNearClient_TypeA(sOwnerH, hb::shared::owner_class::Npc, MsgId::EventMotion, Type::Damage, iDamage, 0, 0);
								}
								break;
							}
						}

						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->GetDeadOwner(&sOwnerH, &cOwnerType, ix, iy);
						if ((cOwnerType == hb::shared::owner_class::Player) && (m_pGame->m_pClientList[sOwnerH] != 0) &&
							(m_pGame->m_pClientList[sOwnerH]->m_iHP > 0)) {
							iDamage = m_pGame->iDice(1, 6);
							m_pGame->m_pClientList[sOwnerH]->m_iHP -= iDamage;

							if (m_pGame->m_pClientList[sOwnerH]->m_iHP <= 0) {
								m_pGame->m_pCombatManager->ClientKilledHandler(sOwnerH, sOwnerH, cOwnerType, iDamage);
							}
							else {
								if (iDamage > 0) {
									m_pGame->SendNotifyMsg(0, sOwnerH, Notify::Hp, 0, 0, 0, 0);
								}
							}
						}

						// Ice Object   .
						m_pGame->m_pMapList[m_pDynamicObjectList[i]->m_cMapIndex]->bGetDynamicObject(ix, iy, &sType, &dwRegisterTime, &iIndex);
						if ((sType == dynamic_object::IceStorm) && (m_pDynamicObjectList[iIndex] != 0))
							m_pDynamicObjectList[iIndex]->m_dwLastTime = m_pDynamicObjectList[iIndex]->m_dwLastTime - (m_pDynamicObjectList[iIndex]->m_dwLastTime / 10);
					}
				break;
			}
		}
}
