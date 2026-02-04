// ASIOSocket.h: Unified socket class using standalone ASIO
//
// Replaces the legacy XSocket (Winsock2/WSAEventSelect) with ASIO.
// Shared between Client and Server.
//
// Supports both polling mode (client) and async mode (server).
// When using async mode, a shared io_context with background threads
// drives async_read/async_write with per-socket strands.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// ASIO standalone configuration (must be before asio.hpp)
#define ASIO_STANDALONE
#define ASIO_NO_DEPRECATED
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7+
#endif

#include "ASIO/asio.hpp"

#include "NetConstants.h"
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <mutex>
#include <atomic>

// Socket type constants
#define DEF_XSOCK_LISTENSOCK			1
#define DEF_XSOCK_NORMALSOCK			2
#define DEF_XSOCK_SHUTDOWNEDSOCK		3

// Read state machine
#define DEF_XSOCKSTATUS_READINGHEADER	11
#define DEF_XSOCKSTATUS_READINGBODY		12

// Event codes (return values from Poll / iSendMsg)
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

// Native socket handle type (for iGetSocket compatibility)
using NativeSocketHandle = asio::ip::tcp::socket::native_handle_type;

struct NetworkPacket {
	std::vector<uint8_t> data;
	size_t reportedSize;

	NetworkPacket() : reportedSize(0) {}
	NetworkPacket(const char* pData, size_t dwSize)
		: reportedSize(dwSize)
	{
		data.reserve(dwSize + 1024);
		data.assign(reinterpret_cast<const uint8_t*>(pData),
			reinterpret_cast<const uint8_t*>(pData) + dwSize);
		data.insert(data.end(), 1024, 0);
	}

	size_t size() const { return reportedSize; }
	bool empty() const { return reportedSize == 0; }
	const char* ptr() const { return reinterpret_cast<const char*>(data.data()); }
};

// Unsent data block (RAII replacement for raw char* + size pairs)
struct UnsentBlock {
	std::vector<char> data;
	size_t offset = 0;  // bytes already sent from this block

	UnsentBlock() = default;
	UnsentBlock(const char* pData, size_t iSize)
		: data(pData, pData + iSize), offset(0) {}

	const char* remaining() const { return data.data() + offset; }
	size_t remainingSize() const { return data.size() - offset; }
};

// Async callback types
using MessageCallback = std::function<void(int socketIndex, const char* pData, size_t dwSize, char cKey)>;
using ErrorCallback = std::function<void(int socketIndex, int errorCode)>;
using AcceptCallback = std::function<void(asio::ip::tcp::socket peer)>;

class ASIOSocket
{
public:
	// Constructor takes a reference to an external io_context (from IOServicePool)
	ASIOSocket(asio::io_context& ctx, int iBlockLimit);
	virtual ~ASIOSocket();

	// Non-copyable
	ASIOSocket(const ASIOSocket&) = delete;
	ASIOSocket& operator=(const ASIOSocket&) = delete;

	// Buffer initialization
	bool bInitBufferSize(size_t dwBufferSize);

	// Connection management
	bool bConnect(char* pAddr, int iPort);
	bool bBlockConnect(char* pAddr, int iPort);
	bool bListen(char* pAddr, int iPort);
	bool bAccept(ASIOSocket* pSock);

	// Accept a pre-connected socket (for async accept path)
	bool bAcceptFromSocket(asio::ip::tcp::socket&& peer);

	// Polling (replaces WSAEventSelect + iOnSocketEvent) - still works for client
	int Poll();

	// Sending (synchronous path for polling mode)
	int iSendMsg(char* cData, size_t dwSize, char cKey = 0);
	int iSendMsgBlockingMode(char* buf, int nbytes);

	// Async sending (posts to strand, builds frame, chains async_write)
	int iSendMsgAsync(char* cData, size_t dwSize, char cKey = 0);

	// Receiving (synchronous path for polling mode)
	char* pGetRcvDataPointer(size_t* pMsgSize, char* pKey = 0);

	// v4 Networking API (packet queue) - polling mode only
	int DrainToQueue();
	bool PeekPacket(NetworkPacket& outPacket) const;
	bool PopPacket();
	bool HasPendingPackets() const { return !m_RecvQueue.empty(); }
	size_t GetQueueSize() const { return m_RecvQueue.size(); }
	void ClearQueue() { m_RecvQueue.clear(); }

	// Connection info
	int iGetPeerAddress(char* pAddrString);
	NativeSocketHandle iGetSocket();

	// Close
	void CloseConnection();

	// --- Async mode (server) ---
	void SetSocketIndex(int idx) { m_iSocketIndex = idx; }
	void SetCallbacks(MessageCallback onMessage, ErrorCallback onError);
	void StartAsyncRead();
	void StartAsyncAccept(AcceptCallback callback);
	void CancelAsync();

	// Public state
	int  m_WSAErr = 0;
	bool m_bIsAvailable = false;
	bool m_bIsWriteEnabled = false;
	char m_cType = 0;

private:
	// Internal read/send helpers (synchronous polling path)
	int _iOnRead();
	int _iSend(const char* cData, size_t iSize, bool bSaveFlag);
	int _iSend_ForInternalUse(const char* cData, size_t iSize);
	int _iSendUnsentData();
	bool _iRegisterUnsentData(const char* cData, size_t iSize);

	// Async read chain
	void _DoAsyncReadHeader();
	void _DoAsyncReadBody(size_t bodySize);

	// Async write chain
	void _DoAsyncWrite();

	// ASIO internals - reference to external io_context
	asio::io_context& m_ioContext;
	asio::ip::tcp::socket m_socket;
	asio::ip::tcp::acceptor m_acceptor;
	asio::io_context::strand m_strand;

	// Async connect state
	bool m_connectPending = false;
	bool m_connectResultReady = false;
	asio::error_code m_connectError;

	// Pending accept socket (stored between Poll detecting accept and bAccept call)
	std::optional<asio::ip::tcp::socket> m_pendingAcceptSocket;

	// Receive/send buffers (RAII vectors replace raw char*)
	std::vector<char> m_rcvBuffer;
	std::vector<char> m_sndBuffer;
	size_t m_dwBufferSize = 0;

	// Read state machine (polling mode)
	char     m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
	size_t m_dwReadSize = 3;
	size_t m_dwTotalReadSize = 0;

	// Stored connection info (for reconnect)
	char m_pAddr[30] = {};
	int  m_iPortNum = 0;

	// Unsent data queue (replaces raw pointer circular buffer) - polling mode
	std::deque<UnsentBlock> m_unsentQueue;
	int m_iBlockLimit;

	// Packet receive queue - polling mode
	std::deque<NetworkPacket> m_RecvQueue;
	static constexpr size_t MAX_QUEUE_SIZE = 2000;

	// --- Async mode members ---
	int m_iSocketIndex = -1;
	MessageCallback m_onMessage;
	ErrorCallback m_onError;
	AcceptCallback m_onAccept;

	// Async read buffer (separate from polling m_rcvBuffer)
	std::vector<char> m_asyncRcvBuffer;

	// Async write queue (strand-serialized)
	std::deque<std::vector<char>> m_asyncWriteQueue;
	bool m_bAsyncWriteInProgress = false;

	// Async write queue size limit
	static constexpr size_t MAX_ASYNC_WRITE_QUEUE = 5000;

	// Flag to indicate async mode is active
	std::atomic<bool> m_bAsyncMode{false};
};
