#include "Game.h"
#include "GameModeManager.h"
#include "AudioManager.h"

extern char G_cSpriteAlphaDegree;

#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace hb::shared::net;
namespace NetworkMessageHandlers {

void HandleWhetherChange(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyWhetherChange>(
		pData, sizeof(hb::net::PacketNotifyWhetherChange));
	if (!pkt) return;
	pGame->m_cWhetherStatus = static_cast<char>(pkt->status);

	if (pGame->m_cWhetherStatus != 0)
		pGame->SetWhetherStatus(true, pGame->m_cWhetherStatus);
	else pGame->SetWhetherStatus(false, 0);
}

void HandleTimeChange(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyTimeChange>(
		pData, sizeof(hb::net::PacketNotifyTimeChange));
	if (!pkt) return;
	G_cSpriteAlphaDegree = static_cast<char>(pkt->sprite_alpha);
	switch (G_cSpriteAlphaDegree) {
	case 1:	pGame->m_bIsXmas = false; pGame->PlayGameSound('E', 32, 0); break;
	case 2: pGame->m_bIsXmas = false; pGame->PlayGameSound('E', 31, 0); break;
	case 3: // Snoopy Special night with chrismas bulbs
		if (pGame->m_cWhetherEffectType > 3) pGame->m_bIsXmas = true;
		else pGame->m_bIsXmas = false;
		pGame->PlayGameSound('E', 31, 0);
		G_cSpriteAlphaDegree = 2; break;
	}
}

void HandleNoticeMsg(CGame* pGame, char* pData)
{
	char cMsg[1000];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyNoticeMsg>(
		pData, sizeof(hb::net::PacketNotifyNoticeMsg));
	if (!pkt) return;
	std::snprintf(cMsg, sizeof(cMsg), "%s", pkt->text);
	pGame->AddEventList(cMsg, 10);
}

void HandleStatusText(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyStatusText>(
		pData, sizeof(hb::net::PacketNotifyStatusText));
	if (!pkt) return;

	// Display floating text above the local player's head (like "* Failed! *")
	pGame->m_floatingText.AddDamageText(DamageTextType::Medium, pkt->text, pGame->m_dwCurTime,
		pGame->m_pPlayer->m_sPlayerObjectID, pGame->m_pMapData.get());
}

void HandleForceDisconn(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyForceDisconn>(
		pData, sizeof(hb::net::PacketNotifyForceDisconn));
	if (!pkt) return;
	const auto wpCount = pkt->seconds;
	pGame->m_bForceDisconn = true;
	//m_cLogOutCount = (char)*wpCount;
	if (pGame->m_cLogOutCount < 0 || pGame->m_cLogOutCount > 5) pGame->m_cLogOutCount = 5;
	pGame->AddEventList(NOTIFYMSG_FORCE_DISCONN1, 10);
}

void HandleSettingSuccess(CGame* pGame, char* pData)
{
	int iPrevLevel;
	char cTxt[120];
	iPrevLevel = pGame->m_pPlayer->m_iLevel;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyLevelUp>(
		pData, sizeof(hb::net::PacketNotifyLevelUp));
	if (!pkt) return;
	pGame->m_pPlayer->m_iLevel = pkt->level;
	pGame->m_pPlayer->m_iStr = pkt->str;
	pGame->m_pPlayer->m_iVit = pkt->vit;
	pGame->m_pPlayer->m_iDex = pkt->dex;
	pGame->m_pPlayer->m_iInt = pkt->intel;
	pGame->m_pPlayer->m_iMag = pkt->mag;
	pGame->m_pPlayer->m_iCharisma = pkt->chr;
	pGame->m_pPlayer->m_playerStatus.iAttackDelay = pkt->attack_delay;
	std::snprintf(cTxt, sizeof(cTxt), "Your stat has been changed.");
	pGame->AddEventList(cTxt, 10);
	// CLEROTH - LU
	pGame->m_pPlayer->m_iLU_Point = (pGame->m_pPlayer->m_iLevel - 1) * 3 - ((pGame->m_pPlayer->m_iStr + pGame->m_pPlayer->m_iVit + pGame->m_pPlayer->m_iDex + pGame->m_pPlayer->m_iInt + pGame->m_pPlayer->m_iMag + pGame->m_pPlayer->m_iCharisma) - 70);
	pGame->m_pPlayer->m_wLU_Str = pGame->m_pPlayer->m_wLU_Vit = pGame->m_pPlayer->m_wLU_Dex = pGame->m_pPlayer->m_wLU_Int = pGame->m_pPlayer->m_wLU_Mag = pGame->m_pPlayer->m_wLU_Char = 0;
}

void HandleServerChange(CGame* pGame, char* pData)
{
	if (pGame->m_bIsServerChanging) return;

	char cWorldServerAddr[16];
	int iWorldServerPort;

	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyServerChange>(
		pData, sizeof(hb::net::PacketNotifyServerChange));
	if (!pkt) return;

	pGame->m_bIsServerChanging = true;

	std::memset(pGame->m_cMapName, 0, sizeof(pGame->m_cMapName));
	std::memset(pGame->m_cMapMessage, 0, sizeof(pGame->m_cMapMessage));
	std::memset(cWorldServerAddr, 0, sizeof(cWorldServerAddr));

	memcpy(pGame->m_cMapName, pkt->map_name, 10);
	memcpy(cWorldServerAddr, pkt->log_server_addr, 15);
	iWorldServerPort = pkt->log_server_port;
	if (pGame->m_pGSock != 0)
	{
		pGame->m_pGSock.reset();
	}
	if (pGame->m_pLSock != 0)
	{
		pGame->m_pLSock.reset();
	}
	pGame->m_pLSock = std::make_unique<hb::shared::net::ASIOSocket>(pGame->m_pIOPool->GetContext(), game_limits::socket_block_limit);
	pGame->m_pLSock->bConnect(pGame->m_cLogServerAddr, iWorldServerPort);
	pGame->m_pLSock->bInitBufferSize(hb::shared::limits::MsgBufferSize);

	pGame->m_pPlayer->m_bIsPoisoned = false;

	pGame->ChangeGameMode(GameMode::Connecting);
	pGame->m_dwConnectMode = MsgId::RequestEnterGame;
	//m_wEnterGameType = EnterGameMsg::New; //Gateway
	pGame->m_wEnterGameType = EnterGameMsg::NewToWlsButMls;
	std::memset(pGame->m_cMsg, 0, sizeof(pGame->m_cMsg));
	std::snprintf(pGame->m_cMsg, sizeof(pGame->m_cMsg), "%s", "55");
}

void HandleTotalUsers(CGame* pGame, char* pData)
{
	int iTotal;
	char cTxt[128];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyTotalUsers>(
		pData, sizeof(hb::net::PacketNotifyTotalUsers));
	if (!pkt) return;
	iTotal = pkt->total;
	snprintf(cTxt, sizeof(cTxt), NOTIFYMSG_TOTAL_USER1, iTotal);
	pGame->AddEventList(cTxt, 10);
}

void HandleChangePlayMode(CGame* pGame, char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyChangePlayMode>(
		pData, sizeof(hb::net::PacketNotifyChangePlayMode));
	if (!pkt) return;
	memcpy(pGame->m_cLocation, pkt->location, sizeof(pkt->location));

	if (memcmp(pGame->m_cLocation, "aresden", 7) == 0)
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(pGame->m_cLocation, "arehunter", 9) == 0)
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = true;
	}
	else if (memcmp(pGame->m_cLocation, "elvine", 6) == 0)
	{
		pGame->m_pPlayer->m_bAresden = false;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(pGame->m_cLocation, "elvhunter", 9) == 0)
	{
		pGame->m_pPlayer->m_bAresden = false;
		pGame->m_pPlayer->m_bCitizen = true;
		pGame->m_pPlayer->m_bHunter = true;
	}
	else
	{
		pGame->m_pPlayer->m_bAresden = true;
		pGame->m_pPlayer->m_bCitizen = false;
		pGame->m_pPlayer->m_bHunter = true;
	}
	pGame->AddEventList(DEF_MSG_GAMEMODE_CHANGED, 10);
}

void HandleForceRecallTime(CGame* pGame, char* pData)
{
	short sV1;
	char cTxt[128];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyForceRecallTime>(
		pData, sizeof(hb::net::PacketNotifyForceRecallTime));
	if (!pkt) return;
	sV1 = static_cast<short>(pkt->seconds_left);
	if ((int)(sV1 / 20) > 0)
		snprintf(cTxt, sizeof(cTxt), NOTIFY_MSG_FORCERECALLTIME1, (int)(sV1 / 20));
	else
		snprintf(cTxt, sizeof(cTxt), "%s", NOTIFY_MSG_FORCERECALLTIME2);
	pGame->AddEventList(cTxt, 10);
}

void HandleNoRecall(CGame* pGame, char* pData)
{
	pGame->AddEventList("You can not recall in this map.", 10);
}

void HandleFightZoneReserve(CGame* pGame, char* pData)
{
	char cTxt[120];
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyFightZoneReserve>(
		pData, sizeof(hb::net::PacketNotifyFightZoneReserve));
	if (!pkt) return;
	switch (pkt->result) {
	case -5:
		pGame->AddEventList(NOTIFY_MSG_HANDLER68, 10);
		break;
	case -4:
		pGame->AddEventList(NOTIFY_MSG_HANDLER69, 10);
		break;
	case -3:
		pGame->AddEventList(NOTIFY_MSG_HANDLER70, 10);
		break;
	case -2:
		pGame->m_iFightzoneNumber = 0;
		pGame->AddEventList(NOTIFY_MSG_HANDLER71, 10);
		break;
	case -1:
		pGame->m_iFightzoneNumber = pGame->m_iFightzoneNumber * -1;
		pGame->AddEventList(NOTIFY_MSG_HANDLER72, 10);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		std::snprintf(cTxt, sizeof(cTxt), NOTIFY_MSG_HANDLER73, pkt->result);
		pGame->AddEventList(cTxt, 10);
		break;
	}
}

void HandleLoteryLost(CGame* pGame, char* pData)
{
	pGame->AddEventList(DEF_MSG_NOTIFY_LOTERY_LOST, 10);
}

void HandleNotFlagSpot(CGame* pGame, char* pData)
{
	pGame->AddEventList(NOTIFY_MSG_HANDLER45, 10);
}

void HandleNpcTalk(CGame* pGame, char* pData)
{
	pGame->NpcTalkHandler(pData);
}

void HandleTravelerLimitedLevel(CGame* pGame, char* pData)
{
	pGame->AddEventList(NOTIFY_MSG_HANDLER64, 10);
}

void HandleLimitedLevel(CGame* pGame, char* pData)
{
	pGame->AddEventList(NOTIFYMSG_LIMITED_LEVEL1, 10);
}

void HandleToBeRecalled(CGame* pGame, char* pData)
{
	pGame->AddEventList(NOTIFY_MSG_HANDLER62, 10);
}

} // namespace NetworkMessageHandlers