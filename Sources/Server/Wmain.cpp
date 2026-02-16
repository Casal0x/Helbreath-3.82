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

	asio::ip::tcp::socket peer(G_pIOPool->get_context());
	while (G_gameAcceptQueue.pop(peer)) {
		G_pGame->accept_from_async(std::move(peer));
	}

	asio::ip::tcp::socket loginPeer(G_pIOPool->get_context());
	while (G_loginAcceptQueue.pop(loginPeer)) {
		G_pGame->accept_login_from_async(std::move(loginPeer));
	}
}

void DrainErrorQueue()
{
	if (G_pGame == nullptr) return;

	hb::shared::net::SocketErrorEvent evt;
	while (G_errorQueue.pop(evt)) {
		if (evt.socket_index > 0 && evt.socket_index < MaxClients) {
			if (G_pGame->m_client_list[evt.socket_index] != nullptr) {
				hb::logger::log("Client {}: disconnected, async error={} ({}) char={}", evt.socket_index, evt.error_code, G_pGame->m_client_list[evt.socket_index]->m_ip_address, G_pGame->m_client_list[evt.socket_index]->m_char_name);
				G_pGame->delete_client(evt.socket_index, true, true);
			}
		}
		else if (evt.socket_index < 0) {
			int loginH = -(evt.socket_index + 1);
			if (loginH >= 0 && loginH < MaxClientLoginSock) {
				G_pGame->delete_login_client(loginH);
			}
		}
	}
}

void PollLoginClients()
{
	if (G_pGame == nullptr) return;

	for (int i = 0; i < MaxClientLoginSock; i++) {
		if (G_pGame->_lclients[i] != nullptr && G_pGame->_lclients[i]->sock != nullptr) {
			G_pGame->on_login_client_socket_event(i);
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
	printf("Version: %s\n", hb::version::server::display_version);
	printf("Build: %s\n", hb::version::server::full_version);
	printf("Started: %d/%d/%d %02d:%02d\n",
		tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_year + 1900,
		tm_buf.tm_hour, tm_buf.tm_min);
	printf("=======================================================================\n\n");
	printf("Initializing server...\n\n");

	// initialize logger
	hb::logger::initialize("gamelogs/");

	// Create I/O service pool
	G_pIOPool = new hb::shared::net::IOServicePool(4);

	// Create and initialize game
	G_pGame = new CGame();
	if (!G_pGame->init()) {
		hb::logger::error("Server initialization failed");
		hb::logger::shutdown();
		delete G_pGame;
		delete G_pIOPool;
		return 1;
	}

	ServerCommandManager::get().initialize(G_pGame);
	GameChatCommandManager::get().initialize(G_pGame);

	GetServerConsole().init();

	// start listen sockets
	G_pListenSock = new hb::shared::net::ASIOSocket(G_pIOPool->get_context(), ServerSocketBlockLimit);
	G_pListenSock->listen(G_pGame->m_game_listen_ip, G_pGame->m_game_listen_port);

	G_pLoginSock = new hb::shared::net::ASIOSocket(G_pIOPool->get_context(), ServerSocketBlockLimit);
	G_pLoginSock->listen(G_pGame->m_login_listen_ip, G_pGame->m_login_listen_port);

	// start async accept
	G_pListenSock->start_async_accept([](asio::ip::tcp::socket peer) {
		G_gameAcceptQueue.push(std::move(peer));
	});
	G_pLoginSock->start_async_accept([](asio::ip::tcp::socket peer) {
		G_loginAcceptQueue.push(std::move(peer));
	});

	// start I/O thread pool
	G_pIOPool->start();

	// --- Main loop ---
	constexpr uint32_t tick_interval = 300 / GameTickMultiplier;
	uint32_t last_tick = GameClock::GetTimeMS();

	while (G_bRunning) {
		uint32_t now_ms = GameClock::GetTimeMS();

		if (now_ms - last_tick >= tick_interval) {
			G_pGame->on_timer(0);
			G_pGame->game_process();
			last_tick = now_ms;
		}

		DrainAcceptQueues();
		DrainErrorQueue();
		PollLoginClients();

		char cmd[256];
		if (GetServerConsole().poll_input(cmd, sizeof(cmd))) {
			ServerCommandManager::get().process_command(cmd);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// --- shutdown ---
	G_pIOPool->stop();

	delete G_pListenSock;
	G_pListenSock = nullptr;

	delete G_pLogSock;
	G_pLogSock = nullptr;

	delete G_pLoginSock;
	G_pLoginSock = nullptr;

	G_pGame->quit();
	delete G_pGame;
	G_pGame = nullptr;

	delete g_login;
	g_login = nullptr;

	delete G_pIOPool;
	G_pIOPool = nullptr;

	hb::logger::shutdown();

	return 0;
}
