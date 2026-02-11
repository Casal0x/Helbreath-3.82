// QuestManager.cpp: Handles client-side quest network messages.
// Extracted from NetworkMessages_Quest.cpp (Phase B3).

#include "QuestManager.h"
#include "Game.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

void QuestManager::HandleQuestCounter(char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQuestCounter>(
		pData, sizeof(hb::net::PacketNotifyQuestCounter));
	if (!pkt) return;
	m_pGame->m_stQuest.sCurrentCount = static_cast<short>(pkt->current_count);
}

void QuestManager::HandleQuestContents(char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQuestContents>(
		pData, sizeof(hb::net::PacketNotifyQuestContents));
	if (!pkt) return;
	m_pGame->m_stQuest.sWho = pkt->who;
	m_pGame->m_stQuest.sQuestType = pkt->quest_type;
	m_pGame->m_stQuest.sContribution = pkt->contribution;
	m_pGame->m_stQuest.sTargetType = pkt->target_type;
	m_pGame->m_stQuest.sTargetCount = pkt->target_count;
	m_pGame->m_stQuest.sX = pkt->x;
	m_pGame->m_stQuest.sY = pkt->y;
	m_pGame->m_stQuest.sRange = pkt->range;
	m_pGame->m_stQuest.bIsQuestCompleted = (pkt->is_completed != 0);
	std::memset(m_pGame->m_stQuest.cTargetName, 0, sizeof(m_pGame->m_stQuest.cTargetName));
	memcpy(m_pGame->m_stQuest.cTargetName, pkt->target_name, 20);
}

void QuestManager::HandleQuestReward(char* pData)
{
	short sWho, sFlag;
	char cRewardName[hb::shared::limits::ItemNameLen], cTxt[120];
	int iAmount, iIndex, iPreCon;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyQuestReward>(
		pData, sizeof(hb::net::PacketNotifyQuestReward));
	if (!pkt) return;
	sWho = pkt->who;
	sFlag = pkt->flag;
	iAmount = pkt->amount;
	std::memset(cRewardName, 0, sizeof(cRewardName));
	memcpy(cRewardName, pkt->reward_name, sizeof(pkt->reward_name));
	iPreCon = m_pGame->m_pPlayer->m_iContribution;
	m_pGame->m_pPlayer->m_iContribution = pkt->contribution;

	if (sFlag == 1)
	{
		m_pGame->m_stQuest.sWho = 0;
		m_pGame->m_stQuest.sQuestType = 0;
		m_pGame->m_stQuest.sContribution = 0;
		m_pGame->m_stQuest.sTargetType = 0;
		m_pGame->m_stQuest.sTargetCount = 0;
		m_pGame->m_stQuest.sX = 0;
		m_pGame->m_stQuest.sY = 0;
		m_pGame->m_stQuest.sRange = 0;
		m_pGame->m_stQuest.sCurrentCount = 0;
		m_pGame->m_stQuest.bIsQuestCompleted = false;
		std::memset(m_pGame->m_stQuest.cTargetName, 0, sizeof(m_pGame->m_stQuest.cTargetName));
		m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcTalk, 0, sWho + 110, 0);
		iIndex = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sV1;
		m_pGame->m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
		iIndex++;
		std::memset(cTxt, 0, sizeof(cTxt));
		if (memcmp(cRewardName, "ġ", 6) == 0)
		{
			if (iAmount > 0) std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_QUEST_REWARD1, iAmount);
		}
		else
		{
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_QUEST_REWARD2, iAmount, cRewardName);
		}
		m_pGame->m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
		iIndex++;
		m_pGame->m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
		iIndex++;
		std::memset(cTxt, 0, sizeof(cTxt));
		if (iPreCon < m_pGame->m_pPlayer->m_iContribution)
			std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_QUEST_REWARD3, m_pGame->m_pPlayer->m_iContribution - iPreCon);
		else std::snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_QUEST_REWARD4, iPreCon - m_pGame->m_pPlayer->m_iContribution);

		m_pGame->m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
		iIndex++;
	}
	else m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcTalk, 0, sWho + 120, 0);
}

void QuestManager::HandleQuestCompleted(char* pData)
{
	m_pGame->m_stQuest.bIsQuestCompleted = true;
	m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Quest);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::Quest, 1, 0, 0);
	switch (m_pGame->m_pPlayer->m_sPlayerType) {
	case 1:
	case 2:
	case 3:
		m_pGame->PlayGameSound('C', 21, 0);
		break;
	case 4:
	case 5:
	case 6:
		m_pGame->PlayGameSound('C', 22, 0);
		break;
	}
	m_pGame->PlayGameSound('E', 23, 0);
	m_pGame->AddEventList(NOTIFY_MSG_HANDLER44, 10);
}

void QuestManager::HandleQuestAborted(char* pData)
{
	m_pGame->m_stQuest.sQuestType = 0;
	m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Quest);
	m_pGame->m_dialogBoxManager.EnableDialogBox(DialogBoxId::Quest, 2, 0, 0);
}
