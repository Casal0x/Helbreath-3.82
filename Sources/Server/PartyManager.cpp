// PartyManager.cpp: implementation of the PartyManager class.
//
//////////////////////////////////////////////////////////////////////

#include "CommonTypes.h"
#include "PartyManager.h"
#include "Packet/SharedPackets.h"
#include "Log.h"
#include "StringCompat.h"

extern char G_cTxt[120];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PartyManager::PartyManager(class CGame* pGame)
{
	

	for(int i = 0; i < hb::server::party::MaxParty; i++) {
		m_iMemberNumList[i] = 0;
		m_stMemberNameList[i].m_iPartyID = 0;
		m_stMemberNameList[i].m_iIndex = 0;
		m_stMemberNameList[i].m_dwServerChangeTime = 0;
		std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));
	}

	m_pGame = pGame;
	m_dwCheckMemberActTime = GameClock::GetTimeMS();
}

PartyManager::~PartyManager()
{

}

int PartyManager::iCreateNewParty(char* pMasterName)
{
	int iPartyID;

	// ??? PartyMaster? ????? ?? 
	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID != 0) && (hb_stricmp(m_stMemberNameList[i].m_cName, pMasterName) == 0)) return 0;

	iPartyID = 0;
	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if (m_iMemberNumList[i] == 0) {
			// Party ID? i, ??? ??
			iPartyID = i;
			m_iMemberNumList[iPartyID]++;
			goto CNP_BREAKLOOP;
		}

	return 0;

CNP_BREAKLOOP:;
	// ?? ??? ????.
	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if (m_stMemberNameList[i].m_iPartyID == 0) {
			m_stMemberNameList[i].m_iPartyID = iPartyID;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));
			strcpy(m_stMemberNameList[i].m_cName, pMasterName);
			m_stMemberNameList[i].m_iIndex = 1;

			//testcode
			hb::logger::log("New party(ID:{} Master:{})", iPartyID, pMasterName);

			return iPartyID;
		}

	return 0;
}

bool PartyManager::bDeleteParty(int iPartyID)
{
	bool bFlag;
	char cData[120];

	bFlag = false;
	m_iMemberNumList[iPartyID] = 0;

	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if (m_stMemberNameList[i].m_iPartyID == iPartyID) {
			m_stMemberNameList[i].m_iPartyID = 0;
			m_stMemberNameList[i].m_iIndex = 0;
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));
			bFlag = true;
		}

	// Notify game server about party deletion
	std::memset(cData, 0, sizeof(cData));
	hb::net::PartyOpResultDelete delResult{};
	delResult.op_type = 2;
	delResult.party_id = static_cast<uint16_t>(iPartyID);
	std::memcpy(cData, &delResult, sizeof(delResult));
	m_pGame->PartyOperationResultHandler(cData);

	//testcode
	hb::logger::log("Delete party(ID:{})", iPartyID);

	return bFlag;
}

bool PartyManager::bAddMember(int iPartyID, char* pMemberName)
{
	

	if (m_iMemberNumList[iPartyID] >= hb::server::party::MaxPartyMember) return false;

	// ?? ??? ?? ???? ??
	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID != 0) && (hb_stricmp(m_stMemberNameList[i].m_cName, pMemberName) == 0))
		{
			m_stMemberNameList[i].m_iPartyID = 0;
			m_stMemberNameList[i].m_iIndex = 0;
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));

			m_iMemberNumList[iPartyID]--;
			if (m_iMemberNumList[iPartyID] <= 1) bDeleteParty(iPartyID); // ???? 1?? ??? ?? ??
		}
	//		return false;

	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if (m_stMemberNameList[i].m_iPartyID == 0) {
			m_stMemberNameList[i].m_iPartyID = iPartyID;
			m_stMemberNameList[i].m_iIndex = 1;
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));
			strcpy(m_stMemberNameList[i].m_cName, pMemberName);
			m_iMemberNumList[iPartyID]++;

			//testcode
			hb::logger::log("Add Member: PartyID:{} Name:{}", iPartyID, pMemberName);
			return true;
		}

	return false;
}

bool PartyManager::bRemoveMember(int iPartyID, char* pMemberName)
{
	

	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (hb_stricmp(m_stMemberNameList[i].m_cName, pMemberName) == 0)) {

			m_stMemberNameList[i].m_iPartyID = 0;
			m_stMemberNameList[i].m_iIndex = 0;
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));

			m_iMemberNumList[iPartyID]--;
			if (m_iMemberNumList[iPartyID] <= 1) bDeleteParty(iPartyID); // ???? 1?? ??? ?? ?? 

			//testcode
			hb::logger::log("Remove Member: PartyID:{} Name:{}", iPartyID, pMemberName);
			return true;
		}

	return false;
}

bool PartyManager::bCheckPartyMember(int iGSCH, int iPartyID, char* pName)
{
	char cData[120]{};

	for (int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (hb_stricmp(m_stMemberNameList[i].m_cName, pName) == 0)) {
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			return true;
		}

	// Member not found — notify game server to remove
	hb::net::PartyOpCreateRequest req{};
	req.op_type = 3;
	req.client_h = static_cast<uint16_t>(iGSCH);
	std::memcpy(req.name, pName, sizeof(req.name));
	std::memcpy(cData, &req, sizeof(req));
	m_pGame->PartyOperationResultHandler(cData);

	return false;
}

bool PartyManager::bGetPartyInfo(int iGSCH, char* pName, int iPartyID)
{
	char cData[1024]{};

	auto& hdr = *reinterpret_cast<hb::net::PartyOpResultInfoHeader*>(cData);
	hdr.op_type = 5;
	hdr.client_h = static_cast<uint16_t>(iGSCH);
	std::memcpy(hdr.name, pName, sizeof(hdr.name));

	char* cp = cData + sizeof(hb::net::PartyOpResultInfoHeader);
	int iTotal = 0;
	for (int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (m_stMemberNameList[i].m_iPartyID != 0)) {
			std::memcpy(cp, m_stMemberNameList[i].m_cName, hb::shared::limits::CharNameLen - 1);
			cp += 11;
			iTotal++;
		}

	hdr.total = static_cast<uint16_t>(iTotal);
	m_pGame->PartyOperationResultHandler(cData);

	return true;
}

void PartyManager::GameServerDown()
{
	

	for(int i = 0; i < hb::server::party::MaxParty; i++)
		if (m_stMemberNameList[i].m_iIndex == 1) {
			//testcode
			hb::logger::log("Removing party member '{}' (server shutdown)", m_stMemberNameList[i].m_cName);

			m_iMemberNumList[m_stMemberNameList[i].m_iPartyID]--;
			m_stMemberNameList[i].m_iPartyID = 0;
			m_stMemberNameList[i].m_iIndex = 0;
			m_stMemberNameList[i].m_dwServerChangeTime = 0;
			std::memset(m_stMemberNameList[i].m_cName, 0, sizeof(m_stMemberNameList[i].m_cName));
		}
}

void PartyManager::SetServerChangeStatus(char* pName, int iPartyID)
{
	

	for(int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (hb_stricmp(m_stMemberNameList[i].m_cName, pName) == 0)) {
			m_stMemberNameList[i].m_dwServerChangeTime = GameClock::GetTimeMS();
			return;
		}
}

void PartyManager::CheckMemberActivity()
{
	uint32_t dwTime = GameClock::GetTimeMS();
	char cData[120];

	if ((dwTime - m_dwCheckMemberActTime) > 1000 * 2) {
		m_dwCheckMemberActTime = dwTime;
	}
	else return;

	for (int i = 1; i < hb::server::party::MaxParty; i++)
		if ((m_stMemberNameList[i].m_dwServerChangeTime != 0) && ((dwTime - m_stMemberNameList[i].m_dwServerChangeTime) > 1000 * 20)) {
			std::memset(cData, 0, sizeof(cData));
			hb::net::PartyOpResultWithStatus dismissOp{};
			dismissOp.op_type = 6;
			dismissOp.result = 1;
			dismissOp.client_h = 0;
			std::memcpy(dismissOp.name, m_stMemberNameList[i].m_cName, sizeof(dismissOp.name));
			dismissOp.party_id = static_cast<uint16_t>(m_stMemberNameList[i].m_iPartyID);
			std::memcpy(cData, &dismissOp, sizeof(dismissOp));
			m_pGame->PartyOperationResultHandler(cData);

			bRemoveMember(m_stMemberNameList[i].m_iPartyID, m_stMemberNameList[i].m_cName);
		}
}
