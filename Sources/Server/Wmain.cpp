#include "CommonTypes.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>

#include "Game.h"
#include "LoginServer.h"
#include "ServerConsole.h"
#include "ServerCommand.h"
#include "GameChatCommand.h"
#include "IOServicePool.h"
#include "ConcurrentMsgQueue.h"
#include "Log.h"
#include "ServerLogChannels.h"

using namespace hb::server::config;
using hb::log_channel;

// --- Globals ---

char            G_cTxt[512];
char            G_cData50000[50000];

class hb::shared::net::ASIOSocket* G_pListenSock = nullptr;
class hb::shared::net::ASIOSocket* G_pLogSock = nullptr;
class CGame* G_pGame = nullptr;
class hb::shared::net::ASIOSocket* G_pLoginSock = nullptr;
class LoginServer* g_login = nullptr;

hb::shared::net::IOServicePool* G_pIOPool = nullptr;

hb::shared::net::ConcurrentQueue<asio::ip::tcp::socket> G_gameAcceptQueue;
hb::shared::net::ConcurrentQueue<asio::ip::tcp::socket> G_loginAcceptQueue;
hb::shared::net::ConcurrentQueue<hb::shared::net::SocketErrorEvent> G_errorQueue;

bool G_bRunning = true;

// --- Async I/O helpers (called from main loop) ---

void DrainAcceptQueues()
{
	if (G_pGame == nullptr) return;

	asio::ip::tcp::socket peer(G_pIOPool->GetContext());
	while (G_gameAcceptQueue.Pop(peer)) {
		G_pGame->bAcceptFromAsync(std::move(peer));
	}

	asio::ip::tcp::socket loginPeer(G_pIOPool->GetContext());
	while (G_loginAcceptQueue.Pop(loginPeer)) {
		G_pGame->bAcceptLoginFromAsync(std::move(loginPeer));
	}
}

void DrainErrorQueue()
{
	if (G_pGame == nullptr) return;

	hb::shared::net::SocketErrorEvent evt;
	while (G_errorQueue.Pop(evt)) {
		if (evt.iSocketIndex > 0 && evt.iSocketIndex < MaxClients) {
			if (G_pGame->m_pClientList[evt.iSocketIndex] != nullptr) {
				hb::logger::log("Client {}: disconnected, async error={} ({}) char={}", evt.iSocketIndex, evt.iErrorCode, G_pGame->m_pClientList[evt.iSocketIndex]->m_cIPaddress, G_pGame->m_pClientList[evt.iSocketIndex]->m_cCharName);
				G_pGame->DeleteClient(evt.iSocketIndex, true, true);
			}
		}
		else if (evt.iSocketIndex < 0) {
			int loginH = -(evt.iSocketIndex + 1);
			if (loginH >= 0 && loginH < MaxClientLoginSock) {
				G_pGame->DeleteLoginClient(loginH);
			}
		}
	}
}

void PollLoginClients()
{
	if (G_pGame == nullptr) return;

	for (int i = 0; i < MaxClientLoginSock; i++) {
		if (G_pGame->_lclients[i] != nullptr && G_pGame->_lclients[i]->_sock != nullptr) {
			G_pGame->OnLoginClientSocketEvent(i);
		}
	}
}

// Called from EntityManager during NPC processing
void PollAllSockets()
{
	DrainAcceptQueues();
	DrainErrorQueue();
	PollLoginClients();
}

// --- Entry point ---

int main()
{
	// Portable startup timestamp
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &t);
#else
	localtime_r(&t, &tm_buf);
#endif

	printf("\n");
	printf("=======================================================================\n");
	printf("         HELBREATH GAME SERVER                                         \n");
	printf("=======================================================================\n");
	printf("Version: %s.%s\n", hb::server::version::Upper, hb::server::version::Lower);
	printf("Build: %d\n", hb::server::version::BuildDate);
	printf("Started: %d/%d/%d %02d:%02d\n",
		tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_year + 1900,
		tm_buf.tm_hour, tm_buf.tm_min);
	printf("=======================================================================\n\n");
	printf("Initializing server...\n\n");

	// Initialize logger
	hb::logger::initialize("GameLogs/");

	// Create I/O service pool
	G_pIOPool = new hb::shared::net::IOServicePool(4);

	// Create and initialize game
	G_pGame = new CGame();
	if (!G_pGame->bInit()) {
		hb::logger::error("Server initialization failed");
		hb::logger::shutdown();
		delete G_pGame;
		delete G_pIOPool;
		return 1;
	}

	ServerCommandManager::Get().Initialize(G_pGame);
	GameChatCommandManager::Get().Initialize(G_pGame);

	GetServerConsole().Init();

	// Start listen sockets
	G_pListenSock = new hb::shared::net::ASIOSocket(G_pIOPool->GetContext(), ServerSocketBlockLimit);
	G_pListenSock->bListen(G_pGame->m_cGameListenIP, G_pGame->m_iGameListenPort);

	G_pLoginSock = new hb::shared::net::ASIOSocket(G_pIOPool->GetContext(), ServerSocketBlockLimit);
	G_pLoginSock->bListen(G_pGame->m_cLoginListenIP, G_pGame->m_iLoginListenPort);

	// Start async accept
	G_pListenSock->StartAsyncAccept([](asio::ip::tcp::socket peer) {
		G_gameAcceptQueue.Push(std::move(peer));
	});
	G_pLoginSock->StartAsyncAccept([](asio::ip::tcp::socket peer) {
		G_loginAcceptQueue.Push(std::move(peer));
	});

	// Start I/O thread pool
	G_pIOPool->Start();

	// --- Main loop ---
	constexpr uint32_t tick_interval = 300 / GameTickMultiplier;
	uint32_t last_tick = GameClock::GetTimeMS();

	while (G_bRunning) {
		uint32_t now_ms = GameClock::GetTimeMS();

		if (now_ms - last_tick >= tick_interval) {
			G_pGame->OnTimer(0);
			G_pGame->GameProcess();
			last_tick = now_ms;
		}

		DrainAcceptQueues();
		DrainErrorQueue();
		PollLoginClients();

		char szCmd[256];
		if (GetServerConsole().PollInput(szCmd, sizeof(szCmd))) {
			ServerCommandManager::Get().ProcessCommand(szCmd);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// --- Shutdown ---
	G_pIOPool->Stop();

	delete G_pListenSock;
	G_pListenSock = nullptr;

	delete G_pLogSock;
	G_pLogSock = nullptr;

	delete G_pLoginSock;
	G_pLoginSock = nullptr;

	G_pGame->Quit();
	delete G_pGame;
	G_pGame = nullptr;

	delete g_login;
	g_login = nullptr;

	delete G_pIOPool;
	G_pIOPool = nullptr;

	hb::logger::shutdown();

	return 0;
}
