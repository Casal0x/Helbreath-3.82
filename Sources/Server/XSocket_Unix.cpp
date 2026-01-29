// XSocket_Unix.cpp: Unix/Linux/macOS socket implementation
// Provides BSD socket-based implementation of XSocket class
// Only compiled on non-Windows platforms

#ifndef _WIN32

#include "CommonTypes.h"
#include "Platform.h"
#include "XSocket.h"
#include <cstdio>
#include <cstring>
#include <netinet/tcp.h>
#include <sys/time.h>

// Message buffer size constant
#ifndef DEF_MSGBUFFERSIZE
#define DEF_MSGBUFFERSIZE 60000
#endif

extern void PutLogList(char *cMsg);

// Queue overflow tracking (shared across all sockets)
static uint32_t s_dwQueueFullCount = 0;
static uint32_t s_dwLastQueueFullLogTime = 0;
static uint32_t s_dwQueueFullCountSinceLastLog = 0;

// Get current time in milliseconds (Unix equivalent of GetTickCount)
static uint32_t GetTickCount() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

XSocket::XSocket(int iBlockLimit) {
  int i;

  m_cType = 0;
  m_pRcvBuffer = 0;
  m_pSndBuffer = 0;
  m_Sock = INVALID_SOCKET;
  m_dwBufferSize = 0;

  m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
  m_dwReadSize = 3;
  m_dwTotalReadSize = 0;

  for (i = 0; i < DEF_XSOCKBLOCKLIMIT; i++) {
    m_iUnsentDataSize[i] = 0;
    m_pUnsentDataList[i] = 0;
  }

  m_sHead = 0;
  m_sTail = 0;

  m_WSAErr = 0;
  m_hEvent = WSA_INVALID_EVENT; // Not used on Unix
  m_bIsAvailable = false;

  m_iBlockLimit = iBlockLimit;

  m_bIsWriteEnabled = false;
}

XSocket::~XSocket() {
  int i;

  if (m_pRcvBuffer != 0)
    delete[] m_pRcvBuffer;
  if (m_pSndBuffer != 0)
    delete[] m_pSndBuffer;

  for (i = 0; i < DEF_XSOCKBLOCKLIMIT; i++)
    if (m_pUnsentDataList[i] != 0)
      delete[] m_pUnsentDataList[i];

  _CloseConn();
}

bool XSocket::bInitBufferSize(uint32_t dwBufferSize) {
  if (m_pRcvBuffer != 0)
    delete[] m_pRcvBuffer;
  if (m_pSndBuffer != 0)
    delete[] m_pSndBuffer;

  m_pRcvBuffer = new char[dwBufferSize + 8];
  if (m_pRcvBuffer == 0)
    return false;

  m_pSndBuffer = new char[dwBufferSize + 8];
  if (m_pSndBuffer == 0)
    return false;

  m_dwBufferSize = dwBufferSize;

  return true;
}

// Unix implementation using poll() instead of WSAEventSelect
int XSocket::Poll() {
  struct pollfd pfd;
  int pollResult;
  int iResult = 0;

  // Not initialized
  if (m_cType == 0)
    return DEF_XSOCKEVENT_NOTINITIALIZED;
  if (m_cType != DEF_XSOCK_NORMALSOCK && m_cType != DEF_XSOCK_LISTENSOCK)
    return DEF_XSOCKEVENT_SOCKETMISMATCH;

  if (m_Sock == INVALID_SOCKET)
    return DEF_XSOCKEVENT_NOTINITIALIZED;

  // Set up poll structure
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = m_Sock;
  pfd.events = POLLIN; // Always interested in reading
  if (m_cType == DEF_XSOCK_NORMALSOCK) {
    pfd.events |= POLLOUT; // Also interested in writing for normal sockets
  }

  // Non-blocking poll (0ms timeout)
  pollResult = poll(&pfd, 1, 0);

  if (pollResult < 0) {
    m_WSAErr = errno;
    return DEF_XSOCKEVENT_SOCKETERROR;
  }

  if (pollResult == 0) {
    return 0; // No events pending
  }

  // Check for errors first
  if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
    return DEF_XSOCKEVENT_SOCKETCLOSED;
  }

  // Handle accept for listen sockets
  if (m_cType == DEF_XSOCK_LISTENSOCK && (pfd.revents & POLLIN)) {
    return DEF_XSOCKEVENT_CONNECTIONESTABLISH;
  }

  // Handle read events
  if (pfd.revents & POLLIN) {
    int readResult = _iOnRead();
    if (readResult != DEF_XSOCKEVENT_ONREAD) {
      iResult = readResult;
    }
  }

  // Handle write events
  if (pfd.revents & POLLOUT) {
    m_bIsWriteEnabled = true;
    int writeResult = _iSendUnsentData();
    if (writeResult != DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE) {
      iResult = writeResult;
    }
  }

  return iResult;
}

// Unix connect implementation
bool XSocket::bConnect(char *pAddr, int iPort) {
  struct sockaddr_in saTemp;
  int flags;
  int iRet;
  int dwOpt;

  if (m_cType == DEF_XSOCK_LISTENSOCK)
    return false;
  if (m_Sock != INVALID_SOCKET)
    closesocket(m_Sock);

  m_Sock = socket(AF_INET, SOCK_STREAM, 0);
  if (m_Sock == INVALID_SOCKET)
    return false;

  // Set non-blocking mode
  flags = fcntl(m_Sock, F_GETFL, 0);
  fcntl(m_Sock, F_SETFL, flags | O_NONBLOCK);

  // Set up address
  memset(&saTemp, 0, sizeof(saTemp));
  saTemp.sin_family = AF_INET;
  saTemp.sin_addr.s_addr = inet_addr(pAddr);
  saTemp.sin_port = htons(iPort);

  iRet = connect(m_Sock, (struct sockaddr *)&saTemp, sizeof(saTemp));
  if (iRet == SOCKET_ERROR) {
    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
      m_WSAErr = errno;
      return false;
    }
  }

  // Set socket options
  dwOpt = 8192 * 5;
  setsockopt(m_Sock, SOL_SOCKET, SO_RCVBUF, (const char *)&dwOpt,
             sizeof(dwOpt));
  setsockopt(m_Sock, SOL_SOCKET, SO_SNDBUF, (const char *)&dwOpt,
             sizeof(dwOpt));

  strncpy(m_pAddr, pAddr, sizeof(m_pAddr) - 1);
  m_pAddr[sizeof(m_pAddr) - 1] = '\0';
  m_iPortNum = iPort;

  m_cType = DEF_XSOCK_NORMALSOCK;
  m_bIsAvailable = true; // Unix connects are immediate or EINPROGRESS

  return true;
}

int XSocket::_iOnRead() {
  int iRet;
  uint16_t *wp;

  if (m_cStatus == DEF_XSOCKSTATUS_READINGHEADER) {

    iRet = recv(m_Sock, (char *)(m_pRcvBuffer + m_dwTotalReadSize),
                m_dwReadSize, 0);

    if (iRet == SOCKET_ERROR) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        m_WSAErr = errno;
        return DEF_XSOCKEVENT_SOCKETERROR;
      } else
        return DEF_XSOCKEVENT_BLOCK;
    } else if (iRet == 0) {
      // Socket closed
      m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
      return DEF_XSOCKEVENT_SOCKETCLOSED;
    }

    m_dwReadSize -= iRet;
    m_dwTotalReadSize += iRet;

    if (m_dwReadSize == 0) {
      // Read header complete
      m_cStatus = DEF_XSOCKSTATUS_READINGBODY;
      wp = (uint16_t *)(m_pRcvBuffer + 1);
      m_dwReadSize = (int)(*wp - 3);

      if (m_dwReadSize == 0) {
        m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
        m_dwReadSize = 3;
        m_dwTotalReadSize = 0;
        return DEF_XSOCKEVENT_READCOMPLETE;
      } else if (m_dwReadSize > m_dwBufferSize) {
        m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
        m_dwReadSize = 3;
        m_dwTotalReadSize = 0;
        return DEF_XSOCKEVENT_MSGSIZETOOLARGE;
      }
    }
    return DEF_XSOCKEVENT_ONREAD;
  } else if (m_cStatus == DEF_XSOCKSTATUS_READINGBODY) {

    iRet = recv(m_Sock, (char *)(m_pRcvBuffer + m_dwTotalReadSize),
                m_dwReadSize, 0);

    if (iRet == SOCKET_ERROR) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        m_WSAErr = errno;
        return DEF_XSOCKEVENT_SOCKETERROR;
      } else
        return DEF_XSOCKEVENT_BLOCK;
    } else if (iRet == 0) {
      m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
      return DEF_XSOCKEVENT_SOCKETCLOSED;
    }

    m_dwReadSize -= iRet;
    m_dwTotalReadSize += iRet;

    if (m_dwReadSize == 0) {
      // Body read complete
      m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
      m_dwReadSize = 3;
      m_dwTotalReadSize = 0;
    } else
      return DEF_XSOCKEVENT_ONREAD;
  }

  return DEF_XSOCKEVENT_READCOMPLETE;
}

int XSocket::_iSend(char *cData, int iSize, bool bSaveFlag) {
  int iOutLen, iRet;

  if (m_pUnsentDataList[m_sHead] != 0) {
    if (bSaveFlag) {
      iRet = _iRegisterUnsentData(cData, iSize);
      switch (iRet) {
      case -1:
        return DEF_XSOCKEVENT_CRITICALERROR;
      case 0:
        return DEF_XSOCKEVENT_QUENEFULL;
      case 1:
        break;
      }
      return DEF_XSOCKEVENT_BLOCK;
    } else
      return 0;
  }

  iOutLen = 0;
  while (iOutLen < iSize) {

    iRet = send(m_Sock, (cData + iOutLen), iSize - iOutLen, 0);

    if (iRet == SOCKET_ERROR) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        m_WSAErr = errno;
        return DEF_XSOCKEVENT_SOCKETERROR;
      } else {
        if (bSaveFlag) {
          iRet = _iRegisterUnsentData((cData + iOutLen), (iSize - iOutLen));
          switch (iRet) {
          case -1:
            return DEF_XSOCKEVENT_CRITICALERROR;
            break;
          case 0:
            return DEF_XSOCKEVENT_QUENEFULL;
            break;
          case 1:
            break;
          }
        }
        return DEF_XSOCKEVENT_BLOCK;
      }
    } else
      iOutLen += iRet;
  }

  return iOutLen;
}

int XSocket::_iSend_ForInternalUse(char *cData, int iSize) {
  int iOutLen, iRet;

  iOutLen = 0;
  while (iOutLen < iSize) {

    iRet = send(m_Sock, (cData + iOutLen), iSize - iOutLen, 0);

    if (iRet == SOCKET_ERROR) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        m_WSAErr = errno;
        return DEF_XSOCKEVENT_SOCKETERROR;
      } else {
        return iOutLen;
      }
    } else
      iOutLen += iRet;
  }

  return iOutLen;
}

int XSocket::_iRegisterUnsentData(char *cData, int iSize) {
  // Queue is full
  if (m_pUnsentDataList[m_sTail] != 0) {
    s_dwQueueFullCount++;
    s_dwQueueFullCountSinceLastLog++;

    uint32_t dwNow = GetTickCount();
    if (dwNow - s_dwLastQueueFullLogTime > 5000) {
      char szLog[256];
      snprintf(
          szLog, sizeof(szLog),
          "[XSOCKET] Queue full! Dropped %u packets in last 5s (total: %u).",
          s_dwQueueFullCountSinceLastLog, s_dwQueueFullCount);
      PutLogList(szLog);
      s_dwLastQueueFullLogTime = dwNow;
      s_dwQueueFullCountSinceLastLog = 0;
    }
    return 0;
  }

  m_pUnsentDataList[m_sTail] = new char[iSize];
  if (m_pUnsentDataList[m_sTail] == 0)
    return -1;

  memcpy(m_pUnsentDataList[m_sTail], cData, iSize);
  m_iUnsentDataSize[m_sTail] = iSize;

  m_sTail++;
  if (m_sTail >= m_iBlockLimit)
    m_sTail = 0;

  return 1;
}

int XSocket::_iSendUnsentData() {
  int iRet;
  char *pTemp;

  while (m_pUnsentDataList[m_sHead] != 0) {

    iRet = _iSend_ForInternalUse(m_pUnsentDataList[m_sHead],
                                 m_iUnsentDataSize[m_sHead]);

    if (iRet == m_iUnsentDataSize[m_sHead]) {
      delete[] m_pUnsentDataList[m_sHead];
      m_pUnsentDataList[m_sHead] = 0;
      m_iUnsentDataSize[m_sHead] = 0;
      m_sHead++;
      if (m_sHead >= m_iBlockLimit)
        m_sHead = 0;
    } else {
      if (iRet < 0)
        return iRet;

      pTemp = new char[m_iUnsentDataSize[m_sHead] - iRet];
      memcpy(pTemp, m_pUnsentDataList[m_sHead] + iRet,
             m_iUnsentDataSize[m_sHead] - iRet);

      delete[] m_pUnsentDataList[m_sHead];
      m_pUnsentDataList[m_sHead] = pTemp;

      return DEF_XSOCKEVENT_UNSENTDATASENDBLOCK;
    }
  }

  return DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE;
}

int XSocket::iSendMsg(char *cData, uint32_t dwSize, char cKey) {
  uint16_t *wp;
  uint32_t i;
  int iRet;

  if (dwSize > m_dwBufferSize)
    return DEF_XSOCKEVENT_MSGSIZETOOLARGE;

  if (m_cType != DEF_XSOCK_NORMALSOCK)
    return DEF_XSOCKEVENT_SOCKETMISMATCH;
  if (m_cType == 0)
    return DEF_XSOCKEVENT_NOTINITIALIZED;

  // Build message with key and size
  m_pSndBuffer[0] = cKey;

  wp = (uint16_t *)(m_pSndBuffer + 1);
  *wp = static_cast<uint16_t>(dwSize + 3);

  memcpy((char *)(m_pSndBuffer + 3), cData, dwSize);

  // Encryption
  if (cKey != 0) {
    for (i = 0; i < dwSize; i++) {
      m_pSndBuffer[3 + i] += static_cast<char>(i ^ cKey);
      m_pSndBuffer[3 + i] = static_cast<char>(
          m_pSndBuffer[3 + i] ^ (cKey ^ static_cast<char>(dwSize - i)));
    }
  }

  if (m_bIsWriteEnabled == false) {
    iRet = _iRegisterUnsentData(m_pSndBuffer, dwSize + 3);
  } else
    iRet = _iSend(m_pSndBuffer, dwSize + 3, true);

  if (iRet < 0)
    return iRet;
  else
    return (iRet - 3);
}

// Unix listen implementation
bool XSocket::bListen(char *pAddr, int iPort) {
  struct sockaddr_in saTemp;
  int reuse = 1;

  if (m_cType != 0)
    return false;
  if (m_Sock != INVALID_SOCKET)
    closesocket(m_Sock);

  m_Sock = socket(AF_INET, SOCK_STREAM, 0);
  if (m_Sock == INVALID_SOCKET)
    return false;

  // Allow address reuse
  setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // Bind address
  memset(&saTemp, 0, sizeof(saTemp));
  saTemp.sin_family = AF_INET;
  saTemp.sin_addr.s_addr = inet_addr(pAddr);
  saTemp.sin_port = htons(iPort);

  if (bind(m_Sock, (struct sockaddr *)&saTemp, sizeof(saTemp)) ==
      SOCKET_ERROR) {
    closesocket(m_Sock);
    return false;
  }

  if (listen(m_Sock, 5) == SOCKET_ERROR) {
    closesocket(m_Sock);
    return false;
  }

  // Set non-blocking
  int flags = fcntl(m_Sock, F_GETFL, 0);
  fcntl(m_Sock, F_SETFL, flags | O_NONBLOCK);

  m_cType = DEF_XSOCK_LISTENSOCK;

  return true;
}

// Unix accept implementation
bool XSocket::bAccept(class XSocket *pXSock) {
  SOCKET AcceptedSock;
  struct sockaddr Addr;
  socklen_t iLength;
  int dwOpt;

  if (m_cType != DEF_XSOCK_LISTENSOCK)
    return false;
  if (pXSock == 0)
    return false;

  iLength = sizeof(Addr);
  AcceptedSock = accept(m_Sock, (struct sockaddr *)&Addr, &iLength);
  if (AcceptedSock == INVALID_SOCKET)
    return false;

  pXSock->m_Sock = AcceptedSock;

  // Set non-blocking
  int flags = fcntl(AcceptedSock, F_GETFL, 0);
  fcntl(AcceptedSock, F_SETFL, flags | O_NONBLOCK);

  pXSock->m_cType = DEF_XSOCK_NORMALSOCK;

  // Set socket options
  dwOpt = 8192 * 5;
  setsockopt(pXSock->m_Sock, SOL_SOCKET, SO_RCVBUF, (const char *)&dwOpt,
             sizeof(dwOpt));
  setsockopt(pXSock->m_Sock, SOL_SOCKET, SO_SNDBUF, (const char *)&dwOpt,
             sizeof(dwOpt));

  // Enable TCP_NODELAY
  int iNoDelay = 1;
  setsockopt(pXSock->m_Sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&iNoDelay,
             sizeof(iNoDelay));

  return true;
}

void XSocket::_CloseConn() {
  char cTmp[100];
  bool bFlag = true;
  int iRet;

  if (m_Sock == INVALID_SOCKET)
    return;

  shutdown(m_Sock, SHUT_WR);
  while (bFlag) {
    iRet = recv(m_Sock, cTmp, sizeof(cTmp), 0);
    if (iRet == SOCKET_ERROR)
      bFlag = false;
    if (iRet == 0)
      bFlag = false;
  }

  closesocket(m_Sock);

  m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
}

SOCKET XSocket::iGetSocket() { return m_Sock; }

char *XSocket::pGetRcvDataPointer(uint32_t *pMsgSize, char *pKey) {
  uint16_t *wp;
  uint32_t dwSize;
  uint32_t i;
  char cKey;

  cKey = m_pRcvBuffer[0];
  if (pKey != 0)
    *pKey = cKey;

  wp = (uint16_t *)(m_pRcvBuffer + 1);
  *pMsgSize = (*wp) - 3;
  dwSize = (*wp) - 3;

  if (dwSize > DEF_MSGBUFFERSIZE)
    dwSize = DEF_MSGBUFFERSIZE;

  // Decryption
  if (cKey != 0) {
    for (i = 0; i < dwSize; i++) {
      m_pRcvBuffer[3 + i] = static_cast<char>(
          m_pRcvBuffer[3 + i] ^ (cKey ^ static_cast<char>(dwSize - i)));
      m_pRcvBuffer[3 + i] -= static_cast<char>(i ^ cKey);
    }
  }
  return (m_pRcvBuffer + 3);
}

// Unix socket initialization (from Platform_Unix.cpp)
bool _InitWinsock() { return Platform::InitSockets(); }

void _TermWinsock() { Platform::TermSockets(); }

int XSocket::iGetPeerAddress(char *pAddrString) {
  struct sockaddr_in sockaddr;
  socklen_t iLen;
  int iRet;

  iLen = sizeof(sockaddr);
  iRet = getpeername(m_Sock, (struct sockaddr *)&sockaddr, &iLen);
  strcpy(pAddrString, (const char *)inet_ntoa(sockaddr.sin_addr));

  return iRet;
}

#endif // !_WIN32
