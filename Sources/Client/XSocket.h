// XSocket.h: interface for the XSocket class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// MODERNIZED: Prevent old winsock.h from loading (conflicts with winsock2.h)
#define _WINSOCKAPI_   // Stops windows.h from including winsock.h

//#define  FD_SETSIZE 2000
#include <winsock2.h>  // MUST be before windows.h to use WSAEventSelect
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <winbase.h>
#include "CommonTypes.h"
#include <deque>
#include <vector>
#include <cstdint>

#define DEF_XSOCK_LISTENSOCK			1
#define DEF_XSOCK_NORMALSOCK			2				
#define DEF_XSOCK_SHUTDOWNEDSOCK		3				

#define DEF_XSOCKSTATUS_READINGHEADER	11
#define DEF_XSOCKSTATUS_READINGBODY		12

#define DEF_XSOCKEVENT_SOCKETMISMATCH			-121
#define DEF_XSOCKEVENT_CONNECTIONESTABLISH		-122
#define DEF_XSOCKEVENT_RETRYINGCONNECTION		-123
#define DEF_XSOCKEVENT_ONREAD					-124
#define DEF_XSOCKEVENT_READCOMPLETE				-125
#define DEF_XSOCKEVENT_UNKNOWN					-126
#define DEF_XSOCKEVENT_SOCKETCLOSED				-127
#define DEF_XSOCKEVENT_BLOCK					-128
#define DEF_XSOCKEVENT_SOCKETERROR				-129
#define DEF_XSOCKEVENT_CRITICALERROR			-130
#define DEF_XSOCKEVENT_NOTINITIALIZED			-131
#define DEF_XSOCKEVENT_MSGSIZETOOLARGE			-132
#define DEF_XSOCKEVENT_CONFIRMCODENOTMATCH		-133
#define DEF_XSOCKEVENT_QUENEFULL                -134
#define DEF_XSOCKEVENT_UNSENTDATASENDBLOCK		-135
#define DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE	-136

#define DEF_XSOCKBLOCKLIMIT						300	

struct NetworkPacket {
    std::vector<uint8_t> data;
    uint32_t reportedSize;

    NetworkPacket() : reportedSize(0) {}
    NetworkPacket(const char* pData, uint32_t dwSize)
    {
        reportedSize = dwSize;
        // Allocate size + padding to mimic legacy safe-buffer behavior
        data.reserve(dwSize + 1024);
        data.assign(reinterpret_cast<const uint8_t*>(pData),
               reinterpret_cast<const uint8_t*>(pData) + dwSize);
        // Add zero padding at the end
        data.insert(data.end(), 1024, 0);
    }

    size_t size() const { return reportedSize; }
    bool empty() const { return reportedSize == 0; }
    const char* ptr() const { return reinterpret_cast<const char*>(data.data()); }
};

class XSocket
{
public:
	int iSendMsgBlockingMode(char *buf,int nbytes);
	int iGetPeerAddress(char * pAddrString);
	char * pGetRcvDataPointer(uint32_t * pMsgSize, char * pKey = 0);
	bool bAccept(class XSocket * pXSock);
	bool bListen(char * pAddr, int iPort);

	int iSendMsg(char * cData, uint32_t dwSize, char cKey = 0);
	bool bConnect(char * pAddr, int iPort);
	bool bBlockConnect(char * pAddr, int iPort);
	int  Poll();  // MODERNIZED: Replaces iOnSocketEvent, polls for network events
	
	// === v4 Networking API ===
	int DrainToQueue();
	bool PeekPacket(NetworkPacket& outPacket) const;
	bool PopPacket();
	bool HasPendingPackets() const { return !m_RecvQueue.empty(); }
	size_t GetQueueSize() const { return m_RecvQueue.size(); }
	void ClearQueue() { m_RecvQueue.clear(); }

	bool bInitBufferSize(uint32_t dwBufferSize);
	XSocket(int iBlockLimit);  // MODERNIZED: Removed HWND parameter
	virtual ~XSocket();

	int  m_WSAErr;
	bool m_bIsAvailable;
	bool m_bIsWriteEnabled;

	void _CloseConn();
	
	int _iSendUnsentData();
	int _iRegisterUnsentData(char * cData, int iSize);
	int _iSend(char * cData, int iSize, bool bSaveFlag);
	int _iSend_ForInternalUse(char * cData, int iSize);
	int _iOnRead();
	
	char   m_cType;
	char * m_pRcvBuffer;
	char * m_pSndBuffer;
	uint32_t  m_dwBufferSize;

	SOCKET m_Sock;
	char   m_cStatus;
	uint32_t  m_dwReadSize;
	uint32_t  m_dwTotalReadSize;
	char   m_pAddr[30];
	int    m_iPortNum;

	char * m_pUnsentDataList[DEF_XSOCKBLOCKLIMIT];
	int    m_iUnsentDataSize[DEF_XSOCKBLOCKLIMIT];
	short  m_sHead, m_sTail;

	WSAEVENT     m_hEvent;  // MODERNIZED: WSAEventSelect event handle instead of window messages

	int			 m_iBlockLimit;

private:
	std::deque<NetworkPacket> m_RecvQueue;
	static constexpr size_t MAX_QUEUE_SIZE = 2000;
};
