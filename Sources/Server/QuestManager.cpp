// QuestManager.cpp: Manages quest assignment, progress tracking, and completion.
// Extracted from CGame (Phase B3).

#include "QuestManager.h"
#include "Game.h"
#include "ItemManager.h"
#include "Quest.h"
#include "Item.h"
#include "Packet/SharedPackets.h"

using namespace hb::shared::net;
using namespace hb::server::config;
namespace squest = hb::server::quest;

QuestManager::QuestManager()
{
	for (int i = 0; i < MaxQuestType; i++)
		m_pQuestConfigList[i] = 0;
}

QuestManager::~QuestManager()
{
	CleanupArrays();
}

void QuestManager::InitArrays()
{
	for (int i = 0; i < MaxQuestType; i++)
		m_pQuestConfigList[i] = 0;
}

void QuestManager::CleanupArrays()
{
	for (int i = 0; i < MaxQuestType; i++)
		if (m_pQuestConfigList[i] != 0) delete m_pQuestConfigList[i];
}

void QuestManager::NpcTalkHandler(int iClientH, int iWho)
{
	char cRewardName[hb::shared::limits::ItemNameLen], cTargetName[hb::shared::limits::NpcNameLen];
	int iResMode, iQuestNum, iQuestType, iRewardType, iRewardAmount, iContribution, iX, iY, iRange, iTargetType, iTargetCount;

	iQuestNum = 0;
	std::memset(cTargetName, 0, sizeof(cTargetName));
	if (m_pGame->m_pClientList[iClientH] == 0) return;
	switch (iWho) {
	case 1: break;
	case 2:	break;
	case 3:	break;
	case 4:
		iQuestNum = _iTalkToNpcResult_Cityhall(iClientH, &iQuestType, &iResMode, &iRewardType, &iRewardAmount, &iContribution, cTargetName, &iTargetType, &iTargetCount, &iX, &iY, &iRange);
		break;
	case 5: break;
	case 6:	break;
	case 32: break;
	case 21:
		iQuestNum = _iTalkToNpcResult_Guard(iClientH, &iQuestType, &iResMode, &iRewardType, &iRewardAmount, &iContribution, cTargetName, &iTargetType, &iTargetCount, &iX, &iY, &iRange);
		if (iQuestNum >= 1000) return;
		break;
	}

	std::memset(cRewardName, 0, sizeof(cRewardName));
	if (iQuestNum > 0) {
		if (iRewardType > 1) {
			strcpy(cRewardName, m_pGame->m_pItemConfigList[iRewardType]->m_cName);
		}
		else {
			switch (iRewardType) {
			case -10: strcpy(cRewardName, "���F-�"); break;
			}
		}

		m_pGame->m_pClientList[iClientH]->m_iAskedQuest = iQuestNum;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardType = iRewardType;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount = iRewardAmount;

		m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, iQuestType, iResMode, iRewardAmount, cRewardName, iContribution,
			iTargetType, iTargetCount, iX, iY, iRange, cTargetName);
	}
	else {
		switch (iQuestNum) {
		case  0: m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (iWho + 130), 0, 0, 0, 0); break;
		case -1:
		case -2:
		case -3:
		case -4: m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, abs(iQuestNum) + 100, 0, 0, 0, 0); break;
		case -5: break;
		}
	}
}

int QuestManager::_iTalkToNpcResult_Cityhall(int iClientH, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange)
{
	int iQuest, iEraseReq;
	CItem* pItem;
	uint32_t iExp;

	if (m_pGame->m_pClientList[iClientH] == 0) return 0;

	if (m_pGame->m_pClientList[iClientH]->m_iQuest != 0) {
		if (m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest] == 0) return -4;
		else if (m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest]->m_iFrom == 4) {
			if (m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted) {
				if ((m_pGame->m_pClientList[iClientH]->m_iQuestRewardType > 0) &&
					(m_pGame->m_pItemConfigList[m_pGame->m_pClientList[iClientH]->m_iQuestRewardType] != 0)) {
					pItem = new CItem;
					m_pGame->m_pItemManager->_bInitItemAttr(pItem, m_pGame->m_pItemConfigList[m_pGame->m_pClientList[iClientH]->m_iQuestRewardType]->m_cName);
					pItem->m_dwCount = m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount;
					if (m_pGame->m_pItemManager->_bCheckItemReceiveCondition(iClientH, pItem)) {
						m_pGame->m_pItemManager->_bAddClientItemList(iClientH, pItem, &iEraseReq);
						m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::ItemObtained, pItem, 0);
						if (iEraseReq == 1) delete pItem;

						m_pGame->m_pClientList[iClientH]->m_iContribution += m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest]->m_iContribution;

						m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestReward, 4, 1, m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount,
							m_pGame->m_pItemConfigList[m_pGame->m_pClientList[iClientH]->m_iQuestRewardType]->m_cName, m_pGame->m_pClientList[iClientH]->m_iContribution);

						_ClearQuestStatus(iClientH);
						return -5;
					}
					else {
						delete pItem;
						m_pGame->m_pItemManager->SendItemNotifyMsg(iClientH, Notify::CannotCarryMoreItem, 0, 0);

						m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestReward, 4, 0, m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount,
							m_pGame->m_pItemConfigList[m_pGame->m_pClientList[iClientH]->m_iQuestRewardType]->m_cName, m_pGame->m_pClientList[iClientH]->m_iContribution);

						return -5;
					}
				}
				else if (m_pGame->m_pClientList[iClientH]->m_iQuestRewardType == -1) {
					m_pGame->m_pClientList[iClientH]->m_iExpStock += m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount;
					m_pGame->m_pClientList[iClientH]->m_iContribution += m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest]->m_iContribution;

					m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestReward, 4, 1, m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount,
						"°æÇèÄ¡              ", m_pGame->m_pClientList[iClientH]->m_iContribution);

					_ClearQuestStatus(iClientH);
					return -5;
				}
				else if (m_pGame->m_pClientList[iClientH]->m_iQuestRewardType == -2) {
					iExp = m_pGame->iDice(1, (10 * m_pGame->m_pClientList[iClientH]->m_iLevel));
					iExp = iExp * m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount;

					m_pGame->m_pClientList[iClientH]->m_iExpStock += iExp;
					m_pGame->m_pClientList[iClientH]->m_iContribution += m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest]->m_iContribution;

					m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestReward, 4, 1, iExp,
						"°æÇèÄ¡              ", m_pGame->m_pClientList[iClientH]->m_iContribution);

					_ClearQuestStatus(iClientH);
					return -5;
				}
				else {
					m_pGame->m_pClientList[iClientH]->m_iContribution += m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iQuest]->m_iContribution;

					m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestReward, 4, 1, 0,
						"                     ", m_pGame->m_pClientList[iClientH]->m_iContribution);

					_ClearQuestStatus(iClientH);
					return -5;
				}
			}
			else return -1;
		}

		return -4;
	}

	if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cLocationName, 10) == 0) {
		if (m_pGame->m_pClientList[iClientH]->m_iPKCount > 0) return -3;

		iQuest = __iSearchForQuest(iClientH, 4, pQuestType, pMode, pRewardType, pRewardAmount, pContribution, pTargetName, pTargetType, pTargetCount, pX, pY, pRange);
		if (iQuest <= 0) return -4;

		return iQuest;
	}
	else return -2;

	return -4;
}


int QuestManager::_iTalkToNpcResult_Guard(int iClientH, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return 0;

	if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "are", 3) == 0) {
		if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "aresden", 7) == 0) {
			m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (200), 0, 0, 0, 0);
			return 1000;
		}
		else
			if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elv", 3) == 0) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (201), 0, 0, 0, 0);
				return 1001;
			}
	}
	else
		if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "elv", 3) == 0) {
			if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "aresden", 7) == 0) {
				m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (202), 0, 0, 0, 0);
				return 1002;
			}
			else
				if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elv", 3) == 0) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (203), 0, 0, 0, 0);
					return 1003;
				}
		}
		else
			if (memcmp(m_pGame->m_pClientList[iClientH]->m_cLocation, "NONE", 4) == 0) {
				if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "aresden", 7) == 0) {
					m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (204), 0, 0, 0, 0);
					return 1004;
				}
				else
					if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "elvine", 6) == 0) {
						m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (205), 0, 0, 0, 0);
						return 1005;
					}
					else
						if (memcmp(m_pGame->m_pClientList[iClientH]->m_cMapName, "default", 7) == 0) {
							m_pGame->SendNotifyMsg(0, iClientH, Notify::NpcTalk, (206), 0, 0, 0, 0);
							return 1006;
						}
			}

	return 0;
}


int QuestManager::__iSearchForQuest(int iClientH, int iWho, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange)
{
	int iQuestList[MaxQuestType], iIndex, iQuest, iReward, iQuestIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return -1;

	iIndex = 0;
	for(int i = 0; i < MaxQuestType; i++)
		iQuestList[i] = -1;

	for(int i = 1; i < MaxQuestType; i++)
		if (m_pQuestConfigList[i] != 0) {

			if (m_pQuestConfigList[i]->m_iFrom != iWho) goto SFQ_SKIP;
			if (m_pQuestConfigList[i]->m_cSide != m_pGame->m_pClientList[iClientH]->m_cSide) goto SFQ_SKIP;
			if (m_pQuestConfigList[i]->m_iMinLevel > m_pGame->m_pClientList[iClientH]->m_iLevel) goto SFQ_SKIP;
			if (m_pQuestConfigList[i]->m_iMaxLevel < m_pGame->m_pClientList[iClientH]->m_iLevel) goto SFQ_SKIP;
			if (m_pQuestConfigList[i]->m_iReqContribution > m_pGame->m_pClientList[iClientH]->m_iContribution) goto SFQ_SKIP;

			if (m_pQuestConfigList[i]->m_iRequiredSkillNum != -1) {
				if (m_pGame->m_pClientList[iClientH]->m_cSkillMastery[m_pQuestConfigList[i]->m_iRequiredSkillNum] <
					m_pQuestConfigList[i]->m_iRequiredSkillLevel) goto SFQ_SKIP;
			}

			if ((m_pGame->m_bIsCrusadeMode) && (m_pQuestConfigList[i]->m_iAssignType != 1)) goto SFQ_SKIP;
			if ((m_pGame->m_bIsCrusadeMode == false) && (m_pQuestConfigList[i]->m_iAssignType == 1)) goto SFQ_SKIP;

			if (m_pQuestConfigList[i]->m_iContributionLimit < m_pGame->m_pClientList[iClientH]->m_iContribution) goto SFQ_SKIP;

			iQuestList[iIndex] = i;
			iIndex++;

		SFQ_SKIP:;
		}

	// iIndex     .    1 .
	if (iIndex == 0) return -1;
	iQuest = (m_pGame->iDice(1, iIndex)) - 1;
	iQuestIndex = iQuestList[iQuest];
	iReward = m_pGame->iDice(1, 3);
	*pMode = m_pQuestConfigList[iQuestIndex]->m_iResponseMode;
	*pRewardType = m_pQuestConfigList[iQuestIndex]->m_iRewardType[iReward];
	*pRewardAmount = m_pQuestConfigList[iQuestIndex]->m_iRewardAmount[iReward];
	*pContribution = m_pQuestConfigList[iQuestIndex]->m_iContribution;

	strcpy(pTargetName, m_pQuestConfigList[iQuestIndex]->m_cTargetName);
	*pX = m_pQuestConfigList[iQuestIndex]->m_sX;
	*pY = m_pQuestConfigList[iQuestIndex]->m_sY;
	*pRange = m_pQuestConfigList[iQuestIndex]->m_iRange;

	*pTargetType = m_pQuestConfigList[iQuestIndex]->m_iTargetType;
	*pTargetCount = m_pQuestConfigList[iQuestIndex]->m_iMaxCount;
	*pQuestType = m_pQuestConfigList[iQuestIndex]->m_iType;

	return iQuestIndex;
}

// New 14/05/2004
void QuestManager::QuestAcceptedHandler(int iClientH)
{
	int iIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	// Does the quest exist ??
	if (m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest] == 0) return;

	if (m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest]->m_iAssignType == 1) {
		switch (m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest]->m_iType) {
		case 10:
			_ClearQuestStatus(iClientH);
			m_pGame->RequestTeleportHandler(iClientH, "2   ", m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest]->m_cTargetName,
				m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest]->m_sX, m_pQuestConfigList[m_pGame->m_pClientList[iClientH]->m_iAskedQuest]->m_sY);
			return;
		}
	}

	m_pGame->m_pClientList[iClientH]->m_iQuest = m_pGame->m_pClientList[iClientH]->m_iAskedQuest;
	iIndex = m_pGame->m_pClientList[iClientH]->m_iQuest;
	m_pGame->m_pClientList[iClientH]->m_iQuestID = m_pQuestConfigList[iIndex]->m_iQuestID;
	m_pGame->m_pClientList[iClientH]->m_iCurQuestCount = 0;
	m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted = false;

	_CheckQuestEnvironment(iClientH);
	_SendQuestContents(iClientH);
}


void QuestManager::_SendQuestContents(int iClientH)
{
	int iWho, iIndex, iQuestType, iContribution, iTargetType, iTargetCount, iX, iY, iRange, iQuestCompleted;
	char cTargetName[hb::shared::limits::NpcNameLen];

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	iIndex = m_pGame->m_pClientList[iClientH]->m_iQuest;
	if (iIndex == 0) {
		// Quest .
		m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestContents, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0);
	}
	else {
		// Quest  .
		iWho = m_pQuestConfigList[iIndex]->m_iFrom;
		iQuestType = m_pQuestConfigList[iIndex]->m_iType;
		iContribution = m_pQuestConfigList[iIndex]->m_iContribution;
		iTargetType = m_pQuestConfigList[iIndex]->m_iTargetType;
		iTargetCount = m_pQuestConfigList[iIndex]->m_iMaxCount;
		iX = m_pQuestConfigList[iIndex]->m_sX;
		iY = m_pQuestConfigList[iIndex]->m_sY;
		iRange = m_pQuestConfigList[iIndex]->m_iRange;
		std::memset(cTargetName, 0, sizeof(cTargetName));
		memcpy(cTargetName, m_pQuestConfigList[iIndex]->m_cTargetName, hb::shared::limits::NpcNameLen - 1);
		iQuestCompleted = (int)m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted;

		m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestContents, iWho, iQuestType, iContribution, 0,
			iTargetType, iTargetCount, iX, iY, iRange, iQuestCompleted, cTargetName);
	}
}

void QuestManager::_CheckQuestEnvironment(int iClientH)
{
	int iIndex;
	char cTargetName[hb::shared::limits::NpcNameLen];

	if (m_pGame->m_pClientList[iClientH] == 0) return;

	iIndex = m_pGame->m_pClientList[iClientH]->m_iQuest;
	if (iIndex == 0) return;
	if (m_pQuestConfigList[iIndex] == 0) return;

	if (iIndex >= 35 && iIndex <= 40) {
		m_pGame->m_pClientList[iClientH]->m_iQuest = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestID = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardType = 0;
		m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestAborted, 0, 0, 0, 0);
		return;
	}

	if (m_pQuestConfigList[iIndex]->m_iQuestID != m_pGame->m_pClientList[iClientH]->m_iQuestID) {
		m_pGame->m_pClientList[iClientH]->m_iQuest = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestID = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount = 0;
		m_pGame->m_pClientList[iClientH]->m_iQuestRewardType = 0;

		m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestAborted, 0, 0, 0, 0);
		return;
	}

	switch (m_pQuestConfigList[iIndex]->m_iType) {
	case squest::Type::MonsterHunt:
	case squest::Type::GoPlace:
		std::memset(cTargetName, 0, sizeof(cTargetName));
		memcpy(cTargetName, m_pQuestConfigList[iIndex]->m_cTargetName, hb::shared::limits::NpcNameLen - 1);
		if (memcmp(m_pGame->m_pMapList[m_pGame->m_pClientList[iClientH]->m_cMapIndex]->m_cName, cTargetName, 10) == 0)
			m_pGame->m_pClientList[iClientH]->m_bQuestMatchFlag_Loc = true;
		else m_pGame->m_pClientList[iClientH]->m_bQuestMatchFlag_Loc = false;
		break;
	}

}

bool QuestManager::_bCheckIsQuestCompleted(int iClientH)
{
	int iQuestIndex;

	if (m_pGame->m_pClientList[iClientH] == 0) return false;
	if (m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted) return false;
	iQuestIndex = m_pGame->m_pClientList[iClientH]->m_iQuest;
	if (iQuestIndex == 0) return false;

	if (m_pQuestConfigList[iQuestIndex] != 0) {
		switch (m_pQuestConfigList[iQuestIndex]->m_iType) {
		case squest::Type::MonsterHunt:
			if ((m_pGame->m_pClientList[iClientH]->m_bQuestMatchFlag_Loc) &&
				(m_pGame->m_pClientList[iClientH]->m_iCurQuestCount >= m_pQuestConfigList[iQuestIndex]->m_iMaxCount)) {
				m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted = true;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestCompleted, 0, 0, 0, 0);
				return true;
			}
			break;

		case squest::Type::GoPlace:
			if ((m_pGame->m_pClientList[iClientH]->m_bQuestMatchFlag_Loc) &&
				(m_pGame->m_pClientList[iClientH]->m_sX >= m_pQuestConfigList[iQuestIndex]->m_sX - m_pQuestConfigList[iQuestIndex]->m_iRange) &&
				(m_pGame->m_pClientList[iClientH]->m_sX <= m_pQuestConfigList[iQuestIndex]->m_sX + m_pQuestConfigList[iQuestIndex]->m_iRange) &&
				(m_pGame->m_pClientList[iClientH]->m_sY >= m_pQuestConfigList[iQuestIndex]->m_sY - m_pQuestConfigList[iQuestIndex]->m_iRange) &&
				(m_pGame->m_pClientList[iClientH]->m_sY <= m_pQuestConfigList[iQuestIndex]->m_sY + m_pQuestConfigList[iQuestIndex]->m_iRange)) {
				m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted = true;
				m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestCompleted, 0, 0, 0, 0);
				return true;
			}
			break;
		}
	}

	return false;
}

void QuestManager::_ClearQuestStatus(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;

	m_pGame->m_pClientList[iClientH]->m_iQuest = 0;
	m_pGame->m_pClientList[iClientH]->m_iQuestID = 0;
	m_pGame->m_pClientList[iClientH]->m_iQuestRewardType = 0;
	m_pGame->m_pClientList[iClientH]->m_iQuestRewardAmount = 0;
	m_pGame->m_pClientList[iClientH]->m_bIsQuestCompleted = false;
}

void QuestManager::CancelQuestHandler(int iClientH)
{
	if (m_pGame->m_pClientList[iClientH] == 0) return;

	_ClearQuestStatus(iClientH);
	m_pGame->SendNotifyMsg(0, iClientH, Notify::QuestAborted, 0, 0, 0, 0);
}
