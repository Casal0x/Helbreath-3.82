// Game.cpp: implementation of the CGame class.
//
//////////////////////////////////////////////////////////////////////


#include "Game.h"
#include "CommonTypes.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "Benchmark.h"
#include "FrameTiming.h"
#include "lan_eng.h"
#include "Packet/SharedPackets.h"
#include "SharedCalculations.h"
#include <cstdio>
#include <windows.h>
#include <charconv>

// Renderer
#include "RendererFactory.h"
#include "SpriteLoader.h"

// Manager singletons
#include "ConfigManager.h"
#include "AudioManager.h"
#include "WeatherManager.h"
#include "ChatCommandManager.h"
#include "HotkeyManager.h"

// DialogBox system
#include "IDialogBox.h"

// Entity render state
#include "EntityRenderState.h"

// Screen system
#include "IGameScreen.h"
#include "Screen_OnGame.h"
#include "Screen_MainMenu.h"
#include "Screen_Login.h"
#include "Screen_SelectCharacter.h"
#include "Screen_CreateNewCharacter.h"
#include "Screen_CreateAccount.h"
#include "Screen_Quit.h"
#include "Screen_Loading.h"
#include "Screen_Splash.h"
#include "Screen_Test.h"
#include "IInput.h"

// Overlay system
#include "Overlay_Connecting.h"
#include "Overlay_WaitingResponse.h"
#include "Overlay_QueryForceLogin.h"
#include "Overlay_QueryDeleteCharacter.h"
#include "Overlay_LogResMsg.h"
#include "Overlay_ChangePassword.h"
#include "Overlay_VersionNotMatch.h"
#include "Overlay_ConnectionLost.h"
#include "Overlay_Msg.h"
#include "Overlay_WaitInitData.h"

extern char G_cSpriteAlphaDegree;

extern class XSocket* G_pCalcSocket;
extern bool G_bIsCalcSocketConnected;
extern uint32_t G_dwCalcSocketTime, G_dwCalcSocketSendTime;
extern HWND	G_hWnd, G_hEditWnd;

char _cDrawingOrder[] = { 0, 1, 0, 0, 0, 0, 0, 1, 1 };
char _cMantleDrawingOrder[] = { 0, 1, 1, 1, 0, 0, 0, 2, 2 };
char _cMantleDrawingOrderOnRun[] = { 0, 1, 1, 1, 1, 1, 1, 1, 1 };

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CGame::ReadSettings()
{
	// Initialize and load settings from JSON file via ConfigManager
	ConfigManager::Get().Initialize();
	ConfigManager::Get().Load();

	// Copy values to CGame member variables
	m_sMagicShortCut = ConfigManager::Get().GetMagicShortcut();
	m_sRecentShortCut = ConfigManager::Get().GetRecentShortcut();
	for (int i = 0; i < 5; i++)
	{
		m_sShortCut[i] = ConfigManager::Get().GetShortcut(i);
	}
	m_sShortCut[5] = -1; // 6th slot unused

	// Audio settings loaded into AudioManager
	AudioManager::Get().SetSoundVolume(ConfigManager::Get().GetSoundVolume());
	AudioManager::Get().SetMusicVolume(ConfigManager::Get().GetMusicVolume());
	AudioManager::Get().SetSoundEnabled(ConfigManager::Get().IsSoundEnabled());
	AudioManager::Get().SetMusicEnabled(ConfigManager::Get().IsMusicEnabled());
}

void CGame::WriteSettings()
{
	// Copy CGame member variables to ConfigManager
	ConfigManager::Get().SetMagicShortcut(m_sMagicShortCut);
	for (int i = 0; i < 5; i++)
	{
		ConfigManager::Get().SetShortcut(i, m_sShortCut[i]);
	}

	// Audio settings from AudioManager
	ConfigManager::Get().SetSoundVolume(AudioManager::Get().GetSoundVolume());
	ConfigManager::Get().SetMusicVolume(AudioManager::Get().GetMusicVolume());
	ConfigManager::Get().SetSoundEnabled(AudioManager::Get().IsSoundEnabled());
	ConfigManager::Get().SetMusicEnabled(AudioManager::Get().IsMusicEnabled());

	// Save to JSON file
	ConfigManager::Get().Save();
}

CGame::CGame()
{
	m_pInputBuffer = nullptr;
	m_Renderer = nullptr;
	m_dialogBoxManager.Initialize(this);
	m_dialogBoxManager.InitializeDialogBoxes();
	srand((unsigned)time(0));
	ReadSettings();
	RegisterHotkeys();

	iMaxStats = 0;
	iMaxLevel = 0;
	iMaxBankItems = 200; // Default soft cap, server overrides
	m_cLoading = 0;
	m_bIsFirstConn = true;
	m_bIsProgramActive = true;  // Start active, OnActivate(false) will set to false if window loses focus
	m_iItemDropCnt = 0;
	m_bItemDrop = false;
	m_bIsSpecial = false;
	m_cWhisperIndex = DEF_MAXWHISPERMSG;
	std::memset(m_cMapName, 0, sizeof(m_cMapName));
	std::memset(G_cTxt, 0, sizeof(G_cTxt));

	// Initialize CPlayer first since it's used below
	m_pPlayer = std::make_unique<CPlayer>();
	m_pPlayer->m_sPlayerX = 0;
	m_pPlayer->m_sPlayerY = 0;
	m_pPlayer->m_Controller.ResetCommandCount();
	m_pPlayer->m_Controller.SetCommandTime(0); //v2.15 SpeedHack
	// Camera is initialized via its default constructor (calls Reset())
	m_sVDL_X = 0;
	m_sVDL_Y = 0;
	m_wCommObjectID = 0;
	m_wLastAttackTargetID = 0;
	m_wEnterGameType = 0;
	m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
	m_bIsObserverMode = false;

	// Initialize Managers (Networking v4) - using make_unique
	m_pEffectManager = std::make_unique<EffectManager>(this);
	m_pNetworkMessageManager = std::make_unique<NetworkMessageManager>(this);

	// All pointer arrays (std::array<std::unique_ptr<T>, N>) default to nullptr
	// Dialog box order initialization
	for (int i = 0; i < 61; i++) m_dialogBoxManager.SetOrderAt(i, 0);

	// Previous cursor status tracking removed
	CursorTarget::ResetSelectionClickTime();

	std::memset(m_cLogServerAddr, 0, sizeof(m_cLogServerAddr));
	m_iGameServerMode = 2; // Snoopy: Default is INTERNET
	// m_pItemForSaleList defaults to nullptr (std::array<std::unique_ptr<T>, N>)
	m_sPendingShopType = 0;

	// Dialog boxes now self-initialize via SetDefaultRect() in their constructors
	// Dialogs without classes (GiveItem) are initialized in DialogBoxManager::InitDefaults()

	m_iTimeLeftSecAccount = 0;
	m_iTimeLeftSecIP = 0;
	m_bWhisper = true;
	m_bShout = true;

	// Initialize char arrays that were previously zero-initialized by HEAP_ZERO_MEMORY
	std::memset(m_cEdit, 0, sizeof(m_cEdit));
	std::memset(m_cMsg, 0, sizeof(m_cMsg));
	std::memset(m_cChatMsg, 0, sizeof(m_cChatMsg));
	std::memset(m_cBackupChatMsg, 0, sizeof(m_cBackupChatMsg));
	std::memset(m_cAmountString, 0, sizeof(m_cAmountString));
	std::memset(m_cCurLocation, 0, sizeof(m_cCurLocation));
	std::memset(m_cMapMessage, 0, sizeof(m_cMapMessage));
	std::memset(m_cGameServerName, 0, sizeof(m_cGameServerName));
	std::memset(m_cAccountAge, 0, sizeof(m_cAccountAge));
	std::memset(m_cNewPassword, 0, sizeof(m_cNewPassword));
	std::memset(m_cNewPassConfirm, 0, sizeof(m_cNewPassConfirm));
	std::memset(m_cEmailAddr, 0, sizeof(m_cEmailAddr));
	std::memset(m_cAccountQuiz, 0, sizeof(m_cAccountQuiz));
	std::memset(m_cAccountAnswer, 0, sizeof(m_cAccountAnswer));
	std::memset(m_cName_IE, 0, sizeof(m_cName_IE));
	std::memset(m_cTakeHeroItemName, 0, sizeof(m_cTakeHeroItemName));
}

CGame::~CGame()
{
	Renderer::Destroy();
	m_Renderer = nullptr;
}

bool CGame::bInit()
{
	m_pPlayer->m_Controller.SetCommandAvailable(true);
	m_dwTime = GameClock::GetTimeMS();

	// Initialize AudioManager (sounds loaded later during loading screen)
	AudioManager::Get().Initialize(G_hWnd);

	// Initialize ChatCommandManager
	ChatCommandManager::Get().Initialize(this);

	// Initialize GameModeManager
	GameModeManager::Initialize(this);

	GameModeManager::set_screen<Screen_Splash>();

	m_bHideLocalCursor = false;

	// Create and initialize the renderer based on build configuration
#if defined(SFML_ENGINE)
	if (!Renderer::Set(RendererType::SFML))
#elif defined(DDRAW_ENGINE)
	if (!Renderer::Set(RendererType::DirectDraw))
#else
#error "No renderer engine defined. Define SFML_ENGINE or DDRAW_ENGINE."
#endif
	{
		MessageBox(G_hWnd, "Failed to create renderer!", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	m_Renderer = Renderer::Get();
	if (m_Renderer->Init(G_hWnd) == false)
	{
		MessageBox(G_hWnd, "This program requires DirectX7.0a!", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Preload the default font for text rendering
	if (TextLib::GetTextRenderer())
	{
		if (!TextLib::GetTextRenderer()->LoadFontFromFile("FONTS/default.ttf"))
		{
			printf("[FONT] Failed to load default.ttf font file!\n");
		}
	}

	// Initialize sprite factory and register it globally
	m_pSpriteFactory.reset(CreateSpriteFactory(m_Renderer));
	SpriteLib::Sprites::SetFactory(m_pSpriteFactory.get());

	if (bCheckImportantFile() == false)
	{
		MessageBox(G_hWnd, "File checksum error! Get Update again please!", "ERROR1", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	if (_bDecodeBuildItemContents() == false)
	{
		MessageBox(G_hWnd, "File checksum error! Get Update again please!", "ERROR2", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	std::memset(m_cLogServerAddr, 0, sizeof(m_cLogServerAddr));
	strcpy(m_cLogServerAddr, DEF_SERVER_IP);
	m_iLogServerPort = DEF_SERVER_PORT;
	m_iGameServerPort = DEF_GSERVER_PORT;

	if (bInitMagicCfgList() == false) {
		MessageBox(G_hWnd, "MAGICCFG.TXT file contains wrong infomation.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	// Skill
	if (bInitSkillCfgList() == false)
	{
		MessageBox(G_hWnd, "SKILLCFG.TXT file contains wrong infomation.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Mouse position tracking removed - use Input::GetMouseX/Y
	m_pMapData = std::make_unique<CMapData>(this);
	std::memset(m_pPlayer->m_cPlayerName, 0, sizeof(m_pPlayer->m_cPlayerName));
	std::memset(m_pPlayer->m_cAccountName, 0, sizeof(m_pPlayer->m_cAccountName));
	std::memset(m_pPlayer->m_cAccountPassword, 0, sizeof(m_pPlayer->m_cAccountPassword));

	m_pPlayer->m_sPlayerType = 2;
	m_pPlayer->m_Controller.SetPlayerTurn(0);
	// Snoopy: fixed here
	m_dialogBoxManager.SetOrderAt(60, DialogBoxId::HudPanel);
	m_dialogBoxManager.SetOrderAt(59, DialogBoxId::HudPanel); // 29�� GaugePannel

	m_cMenuDir = 4;
	m_cMenuDirCnt = 0;
	m_cMenuFrame = 0;

	_LoadGameMsgTextContents();
	std::memset(m_cWorldServerName, 0, sizeof(m_cWorldServerName));
	strcpy(m_cWorldServerName, NAME_WORLDNAME1);

	// AudioManager initialized in bInit() with HWND
	WeatherManager::Get().Initialize();

	return true;
}

void CGame::Quit()
{
	WriteSettings();
	ChangeGameMode(GameMode::Null);

	// Shutdown manager singletons
	WeatherManager::Get().Shutdown();
	AudioManager::Get().Shutdown();
	ConfigManager::Get().Shutdown();

	// Clear all unique_ptr arrays using range-based for loops
	for (auto& item : m_pItemConfigList) item.reset();

	m_pSprite.clear();
	m_pTileSpr.clear();
	m_pEffectSpr.clear();

	// Clean up sprite factory
	SpriteLib::Sprites::SetFactory(nullptr);
	if (m_pSpriteFactory) {
		m_pSpriteFactory.reset();
	}

	// Sound cleanup handled by AudioManager::Shutdown()
	// Effects now managed by EffectManager (cleaned up in destructor)

	for (auto& ch : m_pCharList) ch.reset();
	for (auto& item : m_pItemList) item.reset();
	for (auto& item : m_pBankList) item.reset();
	for (auto& msg : m_pChatMsgList) msg.reset();
	for (auto& msg : m_pChatScrollList) msg.reset();
	for (auto& msg : m_pWhisperMsg) msg.reset();
	for (auto& item : m_pItemForSaleList) item.reset();
	for (auto& magic : m_pMagicCfgList) magic.reset();
	for (auto& skill : m_pSkillCfgList) skill.reset();
	for (auto& msg : m_pMsgTextList) msg.reset();
	for (auto& msg : m_pMsgTextList2) msg.reset();
	for (auto& msg : m_pAgreeMsgTextList) msg.reset();
	m_pExID.reset();
	for (auto& item : m_pBuildItemList) item.reset();
	for (auto& item : m_pDispBuildItemList) item.reset();
	for (auto& msg : m_pGameMsgList) msg.reset();

	// Clean up single pointers (unique_ptr handles null checks automatically)
	m_pMapData.reset();
	m_pGSock.reset();
	m_pLSock.reset();
	if (G_pCalcSocket != 0) delete G_pCalcSocket;
	m_pEffectManager.reset();
	m_pNetworkMessageManager.reset();
}

// UpdateScreen and DrawScreen removed - all modes now handled by Screen/Overlay system via GameModeManager

// DrawCursor: Centralized cursor drawing at end of frame
// Called at the end of DrawScreen() to ensure cursor is always on top
void CGame::DrawCursor()
{
	// Check if cursor should be hidden
	if (m_bHideLocalCursor)
		return;

	// Get mouse position
	int msX = Input::GetMouseX();
	int msY = Input::GetMouseY();

	// Skip if mouse position is invalid (not yet initialized)
	if (msX == 0 && msY == 0)
		return;

	// Determine cursor frame based on game mode from manager (source of truth)
	int iCursorFrame = 0;  // Default arrow cursor

	switch (GameModeManager::GetMode()) {
	case GameMode::MainGame:
		// In-game uses context-sensitive cursor from CursorTarget
		if (m_bIsObserverMode) {
			// Observer mode shows a small crosshair instead of cursor sprite
			m_Renderer->PutPixel(msX, msY, 255, 255, 255);
			m_Renderer->PutPixel(msX + 1, msY, 255, 255, 255);
			m_Renderer->PutPixel(msX - 1, msY, 255, 255, 255);
			m_Renderer->PutPixel(msX, msY + 1, 255, 255, 255);
			m_Renderer->PutPixel(msX, msY - 1, 255, 255, 255);
			return;
		}
		iCursorFrame = CursorTarget::GetCursorFrame();
		break;

	case GameMode::Connecting:
	case GameMode::WaitingResponse:
	case GameMode::WaitingInitData:
		// Waiting/connecting states use hourglass cursor (frame 8)
		iCursorFrame = 8;
		break;

	default:
		// All other modes use default arrow cursor (frame 0)
		iCursorFrame = 0;
		break;
	}

	// Draw the cursor sprite
	if (m_pSprite[DEF_SPRID_MOUSECURSOR])
		m_pSprite[DEF_SPRID_MOUSECURSOR]->Draw(msX, msY, iCursorFrame);
}



// RenderFrame: Centralized rendering frame wrapper
// Handles: Update -> (skip check) -> Clear backbuffer -> Draw -> Fade overlay -> Flip
// This centralizes surface operations that were previously scattered across all screen methods
void CGame::RenderFrame()
{
	// Update manager singletons with frame delta time
	double deltaTime = FrameTiming::GetDeltaTime();
	AudioManager::Get().Update(deltaTime);

	// Process timer and network events (must happen before any update logic)
	// These were previously in UpdateScreen but need to run regardless of screen system
	extern std::atomic<bool> G_bTimerSignal;
	if (G_bTimerSignal.exchange(false)) {
		OnTimer();
	}
	OnGameSocketEvent();
	OnLogSocketEvent();

	// Update game mode transition state (fade in/out progress)
	GameModeManager::Update();

	// ============== Update Phase ==============
	FrameTiming::BeginProfile(ProfileStage::Update);
	GameModeManager::UpdateScreens();
	FrameTiming::EndProfile(ProfileStage::Update);

	// ============== Render Phase ==============
	FrameTiming::BeginProfile(ProfileStage::ClearBuffer);
	m_Renderer->BeginFrame();
	FrameTiming::EndProfile(ProfileStage::ClearBuffer);

	// Render screens/overlays (skipped during Switching phase)
	if (GameModeManager::GetTransitionState() != TransitionState::Switching)
	{
		GameModeManager::Render();
	}

	// Draw fade overlay if transitioning between game modes
	if (GameModeManager::IsTransitioning())
	{
		float alpha = GameModeManager::GetFadeAlpha();
		m_Renderer->DrawFadeOverlay(alpha);
	}

	// Cursor always on top - drawn LAST after everything including fade overlay
	DrawCursor();

	// Flip to show the drawn content
	FrameTiming::BeginProfile(ProfileStage::Flip);
	if (m_Renderer->EndFrameCheckLostSurface())
		RestoreSprites();
	FrameTiming::EndProfile(ProfileStage::Flip);

	// Count this as a displayed frame for FPS calculation
	FrameTiming::CountDisplayedFrame();
}


// MODERNIZED: v4 Networking Architecture (Drain -> Queue -> Process)
void CGame::OnGameSocketEvent()
{
    if (m_pGSock == 0) return;

    // 1. Check for socket state changes (Connect, Close, Error)
    int iRet = m_pGSock->Poll();

    switch (iRet) {
    case DEF_XSOCKEVENT_SOCKETCLOSED:
        ChangeGameMode(GameMode::ConnectionLost);
        m_pGSock.reset();
        m_pGSock.reset();
        return;
    case DEF_XSOCKEVENT_SOCKETERROR:
        printf("[ERROR] Game socket error\n");
        ChangeGameMode(GameMode::ConnectionLost);
        m_pGSock.reset();
        m_pGSock.reset();
        return;

    case DEF_XSOCKEVENT_CONNECTIONESTABLISH:
        ConnectionEstablishHandler(DEF_SERVERTYPE_GAME);
        break;
    }

    // 2. Drain all available data from TCP buffer to the Queue
    // Only drain if socket is connected (m_bIsAvailable is set on FD_CONNECT)
    if (!m_pGSock->m_bIsAvailable) {
        return; // Still connecting, don't try to read yet
    }

    int iDrained = m_pGSock->DrainToQueue();

    if (iDrained < 0) {
        printf("[ERROR] Game socket DrainToQueue failed: %d\n", iDrained);
        ChangeGameMode(GameMode::ConnectionLost);
        m_pGSock.reset();
        m_pGSock.reset();
        return;
    }

    // 3. Process the queue with a Time Budget
    //    We process as many packets as possible within the budget to keep the game responsive.
    constexpr int MAX_PACKETS_PER_FRAME = 120; // Safety limit
    constexpr uint32_t MAX_TIME_MS = 3;        // 3ms budget for network processing

    uint32_t dwStartTime = GameClock::GetTimeMS();
    int iProcessed = 0;

    NetworkPacket packet;
    while (iProcessed < MAX_PACKETS_PER_FRAME) {

        // Check budget
        if (GameClock::GetTimeMS() - dwStartTime > MAX_TIME_MS) {
            break;
        }

        // Peek next packet
        if (!m_pGSock->PeekPacket(packet)) {
            break; // Queue empty
        }

        // Update functionality timestamps (legacy requirement)
        m_dwLastNetRecvTime = GameClock::GetTimeMS();
        m_dwTime = GameClock::GetTimeMS();

        // Process (using the pointer directly from the packet vector)
        if (!packet.empty()) {
             GameRecvMsgHandler(static_cast<uint32_t>(packet.size()), 
                                const_cast<char*>(packet.ptr()));
        }

        // CRITICAL FIX: The handler might have closed/deleted the socket!
        if (m_pGSock == nullptr) return;

        // Pop logic (remove from queue)
        m_pGSock->PopPacket();
        iProcessed++;
    }
}

void CGame::RestoreSprites()
{
	for (auto& [idx, spr] : m_pSprite)
	{
		spr->Restore();
	}
}

bool CGame::bSendCommand(uint32_t dwMsgID, uint16_t wCommand, char cDir, int iV1, int iV2, int iV3, char* pString, int iV4)
{
	char cMsg[300], cKey;
	DWORD dwTime;
	int iRet, i;

	if ((m_pGSock == 0) && (m_pLSock == 0)) return false;
	dwTime = GameClock::GetTimeMS();
	std::memset(cMsg, 0, sizeof(cMsg));
	cKey = (char)(rand() % 255) + 1;

	switch (dwMsgID) {

	case DEF_REQUEST_ANGEL:	// to Game Server
	{
		hb::net::PacketRequestAngel req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.name, 0, sizeof(req.name));
		if (pString != nullptr) {
			std::size_t name_len = std::strlen(pString);
			if (name_len >= sizeof(req.name)) name_len = sizeof(req.name) - 1;
			std::memcpy(req.name, pString, name_len);
		}
		req.angel_id = iV1;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case DEF_REQUEST_RESURRECTPLAYER_YES: // By snoopy
	case DEF_REQUEST_RESURRECTPLAYER_NO:  // By snoopy
	{
		hb::net::PacketRequestHeaderOnly req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_HELDENIAN_SCROLL:// By snoopy
	{
		hb::net::PacketRequestHeldenianScroll req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.name, 0, sizeof(req.name));
		if (pString != nullptr) {
			std::size_t name_len = std::strlen(pString);
			if (name_len >= sizeof(req.name)) name_len = sizeof(req.name) - 1;
			std::memcpy(req.name, pString, name_len);
		}
		req.item_id = wCommand;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_TELEPORT_LIST:
	{
		hb::net::PacketRequestName20 req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.name, 0, sizeof(req.name));
		std::memcpy(req.name, "William", sizeof(req.name));
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_HELDENIAN_TP_LIST: // Snoopy: Heldenian TP
	{
		hb::net::PacketRequestName20 req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.name, 0, sizeof(req.name));
		std::memcpy(req.name, "Gail", sizeof(req.name));
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_HELDENIAN_TP: // Snoopy: Heldenian TP
	case MSGID_REQUEST_CHARGED_TELEPORT:
	{
		hb::net::PacketRequestTeleportId req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.teleport_id = iV1;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_SELLITEMLIST:
	{
		hb::net::PacketRequestSellItemList req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		for (i = 0; i < DEF_MAXSELLLIST; i++) {
			req.entries[i].index = static_cast<uint8_t>(m_stSellItemList[i].iIndex);
			req.entries[i].amount = m_stSellItemList[i].iAmount;
		}
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_RESTART:
	{
		hb::net::PacketRequestHeaderOnly req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_PANNING:
	{
		hb::net::PacketRequestPanning req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.dir = static_cast<uint8_t>(cDir);
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_CHANGEPASSWORD:
	{
		hb::net::ChangePasswordRequest req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.new_password, 0, sizeof(req.new_password));
		std::memcpy(req.new_password, m_cNewPassword, sizeof(req.new_password));
		std::memset(req.new_password_confirm, 0, sizeof(req.new_password_confirm));
		std::memcpy(req.new_password_confirm, m_cNewPassConfirm, sizeof(req.new_password_confirm));
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_CREATENEWACCOUNT:
		// to Log Server
	{
		hb::net::CreateAccountRequest req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.email, 0, sizeof(req.email));
		std::memcpy(req.email, m_cEmailAddr, sizeof(req.email));
		std::memset(req.quiz, 0, sizeof(req.quiz));
		std::memcpy(req.quiz, m_cAccountQuiz, sizeof(req.quiz));
		std::memset(req.answer, 0, sizeof(req.answer));
		std::memcpy(req.answer, m_cAccountAnswer, sizeof(req.answer));
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_GETMINIMUMLOADGATEWAY:
	case MSGID_REQUEST_LOGIN:
		// to Log Server
	{
		hb::net::LoginRequest req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.world_name, 0, sizeof(req.world_name));
		std::memcpy(req.world_name, m_cWorldServerName, sizeof(req.world_name));
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}

	break;

	case MSGID_REQUEST_CREATENEWCHARACTER:
		// to Log Server
	{
		hb::net::CreateCharacterRequest req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.character_name, 0, sizeof(req.character_name));
		std::memcpy(req.character_name, m_pPlayer->m_cPlayerName, sizeof(req.character_name));
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.world_name, 0, sizeof(req.world_name));
		std::memcpy(req.world_name, m_cWorldServerName, sizeof(req.world_name));
		req.gender = static_cast<uint8_t>(m_pPlayer->m_iGender);
		req.skin = static_cast<uint8_t>(m_pPlayer->m_iSkinCol);
		req.hairstyle = static_cast<uint8_t>(m_pPlayer->m_iHairStyle);
		req.haircolor = static_cast<uint8_t>(m_pPlayer->m_iHairCol);
		req.underware = static_cast<uint8_t>(m_pPlayer->m_iUnderCol);
		req.str = static_cast<uint8_t>(m_pPlayer->m_iStatModStr);
		req.vit = static_cast<uint8_t>(m_pPlayer->m_iStatModVit);
		req.dex = static_cast<uint8_t>(m_pPlayer->m_iStatModDex);
		req.intl = static_cast<uint8_t>(m_pPlayer->m_iStatModInt);
		req.mag = static_cast<uint8_t>(m_pPlayer->m_iStatModMag);
		req.chr = static_cast<uint8_t>(m_pPlayer->m_iStatModChr);
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_ENTERGAME:
		// to Log Server
	{
		hb::net::EnterGameRequestFull req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = static_cast<uint16_t>(m_wEnterGameType);
		std::memset(req.character_name, 0, sizeof(req.character_name));
		std::memcpy(req.character_name, m_pPlayer->m_cPlayerName, sizeof(req.character_name));
		std::memset(req.map_name, 0, sizeof(req.map_name));
		std::memcpy(req.map_name, m_cMapName, sizeof(req.map_name));
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		req.level = m_pPlayer->m_iLevel;
		std::memset(req.world_name, 0, sizeof(req.world_name));
		std::memcpy(req.world_name, m_cWorldServerName, sizeof(req.world_name));
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_DELETECHARACTER:
		// to Log Server
	{
		hb::net::DeleteCharacterRequest req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = static_cast<uint16_t>(m_wEnterGameType);
		std::memset(req.character_name, 0, sizeof(req.character_name));
		std::memcpy(req.character_name, m_pCharList[m_wEnterGameType - 1]->m_cName, sizeof(req.character_name));
		std::memset(req.account_name, 0, sizeof(req.account_name));
		std::memcpy(req.account_name, m_pPlayer->m_cAccountName, sizeof(req.account_name));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.world_name, 0, sizeof(req.world_name));
		std::memcpy(req.world_name, m_cWorldServerName, sizeof(req.world_name));
		iRet = m_pLSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_SETITEMPOS:
		// to Game Server
	{
		hb::net::PacketRequestSetItemPos req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.dir = static_cast<uint8_t>(cDir);
		req.x = static_cast<int16_t>(iV1);
		req.y = static_cast<int16_t>(iV2);
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}
	break;

	case MSGID_COMMAND_CHECKCONNECTION:
	{
		hb::net::PacketCommandCheckConnection req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.time_ms = dwTime;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}

	break;

	case MSGID_REQUEST_INITDATA:
	case MSGID_REQUEST_INITPLAYER:
		// to Game Server
	{
		hb::net::PacketRequestInitPlayer req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		std::memset(req.player, 0, sizeof(req.player));
		std::memcpy(req.player, m_pPlayer->m_cPlayerName, sizeof(req.player));
		std::memset(req.account, 0, sizeof(req.account));
		std::memcpy(req.account, m_pPlayer->m_cAccountName, sizeof(req.account));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		req.is_observer = static_cast<uint8_t>(m_bIsObserverMode);
		std::memset(req.server, 0, sizeof(req.server));
		std::memcpy(req.server, m_cGameServerName, sizeof(req.server));
		req.padding = 0;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}

	//m_bIsObserverMode = false;
	break;
	case MSGID_LEVELUPSETTINGS:
	{
		hb::net::PacketRequestLevelUpSettings req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.str = m_pPlayer->m_wLU_Str;
		req.vit = m_pPlayer->m_wLU_Vit;
		req.dex = m_pPlayer->m_wLU_Dex;
		req.intel = m_pPlayer->m_wLU_Int;
		req.mag = m_pPlayer->m_wLU_Mag;
		req.chr = m_pPlayer->m_wLU_Char;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_COMMAND_CHATMSG:
		if (m_bIsTeleportRequested == true) return false;
		if (pString == 0) return false;

		// to Game Server
		{
			hb::net::PacketCommandChatMsgHeader req{};
			req.header.msg_id = dwMsgID;
			req.header.msg_type = 0;
			req.x = m_pPlayer->m_sPlayerX;
			req.y = m_pPlayer->m_sPlayerY;
			std::memset(req.name, 0, sizeof(req.name));
			std::memcpy(req.name, m_pPlayer->m_cPlayerName, sizeof(req.name));
			req.chat_type = static_cast<uint8_t>(iV1);
			if (bCheckLocalChatCommand(pString) == true) return false;
			std::size_t text_len = std::strlen(pString);
			std::memset(cMsg, 0, sizeof(cMsg));
			std::memcpy(cMsg, &req, sizeof(req));
			std::memcpy(cMsg + sizeof(req), pString, text_len + 1);
			iRet = m_pGSock->iSendMsg(cMsg, static_cast<int>(sizeof(req) + text_len + 1));
		}
		break;

	case MSGID_COMMAND_COMMON:
		if (m_bIsTeleportRequested == true) return false;
		switch (wCommand) {
		case DEF_COMMONTYPE_BUILDITEM:
		{
			hb::net::PacketCommandCommonBuild req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			std::memset(req.name, 0, sizeof(req.name));
			if (pString != 0) {
				std::size_t name_len = std::strlen(pString);
				if (name_len > sizeof(req.name)) name_len = sizeof(req.name);
				std::memcpy(req.name, pString, name_len);
			}
			req.item_ids[0] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1);
			req.item_ids[1] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2);
			req.item_ids[2] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3);
			req.item_ids[3] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4);
			req.item_ids[4] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5);
			req.item_ids[5] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6);
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
		}
		break;

		case DEF_COMMONTYPE_REQ_CREATEPORTION:
		{
			hb::net::PacketCommandCommonItems req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			req.item_ids[0] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1);
			req.item_ids[1] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2);
			req.item_ids[2] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3);
			req.item_ids[3] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4);
			req.item_ids[4] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5);
			req.item_ids[5] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6);
			req.padding = 0;
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
		}
		break;

		//Crafting
		case DEF_COMMONTYPE_CRAFTITEM:
		{
			hb::net::PacketCommandCommonBuild req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			std::memset(req.name, ' ', sizeof(req.name));
			req.item_ids[0] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1);
			req.item_ids[1] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2);
			req.item_ids[2] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3);
			req.item_ids[3] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4);
			req.item_ids[4] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5);
			req.item_ids[5] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6);
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
		}
		break;

		// Create Slate Request - Diuuude
		case DEF_COMMONTYPE_REQ_CREATESLATE:
		{
			hb::net::PacketCommandCommonItems req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			req.item_ids[0] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV1);
			req.item_ids[1] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV2);
			req.item_ids[2] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV3);
			req.item_ids[3] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV4);
			req.item_ids[4] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV5);
			req.item_ids[5] = static_cast<uint8_t>(m_dialogBoxManager.Info(DialogBoxId::Slates).sV6);
			req.padding = 0;
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
		}
		break;

		default:
			if (pString == 0)
			{
				hb::net::PacketCommandCommonWithTime req{};
				req.base.header.msg_id = dwMsgID;
				req.base.header.msg_type = wCommand;
				req.base.x = m_pPlayer->m_sPlayerX;
				req.base.y = m_pPlayer->m_sPlayerY;
				req.base.dir = static_cast<uint8_t>(cDir);
				req.v1 = iV1;
				req.v2 = iV2;
				req.v3 = iV3;
				req.time_ms = dwTime;
				iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
			}
			else
			{
				hb::net::PacketCommandCommonWithString req{};
				req.base.header.msg_id = dwMsgID;
				req.base.header.msg_type = wCommand;
				req.base.x = m_pPlayer->m_sPlayerX;
				req.base.y = m_pPlayer->m_sPlayerY;
				req.base.dir = static_cast<uint8_t>(cDir);
				req.v1 = iV1;
				req.v2 = iV2;
				req.v3 = iV3;
				std::memset(req.text, 0, sizeof(req.text));
				std::size_t text_len = std::strlen(pString);
				if (text_len > sizeof(req.text)) text_len = sizeof(req.text);
				std::memcpy(req.text, pString, text_len);
				req.v4 = iV4;
				iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
			}
			break;
		}

		break;

	case MSGID_REQUEST_CREATENEWGUILD:
	case MSGID_REQUEST_DISBANDGUILD:
		// to Game Server
	{
		hb::net::PacketRequestGuildAction req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = DEF_MSGTYPE_CONFIRM;
		std::memset(req.player, 0, sizeof(req.player));
		std::memcpy(req.player, m_pPlayer->m_cPlayerName, sizeof(req.player));
		std::memset(req.account, 0, sizeof(req.account));
		std::memcpy(req.account, m_pPlayer->m_cAccountName, sizeof(req.account));
		std::memset(req.password, 0, sizeof(req.password));
		std::memcpy(req.password, m_pPlayer->m_cAccountPassword, sizeof(req.password));
		std::memset(req.guild, 0, sizeof(req.guild));
		char cTemp[21];
		std::memset(cTemp, 0, sizeof(cTemp));
		std::memcpy(cTemp, m_pPlayer->m_cGuildName, 20);
		CMisc::ReplaceString(cTemp, ' ', '_');
		std::memcpy(req.guild, cTemp, sizeof(req.guild));
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case MSGID_REQUEST_TELEPORT:
	{
		hb::net::PacketRequestHeaderOnly req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = DEF_MSGTYPE_CONFIRM;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}

	m_bIsTeleportRequested = true;
	break;

	case MSGID_REQUEST_CIVILRIGHT:
	{
		hb::net::PacketRequestHeaderOnly req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = DEF_MSGTYPE_CONFIRM;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}
	break;

	case MSGID_REQUEST_RETRIEVEITEM:
	{
		hb::net::PacketRequestRetrieveItem req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = DEF_MSGTYPE_CONFIRM;
		req.item_slot = static_cast<uint8_t>(iV1);
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}
	break;

	case MSGID_REQUEST_NOTICEMENT:
	{
		hb::net::PacketRequestNoticement req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.value = iV1;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req), cKey);
	}
	break;

	case  MSGID_REQUEST_FIGHTZONE_RESERVE:
	{
		hb::net::PacketRequestFightzoneReserve req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.fightzone = iV1;
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}
	break;

	case MSGID_STATECHANGEPOINT:
		// Diuuude
	{
		hb::net::PacketRequestStateChange req{};
		req.header.msg_id = dwMsgID;
		req.header.msg_type = 0;
		req.change1 = static_cast<uint8_t>(cStateChange1);
		req.change2 = static_cast<uint8_t>(cStateChange2);
		req.change3 = static_cast<uint8_t>(cStateChange3);
		iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
	}
	break;

	default:
		if (m_bIsTeleportRequested == true) return false;
		if ((wCommand == DEF_OBJECTATTACK) || (wCommand == DEF_OBJECTATTACKMOVE))
		{
			hb::net::PacketCommandMotionAttack req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			req.base.dx = static_cast<int16_t>(iV1);
			req.base.dy = static_cast<int16_t>(iV2);
			req.base.type = static_cast<int16_t>(iV3);
			req.target_id = static_cast<uint16_t>(iV4);
			req.time_ms = dwTime;
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req));
		}
		else
		{
			hb::net::PacketCommandMotionSimple req{};
			req.base.header.msg_id = dwMsgID;
			req.base.header.msg_type = wCommand;
			req.base.x = m_pPlayer->m_sPlayerX;
			req.base.y = m_pPlayer->m_sPlayerY;
			req.base.dir = static_cast<uint8_t>(cDir);
			req.base.dx = static_cast<int16_t>(iV1);
			req.base.dy = static_cast<int16_t>(iV2);
			req.base.type = static_cast<int16_t>(iV3);
			req.time_ms = dwTime;
			iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&req), sizeof(req)); //v2.171
		}
		m_pPlayer->m_Controller.IncrementCommandCount();
		break;
	}
	switch (iRet) {
	case DEF_XSOCKEVENT_SOCKETCLOSED:
	case DEF_XSOCKEVENT_SOCKETERROR:
	case DEF_XSOCKEVENT_QUENEFULL:
		printf("[ERROR] bSendCommand failed: ret=%d msgid=0x%X cmd=0x%X\n", iRet, dwMsgID, wCommand);
		ChangeGameMode(GameMode::ConnectionLost);
	m_pGSock.reset();
	m_pGSock.reset();
	break;

	case DEF_XSOCKEVENT_CRITICALERROR:
	{
		char cDbg[160];
		wsprintf(cDbg, "[NETWARN] bSendCommand: CRITICAL ret=%d msgid=0x%X cmd=0x%X\n", iRet, dwMsgID, wCommand);
		printf("%s", cDbg);
	}
	m_pGSock.reset();
	m_pGSock.reset();
	if (G_pCalcSocket != 0) {
		delete G_pCalcSocket;
		G_pCalcSocket = 0;
	}
	SendMessage(G_hWnd, WM_DESTROY, 0, 0);
	break;
	}
	return true;
}

void CGame::DrawObjects(short sPivotX, short sPivotY, short sDivX, short sDivY, short sModX, short sModY, short msX, short msY)
{
	int ix, iy, indexX, indexY, dX, dY, iDvalue;
	char cItemColor;
	bool bIsPlayerDrawed = false;
	bool bRet = false;
	short sItemSprite, sItemSpriteFrame, sObjSpr, sObjSprFrame, sDynamicObject, sDynamicObjectFrame;
	char cDynamicObjectData1, cDynamicObjectData2, cDynamicObjectData3, cDynamicObjectData4;
	// Xmas
	static int ix1[100];
	static int iy2[100];
	static int iXmasTreeBulbDelay = 76;
	int idelay = 75;
	bool frame_omit = false;

	// Item's desc on floor
	uint32_t dwItemAttr, dwItemSelectedAttr;
	int iItemSelectedx, iItemSelectedy;
	short sItemID, sItemSelectedID = -1;

	int res_x = LOGICAL_MAX_X;
	int res_y = LOGICAL_MAX_Y;
	int res_msy = LOGICAL_HEIGHT - 49;

	if (sDivY < 0 || sDivX < 0) return;

	// Initialize Picking system for this frame
	CursorTarget::BeginFrame();

	//dwTime = GameClock::GetTimeMS();
	uint32_t dwTime = m_dwCurTime;

	// Pre-calculate map data bounds for efficient boundary checking
	const short mapMinX = m_pMapData->m_sPivotX;
	const short mapMaxX = m_pMapData->m_sPivotX + MAPDATASIZEX;
	const short mapMinY = m_pMapData->m_sPivotY;
	const short mapMaxY = m_pMapData->m_sPivotY + MAPDATASIZEY;

	// Tile-based loop bounds (much cleaner than pixel-based)
	// Buffer: 7 tiles around visible area for smooth object sliding
	// Extra 2 tiles on bottom for layering/depth sorting of tall objects
	constexpr int TILE_SIZE = 32;
	constexpr int BUFFER_TILES = 7;
	constexpr int EXTRA_BOTTOM_TILES = 2;  // For depth sorting of tall objects

	const int visibleTilesX = (res_x / TILE_SIZE) + 1;  // ~20 tiles
	const int visibleTilesY = (res_y / TILE_SIZE) + 1;  // ~15 tiles

	const int startTileX = -BUFFER_TILES;
	const int endTileX = visibleTilesX + BUFFER_TILES;
	const int startTileY = -BUFFER_TILES;
	const int endTileY = visibleTilesY + BUFFER_TILES + EXTRA_BOTTOM_TILES;

	// Visibility bounds in pixels (for culling non-visible tiles from detailed processing)
	const int visMinX = -sModX;
	const int visMaxX = res_x + 16;
	const int visMinY = -sModY;
	const int visMaxY = res_y + 32 + 16;

	// Loop over tiles, calculate pixel positions when needed
	for (int tileY = startTileY; tileY <= endTileY; tileY++)
	{
		indexY = sDivY + sPivotY + tileY;
		iy = tileY * TILE_SIZE - sModY;

		for (int tileX = startTileX; tileX <= endTileX; tileX++)
		{
			indexX = sDivX + sPivotX + tileX;
			ix = tileX * TILE_SIZE - sModX;

			sDynamicObject = 0;
			bRet = false;
			if ((ix >= visMinX) && (ix <= visMaxX) && (iy >= visMinY) && (iy <= visMaxY))
			{
				m_entityState.m_wObjectID = m_entityState.m_sOwnerType = m_entityState.m_sAppr1 = m_entityState.m_sAppr2 = m_entityState.m_sAppr3 = m_entityState.m_sAppr4 = m_entityState.m_iStatus = 0;
				m_entityState.m_iDir = m_entityState.m_iFrame = 0;
				m_entityState.m_iEffectType = m_entityState.m_iEffectFrame = m_entityState.m_iChatIndex = 0;
				std::memset(m_entityState.m_cName.data(), 0, m_entityState.m_cName.size());
				if ((indexX < mapMinX) || (indexX > mapMaxX) ||
					(indexY < mapMinY) || (indexY > mapMaxY))
				{
					sItemID = 0;
					bRet = false;
					cItemColor = 0;
					dwItemAttr = 0;
				}
				else
				{
					m_entityState.m_iDataX = dX = indexX - mapMinX;
					m_entityState.m_iDataY = dY = indexY - mapMinY;
					m_entityState.m_wObjectID = m_pMapData->m_pData[dX][dY].m_wDeadObjectID;
					m_entityState.m_sOwnerType = m_pMapData->m_pData[dX][dY].m_sDeadOwnerType;
					m_entityState.m_iDir = m_pMapData->m_pData[dX][dY].m_cDeadDir;
					m_entityState.m_sAppr1 = m_pMapData->m_pData[dX][dY].m_sDeadAppr1;
					m_entityState.m_sAppr2 = m_pMapData->m_pData[dX][dY].m_sDeadAppr2;
					m_entityState.m_sAppr3 = m_pMapData->m_pData[dX][dY].m_sDeadAppr3;
					m_entityState.m_sAppr4 = m_pMapData->m_pData[dX][dY].m_sDeadAppr4;
					m_entityState.m_iApprColor = m_pMapData->m_pData[dX][dY].m_iDeadApprColor;
					m_entityState.m_iFrame = m_pMapData->m_pData[dX][dY].m_cDeadOwnerFrame;
					m_entityState.m_iChatIndex = m_pMapData->m_pData[dX][dY].m_iDeadChatMsg;
					m_entityState.m_iStatus = m_pMapData->m_pData[dX][dY].m_iDeadStatus;
					strcpy(m_entityState.m_cName.data(), m_pMapData->m_pData[dX][dY].m_cDeadOwnerName);
					sItemID = m_pMapData->m_pData[dX][dY].m_sItemID;
					dwItemAttr = m_pMapData->m_pData[dX][dY].m_dwItemAttr;
					cItemColor = m_pMapData->m_pData[dX][dY].m_cItemColor;
					sDynamicObject = m_pMapData->m_pData[dX][dY].m_sDynamicObjectType;
					sDynamicObjectFrame = (short)m_pMapData->m_pData[dX][dY].m_cDynamicObjectFrame;
					cDynamicObjectData1 = m_pMapData->m_pData[dX][dY].m_cDynamicObjectData1;
					cDynamicObjectData2 = m_pMapData->m_pData[dX][dY].m_cDynamicObjectData2;
					cDynamicObjectData3 = m_pMapData->m_pData[dX][dY].m_cDynamicObjectData3;
					cDynamicObjectData4 = m_pMapData->m_pData[dX][dY].m_cDynamicObjectData4;
					m_entityState.m_bSpriteOmit = m_pMapData->m_pData[dX][dY].m_bSpriteOmit;
					bRet = true;
				}

				if ((bRet == true) && (sItemID != 0) && m_pItemConfigList[sItemID] != 0)
				{
					if (cItemColor == 0)
						m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + m_pItemConfigList[sItemID]->m_sSprite]->Draw(ix, iy, m_pItemConfigList[sItemID]->m_sSpriteFrame);
					else
					{
						switch (m_pItemConfigList[sItemID]->m_sSprite) {
						case 1: // Swds
						case 2: // Bows
						case 3: // Shields
						case 15: // Axes hammers
							m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + m_pItemConfigList[sItemID]->m_sSprite]->Draw(ix, iy, m_pItemConfigList[sItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[cItemColor].r - GameColors::Base.r, GameColors::Weapons[cItemColor].g - GameColors::Base.g, GameColors::Weapons[cItemColor].b - GameColors::Base.b));
							break;
						default:
							m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + m_pItemConfigList[sItemID]->m_sSprite]->Draw(ix, iy, m_pItemConfigList[sItemID]->m_sSpriteFrame, SpriteLib::DrawParams::Tint(GameColors::Items[cItemColor].r - GameColors::Base.r, GameColors::Items[cItemColor].g - GameColors::Base.g, GameColors::Items[cItemColor].b - GameColors::Base.b));
							break;
						}
					}

					if (Input::IsShiftDown() && msX >= ix - 16 && msY >= iy - 16 && msX <= ix + 16 && msY <= iy + 16) {
						sItemSelectedID = sItemID;
						dwItemSelectedAttr = dwItemAttr;
						iItemSelectedx = ix;
						iItemSelectedy = iy;
					}

					// Test ground item with Picking system
					CursorTarget::TestGroundItem(ix, iy, res_msy);
				}

				if ((bRet == true) && (m_entityState.m_wObjectID != 0))
				{
					SpriteLib::BoundRect bounds = DrawObject_OnDead(indexX, indexY, ix, iy, false, dwTime);

					// Build picking info for dead object
					TargetObjectInfo info = {};
					info.objectID = m_entityState.m_wObjectID;
					info.mapX = indexX;
					info.mapY = indexY;
					info.screenX = ix;
					info.screenY = iy;
					info.dataX = m_entityState.m_iDataX;
					info.dataY = m_entityState.m_iDataY;
					info.ownerType = m_entityState.m_sOwnerType;
					info.action = DEF_OBJECTDEAD;
					info.direction = m_entityState.m_iDir;
					info.frame = m_entityState.m_iFrame;
					info.name = m_entityState.m_cName.data();
					info.appr1 = m_entityState.m_sAppr1;
					info.appr2 = m_entityState.m_sAppr2;
					info.appr3 = m_entityState.m_sAppr3;
					info.appr4 = m_entityState.m_sAppr4;
					info.apprColor = m_entityState.m_iApprColor;
					info.status = m_entityState.m_iStatus;
					info.type = FocusedObjectType::DeadBody;
					CursorTarget::TestObject(bounds, info, iy, res_msy);
				}

				m_entityState.m_wObjectID = m_entityState.m_sOwnerType = m_entityState.m_sAppr1 = m_entityState.m_sAppr2 = m_entityState.m_sAppr3 = m_entityState.m_sAppr4 = m_entityState.m_iStatus = 0;
				m_entityState.m_iFrame = m_entityState.m_iDir = 0;
				m_entityState.m_iEffectType = m_entityState.m_iEffectFrame = m_entityState.m_iApprColor = m_entityState.m_iChatIndex = 0;
				std::memset(m_entityState.m_cName.data(), 0, m_entityState.m_cName.size());

				if ((indexX < mapMinX) || (indexX > mapMaxX) ||
					(indexY < mapMinY) || (indexY > mapMaxY))
				{
					sItemID = 0;
					bRet = false;
				}
				else
				{
					m_entityState.m_iDataX = dX = indexX - mapMinX;
					m_entityState.m_iDataY = dY = indexY - mapMinY;
					m_entityState.m_wObjectID = m_pMapData->m_pData[dX][dY].m_wObjectID;
					m_entityState.m_sOwnerType = m_pMapData->m_pData[dX][dY].m_sOwnerType;
					m_entityState.m_iAction = m_pMapData->m_pData[dX][dY].m_cOwnerAction;
					m_entityState.m_iStatus = m_pMapData->m_pData[dX][dY].m_iStatus;
					m_entityState.m_iDir = m_pMapData->m_pData[dX][dY].m_cDir;
					m_entityState.m_sAppr1 = m_pMapData->m_pData[dX][dY].m_sAppr1;
					m_entityState.m_sAppr2 = m_pMapData->m_pData[dX][dY].m_sAppr2;
					m_entityState.m_sAppr3 = m_pMapData->m_pData[dX][dY].m_sAppr3;
					m_entityState.m_sAppr4 = m_pMapData->m_pData[dX][dY].m_sAppr4;
					m_entityState.m_iApprColor = m_pMapData->m_pData[dX][dY].m_iApprColor; // v1.4
					m_entityState.m_iFrame = m_pMapData->m_pData[dX][dY].m_cOwnerFrame;
					m_entityState.m_iChatIndex = m_pMapData->m_pData[dX][dY].m_iChatMsg;
					m_entityState.m_iEffectType = m_pMapData->m_pData[dX][dY].m_iEffectType;
					m_entityState.m_iEffectFrame = m_pMapData->m_pData[dX][dY].m_iEffectFrame;
					m_entityState.m_bSpriteOmit = m_pMapData->m_pData[dX][dY].m_bSpriteOmit;
					strcpy(m_entityState.m_cName.data(), m_pMapData->m_pData[dX][dY].m_cOwnerName);
					bRet = true;

					if (m_iIlusionOwnerH != 0)
					{
						if ((strcmp(m_entityState.m_cName.data(), m_pPlayer->m_cPlayerName) != 0) && (m_entityState.m_sOwnerType < 10))
						{
							m_entityState.m_sOwnerType = m_cIlusionOwnerType;
							m_entityState.m_iStatus = m_pPlayer->m_iStatus_IE;
							m_entityState.m_sAppr1 = m_pPlayer->m_sAppr1_IE;
							m_entityState.m_sAppr2 = m_pPlayer->m_sAppr2_IE;
							m_entityState.m_sAppr3 = m_pPlayer->m_sAppr3_IE;
							m_entityState.m_sAppr4 = m_pPlayer->m_sAppr4_IE;
							m_entityState.m_iApprColor = m_pPlayer->m_iApprColor_IE;
						}
					}
				}

				if ((bRet == true) && (strlen(m_entityState.m_cName.data()) > 0))
				{
					m_entityState.m_iMoveOffsetX = 0;
					m_entityState.m_iMoveOffsetY = 0;
					SpriteLib::BoundRect bounds = {0, -1, 0, 0};
					switch (m_entityState.m_iAction) {
					case DEF_OBJECTSTOP:
						bounds = DrawObject_OnStop(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTMOVE:
						bounds = DrawObject_OnMove(indexX, indexY, ix, iy, false, dwTime, m_entityState.m_bSpriteOmit);
						break;

					case DEF_OBJECTDAMAGEMOVE:
						bounds = DrawObject_OnDamageMove(indexX, indexY, ix, iy, false, dwTime, m_entityState.m_bSpriteOmit);
						break;

					case DEF_OBJECTRUN:
						bounds = DrawObject_OnRun(indexX, indexY, ix, iy, false, dwTime, m_entityState.m_bSpriteOmit);
						break;

					case DEF_OBJECTATTACK:
						bounds = DrawObject_OnAttack(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTATTACKMOVE:
						bounds = DrawObject_OnAttackMove(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTMAGIC:
						bounds = DrawObject_OnMagic(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTGETITEM:
						bounds = DrawObject_OnGetItem(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTDAMAGE:
						bounds = DrawObject_OnDamage(indexX, indexY, ix, iy, false, dwTime);
						break;

					case DEF_OBJECTDYING:
						bounds = DrawObject_OnDying(indexX, indexY, ix, iy, false, dwTime);
						break;
					}

					// Build picking info for living object
					TargetObjectInfo info = {};
					info.objectID = m_entityState.m_wObjectID;
					info.mapX = indexX;
					info.mapY = indexY;
					info.screenX = ix;
					info.screenY = iy;
					info.dataX = m_entityState.m_iDataX;
					info.dataY = m_entityState.m_iDataY;
					info.ownerType = m_entityState.m_sOwnerType;
					info.action = m_entityState.m_iAction;
					info.direction = m_entityState.m_iDir;
					info.frame = m_entityState.m_iFrame;
					info.name = m_entityState.m_cName.data();
					info.appr1 = m_entityState.m_sAppr1;
					info.appr2 = m_entityState.m_sAppr2;
					info.appr3 = m_entityState.m_sAppr3;
					info.appr4 = m_entityState.m_sAppr4;
					info.apprColor = m_entityState.m_iApprColor;
					info.status = m_entityState.m_iStatus;
					// Determine type based on owner type
					info.type = (m_entityState.m_sOwnerType >= 1 && m_entityState.m_sOwnerType <= 6) ?
						FocusedObjectType::Player : FocusedObjectType::NPC;
					CursorTarget::TestObject(bounds, info, iy, res_msy);

					if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0)
					{
						if (m_bIsObserverMode == false)
						{
							m_Camera.SetDestination((indexX - VIEW_CENTER_TILE_X) * 32, (indexY - (VIEW_CENTER_TILE_Y + 1)) * 32);
						}
						SetRect(&m_rcPlayerRect, m_rcBodyRect.left, m_rcBodyRect.top, m_rcBodyRect.right, m_rcBodyRect.bottom);
						bIsPlayerDrawed = true;
					}
				}
			}

			// CLEROTH - Object sprites on tiles
			// Bounds check for tile array access (752x752)
			if (indexX >= 0 && indexX < 752 && indexY >= 0 && indexY < 752)
			{
				sObjSpr = m_pMapData->m_tile[indexX][indexY].m_sObjectSprite;
				sObjSprFrame = m_pMapData->m_tile[indexX][indexY].m_sObjectSpriteFrame;
			}
			else
			{
				sObjSpr = 0;
				sObjSprFrame = 0;
			}

			if (sObjSpr != 0)
			{
				if ((sObjSpr < 100) || (sObjSpr >= 200))
				{
					switch (sObjSpr) {
					case 200:
					case 223:
						m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame, SpriteLib::DrawParams::Shadow());
						break;

					case 224:
						switch (sObjSprFrame) {
						case 24:
						case 34:
						case 35:
						case 36:
						case 37:
						case 38:
							break;
						default:
							m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame, SpriteLib::DrawParams::Shadow());
							break;
						}
					}
					if (ConfigManager::Get().GetDetailLevel() == 0) // Special Grass & Flowers
					{
						if ((sObjSpr != 6) && (sObjSpr != 9))
							m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame);
					}
					else
					{
						m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame);
					}

					switch (sObjSpr) {
					case 223:
						if (sObjSprFrame == 4)
						{
							if (G_cSpriteAlphaDegree == 2) //nuit
							{
								// Configurable ground lighting parameters
								int baseX = ix;
								int baseY = iy;
								constexpr int LIGHT_RADIUS = 4;           // Radius in tiles
								constexpr float CENTER_INTENSITY = 0.4f;  // Intensity at center (0.0 - 1.0)
								constexpr float EDGE_INTENSITY = 0.1f;    // Intensity at edge (0.0 - 1.0)
								constexpr int TILE_SIZE = 32;             // Pixels per tile

								// Draw ground lights in a circular pattern with distance-based falloff
								for (int ty = -LIGHT_RADIUS; ty <= LIGHT_RADIUS; ty++) {
									for (int tx = -LIGHT_RADIUS; tx <= LIGHT_RADIUS; tx++) {
										// Calculate distance from center (in tiles)
										float distance = sqrtf(static_cast<float>(tx * tx + ty * ty));

										// Skip if outside radius
										if (distance > LIGHT_RADIUS) continue;

										// Calculate intensity based on distance (linear falloff)
										float t = distance / LIGHT_RADIUS;  // 0 at center, 1 at edge
										float intensity = CENTER_INTENSITY * (1.0f - t) + EDGE_INTENSITY * t;

										// Skip very dim lights for performance
										if (intensity < 0.05f) continue;

										// Calculate screen position (isometric offset)
										int lightX = baseX + tx * TILE_SIZE;
										int lightY = baseY + ty * (TILE_SIZE / 2);  // Half for isometric

										// Warm color that gets slightly cooler at edges
										int r = 255;
										int g = static_cast<int>(200 + 30 * (1.0f - t));  // 230 at center, 200 at edge
										int b = static_cast<int>(150 + 30 * (1.0f - t));  // 180 at center, 150 at edge

										m_pEffectSpr[0]->Draw(lightX, lightY, 1,
											SpriteLib::DrawParams::AdditiveColored(r, g, b, intensity));
									}
								}

								// Lamp fixture lights (the actual light sources on the lamp)
								m_pEffectSpr[0]->Draw(ix + 2, iy - 147, 1, SpriteLib::DrawParams::AdditiveColored(255, 230, 180, 0.8f));
								m_pEffectSpr[0]->Draw(ix + 16, iy - 94, 1, SpriteLib::DrawParams::AdditiveColored(255, 230, 180, 0.8f));
								m_pEffectSpr[0]->Draw(ix - 19, iy - 126, 1, SpriteLib::DrawParams::AdditiveColored(255, 230, 180, 0.8f));
							}
						}
						break;

					case 370: // nuit
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 9) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 16 + 30, m_Camera.GetY() + iy - 16 - 334, 0, 0, 0, 0);
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 11) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 16 + 17, m_Camera.GetY() + iy - 16 - 300, 0, 0, 0, 0);
						break;

					case 374: // nuit
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 2) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 7, m_Camera.GetY() + iy - 122, 0, 0, 0, 0);
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 6) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 14, m_Camera.GetY() + iy - 321, 0, 0, 0, 0);
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 7) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix + 7, m_Camera.GetY() + iy - 356, 0, 0, 0, 0);
						break;

					case 376: // nuit
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 12) && (G_cSpriteAlphaDegree == 2)) {
							m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 16, m_Camera.GetY() + iy - 346, 0, 0, 0, 0);
							m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix + 11, m_Camera.GetY() + iy - 308, 0, 0, 0, 0);
						}
						break;

					case 378: // nuit
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 11) && (G_cSpriteAlphaDegree == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix, m_Camera.GetY() + iy - 91, 0, 0, 0, 0);
						break;

					case 382: // nuit
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 9) && (G_cSpriteAlphaDegree == 2)) {
							m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix + 73, m_Camera.GetY() + iy - 264, 0, 0, 0, 0);
							m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix + 23, m_Camera.GetY() + iy - 228, 0, 0, 0, 0);
						}
						break;

					case 429:
						if (((dwTime - m_dwEnvEffectTime) > 400) && (sObjSprFrame == 2)) m_pEffectManager->AddEffect(EffectType::MS_CRUSADE_CASTING, m_Camera.GetX() + ix - 15, m_Camera.GetY() + iy - 224, 0, 0, 0, 0);
						break;
					}
				}
				else // sprites 100..199: Trees and tree shadows
				{
					m_pTileSpr[sObjSpr]->CalculateBounds(ix - 16, iy - 16, sObjSprFrame);
					if (ConfigManager::Get().GetDetailLevel() == 0)
					{
						if (sObjSpr < 100 + 11) m_pTileSpr[100 + 4]->Draw(ix - 16, iy - 16, sObjSprFrame);
						else if (sObjSpr < 100 + 23) m_pTileSpr[100 + 9]->Draw(ix - 16, iy - 16, sObjSprFrame);
						else if (sObjSpr < 100 + 32) m_pTileSpr[100 + 23]->Draw(ix - 16, iy - 16, sObjSprFrame);
						else m_pTileSpr[100 + 32]->Draw(ix - 16, iy - 16, sObjSprFrame);
					}
					else
					{
						if ((bIsPlayerDrawed == true) && (m_pTileSpr[sObjSpr]->GetBoundRect().top <= m_rcPlayerRect.top) && (m_pTileSpr[sObjSpr]->GetBoundRect().bottom >= m_rcPlayerRect.bottom) &&
							(ConfigManager::Get().GetDetailLevel() >= 2) && (m_pTileSpr[sObjSpr]->GetBoundRect().left <= m_rcPlayerRect.left) && (m_pTileSpr[sObjSpr]->GetBoundRect().right >= m_rcPlayerRect.right))
						{
							m_pTileSpr[sObjSpr + 50]->Draw(ix, iy, sObjSprFrame, SpriteLib::DrawParams::Fade());
							m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame, SpriteLib::DrawParams::Average());
						}
						else
						{
							// Normal rendering - draw shadow and tree opaque (matches original PutSpriteFast)
							m_pTileSpr[sObjSpr + 50]->Draw(ix, iy, sObjSprFrame);
							m_pTileSpr[sObjSpr]->Draw(ix - 16, iy - 16, sObjSprFrame);
						}
						if (m_bIsXmas == true)
						{
							if (G_cSpriteAlphaDegree == 2) // nuit
							{
								if (iXmasTreeBulbDelay < 0 || iXmasTreeBulbDelay > idelay + 1) iXmasTreeBulbDelay = 0;
								if (iXmasTreeBulbDelay > idelay)
								{
									for (int i = 0; i < 100; i++) {
										ix1[i] = 1 * (rand() % 400) - 200;
										iy2[i] = -1 * (rand() % 300);
									}
									iXmasTreeBulbDelay = 0;
								}
								else iXmasTreeBulbDelay++;

								for (int j = 0; j < 100; j++)
								{
									if (m_pTileSpr[sObjSpr]->CheckCollision(ix - 16, iy - 16, sObjSprFrame, ix + ix1[j], iy + iy2[j]))
									{
										m_pEffectSpr[66 + (j % 6)]->Draw(ix + ix1[j], iy + iy2[j], (iXmasTreeBulbDelay >> 2), SpriteLib::DrawParams::Alpha(0.5f));
									}
								}
							}
						}
					}
				}
			}

			// Dynamic Object
			if ((bRet == true) && (sDynamicObject != 0))
			{
				switch (sDynamicObject) {
				case DEF_DYNAMICOBJECT_PCLOUD_BEGIN:	// 10
					if (sDynamicObjectFrame >= 0)
						m_pEffectSpr[23]->Draw(ix + (rand() % 2), iy + (rand() % 2), sDynamicObjectFrame, SpriteLib::DrawParams{0.5f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_PCLOUD_LOOP:		// 11
					m_pEffectSpr[23]->Draw(ix + (rand() % 2), iy + (rand() % 2), sDynamicObjectFrame + 8, SpriteLib::DrawParams{0.5f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_PCLOUD_END:		// 12
					m_pEffectSpr[23]->Draw(ix + (rand() % 2), iy + (rand() % 2), sDynamicObjectFrame + 16, SpriteLib::DrawParams{0.5f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_ICESTORM:		// 8
					iDvalue = (rand() % 5) * (-1);
					m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams::TintedAlpha(iDvalue, iDvalue, iDvalue, 0.7f));
					m_pEffectSpr[13]->Draw(ix, iy, sDynamicObjectFrame, SpriteLib::DrawParams{0.7f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_FIRE:			// 1
				case DEF_DYNAMICOBJECT_FIRE3:			// 14
					switch (rand() % 3) {
					case 0: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.25f, 0, 0, 0, false}); break;
					case 1: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.5f, 0, 0, 0, false}); break;
					case 2: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.7f, 0, 0, 0, false}); break;
					}
					m_pEffectSpr[9]->Draw(ix, iy, sDynamicObjectFrame / 3, SpriteLib::DrawParams{0.7f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_FIRE2:			// 13
					switch (rand() % 3) {
					case 0: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.25f, 0, 0, 0, false}); break;
					case 1: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.5f, 0, 0, 0, false}); break;
					case 2: m_pEffectSpr[0]->Draw(ix, iy, 1, SpriteLib::DrawParams{0.7f, 0, 0, 0, false}); break;
					}
					break;

				case DEF_DYNAMICOBJECT_FISH:			// 2
				{
					char cTmpDOdir, cTmpDOframe;
					cTmpDOdir = CMisc::cCalcDirection(cDynamicObjectData1, cDynamicObjectData2, cDynamicObjectData1 + cDynamicObjectData3, cDynamicObjectData2 + cDynamicObjectData4);
					cTmpDOframe = ((cTmpDOdir - 1) * 4) + (rand() % 4);
					m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 0]->Draw(ix + cDynamicObjectData1, iy + cDynamicObjectData2, cTmpDOframe, SpriteLib::DrawParams::Alpha(0.25f));
				}
				break;

				case DEF_DYNAMICOBJECT_MINERAL1:		// 4
					if (ConfigManager::Get().GetDetailLevel() != 0) m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->Draw(ix, iy, 0, SpriteLib::DrawParams::Shadow());
					m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->Draw(ix, iy, 0);
					CursorTarget::TestDynamicObject(m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->GetBoundRect(), indexX, indexY, res_msy);
					break;

				case DEF_DYNAMICOBJECT_MINERAL2:		// 5
					if (ConfigManager::Get().GetDetailLevel() != 0) m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->Draw(ix, iy, 1, SpriteLib::DrawParams::Shadow());
					m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->Draw(ix, iy, 1);
					CursorTarget::TestDynamicObject(m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 1]->GetBoundRect(), indexX, indexY, res_msy);
					break;

				case DEF_DYNAMICOBJECT_SPIKE:			// 9
					m_pEffectSpr[17]->Draw(ix, iy, sDynamicObjectFrame, SpriteLib::DrawParams{0.7f, 0, 0, 0, false});
					break;

				case DEF_DYNAMICOBJECT_ARESDENFLAG1:  // 6
					m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 2]->Draw(ix, iy, sDynamicObjectFrame);
					break;

				case DEF_DYNAMICOBJECT_ELVINEFLAG1: // 7
					m_pSprite[DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT + 2]->Draw(ix, iy, sDynamicObjectFrame);
					break;
				}
			}
		}
	}

	if ((dwTime - m_dwEnvEffectTime) > 400) m_dwEnvEffectTime = dwTime;

	// Finalize Picking system - determines cursor type
	int foeResult = CursorTarget::HasFocusedObject() ? _iGetFOE(CursorTarget::GetFocusStatus()) : 0;
	CursorTarget::EndFrame(foeResult, m_iPointCommandType, m_pPlayer->m_Controller.IsCommandAvailable(), m_bIsGetPointingMode);

	// Update legacy compatibility variables from Picking system
	m_sMCX = CursorTarget::GetFocusedMapX();
	m_sMCY = CursorTarget::GetFocusedMapY();
	std::memset(m_cMCName, 0, sizeof(m_cMCName));
	std::strncpy(m_cMCName, CursorTarget::GetFocusedName(), sizeof(m_cMCName) - 1);

	// Draw focused object with highlight (transparency)
	if (CursorTarget::HasFocusedObject())
	{
		short focusSX, focusSY;
		uint16_t focusObjID;
		short focusOwnerType;
		char focusAction, focusDir, focusFrame;
		short focusAppr1, focusAppr2, focusAppr3, focusAppr4;
		int focusApprColor, focusStatus;
		short focusDataX, focusDataY;

		if (CursorTarget::GetFocusHighlightData(focusSX, focusSY, focusObjID, focusOwnerType,
			focusAction, focusDir, focusFrame, focusAppr1, focusAppr2, focusAppr3, focusAppr4,
			focusApprColor, focusStatus, focusDataX, focusDataY))
		{
			// Set up temporary vars for drawing
			m_entityState.m_wObjectID = focusObjID;
			m_entityState.m_sOwnerType = focusOwnerType;
			m_entityState.m_iAction = focusAction;
			m_entityState.m_iFrame = focusFrame;
			m_entityState.m_iDir = focusDir;
			m_entityState.m_sAppr1 = focusAppr1;
			m_entityState.m_sAppr2 = focusAppr2;
			m_entityState.m_sAppr3 = focusAppr3;
			m_entityState.m_sAppr4 = focusAppr4;
			m_entityState.m_iApprColor = focusApprColor;
			m_entityState.m_iStatus = focusStatus;
			m_entityState.m_iDataX = focusDataX;
			m_entityState.m_iDataY = focusDataY;
			std::memset(m_entityState.m_cName.data(), 0, m_entityState.m_cName.size());
			std::strncpy(m_entityState.m_cName.data(), CursorTarget::GetFocusedName(), m_entityState.m_cName.size() - 1);

			if ((focusAction != DEF_OBJECTDEAD) && (focusFrame < 0)) {
				// Skip drawing invalid frame
			} else {
				switch (focusAction) {
				case DEF_OBJECTSTOP:
					DrawObject_OnStop(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;
				case DEF_OBJECTMOVE:
					switch (focusOwnerType) {
					case 1:
					case 2:
					case 3: // Human M
					case 4:
					case 5:
					case 6: // Human F

					case 28: // Troll.
					case 29: // Ogre
					case 30: // Liche
					case 31: // DD
					case 32: // Uni
					case 33: // WW
					case 43: // LWB
					case 44: // GHK
					case 45: // GHKABS
					case 46: // TK
					case 47: // BG
					case 48: // SK
					case 49: // HC
					case 50: // TW
					case 51: // CP
					case 52: // GG
					case 53: // BB
					case 54: // DE
					case 55: // Rabbit
					case 56: // Cat
					case 57: // Frog
					case 58: // MG
					case 59: // Ettin
					case 60: // Plant
					case 61: // Rudolph
					case 62: // DireBoar
					case 63: // Frost
					case 65: // Ice-Golem
					case 66: // Wyvern
					case 70: // Dragon..........Ajouts par Snoopy
					case 71: // Centaur
					case 72: // ClawTurtle
					case 73: // FireWyvern
					case 74: // GiantCrayfish
					case 75: // Gi Lizard
					case 76: // Gi Tree
					case 77: // Master Orc
					case 78: // Minaus
					case 79: // Nizie
					case 80: // Tentocle
					case 81: // Abaddon
					case 82: // Sorceress
					case 83: // ATK
					case 84: // MasterElf
					case 85: // DSK
					case 86: // HBT
					case 87: // CT
					case 88: // Barbarian
					case 89: // AGC
					case 91: // Gate
						break;

					default: // 10..27
						m_entityState.m_iFrame = m_entityState.m_iFrame * 2;
						break;
					}

					DrawObject_OnMove(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime, frame_omit);
					break;

				case DEF_OBJECTDAMAGEMOVE:
					DrawObject_OnDamageMove(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime, frame_omit);
					break;

				case DEF_OBJECTRUN:
					DrawObject_OnRun(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime, frame_omit);
					break;

				case DEF_OBJECTATTACK:
					DrawObject_OnAttack(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;

				case DEF_OBJECTATTACKMOVE:
					DrawObject_OnAttackMove(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;

				case DEF_OBJECTMAGIC:
					DrawObject_OnMagic(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;

				case DEF_OBJECTDAMAGE:
					DrawObject_OnDamage(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;

				case DEF_OBJECTDYING:
					DrawObject_OnDying(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;

				case DEF_OBJECTDEAD:
					DrawObject_OnDead(m_sMCX, m_sMCY, focusSX, focusSY, true, dwTime);
					break;
				}
			}
		}
	}

	if (sItemSelectedID != -1) {
		char cStr1[64], cStr2[64], cStr3[64];
		int  iLoc;
		GetItemName(m_pItemConfigList[sItemSelectedID].get(), cStr1, cStr2, cStr3);

		iLoc = 0;
		if (strlen(cStr1) != 0)
		{
			if (m_bIsSpecial)
				TextLib::DrawText(GameFont::Default, msX, msY + 25, cStr1, TextLib::TextStyle::WithShadow(GameColors::NameSpecial.r, GameColors::NameSpecial.g, GameColors::NameSpecial.b));
			else
				TextLib::DrawText(GameFont::Default, msX, msY + 25, cStr1, TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
			iLoc += 15;
		}
		if (strlen(cStr2) != 0)
		{
			TextLib::DrawText(GameFont::Default, msX, msY + 25 + iLoc, cStr2, TextLib::TextStyle::WithShadow(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
			iLoc += 15;
		}
		if (strlen(cStr3) != 0)
		{
			TextLib::DrawText(GameFont::Default, msX, msY + 25 + iLoc, cStr3, TextLib::TextStyle::WithShadow(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
			iLoc += 15;
		}
	}
}

bool CGame::_bDecodeItemConfigFileContents(char* pData, uint32_t dwMsgSize)
{
	// Parse binary item config packet
	constexpr size_t headerSize = sizeof(hb::net::PacketItemConfigHeader);
	constexpr size_t entrySize = sizeof(hb::net::PacketItemConfigEntry);

	if (dwMsgSize < headerSize) {
		printf("[ERROR] Item config packet too small for header\n");
		return false;
	}

	const auto* pktHeader = reinterpret_cast<const hb::net::PacketItemConfigHeader*>(pData);
	uint16_t itemCount = pktHeader->itemCount;

	if (dwMsgSize < headerSize + (itemCount * entrySize)) {
		return false;
	}

	const auto* entries = reinterpret_cast<const hb::net::PacketItemConfigEntry*>(pData + headerSize);

	for (uint16_t i = 0; i < itemCount; i++) {
		const auto& entry = entries[i];
		int itemId = entry.itemId;

		if (itemId <= 0 || itemId >= 5000) {
			continue;
		}

		// Delete existing item if present (shouldn't happen, but be safe)
		if (m_pItemConfigList[itemId] != 0) {
			m_pItemConfigList[itemId].reset();
		}

		m_pItemConfigList[itemId] = std::make_unique<CItem>();
		CItem* pItem = m_pItemConfigList[itemId].get();

		pItem->m_sIDnum = entry.itemId;
		std::memset(pItem->m_cName, 0, sizeof(pItem->m_cName));
		std::strncpy(pItem->m_cName, entry.name, sizeof(pItem->m_cName) - 1);
		pItem->m_cItemType = entry.itemType;
		pItem->m_cEquipPos = entry.equipPos;
		pItem->m_sItemEffectType = entry.effectType;
		pItem->m_sItemEffectValue1 = entry.effectValue1;
		pItem->m_sItemEffectValue2 = entry.effectValue2;
		pItem->m_sItemEffectValue3 = entry.effectValue3;
		pItem->m_sItemEffectValue4 = entry.effectValue4;
		pItem->m_sItemEffectValue5 = entry.effectValue5;
		pItem->m_sItemEffectValue6 = entry.effectValue6;
		pItem->m_wMaxLifeSpan = entry.maxLifeSpan;
		pItem->m_sSpecialEffect = entry.specialEffect;
		pItem->m_sSprite = entry.sprite;
		pItem->m_sSpriteFrame = entry.spriteFrame;
		pItem->m_bIsForSale = (entry.price >= 0);
		pItem->m_wPrice = static_cast<uint32_t>(entry.price >= 0 ? entry.price : -entry.price);
		pItem->m_wWeight = entry.weight;
		pItem->m_cApprValue = entry.apprValue;
		pItem->m_cSpeed = entry.speed;
		pItem->m_sLevelLimit = entry.levelLimit;
		pItem->m_cGenderLimit = entry.genderLimit;
		pItem->m_sSpecialEffectValue1 = entry.specialEffectValue1;
		pItem->m_sSpecialEffectValue2 = entry.specialEffectValue2;
		pItem->m_sRelatedSkill = entry.relatedSkill;
		pItem->m_cCategory = entry.category;
		pItem->m_cItemColor = entry.itemColor;
	}

	return true;
}

void CGame::GameRecvMsgHandler(uint32_t dwMsgSize, char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	m_dwLastNetMsgId = header->msg_id;
	m_dwLastNetMsgTime = GameClock::GetTimeMS();
	m_dwLastNetMsgSize = dwMsgSize;
	switch (header->msg_id) {
	case MSGID_ITEMCONFIGURATIONCONTENTS:
		// Pass full pData - PacketItemConfigHeader includes the PacketHeader
		_bDecodeItemConfigFileContents(pData, dwMsgSize);
		break;
	case MSGID_RESPONSE_CHARGED_TELEPORT:
		ResponseChargedTeleport(pData);
		break;

	case MSGID_RESPONSE_TELEPORT_LIST:
		ResponseTeleportList(pData);
		break;

	case MSGID_RESPONSE_HELDENIAN_TP_LIST: // Snoopy Heldenian TP
		ResponseHeldenianTeleportList(pData);
		break;

	case MSGID_RESPONSE_NOTICEMENT:
		NoticementHandler(pData);
		break;

	case MSGID_DYNAMICOBJECT:
		DynamicObjectHandler(pData);
		break;

	case MSGID_RESPONSE_INITPLAYER:
		InitPlayerResponseHandler(pData);
		break;

	case MSGID_RESPONSE_INITDATA:
		InitDataResponseHandler(pData);
		break;

	case MSGID_RESPONSE_MOTION:
		MotionResponseHandler(pData);
		break;

	case MSGID_EVENT_COMMON:
		CommonEventHandler(pData);
		break;

	case MSGID_EVENT_MOTION:
		MotionEventHandler(pData);
		break;

	case MSGID_EVENT_LOG:
		LogEventHandler(pData);
		break;

	case MSGID_COMMAND_CHATMSG:
		ChatMsgHandler(pData);
		break;

	case MSGID_PLAYERITEMLISTCONTENTS:
		InitItemList(pData);
		break;

	case MSGID_NOTIFY:
		if (m_pNetworkMessageManager) {

			m_pNetworkMessageManager->ProcessMessage(MSGID_NOTIFY, pData, dwMsgSize);
		}
		break;

	case MSGID_RESPONSE_CREATENEWGUILD:
		CreateNewGuildResponseHandler(pData);
		break;

	case MSGID_RESPONSE_DISBANDGUILD:
		DisbandGuildResponseHandler(pData);
		break;

	case MSGID_PLAYERCHARACTERCONTENTS:
		InitPlayerCharacteristics(pData);
		break;

	case MSGID_RESPONSE_CIVILRIGHT:
		CivilRightAdmissionHandler(pData);
		break;

	case MSGID_RESPONSE_RETRIEVEITEM:
		RetrieveItemHandler(pData);
		break;

	case MSGID_RESPONSE_PANNING:
		ResponsePanningHandler(pData);
		break;

	case MSGID_RESPONSE_FIGHTZONE_RESERVE:
		ReserveFightzoneResponseHandler(pData);
		break;

	case MSGID_RESPONSE_SHOP_CONTENTS:
		ResponseShopContentsHandler(pData);
		break;

	case MSGID_COMMAND_CHECKCONNECTION:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandCheckConnection>(
			pData, sizeof(hb::net::PacketCommandCheckConnection));
		if (!pkt) return;
		uint32_t dwRecvTime = m_dwLastNetRecvTime;
		if (dwRecvTime == 0) dwRecvTime = GameClock::GetTimeMS();
		if (dwRecvTime >= pkt->time_ms)
		{
			m_iLatencyMs = static_cast<int>(dwRecvTime - pkt->time_ms);
		}
	}
	break;
	}
}

void CGame::ConnectionEstablishHandler(char cWhere)
{
	ChangeGameMode(GameMode::WaitingResponse);

	switch (cWhere) {
	case DEF_SERVERTYPE_GAME:
		bSendCommand(MSGID_REQUEST_INITPLAYER, 0, 0, 0, 0, 0, 0);
		break;

	case DEF_SERVERTYPE_LOG:
		switch (m_dwConnectMode) {
		case MSGID_REQUEST_LOGIN:
			if (strlen(m_cWorldServerName) == 0) {
				printf("[ERROR] Login failed - m_cWorldServerName is empty\n");
			}
			bSendCommand(MSGID_REQUEST_LOGIN, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_CREATENEWACCOUNT:
			bSendCommand(MSGID_REQUEST_CREATENEWACCOUNT, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_CREATENEWCHARACTER:
			bSendCommand(MSGID_REQUEST_CREATENEWCHARACTER, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_ENTERGAME:
			bSendCommand(MSGID_REQUEST_ENTERGAME, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_DELETECHARACTER:
			bSendCommand(MSGID_REQUEST_DELETECHARACTER, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_CHANGEPASSWORD:
			bSendCommand(MSGID_REQUEST_CHANGEPASSWORD, 0, 0, 0, 0, 0, 0);
			break;
		case MSGID_REQUEST_INPUTKEYCODE:
			bSendCommand(MSGID_REQUEST_INPUTKEYCODE, 0, 0, 0, 0, 0, 0);
			break;
		}
		break;
	}
}

void CGame::InitPlayerResponseHandler(char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case DEF_MSGTYPE_CONFIRM:
		bSendCommand(MSGID_REQUEST_INITDATA, 0, 0, 0, 0, 0, 0);
		ChangeGameMode(GameMode::WaitingInitData);
		break;

	case DEF_MSGTYPE_REJECT:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "3J");
		ChangeGameMode(GameMode::LogResMsg);
		break;
	}
}

void CGame::OnTimer()
{
	if (GameModeManager::GetModeValue() < 0) return;
	uint32_t dwTime = GameClock::GetTimeMS();

	if (GameModeManager::GetMode() != GameMode::Loading) {
		if ((dwTime - m_dwCheckSprTime) > 8000)
		{
			m_dwCheckSprTime = dwTime;
			if (m_bIsProgramActive) ReleaseUnusedSprites();
		}
		if ((dwTime - m_dwCheckConnectionTime) > 1000)
		{
			m_dwCheckConnectionTime = dwTime;
			if ((m_pGSock != 0) && (m_pGSock->m_bIsAvailable == true))
				bSendCommand(MSGID_COMMAND_CHECKCONNECTION, DEF_MSGTYPE_CONFIRM, 0, 0, 0, 0, 0);
		}
	}

	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		if ((dwTime - m_dwCheckConnTime) > 5000)
		{
			m_dwCheckConnTime = dwTime;
			if ((m_bIsCrusadeMode) && (m_pPlayer->m_iCrusadeDuty == 0)) m_dialogBoxManager.EnableDialogBox(DialogBoxId::CrusadeJob, 1, 0, 0);
		}

		if ((dwTime - m_dwCheckChatTime) > 2000)
		{
			m_dwCheckChatTime = m_dwTime;
			ReleaseTimeoverChatMsg();
			if (m_pPlayer->m_Controller.GetCommandCount() >= 6)
			{
				m_iNetLagCount++;
				if (m_iNetLagCount >= 7)
				{
					printf("[ERROR] NetLag threshold reached, disconnecting\n");
					ChangeGameMode(GameMode::ConnectionLost);
					m_pGSock.reset();
					m_pGSock.reset();
					return;
				}
			}
			else m_iNetLagCount = 0;
		}

		if ((G_bIsCalcSocketConnected == false) && ((dwTime - G_dwCalcSocketTime) > 5000))
		{
			m_pGSock.reset();
			m_pGSock.reset();
			ChangeGameMode(GameMode::Quit);
			PlaySound('E', 14, 5);
			AudioManager::Get().StopSound(SoundType::Effect, 38);
			AudioManager::Get().StopMusic();
			return;
		}

		if ((G_pCalcSocket != 0) && (G_bIsCalcSocketConnected == true)) {
			if ((dwTime - G_dwCalcSocketSendTime) > 1000 * 5) {
				hb::net::PacketCalcSocketPing ping{};
				ping.key = 0;
				ping.length = static_cast<std::uint16_t>(sizeof(ping));
				ping.reserved = 0;
				G_pCalcSocket->iSendMsgBlockingMode(reinterpret_cast<char*>(&ping), sizeof(ping));
				G_dwCalcSocketSendTime = dwTime;
			}
		}
	}
}

void CGame::_SetItemOrder(char cWhere, char cItemID)
{
	int i;

	switch (cWhere) {
	case 0:
		for (i = 0; i < DEF_MAXITEMS; i++)
			if (m_cItemOrder[i] == cItemID)
				m_cItemOrder[i] = -1;

		for (i = 1; i < DEF_MAXITEMS; i++)
			if ((m_cItemOrder[i - 1] == -1) && (m_cItemOrder[i] != -1)) {
				m_cItemOrder[i - 1] = m_cItemOrder[i];
				m_cItemOrder[i] = -1;
			}

		for (i = 0; i < DEF_MAXITEMS; i++)
			if (m_cItemOrder[i] == -1) {
				m_cItemOrder[i] = cItemID;
				return;
			}
		break;
	}
}

void CGame::bItemDrop_ExternalScreen(char cItemID, short msX, short msY)
{
	char  cName[21];
	short sType, tX, tY;
	int iStatus;

	if (bCheckItemOperationEnabled(cItemID) == false) return;

	if ((m_sMCX != 0) && (m_sMCY != 0) && (abs(m_pPlayer->m_sPlayerX - m_sMCX) <= 8) && (abs(m_pPlayer->m_sPlayerY - m_sMCY) <= 8))
	{
		std::memset(cName, 0, sizeof(cName));
		m_pMapData->bGetOwner(m_sMCX, m_sMCY, cName, &sType, &iStatus, &m_wCommObjectID);
		if (memcmp(m_pPlayer->m_cPlayerName, cName, 10) == 0)
		{
		}
		else
		{
			if (((m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_CONSUME) || (m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_ARROW))
				&& (m_pItemList[cItemID]->m_dwCount > 1))
			{
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sX = msX - 140;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY = msY - 70;
				if (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY < 0) m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY = 0;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1 = m_sMCX;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2 = m_sMCY;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3 = sType;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4 = m_wCommObjectID;
				std::memset(m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr));
				if (sType < 10)
					memcpy(m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr, cName, 10);
				else
				{
					GetNpcName(sType, m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr);
				}
				m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropExternal, cItemID, m_pItemList[cItemID]->m_dwCount, 0);
			}
			else
			{
				switch (sType) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
					m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 1, cItemID, sType);
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 1;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_wCommObjectID;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV5 = m_sMCX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV6 = m_sMCY;

					tX = msX - 117;
					tY = msY - 50;
					if (tX < 0) tX = 0;
					if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
					if (tY < 0) tY = 0;
					if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;

					std::memset(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
					strcpy(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, cName);
					//bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, cItemID, 1, m_sMCX, m_sMCY, m_pItemList[cItemID]->m_cName); //v1.4
					break;

				case 20: // Howard
					m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 3, cItemID, sType);
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 1;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_wCommObjectID; // v1.4
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV5 = m_sMCX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV6 = m_sMCY;

					tX = msX - 117;
					tY = msY - 50;
					if (tX < 0) tX = 0;
					if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
					if (tY < 0) tY = 0;
					if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;

					std::memset(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
					GetNpcName(sType, m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr);
					//bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, cItemID, 1, m_sMCX, m_sMCY, m_pItemList[cItemID]->m_cName);
					break;

				case 15: // ShopKeeper-W
				case 24: // Tom
					m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 2, cItemID, sType);
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 1;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV4 = m_wCommObjectID; // v1.4
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV5 = m_sMCX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV6 = m_sMCY;

					tX = msX - 117;
					tY = msY - 50;
					if (tX < 0) tX = 0;
					if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
					if (tY < 0) tY = 0;
					if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
					m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;

					std::memset(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr));
					GetNpcName(sType, m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cStr);
					break;

				default:
					bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GIVEITEMTOCHAR, cItemID, 1, m_sMCX, m_sMCY, m_pItemList[cItemID]->m_cName);
					break;
				}
				//m_bIsItemDisabled[cItemID] = true;
			}
			m_bIsItemDisabled[cItemID] = true;
		}
	}
	else
	{
		if (((m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_CONSUME) || (m_pItemList[cItemID]->m_cItemType == DEF_ITEMTYPE_ARROW))
			&& (m_pItemList[cItemID]->m_dwCount > 1))
		{
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sX = msX - 140;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY = msY - 70;
			if (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY < 0)		m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY = 0;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV1 = 0;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV2 = 0;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV3 = 0;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sV4 = 0;
			std::memset(m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cStr));
			m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropExternal, cItemID, m_pItemList[cItemID]->m_dwCount, 0);
		}
		else
		{
			if (_ItemDropHistory(m_pItemList[cItemID]->m_cName))
			{
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sX = msX - 140;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sY = msY - 70;
				if (m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sY < 0)	m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sY = 0;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sV1 = 0;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sV2 = 0;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sV3 = 1;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sV4 = 0;
				m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sV5 = cItemID;
				std::memset(m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).cStr));
				m_dialogBoxManager.EnableDialogBox(DialogBoxId::ItemDropConfirm, cItemID, m_pItemList[cItemID]->m_dwCount, 0);
			}
			else
			{
				bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_ITEMDROP, 0, cItemID, 1, 0, m_pItemList[cItemID]->m_cName);
			}
		}
		m_bIsItemDisabled[cItemID] = true;
	}
}

void CGame::CommonEventHandler(char* pData)
{
	WORD wEventType;
	short sX, sY, sV1, sV2, sV3, sV4;
	uint32_t dwV4;

	const auto* base = hb::net::PacketCast<hb::net::PacketEventCommonBase>(pData, sizeof(hb::net::PacketEventCommonBase));
	if (!base) return;
	wEventType = base->header.msg_type;
	sX = base->x;
	sY = base->y;
	sV1 = base->v1;
	sV2 = base->v2;
	sV3 = base->v3;

	switch (wEventType) {
	case DEF_COMMONTYPE_ITEMDROP:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketEventCommonItem>(pData, sizeof(hb::net::PacketEventCommonItem));
		if (!pkt) return;
		dwV4 = pkt->v4;
	}
	if ((sV1 == 6) && (sV2 == 0)) {
		m_pEffectManager->AddEffect(EffectType::GOLD_DROP, sX, sY, 0, 0, 0);
	}
	m_pMapData->bSetItem(sX, sY, sV1, (char)sV3, dwV4);
	break;

	case DEF_COMMONTYPE_SETITEM:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketEventCommonItem>(pData, sizeof(hb::net::PacketEventCommonItem));
		if (!pkt) return;
		dwV4 = pkt->v4;
	}
	m_pMapData->bSetItem(sX, sY, sV1, (char)sV3, dwV4, false); // v1.4 color
	break;

	case DEF_COMMONTYPE_MAGIC:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketEventCommonMagic>(pData, sizeof(hb::net::PacketEventCommonMagic));
		if (!pkt) return;
		sV4 = pkt->v4;
	}
	m_pEffectManager->AddEffect(static_cast<EffectType>(sV3), sX, sY, sV1, sV2, 0, sV4);
	break;

	case DEF_COMMONTYPE_CLEARGUILDNAME:
		ClearGuildNameList();
		break;
	}
}

void CGame::ClearGuildNameList()
{
	for (int i = 0; i < DEF_MAXGUILDNAMES; i++) {
		m_stGuildName[i].dwRefTime = 0;
		m_stGuildName[i].iGuildRank = -1;
		std::memset(m_stGuildName[i].cCharName, 0, sizeof(m_stGuildName[i].cCharName));
		std::memset(m_stGuildName[i].cGuildName, 0, sizeof(m_stGuildName[i].cGuildName));
	}
}

void CGame::InitGameSettings()
{
	int i;

	m_pPlayer->m_bForceAttack = false;
	m_pPlayer->m_Controller.SetCommandTime(0);
	m_dwCheckConnectionTime = 0;

	m_bInputStatus = false;
	m_pInputBuffer = 0;

	m_Camera.SetShake(0);

	m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
	m_pPlayer->m_Controller.ResetCommandCount();

	m_bIsGetPointingMode = false;
	m_iPointCommandType = -1; //v2.15 0 -> -1

	m_pPlayer->m_bIsCombatMode = false;

	// Previous cursor status tracking removed
	CursorTarget::ResetSelectionClickTime();

	m_bSkillUsingStatus = false;
	m_bItemUsingStatus = false;
	m_bUsingSlate = false;

	m_bIsWhetherEffect = false;
	m_cWhetherEffectType = 0;

	m_iDownSkillIndex = -1;
	m_dialogBoxManager.Info(DialogBoxId::Skill).bFlag = false;

	m_pPlayer->m_bIsConfusion = false;

	m_iIlusionOwnerH = 0;
	m_cIlusionOwnerType = 0;

	m_iDrawFlag = 0;
	m_bDrawFlagDir = false;
	m_bIsCrusadeMode = false;
	m_pPlayer->m_iCrusadeDuty = 0;

	m_iNetLagCount = 0;
	m_iLatencyMs = -1;
	m_dwLastNetMsgId = 0;
	m_dwLastNetMsgTime = 0;
	m_dwLastNetMsgSize = 0;
	m_dwLastNetRecvTime = 0;
	m_dwLastNpcEventTime = 0;

	m_dwEnvEffectTime = GameClock::GetTimeMS();

	for (i = 0; i < DEF_MAXGUILDNAMES; i++) {
		m_stGuildName[i].dwRefTime = 0;
		m_stGuildName[i].iGuildRank = -1;
		std::memset(m_stGuildName[i].cCharName, 0, sizeof(m_stGuildName[i].cCharName));
		std::memset(m_stGuildName[i].cGuildName, 0, sizeof(m_stGuildName[i].cGuildName));
	}
	//Snoopy: 61
	for (i = 0; i < 61; i++)
		m_dialogBoxManager.SetEnabled(i, false);

	//Snoopy: 58 because 2 last ones alreaddy defined
	for (i = 0; i < 58; i++)
		m_dialogBoxManager.SetOrderAt(i, 0);
	m_dialogBoxManager.SetOrderAt(60, DialogBoxId::HudPanel);
	m_dialogBoxManager.SetOrderAt(59, DialogBoxId::HudPanel);
	m_dialogBoxManager.SetEnabled(DialogBoxId::HudPanel, true);
	m_dialogBoxManager.SetEnabled(DialogBoxId::HudPanel, true);

	if (m_pEffectManager) m_pEffectManager->ClearAllEffects();

	for (i = 0; i < DEF_MAXCHATMSGS; i++) {
		if (m_pChatMsgList[i] != 0) m_pChatMsgList[i].reset();
	}

	for (i = 0; i < DEF_MAXCHATSCROLLMSGS; i++) {
		if (m_pChatScrollList[i] != 0) m_pChatScrollList[i].reset();
	}

	for (i = 0; i < DEF_MAXWHISPERMSG; i++) {
		if (m_pWhisperMsg[i] != 0) m_pWhisperMsg[i].reset();
	}

	std::memset(m_cLocation, 0, sizeof(m_cLocation));

	std::memset(m_pPlayer->m_cGuildName, 0, sizeof(m_pPlayer->m_cGuildName));
	m_pPlayer->m_iGuildRank = -1;
	m_pPlayer->m_iTotalGuildsMan = 0;

	for (i = 0; i < 100; i++) {
		m_stGuildOpList[i].cOpMode = 0;
		std::memset(m_stGuildOpList[i].cName, 0, sizeof(m_stGuildOpList[i].cName));
	}

	for (i = 0; i < 6; i++) {
		std::memset(m_stEventHistory[i].cTxt, 0, sizeof(m_stEventHistory[i].cTxt));
		m_stEventHistory[i].dwTime = GameClock::GetTimeMS();

		std::memset(m_stEventHistory2[i].cTxt, 0, sizeof(m_stEventHistory2[i].cTxt));
		m_stEventHistory2[i].dwTime = GameClock::GetTimeMS();
	}

	for (i = 0; i < DEF_MAXMENUITEMS; i++) {
		if (m_pItemForSaleList[i] != 0) m_pItemForSaleList[i].reset();
	}

	for (i = 0; i < 41; i++) {
		m_dialogBoxManager.Info(i).bFlag = false;
		m_dialogBoxManager.Info(i).sView = 0;
		m_dialogBoxManager.Info(i).bIsScrollSelected = false;
	}

	for (i = 0; i < DEF_MAXITEMS; i++)
		if (m_pItemList[i] != 0) {
			m_pItemList[i].reset();
		}

	for (i = 0; i < DEF_MAXSELLLIST; i++) {
		m_stSellItemList[i].iIndex = -1;
		m_stSellItemList[i].iAmount = 0;
	}

	for (i = 0; i < DEF_MAXBANKITEMS; i++)
		if (m_pBankList[i] != 0) {
			m_pBankList[i].reset();
		}

	for (i = 0; i < DEF_MAXMAGICTYPE; i++)
		m_pPlayer->m_iMagicMastery[i] = 0;

	for (i = 0; i < DEF_MAXSKILLTYPE; i++)
		m_pPlayer->m_iSkillMastery[i] = 0;

	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++) {
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();

		if (m_pMsgTextList2[i] != 0)
			m_pMsgTextList2[i].reset();

		if (m_pAgreeMsgTextList[i] != 0)
			m_pAgreeMsgTextList[i].reset();
	}

	for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) {
		m_stPartyMember[i].cStatus = 0;
		std::memset(m_stPartyMember[i].cName, 0, sizeof(m_stPartyMember[i].cName));
	}

	m_pPlayer->m_iLU_Point = 0;
	m_pPlayer->m_wLU_Str = m_pPlayer->m_wLU_Vit = m_pPlayer->m_wLU_Dex = m_pPlayer->m_wLU_Int = m_pPlayer->m_wLU_Mag = m_pPlayer->m_wLU_Char = 0;
	m_cWhetherStatus = 0;
	m_cLogOutCount = -1;
	m_dwLogOutCountTime = 0;
	m_pPlayer->m_iSuperAttackLeft = 0;
	m_pPlayer->m_bSuperAttackMode = false;
	m_iFightzoneNumber = 0;
	std::memset(m_cBGMmapName, 0, sizeof(m_cBGMmapName));
	m_dwWOFtime = 0;
	m_stQuest.sWho = 0;
	m_stQuest.sQuestType = 0;
	m_stQuest.sContribution = 0;
	m_stQuest.sTargetType = 0;
	m_stQuest.sTargetCount = 0;
	m_stQuest.sCurrentCount = 0;
	m_stQuest.sX = 0;
	m_stQuest.sY = 0;
	m_stQuest.sRange = 0;
	m_stQuest.bIsQuestCompleted = false;
	std::memset(m_stQuest.cTargetName, 0, sizeof(m_stQuest.cTargetName));
	m_bIsObserverMode = false;
	m_bIsObserverCommanded = false;
	m_pPlayer->m_bIsPoisoned = false;
	m_pPlayer->m_Controller.SetPrevMoveBlocked(false);
	m_pPlayer->m_Controller.SetPrevMove(-1, -1);
	m_pPlayer->m_sDamageMove = 0;
	m_pPlayer->m_sDamageMoveAmount = 0;
	m_bForceDisconn = false;
	m_pPlayer->m_bIsSpecialAbilityEnabled = false;
	m_pPlayer->m_iSpecialAbilityType = 0;
	m_dwSpecialAbilitySettingTime = 0;
	m_pPlayer->m_iSpecialAbilityTimeLeftSec = 0;
	CursorTarget::ClearSelection();
	m_bIsF1HelpWindowEnabled = false;
	m_bIsTeleportRequested = false;
	for (i = 0; i < DEF_MAXCRUSADESTRUCTURES; i++)
	{
		m_stCrusadeStructureInfo[i].cType = 0;
		m_stCrusadeStructureInfo[i].cSide = 0;
		m_stCrusadeStructureInfo[i].sX = 0;
		m_stCrusadeStructureInfo[i].sY = 0;
	}
	std::memset(m_cStatusMapName, 0, sizeof(m_cStatusMapName));
	m_dwCommanderCommandRequestedTime = 0;
	std::memset(m_cTopMsg, 0, sizeof(m_cTopMsg));
	m_iTopMsgLastSec = 0;
	m_dwTopMsgTime = 0;
	m_pPlayer->m_iConstructionPoint = 0;
	m_pPlayer->m_iWarContribution = 0;
	std::memset(m_cTeleportMapName, 0, sizeof(m_cTeleportMapName));
	m_iTeleportLocX = m_iTeleportLocY = -1;
	std::memset(m_cConstructMapName, 0, sizeof(m_cConstructMapName));
	m_pPlayer->m_iConstructLocX = m_pPlayer->m_iConstructLocY = -1;

	//Snoopy: Apocalypse Gate
	std::memset(m_cGateMapName, 0, sizeof(m_cGateMapName));
	m_iGatePositX = m_iGatePositY = -1;
	m_iHeldenianAresdenLeftTower = -1;
	m_iHeldenianElvineLeftTower = -1;
	m_iHeldenianAresdenFlags = -1;
	m_iHeldenianElvineFlags = -1;
	m_bIsXmas = false;
	m_iTotalPartyMember = 0;
	m_iPartyStatus = 0;
	for (i = 0; i < DEF_MAXPARTYMEMBERS; i++) std::memset(m_stPartyMemberNameList[i].cName, 0, sizeof(m_stPartyMemberNameList[i].cName));
	m_iGizonItemUpgradeLeft = 0;
	cStateChange1 = 0;
	cStateChange2 = 0;
	cStateChange3 = 0;
	m_dialogBoxManager.EnableDialogBox(DialogBoxId::GuideMap, 0, 0, 0);
}

void CGame::_GetHairColorRGB(int iColorType, int* pR, int* pG, int* pB)
{
	switch (iColorType) {
	case 0: // rouge fonc�
		*pR = 14; *pG = -5; *pB = -5; break;
	case 1: // Orange
		*pR = 20; *pG = 0; *pB = 0; break;
	case 2: // marron tres clair
		*pR = 22; *pG = 13; *pB = -10; break;
	case 3: // vert
		*pR = 0; *pG = 10; *pB = 0; break;
	case 4: // Bleu flashy
		*pR = 0; *pG = 0; *pB = 22; break;
	case 5: // Bleu fonc�
		*pR = -5; *pG = -5; *pB = 15; break;
	case 6: //Mauve
		*pR = 15; *pG = -5; *pB = 16; break;
	case 7: // Noir
		*pR = -6; *pG = -6; *pB = -6; break;
	case 8:
		*pR = 10; *pG = 3; *pB = 10; break;
	case 9:
		*pR = 10; *pG = 3; *pB = -10; break;
	case 10:
		*pR = -10; *pG = 3; *pB = 10; break;
	case 11:
		*pR = 10; *pG = 3; *pB = 20; break;
	case 12:
		*pR = 21; *pG = 3; *pB = 3; break;
	case 13:
		*pR = 3; *pG = 3; *pB = 25; break;
	case 14:
		*pR = 3; *pG = 11; *pB = 3; break;
	case 15:
		*pR = 6; *pG = 8; *pB = 0; break;
	}
}

void CGame::CreateNewGuildResponseHandler(char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case DEF_MSGTYPE_CONFIRM:
		m_pPlayer->m_iGuildRank = 0;
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 3;
		break;
	case DEF_MSGTYPE_REJECT:
		m_pPlayer->m_iGuildRank = -1;
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 4;
		break;
	}
}

void CGame::InitPlayerCharacteristics(char* pData)
{
	// Snoopy: Angels
	m_pPlayer->m_iAngelicStr = 0;
	m_pPlayer->m_iAngelicDex = 0;
	m_pPlayer->m_iAngelicInt = 0;
	m_pPlayer->m_iAngelicMag = 0;

	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponsePlayerCharacterContents>(
		pData, sizeof(hb::net::PacketResponsePlayerCharacterContents));
	if (!pkt) return;

	m_pPlayer->m_iHP = pkt->hp;
	m_pPlayer->m_iMP = pkt->mp;
	m_pPlayer->m_iSP = pkt->sp;
	m_pPlayer->m_iAC = pkt->ac;		//? m_iDefenseRatio
	m_pPlayer->m_iTHAC0 = pkt->thac0;    //? m_iHitRatio
	m_pPlayer->m_iLevel = pkt->level;
	m_pPlayer->m_iStr = pkt->str;
	m_pPlayer->m_iInt = pkt->intel;
	m_pPlayer->m_iVit = pkt->vit;
	m_pPlayer->m_iDex = pkt->dex;
	m_pPlayer->m_iMag = pkt->mag;
	m_pPlayer->m_iCharisma = pkt->chr;

	// CLEROTH - LU
	m_pPlayer->m_iLU_Point = pkt->lu_point - 3;

	m_pPlayer->m_iExp = pkt->exp;
	m_pPlayer->m_iEnemyKillCount = pkt->enemy_kills;
	m_pPlayer->m_iPKCount = pkt->pk_count;
	m_pPlayer->m_iRewardGold = pkt->reward_gold;

	memcpy(m_cLocation, pkt->location, sizeof(pkt->location));
	if (memcmp(m_cLocation, "aresden", 7) == 0)
	{
		m_pPlayer->m_bAresden = true;
		m_pPlayer->m_bCitizen = true;
		m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(m_cLocation, "arehunter", 9) == 0)
	{
		m_pPlayer->m_bAresden = true;
		m_pPlayer->m_bCitizen = true;
		m_pPlayer->m_bHunter = true;
	}
	else if (memcmp(m_cLocation, "elvine", 6) == 0)
	{
		m_pPlayer->m_bAresden = false;
		m_pPlayer->m_bCitizen = true;
		m_pPlayer->m_bHunter = false;
	}
	else if (memcmp(m_cLocation, "elvhunter", 9) == 0)
	{
		m_pPlayer->m_bAresden = false;
		m_pPlayer->m_bCitizen = true;
		m_pPlayer->m_bHunter = true;
	}
	else
	{
		m_pPlayer->m_bAresden = true;
		m_pPlayer->m_bCitizen = false;
		m_pPlayer->m_bHunter = true;
	}

	memcpy(m_pPlayer->m_cGuildName, pkt->guild_name, sizeof(pkt->guild_name));

	if (strcmp(m_pPlayer->m_cGuildName, "NONE") == 0)
		std::memset(m_pPlayer->m_cGuildName, 0, sizeof(m_pPlayer->m_cGuildName));

	CMisc::ReplaceString(m_pPlayer->m_cGuildName, '_', ' ');
	m_pPlayer->m_iGuildRank = pkt->guild_rank;
	m_pPlayer->m_iSuperAttackLeft = pkt->super_attack_left;
	m_iFightzoneNumber = pkt->fightzone_number;
	iMaxStats = pkt->max_stats;
	iMaxLevel = pkt->max_level;
	iMaxBankItems = pkt->max_bank_items;
}

void CGame::DisbandGuildResponseHandler(char* pData)
{
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case DEF_MSGTYPE_CONFIRM:
		std::memset(m_pPlayer->m_cGuildName, 0, sizeof(m_pPlayer->m_cGuildName));
		m_pPlayer->m_iGuildRank = -1;
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 7;
		break;
	case DEF_MSGTYPE_REJECT:
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 8;
		break;
	}
}

void CGame::_PutGuildOperationList(char* pName, char cOpMode)
{
	int i;
	for (i = 0; i < 100; i++)
		if (m_stGuildOpList[i].cOpMode == 0)
		{
			m_stGuildOpList[i].cOpMode = cOpMode;
			std::memset(m_stGuildOpList[i].cName, 0, sizeof(m_stGuildOpList[i].cName));
			memcpy(m_stGuildOpList[i].cName, pName, 20);
			return;
		}
}

void CGame::_ShiftGuildOperationList()
{
	int i;
	std::memset(m_stGuildOpList[0].cName, 0, sizeof(m_stGuildOpList[0].cName));
	m_stGuildOpList[0].cOpMode = 0;

	for (i = 1; i < 100; i++)
		if ((m_stGuildOpList[i - 1].cOpMode == 0) && (m_stGuildOpList[i].cOpMode != 0)) {
			m_stGuildOpList[i - 1].cOpMode = m_stGuildOpList[i].cOpMode;
			std::memset(m_stGuildOpList[i - 1].cName, 0, sizeof(m_stGuildOpList[i - 1].cName));
			memcpy(m_stGuildOpList[i - 1].cName, m_stGuildOpList[i].cName, 20);

			std::memset(m_stGuildOpList[i].cName, 0, sizeof(m_stGuildOpList[i].cName));
			m_stGuildOpList[i].cOpMode = 0;
		}
}

void CGame::AddEventList(char* pTxt, char cColor, bool bDupAllow)
{
	int i;
	if ((bDupAllow == false) && (strcmp(m_stEventHistory[5].cTxt, pTxt) == 0)) return;
	if (cColor == 10)
	{
		for (i = 1; i < 6; i++)
		{
			strcpy(m_stEventHistory2[i - 1].cTxt, m_stEventHistory2[i].cTxt);
			m_stEventHistory2[i - 1].cColor = m_stEventHistory2[i].cColor;
			m_stEventHistory2[i - 1].dwTime = m_stEventHistory2[i].dwTime;
		}
		std::memset(m_stEventHistory2[5].cTxt, 0, sizeof(m_stEventHistory2[5].cTxt));
		strcpy(m_stEventHistory2[5].cTxt, pTxt);
		m_stEventHistory2[5].cColor = cColor;
		m_stEventHistory2[5].dwTime = m_dwCurTime;
	}
	else
	{
		for (i = 1; i < 6; i++)
		{
			strcpy(m_stEventHistory[i - 1].cTxt, m_stEventHistory[i].cTxt);
			m_stEventHistory[i - 1].cColor = m_stEventHistory[i].cColor;
			m_stEventHistory[i - 1].dwTime = m_stEventHistory[i].dwTime;
		}
		std::memset(m_stEventHistory[5].cTxt, 0, sizeof(m_stEventHistory[5].cTxt));
		strcpy(m_stEventHistory[5].cTxt, pTxt);
		m_stEventHistory[5].cColor = cColor;
		m_stEventHistory[5].dwTime = m_dwCurTime;
	}
}

int _iAttackerHeight[] = { 0, 35, 35,35,35,35,35, 0,0,0,
5,  // Slime
35, // Skeleton
40, // Stone-Golem
45, // Cyclops
35,// OrcMage
35,// ShopKeeper
5, // GiantAnt
8, // Scorpion
35,// Zombie
35,// Gandalf
35,// Howard
35,// Guard
10,// Amphis
38,// Clay-Golem
35,// Tom
35,// William
35,// Kennedy
35,// Hellhound
50,// Troll
45,// Orge
55,// Liche
65,// Demon
46,// Unicorn
49,// WereWolf
55,// Dummy
35,// Energysphere
75,// Arrow Guard Tower
75,// Cannon Guard Tower
50,// Mana Collector
50,// Detector
50,// Energy Shield Generator
50,// Grand Magic Generator
50,// ManaStone 42
40,// Light War Beetle
35,// GHK
40,// GHKABS
35,// TK
60,// BG
40,// Stalker
70,// HellClaw
85,// Tigerworm
50,// Catapult
85,// Gargoyle
70,// Beholder
40,// Dark-Elf
20,// Bunny
20,// Cat
40,// Giant-Frog
80,// Mountain-Giant
85,// Ettin
50,// Cannibal-Plant
50, // Rudolph 61 //Snoopy....
80, // Direboar 62
90, // Frost 63
40, // Crops 64
80, // IceGolem 65
190, // Wyvern 66
35, // npc 67
35, // npc 68
35, // npc 69
100, // Dragon 70
90, // Centaur 71
75, // ClawTurtle 72
200, // FireWyvern 73
80, // GiantCrayfish 74
120, // Gi Lizard 75
100, // Gi Tree 76
100, // Master Orc 77
80, // Minaus 78
100, // Nizie 79
25,  // Tentocle 80
200, // Abaddon	 81
60, // Sorceress 82
60, // ATK 83
70, // MasterElf 84
60, // DSK 85
50, // HBT 86
60, // CT 87
60, // Barbarian 88
60, // AGC 89
35, // ncp 90 Gail
35  // Gate 91
};

void CGame::_LoadShopMenuContents(char cType)
{
	// Request shop contents from server using NPC type
	_RequestShopContents(static_cast<int16_t>(cType));
}

void CGame::_RequestShopContents(int16_t npcType)
{
	// Clear existing shop items
	for (int i = 0; i < DEF_MAXSELLLIST; i++) {
		if (m_pItemForSaleList[i] != nullptr) {
			m_pItemForSaleList[i].reset();
			m_pItemForSaleList[i].reset();
		}
	}

	// Build and send shop request packet
	char cData[sizeof(hb::net::PacketShopRequest)];
	std::memset(cData, 0, sizeof(cData));

	auto* req = reinterpret_cast<hb::net::PacketShopRequest*>(cData);
	req->header.msg_id = MSGID_REQUEST_SHOP_CONTENTS;
	req->header.msg_type = DEF_MSGTYPE_CONFIRM;
	req->npcType = npcType;

	m_pGSock->iSendMsg(cData, sizeof(hb::net::PacketShopRequest));
}

void CGame::ResponseShopContentsHandler(char* pData)
{
	const auto* resp = hb::net::PacketCast<hb::net::PacketShopResponseHeader>(
		pData, sizeof(hb::net::PacketShopResponseHeader));
	if (!resp) {
		printf("[SHOP] Response: invalid packet\n");
		return;
	}

	uint16_t itemCount = resp->itemCount;
	printf("[SHOP] Response received: NPC type %d, shop %d, %d items\n",
		resp->npcType, resp->shopId, itemCount);

	// Debug: Count how many items are in the config list
	int configCount = 0;
	for (int i = 0; i < 5000; i++) {
		if (m_pItemConfigList[i] != nullptr) configCount++;
	}
	printf("[SHOP] Client has %d items in m_pItemConfigList\n", configCount);

	if (itemCount > DEF_MAXMENUITEMS) {
		itemCount = DEF_MAXMENUITEMS;
	}

	// Clear existing shop items
	for (int i = 0; i < DEF_MAXMENUITEMS; i++) {
		if (m_pItemForSaleList[i] != nullptr) {
			m_pItemForSaleList[i].reset();
			m_pItemForSaleList[i].reset();
		}
	}

	// Get item IDs from packet (they follow the header)
	const int16_t* itemIds = reinterpret_cast<const int16_t*>(pData + sizeof(hb::net::PacketShopResponseHeader));

	// Populate shop list from item configs
	int shopIndex = 0;
	int skippedCount = 0;
	int notFoundCount = 0;
	for (uint16_t i = 0; i < itemCount && shopIndex < DEF_MAXMENUITEMS; i++) {
		int16_t itemId = itemIds[i];
		if (itemId <= 0 || itemId >= 5000) {
			if (skippedCount < 5) printf("[SHOP] Skipping invalid item ID: %d\n", itemId);
			skippedCount++;
			continue;
		}
		if (m_pItemConfigList[itemId] == nullptr) {
			if (notFoundCount < 10) printf("[SHOP] Item ID %d not in m_pItemConfigList\n", itemId);
			notFoundCount++;
			skippedCount++;
			continue;
		}

		// Create new item for shop based on config
		m_pItemForSaleList[shopIndex] = std::make_unique<CItem>();
		CItem* pItem = m_pItemForSaleList[shopIndex].get();
		CItem* pConfig = m_pItemConfigList[itemId].get();

		// Copy item data from config
		pItem->m_sIDnum = itemId;
		std::memcpy(pItem->m_cName, pConfig->m_cName, sizeof(pItem->m_cName));
		pItem->m_cItemType = pConfig->m_cItemType;
		pItem->m_cEquipPos = pConfig->m_cEquipPos;
		pItem->m_sSprite = pConfig->m_sSprite;
		pItem->m_sSpriteFrame = pConfig->m_sSpriteFrame;
		pItem->m_wPrice = pConfig->m_wPrice;
		pItem->m_wWeight = pConfig->m_wWeight;
		pItem->m_sItemEffectValue1 = pConfig->m_sItemEffectValue1;
		pItem->m_sItemEffectValue2 = pConfig->m_sItemEffectValue2;
		pItem->m_sItemEffectValue3 = pConfig->m_sItemEffectValue3;
		pItem->m_sItemEffectValue4 = pConfig->m_sItemEffectValue4;
		pItem->m_sItemEffectValue5 = pConfig->m_sItemEffectValue5;
		pItem->m_sItemEffectValue6 = pConfig->m_sItemEffectValue6;
		pItem->m_wMaxLifeSpan = pConfig->m_wMaxLifeSpan;
		pItem->m_sLevelLimit = pConfig->m_sLevelLimit;
		pItem->m_cGenderLimit = pConfig->m_cGenderLimit;
		pItem->m_sSpecialEffect = pConfig->m_sSpecialEffect;
		pItem->m_cSpeed = pConfig->m_cSpeed;

		// Copy display name
		pItem->SetDisplayName(pConfig->GetDisplayName());
		shopIndex++;
	}

	printf("[SHOP] Populated: %d items added, %d skipped (%d not found in config list)\n", shopIndex, skippedCount, notFoundCount);

	// Only show shop dialog if we have items and there was a pending request
	if (shopIndex > 0 && m_sPendingShopType != 0) {
		// Enable the SaleMenu dialog - this will call EnableDialogBox which sets up the dialog
		EnableDialogBox(static_cast<int>(DialogBoxId::SaleMenu), m_sPendingShopType, 0, 0, nullptr);
		m_sPendingShopType = 0;  // Clear pending request
	} else if (m_sPendingShopType != 0) {
		// No items available - show message to user
		AddEventList("This shop has no items available.", 10);
		m_sPendingShopType = 0;
	}
}

bool CGame::bInitMagicCfgList()
{
	char cFn[255], cTemp[255];
	char* pContents, * token;
	char seps[] = "= ,\t\n";
	char cReadModeA = 0;
	char cReadModeB = 0;
	int  iMagicCfgListIndex = 0;
	HANDLE hFile;
	FILE* pFile;
	uint32_t dwFileSize;

	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFn, 0, sizeof(cFn));

	// CLEROTH - MAGIC CFG
	strcpy(cTemp, "magiccfg.txt");

	strcat(cFn, "contents");
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	strcat(cFn, cTemp);

	hFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	pFile = fopen(cFn, "rt");
	if (pFile == 0) return false;
	else {
		pContents = new char[dwFileSize + 1];
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
		fclose(pFile);
	}

	token = strtok(pContents, seps);
	while (token != 0) {
		if (cReadModeA != 0) {
			switch (cReadModeA) {
			case 1:
				switch (cReadModeB) {
				case 1:
					if (_bGetIsStringIsNumber(token) == false)
					{
						delete[] pContents;
						return false;
					}
					if (m_pMagicCfgList[atoi(token)] != 0)
					{
						delete[] pContents;
						return false;
					}
					m_pMagicCfgList[atoi(token)] = std::make_unique<CMagic>();
					iMagicCfgListIndex = atoi(token);

					cReadModeB = 2;
					break;

				case 2:
					std::memset(m_pMagicCfgList[iMagicCfgListIndex]->m_cName, 0, sizeof(m_pMagicCfgList[iMagicCfgListIndex]->m_cName));
					memcpy(m_pMagicCfgList[iMagicCfgListIndex]->m_cName, token, strlen(token));
					cReadModeB = 3;
					break;

				case 3: // m_sValue1
					if (_bGetIsStringIsNumber(token) == false) {
						delete[] pContents;
						return false;
					}
					m_pMagicCfgList[iMagicCfgListIndex]->m_sValue1 = atoi(token);
					cReadModeB = 4;
					break;

				case 4: // m_sValue2	// INT
					if (_bGetIsStringIsNumber(token) == false) {
						delete[] pContents;
						return false;
					}
					m_pMagicCfgList[iMagicCfgListIndex]->m_sValue2 = atoi(token);
					cReadModeB = 5;
					break;

				case 5: // m_sValue3	// COST
					if (_bGetIsStringIsNumber(token) == false) {
						delete[] pContents;
						return false;
					}
					m_pMagicCfgList[iMagicCfgListIndex]->m_sValue3 = atoi(token);
					cReadModeB = 6;
					break;

					// CLEROTH MAGIC CFG
				case 6: // m_sValue4	// STR
					if (_bGetIsStringIsNumber(token) == false)
					{
						delete[] pContents;
						return false;
					}
					m_pMagicCfgList[iMagicCfgListIndex]->m_sValue4 = atoi(token);
					cReadModeB = 7;
					break;

				case 7: // m_sValue5
					cReadModeB = 8;
					break;

				case 8: // m_sValue6
					cReadModeB = 9;
					break;

				case 9: // m_bIsVisible
					if (_bGetIsStringIsNumber(token) == false)
					{
						delete[] pContents;
						return false;
					}
					if (atoi(token) == 0) m_pMagicCfgList[iMagicCfgListIndex]->m_bIsVisible = false;
					else m_pMagicCfgList[iMagicCfgListIndex]->m_bIsVisible = true;
					cReadModeA = 0;
					cReadModeB = 0;
					break;
				}
				break;

			default:
				break;
			}
		}
		else {
			if (memcmp(token, "magic", 5) == 0) {
				cReadModeA = 1;
				cReadModeB = 1;
			}
		}
		token = strtok(NULL, seps);
	}

	delete[] pContents;

	if ((cReadModeA != 0) || (cReadModeB != 0)) {
		return false;
	}

	return true;
}

bool CGame::bCheckImportantFile()
{
	HANDLE hFile;

#ifndef _DEBUG
	hFile = CreateFile("CONTENTS\\badword.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return false;
	CloseHandle(hFile);
#endif

	// Use the sprite factory's configured path
	std::string spritePath = SpriteLib::Sprites::GetSpritePath() + "\\TREES1.PAK";
	hFile = CreateFile(spritePath.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	//	// FileSize : 1846406.... Anti Tree1.pak hack....inutile ca peut se modifier sans changer la taille!
	//	if( GetFileSize( hFile, 0 ) != 2945524 )
	//	{	CloseHandle( hFile );
	//		return false;
	//	}

	CloseHandle(hFile);
	return true;
}

bool CGame::bInitSkillCfgList()
{
	char cFn[255], cTemp[255];
	char* pContents, * token;
	char seps[] = "= ,\t\n";
	char cReadModeA = 0;
	char cReadModeB = 0;
	int  iSkillCfgListIndex = 0;
	HANDLE hFile;
	FILE* pFile;
	uint32_t dwFileSize;

	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFn, 0, sizeof(cFn));

	strcpy(cTemp, "Skillcfg.txt");
	strcat(cFn, "contents");
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	strcat(cFn, cTemp);

	hFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	pFile = fopen(cFn, "rt");
	if (pFile == 0) return false;
	else {
		pContents = new char[dwFileSize + 1];
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
		fclose(pFile);
	}

	token = strtok(pContents, seps);
	while (token != 0) {
		if (cReadModeA != 0) {
			switch (cReadModeA) {
			case 1:
				switch (cReadModeB) {
				case 1:
					if (_bGetIsStringIsNumber(token) == false)
					{
						delete[] pContents;
						return false;
					}
					if (m_pSkillCfgList[atoi(token)] != 0)
					{
						delete[] pContents;
						return false;
					}
					m_pSkillCfgList[atoi(token)] = std::make_unique<CSkill>();
					iSkillCfgListIndex = atoi(token);
					cReadModeB = 2;
					break;

				case 2:
					std::memset(m_pSkillCfgList[iSkillCfgListIndex]->m_cName, 0, sizeof(m_pSkillCfgList[iSkillCfgListIndex]->m_cName));
					memcpy(m_pSkillCfgList[iSkillCfgListIndex]->m_cName, token, strlen(token));
					cReadModeB = 3;
					break;

				case 3: // m_bIsUseable
					if (_bGetIsStringIsNumber(token) == false) {
						delete[] pContents;
						return false;
					}
					m_pSkillCfgList[iSkillCfgListIndex]->m_bIsUseable = (bool)atoi(token);
					cReadModeB = 4;
					break;

				case 4: // m_cUseMethod
					if (_bGetIsStringIsNumber(token) == false) {
						delete[] pContents;
						return false;
					}
					m_pSkillCfgList[iSkillCfgListIndex]->m_cUseMethod = atoi(token);
					cReadModeA = 0;
					cReadModeB = 0;
					break;
				}
				break;

			default:
				break;
			}
		}
		else {
			if (memcmp(token, "skill", 5) == 0) {
				cReadModeA = 1;
				cReadModeB = 1;
			}
		}
		token = strtok(NULL, seps);
	}

	delete[] pContents;

	if ((cReadModeA != 0) || (cReadModeB != 0)) {
		return false;
	}

	return true;
}

bool CGame::_bGetIsStringIsNumber(char* pStr)
{
	if (!pStr || *pStr == '\0') return false;
	int result;
	auto [ptr, ec] = std::from_chars(pStr, pStr + strlen(pStr), result);
	return ec == std::errc{} && *ptr == '\0';
}

void CGame::RequestFullObjectData(uint16_t wObjectID)
{
	int     iRet;
	hb::net::PacketHeader header{};
	header.msg_id = MSGID_REQUEST_FULLOBJECTDATA;
	header.msg_type = wObjectID;

	iRet = m_pGSock->iSendMsg(reinterpret_cast<char*>(&header), sizeof(header));

	switch (iRet) {
	case DEF_XSOCKEVENT_SOCKETCLOSED:
	case DEF_XSOCKEVENT_SOCKETERROR:
	case DEF_XSOCKEVENT_QUENEFULL:
		ChangeGameMode(GameMode::ConnectionLost);

		m_pGSock.reset();
		m_pGSock.reset();
		break;

	case DEF_XSOCKEVENT_CRITICALERROR:
		m_pGSock.reset();
		m_pGSock.reset();

		if (G_pCalcSocket != 0) {
			delete G_pCalcSocket;
			G_pCalcSocket = 0;
		}
		SendMessage(G_hWnd, WM_DESTROY, 0, 0);
		break;
	}
}

SpriteLib::BoundRect CGame::DrawObject_OnAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iHelmIndex, iR, iG, iB;
	int iWeaponIndex, iWeapon, iAdd, iShieldIndex, iMantleIndex;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;

	if (m_entityState.m_sOwnerType == 35 || m_entityState.m_sOwnerType == 81 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}
	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		if ((m_entityState.m_sAppr2 & 0xF000) != 0) {
			iWeapon = ((m_entityState.m_sAppr2 & 0x0FF0) >> 4);
			if (iWeapon == 0) iAdd = 6;
			if ((iWeapon >= 1) && (iWeapon <= 39)) iAdd = 6;
			if ((iWeapon >= 40) && (iWeapon <= 59)) iAdd = 7;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 4 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 4;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (5 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 5;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 5;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 5;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 5;
			iWeaponIndex = -1;
			iShieldIndex = -1;
		}
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if ((m_entityState.m_sAppr2 & 0xF000) != 0)
		{
			iWeapon = ((m_entityState.m_sAppr2 & 0x0FF0) >> 4);
			if (iWeapon == 0) iAdd = 6;
			if ((iWeapon >= 1) && (iWeapon <= 39)) iAdd = 6;
			if ((iWeapon >= 40) && (iWeapon <= 59)) iAdd = 7;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 4 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 4;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (5 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 5;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 5;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 5;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 5;
			iWeaponIndex = -1;
			iShieldIndex = -1;
		}
		break;

	default:
		if (m_entityState.m_sAppr2 != 0)
		{
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			m_entityState.m_iFrame = m_entityState.m_sAppr2 - 1;
		}
		else if (m_entityState.m_sOwnerType == 66) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
		else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
		else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
		else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
		iUndiesIndex = -1;
		iHairIndex = -1;
		iBodyArmorIndex = -1;
		iArmArmorIndex = -1;
		iBootsIndex = -1;
		iPantsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, m_entityState.m_iFrame);
	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(sX, sY, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
				{
					m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame);
				}
				else
				{
					m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
				if (m_entityState.m_iFrame == 3) m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame - 1, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			}
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // FireWyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1)
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1)
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1)
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1)
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (((m_entityState.m_sAppr2 & 0x000F) == 8) && (iShieldGlare == 1))
			{
				m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else
				if (iShieldIndex != -1)
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
					switch (iShieldGlare) {
					case 0: break;
						//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
					case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
			}

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1)
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1)
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1)
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1)
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iShieldIndex != -1)
			{
				if (iShieldColor == 0)
					m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
					m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame);
				else m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
				if (m_entityState.m_iFrame == 3) m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame - 1, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			}
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) // Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel((m_entityState.m_iDir - 1), sX + 20, sY - 20, m_entityState.m_iFrame % 8, dwTime);
		CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}

	// Snoopy: Abaddon effects
	if (m_entityState.m_sOwnerType == 81)
	{
		int randFrame = m_entityState.m_iFrame % 12;
		m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		switch (m_entityState.m_iDir) {
		case 1:
			m_pEffectSpr[153]->Draw(sX, sY + 108, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 50, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 2:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 70, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 3:
			m_pEffectSpr[153]->Draw(sX, sY + 105, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 90, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 4:
			m_pEffectSpr[153]->Draw(sX - 35, sY + 100, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 80, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 5:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 65, sY - 5, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 6:
			m_pEffectSpr[153]->Draw(sX + 45, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 31, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 7:
			m_pEffectSpr[153]->Draw(sX + 40, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 30, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 8:
			m_pEffectSpr[153]->Draw(sX + 20, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 20, sY + 16, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		}
	}

	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iHelmIndex, iR, iG, iB;
	int iWeaponIndex, iWeapon, iAdd, iShieldIndex, iMantleIndex, dx, dy, dsx, dsy;
	int cFrameMoveDots;
	bool bInv = false, bDashDraw = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;

	if (m_entityState.m_sOwnerType == 35 || m_entityState.m_sOwnerType == 81 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}

	switch (m_entityState.m_iFrame) {
	case 4:  m_entityState.m_iFrame = 4; break;
	case 5:  m_entityState.m_iFrame = 4; break;
	case 6:  m_entityState.m_iFrame = 4; break;
	case 7:  m_entityState.m_iFrame = 4; break;
	case 8:  m_entityState.m_iFrame = 4; break;
	case 9:  m_entityState.m_iFrame = 4; break;
	case 10: m_entityState.m_iFrame = 5; break;
	case 11: m_entityState.m_iFrame = 6; break;
	case 12: m_entityState.m_iFrame = 7; break;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		if ((m_entityState.m_sAppr2 & 0xF000) != 0) {
			iWeapon = ((m_entityState.m_sAppr2 & 0x0FF0) >> 4);
			if (iWeapon == 0) iAdd = 6;
			if ((iWeapon >= 1) && (iWeapon <= 39)) iAdd = 6;
			if ((iWeapon >= 40) && (iWeapon <= 59)) iAdd = 7;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 4 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 4;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (5 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 5;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 5;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 5;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 5;
			iWeaponIndex = -1;
			iShieldIndex = -1;
		}
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if ((m_entityState.m_sAppr2 & 0xF000) != 0) {
			iWeapon = ((m_entityState.m_sAppr2 & 0x0FF0) >> 4);
			if (iWeapon == 0) iAdd = 6;
			if ((iWeapon >= 1) && (iWeapon <= 39)) iAdd = 6;
			if ((iWeapon >= 40) && (iWeapon <= 59)) iAdd = 7;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 4 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 4;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (5 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 5;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 5;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 5;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 5;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 5;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 5;
			iWeaponIndex = -1;
			iShieldIndex = -1;
		}
		break;
	default:
		iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
		iUndiesIndex = -1;
		iHairIndex = -1;
		iBodyArmorIndex = -1;
		iArmArmorIndex = -1;
		iBootsIndex = -1;
		iPantsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	dx = 0;
	dy = 0;
	if ((m_entityState.m_iFrame >= 1) && (m_entityState.m_iFrame <= 3))
	{
		switch (m_entityState.m_iFrame) {
		case 1: cFrameMoveDots = 26; break;
		case 2: cFrameMoveDots = 16; break;
		case 3: cFrameMoveDots = 0;  break;
		}
		switch (m_entityState.m_iDir) {
		case 1: dy = cFrameMoveDots; break;
		case 2: dy = cFrameMoveDots; dx = -cFrameMoveDots; break;
		case 3: dx = -cFrameMoveDots; break;
		case 4: dx = -cFrameMoveDots; dy = -cFrameMoveDots; break;
		case 5: dy = -cFrameMoveDots; break;
		case 6: dy = -cFrameMoveDots; dx = cFrameMoveDots; break;
		case 7: dx = cFrameMoveDots; break;
		case 8: dx = cFrameMoveDots; dy = cFrameMoveDots; break;
		}
		switch (m_entityState.m_iFrame) {
		case 1: dy++;    break;
		case 2: dy += 2; break;
		case 3: dy++;    break;
		}
		switch (m_entityState.m_iFrame) {
		case 2: bDashDraw = true; cFrameMoveDots = 26; break;
		case 3: bDashDraw = true; cFrameMoveDots = 16; break;
		}
		dsx = 0;
		dsy = 0;
		switch (m_entityState.m_iDir) {
		case 1: dsy = cFrameMoveDots; break;
		case 2: dsy = cFrameMoveDots; dsx = -cFrameMoveDots; break;
		case 3: dsx = -cFrameMoveDots; break;
		case 4: dsx = -cFrameMoveDots; dsy = -cFrameMoveDots; break;
		case 5: dsy = -cFrameMoveDots; break;
		case 6: dsy = -cFrameMoveDots; dsx = cFrameMoveDots; break;
		case 7: dsx = cFrameMoveDots; break;
		case 8: dsx = cFrameMoveDots; dsy = cFrameMoveDots; break;
		}
	}
	else if (m_entityState.m_iFrame > 3)
	{
		dx = 0;
		dy = 0;
	}
	else
	{
		switch (m_entityState.m_iDir) {
		case 1: dy = 32; break;
		case 2: dy = 32; dx = -32; break;
		case 3: dx = -32; break;
		case 4: dx = -32; dy = -32; break;
		case 5: dy = -32; break;
		case 6: dy = -32; dx = 32; break;
		case 7: dx = 32; break;
		case 8: dx = 32; dy = 32; break;
		}
	}

	if (m_bIsCrusadeMode) DrawObjectFOE(sX + dx, sY + dy, m_entityState.m_iFrame);

	if (m_entityState.m_iEffectType != 0) {
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX + dx, sY + dy, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX + dx, sY + dy, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(sX + dx, sY + dy, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
					m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame);
				else m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
				if (m_entityState.m_iFrame == 3) m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame - 1, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			}
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}

			if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else {
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0)) {
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

			if ((iHairIndex != -1) && (iHelmIndex == -1)) {
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1) {
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1) {
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1) {
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1) {
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2)) {
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iShieldIndex != -1)
			{
				if (iShieldColor == 0)
					m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

				else m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1)) {
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;

			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}

			if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else {
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0)) {
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

			if ((iHairIndex != -1) && (iHelmIndex == -1)) {
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1) {
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1) {
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1) {
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1)
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iShieldIndex != -1)
			{
				if (iShieldColor == 0)
					m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(sX - 13 + dx, sY - 34 + dy, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1)) {
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX + dx, sY + dy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
					m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame);
				else m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
				if (m_entityState.m_iFrame == 3) m_pSprite[iWeaponIndex]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame - 1, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			}
		}

		// Berserk
		if ((m_entityState.m_iStatus & 0x20) != 0)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dx, sY + dy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(8 + (m_entityState.m_iDir - 1), sX + dx + 20, sY + dy - 20, m_entityState.m_iFrame % 8, dwTime);
		CheckActiveAura2(sX + dx, sY + dy, dwTime, m_entityState.m_sOwnerType);

		if (bDashDraw == true) {
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX + dsx, sY + dsy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			if (iWeaponIndex != -1) m_pSprite[iWeaponIndex]->Draw(sX + dsx, sY + dsy, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			if (iShieldIndex != -1) m_pSprite[iShieldIndex]->Draw(sX + dsx, sY + dsy, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
		}
	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX + dx, sY + dy, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX + dx, sY + dy, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}

	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID)) {
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX + dx;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY + dy;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	m_entityState.m_iMoveOffsetX = dx;
	m_entityState.m_iMoveOffsetY = dy;

	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iR, iG, iB, iHelmIndex, iMantleIndex;
	bool bInv = false;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;

	if (m_entityState.m_sOwnerType == 35 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}

	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0)
			bInv = true;
		else
		{
			if (m_entityState.m_iChatIndex != 0)
			{
				if (m_pChatMsgList[m_entityState.m_iChatIndex] != 0)
				{
					m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
					m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
				}
				else
				{
					m_pMapData->ClearChatMsg(indexX, indexY);
				}
			}
			return invalidRect;
		}
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (8 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 8;
		iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 8;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 8;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 8;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 8;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 8;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 8;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 8;
		break;
	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;

		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (8 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 8;
		iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 8;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 8;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 8;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 8;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 8;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 8;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 8;
		break;
	}

	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, m_entityState.m_iFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(sX, sY, dwTime, m_entityState.m_sOwnerType);
		switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
		case 10: // Slime
		case 35: // Energy Sphere
		case 50: // TW
		case 51: // CP
		case 60: // Plant
		case 65: // IceGolem
			//case 66: // Wyvern
			//case 73: // Fire Wyvern
		case 81: // Abaddon
		case 91: // Gate
			break;
		default:
			if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
				if (sX < 50)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
			}
			break;
		}

		if (bInv == true)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
		else {
			if ((m_entityState.m_iStatus & 0x40) != 0)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
			else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
		}
		SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

		if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);

		if ((iHairIndex != -1) && (iHelmIndex == -1))
		{
			_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
			m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 1))
		{
			if (iBootsColor == 0)
				m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
		}

		if (iPantsIndex != -1)
		{
			if (iPantsColor == 0)
				m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
		}

		if (iArmArmorIndex != -1)
		{
			if (iArmColor == 0)
				m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 0))
		{
			if (iBootsColor == 0)
				m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
		}

		if (iBodyArmorIndex != -1)
		{
			if (iArmorColor == 0)
				m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
		}

		if (iHelmIndex != -1)
		{
			if (iHelmColor == 0)
				m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
		}

		if (iMantleIndex != -1)
		{
			if (iMantleColor == 0)
				m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame);
			else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 16 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) 	// Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(32 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, m_entityState.m_iFrame % 16, dwTime);
		CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iR, iG, iB, iHelmIndex, iMantleIndex;
	bool bInv = false;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;

	if (m_entityState.m_sOwnerType == 35 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}

	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (9 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 9;
		iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 9;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 9;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 9;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 9;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 9;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 9;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 9; 		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (9 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 9;
		iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 9;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 9;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 9;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 9;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 9;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 9;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 9;
		break;
	default:
		iUndiesIndex = -1;
		iHairIndex = -1;
		iArmArmorIndex = -1;
		iBodyArmorIndex = -1;
		iPantsIndex = -1;
		iBootsIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, m_entityState.m_iFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(sX, sY, dwTime, m_entityState.m_sOwnerType);
		switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
		case 10: // Slime
		case 35: // Energy Sphere
		case 50: // TW
		case 51: // CP
		case 60: // Plant
		case 65: // IceGolem
			//case 66: // Wyvern
			//case 73: // Fire Wyvern
		case 81: // Abaddon
		case 91: // Gate
			break;
		default:
			if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
			{
				if (sX < 50)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
			}
			break;
		}

		if (bInv == true)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
		else {
			if ((m_entityState.m_iStatus & 0x40) != 0)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
			else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
		}
		SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

		if (iUndiesIndex != -1)
		{
			if (bInv) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
		}

		if ((iHairIndex != -1) && (iHelmIndex == -1))
		{
			_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
			m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 1))
		{
			if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}
		}

		if (iPantsIndex != -1)
		{
			if (bInv) m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}
		}

		if (iArmArmorIndex != -1)
		{
			if (bInv) m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 0))
		{
			if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}
		}

		if (iBodyArmorIndex != -1)
		{
			if (bInv) m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}
		}

		if (iHelmIndex != -1)
		{
			if (bInv) m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}
		}

		if (iMantleIndex != -1)
		{
			if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) // Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(40 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, m_entityState.m_iFrame % 4, dwTime);
		CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iWeaponIndex, iShieldIndex, iHelmIndex, iR, iG, iB;
	int iAdd, iDrawMode, iMantleIndex;
	char cFrame;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;

	if (m_entityState.m_sOwnerType == 35 || m_entityState.m_sOwnerType == 81 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}
	cFrame = m_entityState.m_iFrame;
	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		if (cFrame < 4)
		{
			if ((m_entityState.m_sAppr2 & 0xF000) != 0) iAdd = 1;
			else iAdd = 0;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * iAdd + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
			iDrawMode = 0;
		}
		else
		{
			cFrame -= 4;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (10 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 10;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 10;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 10;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 10;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 10;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 10;
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 5;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 5 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 10;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 10;
			iDrawMode = 1;
		}
		break;
	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if (cFrame < 4)
		{
			if ((m_entityState.m_sAppr2 & 0xF000) != 0) iAdd = 1;
			else iAdd = 0;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * iAdd + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
			iDrawMode = 0;
		}
		else
		{
			cFrame -= 4;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (10 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 10;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 10;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 10;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 10;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 10;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 10;
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 5;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 5 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 10;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 10;
			iDrawMode = 1;
		}
		break;

	default:
		if (cFrame < 4)
		{
			if (m_entityState.m_sAppr2 != 0)
			{
				iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
				cFrame = m_entityState.m_sAppr2 - 1;
			}
			else if (m_entityState.m_sOwnerType == 66) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 67) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 68) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 69) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 81) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 91) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		}
		else
		{
			cFrame -= 4;
			if (m_entityState.m_sAppr2 != 0)
			{
				iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
				cFrame = m_entityState.m_sAppr2 - 1;
			}
			else if (m_entityState.m_sOwnerType == 66) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 67) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 68) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 69) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 81) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 91) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
			else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
		}
		iUndiesIndex = -1;
		iHairIndex = -1;
		iArmArmorIndex = -1;
		iBodyArmorIndex = -1;
		iPantsIndex = -1;
		iBootsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		iDrawMode = 0;
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, cFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(sX, sY, dwTime, m_entityState.m_sOwnerType);
		if (iDrawMode == 1) // Etrange, 1 semble impossible avec des mobs !
		{
			if (_cDrawingOrder[m_entityState.m_iDir] == 1)
			{
				if (iWeaponIndex != -1)
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
					DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
					switch (iWeaponGlare) {
					case 0: break;
					case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}
				switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
				case 10: // Slime
				case 35: // Energy Sphere
				case 50: // TW
				case 51: // CP
				case 60: // Plant
				case 65: // IceGolem
					//case 66: // Wyvern
					//case 73: // Fire Wyvern
				case 81: // Abaddon
				case 91: // Gate
					break;
				default:
					if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
						if (sX < 50)
							m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
						else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
					}
					break;
				}
				if (m_entityState.m_sOwnerType == 35)
					m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

				if (m_entityState.m_sOwnerType == 81) // Abaddon
				{
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				}
				else if (bInv == true)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				else
				{
					if ((m_entityState.m_iStatus & 0x40) != 0)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame);
				}
				SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);

				if ((iHairIndex != -1) && (iHelmIndex == -1))
				{
					_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
					m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 1))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iPantsIndex != -1)
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}

				if (iArmArmorIndex != -1)
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 0))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iBodyArmorIndex != -1)
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}

				if (iHelmIndex != -1)
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iShieldIndex != -1)
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
					switch (iShieldGlare) {
					case 0: break;
						//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
					case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}
			else
			{
				switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
				case 10: // Slime
				case 35: // Energy Sphere
				case 50: // TW
				case 51: // CP
				case 60: // Plant
				case 65: // IceGolem
					//case 66: // Wyvern
					//case 73: // Fire Wyvern
				case 81: // Abaddon
				case 91: // Gate
					break;
				default:
					if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
						if (sX < 50)
							m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
						else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
					}
					break;
				}
				if (m_entityState.m_sOwnerType == 35)
					m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

				if (m_entityState.m_sOwnerType == 81) // Abaddon
				{
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				}
				else if (bInv == true)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				else
				{
					if ((m_entityState.m_iStatus & 0x40) != 0)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame);
				}
				SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);

				if ((iHairIndex != -1) && (iHelmIndex == -1))
				{
					_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
					m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 1))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iPantsIndex != -1)
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}

				if (iArmArmorIndex != -1)
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 0))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iBodyArmorIndex != -1)
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}

				if (iHelmIndex != -1)
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iShieldIndex != -1)
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
					switch (iShieldGlare) {
					case 0: break;
						//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
					case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iWeaponIndex != -1)
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
					DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
					switch (iWeaponGlare) {
					case 0: break;
					case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}
			}

			if ((m_entityState.m_iStatus & 0x20) != 0) 	// Berserk
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
			DrawAngel(16 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, cFrame % 4, dwTime);
			CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

		}
		else // DrawMode != 1
		{
			if (_cDrawingOrder[m_entityState.m_iDir] == 1)
			{
				if (iWeaponIndex != -1)
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
					DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
					switch (iWeaponGlare) {
					case 0: break;
					case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}
				switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
				case 10: // Slime
				case 35: // Energy Sphere
				case 50: // TW
				case 51: // CP
				case 60: // Plant
				case 65: // IceGolem
					//case 66: // Wyvern
					//case 73: // Fire Wyvern
				case 81: // Abaddon
				case 91: // Gate
					break;
				default:
					if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
					{
						if (sX < 50)
							m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
						else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					}
					break;
				}
				if (m_entityState.m_sOwnerType == 35)
					m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

				if (m_entityState.m_sOwnerType == 81) // Abaddon
				{
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				}
				else if (bInv == true)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				else {
					if ((m_entityState.m_iStatus & 0x40) != 0)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame);
				}
				SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);

				if ((iHairIndex != -1) && (iHelmIndex == -1))
				{
					_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
					m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 1)) {
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iPantsIndex != -1)
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}

				if (iArmArmorIndex != -1)
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 0)) {
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iBodyArmorIndex != -1)
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}

				if (iHelmIndex != -1)
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iShieldIndex != -1)
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
					switch (iShieldGlare) {
					case 0: break;
						//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
					case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}
			else
			{
				switch (m_entityState.m_sOwnerType) {
				case 10: // Slime
				case 35: // Energy Sphere
				case 50: // TW
				case 51: // CP
				case 60: // Plant
				case 65: // IceGolem
					//case 66: // Wyvern
					//case 73: // Fire Wyvern
				case 81: // Abaddon
				case 91: // Gate
					break;
				default:
					if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
						if (sX < 50)
							m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
						else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
					}
					break;
				}
				if (m_entityState.m_sOwnerType == 35)
					m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

				if (m_entityState.m_sOwnerType == 81) // Abaddon
				{
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				}
				else if (bInv == true)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
				else {
					if ((m_entityState.m_iStatus & 0x40) != 0)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame);
				}

				SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);

				if ((iHairIndex != -1) && (iHelmIndex == -1))
				{
					_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
					m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 1))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iPantsIndex != -1)
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}

				if (iArmArmorIndex != -1)
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}

				if ((iBootsIndex != -1) && (iSkirtDraw == 0))
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}

				if (iBodyArmorIndex != -1)
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}

				if (iHelmIndex != -1)
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iShieldIndex != -1)
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
					switch (iShieldGlare) {
					case 0: break;
						//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
					case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}

				if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}

				if (iWeaponIndex != -1)
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
					DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
					switch (iWeaponGlare) {
					case 0: break;
					case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
					case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
					case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
					}
				}
			}

			if ((m_entityState.m_iStatus & 0x20) != 0)	// Berserk
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
			DrawAngel(16 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, cFrame % 4, dwTime);
			CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);
		}
	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	// Snoopy: Abaddon effects
	if (m_entityState.m_sOwnerType == 81)
	{
		int randFrame = m_entityState.m_iFrame % 12;
		m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		switch (m_entityState.m_iDir) {
		case 1:
			m_pEffectSpr[153]->Draw(sX, sY + 108, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 50, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 2:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 70, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 3:
			m_pEffectSpr[153]->Draw(sX, sY + 105, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 90, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 4:
			m_pEffectSpr[153]->Draw(sX - 35, sY + 100, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 80, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 5:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 65, sY - 5, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 6:
			m_pEffectSpr[153]->Draw(sX + 45, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 31, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 7:
			m_pEffectSpr[153]->Draw(sX + 40, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 30, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 8:
			m_pEffectSpr[153]->Draw(sX + 20, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 20, sY + 16, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		}
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iR, iG, iB, iHelmIndex, iMantleIndex;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	char cFrame;
	int randFrame;

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	cFrame = m_entityState.m_iFrame;

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		if (cFrame < 6)
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (0 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 0;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 0;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 0;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 0;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 0;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 0;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 0;
		}
		else
		{
			cFrame -= 6;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (11 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 11;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 11;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 11;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 11;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 11;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 11;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 11;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 11;
		}
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if (cFrame < 6)
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (0 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 0;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 0;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 0;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 0;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 0;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 0;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 0;
		}
		else
		{
			cFrame -= 6;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (11 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 11;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 11;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 11;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 11;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 11;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 11;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 11;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 11;
		}
		break;

	default:
		if (cFrame < 4)
		{
			if (m_entityState.m_sAppr2 != 0)
			{
				iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
				cFrame = m_entityState.m_sAppr2 - 1;
			}
			else if (m_entityState.m_sOwnerType == 66) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 81) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 91) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
			iUndiesIndex = -1;
			iHairIndex = -1;
			iArmArmorIndex = -1;
			iBodyArmorIndex = -1;
			iPantsIndex = -1;
			iBootsIndex = -1;
			iMantleIndex = -1;
			iHelmIndex = -1;
			switch (m_entityState.m_sOwnerType) {
			case 36: // AGT
			case 37: // CGT
			case 38: // MS
			case 39: // DT
			case 40: // ESG
			case 41: // GMG
			case 42: // ManaStone
				if (m_entityState.m_sAppr2 == 0) cFrame = 0;
				break;
			case 51: cFrame = 0; break;
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) {
			case 51: cFrame = 0; break;
			default: cFrame -= 4; break;
			}
			if (m_entityState.m_sAppr2 != 0)
			{
				iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
				cFrame = m_entityState.m_sAppr2 - 1;
			}
			else if (m_entityState.m_sOwnerType == 66) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else if (m_entityState.m_sOwnerType == 81) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			else if (m_entityState.m_sOwnerType == 91) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			iUndiesIndex = -1;
			iHairIndex = -1;
			iArmArmorIndex = -1;
			iBodyArmorIndex = -1;
			iPantsIndex = -1;
			iBootsIndex = -1;
			iMantleIndex = -1;
			iHelmIndex = -1;
		}
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, cFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
		case 10: // Slime
		case 35: // Energy Sphere
		case 50: // TW
		case 51: // CP
		case 60: // Plant
		case 65: // IceGolem
		case 66: // Wyvern
		case 73: // Fire Wyvern
		case 81: // Abaddon
		case 91: // Gate
			break;
		default:
			if (ConfigManager::Get().GetDetailLevel() != 0)
			{
				if (sX < 50)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Shadow());
			}
			break;
		}
		if (m_entityState.m_sOwnerType == 81)
		{
			m_pEffectSpr[152]->Draw(sX - 80, sY - 15, m_entityState.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f)); // Explosion Abaddon
			m_pEffectSpr[152]->Draw(sX, sY - 15, m_entityState.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[152]->Draw(sX - 40, sY, m_entityState.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[163]->Draw(sX - 90, sY - 80, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f)); // Ames qui s'envolent
			m_pEffectSpr[160]->Draw(sX - 60, sY - 50, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[161]->Draw(sX - 30, sY - 20, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[162]->Draw(sX, sY - 100, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[163]->Draw(sX + 30, sY - 30, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[162]->Draw(sX + 60, sY - 90, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[163]->Draw(sX + 90, sY - 50, m_entityState.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			switch (m_entityState.m_iDir) {
			case 1: m_pEffectSpr[140]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break; // Abbadon dying
			case 2: m_pEffectSpr[141]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break; // fixed sprit IDs
			case 3: m_pEffectSpr[142]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 4: m_pEffectSpr[143]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 5: m_pEffectSpr[144]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 6: m_pEffectSpr[145]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 7: m_pEffectSpr[146]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 8: m_pEffectSpr[147]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			}
		}
		else if (m_entityState.m_sOwnerType == 66) m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
		else if (m_entityState.m_sOwnerType == 73)
		{	//m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
			m_pSprite[33]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
			switch (m_entityState.m_iDir) {
			case 1: m_pEffectSpr[141]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break; // Abbadon qui meurt
			case 2: m_pEffectSpr[142]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 3: m_pEffectSpr[143]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 4: m_pEffectSpr[144]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 5: m_pEffectSpr[145]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 6: m_pEffectSpr[146]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 7: m_pEffectSpr[147]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
				//case 8: m_pEffectSpr[148]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 8: m_pEffectSpr[141]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break; //due to buggy Sprite nb
			}
		}
		else
		{
			if ((m_entityState.m_iStatus & 0x40) != 0)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
			else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame);
		}

		SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

		if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);

		if ((iHairIndex != -1) && (iHelmIndex == -1))
		{
			_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
			m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 1))
		{
			if (iBootsColor == 0)
				m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
		}

		if (iPantsIndex != -1)
		{
			if (iPantsColor == 0)
				m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
		}

		if (iArmArmorIndex != -1)
		{
			if (iArmColor == 0)
				m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
		}

		if ((iBootsIndex != -1) && (iSkirtDraw == 0))
		{
			if (iBootsColor == 0)
				m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
		}

		if (iBodyArmorIndex != -1)
		{
			if (iArmorColor == 0)
				m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
		}

		if (iHelmIndex != -1)
		{
			if (iHelmColor == 0)
				m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
		}

		if (iMantleIndex != -1)
		{
			if (iMantleColor == 0)
				m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame);
			else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) // Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(24 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, m_entityState.m_iFrame, dwTime);
		CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}

	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	int iBodyIndex, iUndiesIndex, iHairIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iR, iG, iB, iFrame, iMantleIndex, iHelmIndex;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};

	if (m_entityState.m_sOwnerType == 66) return invalidRect;

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		iFrame = 7;
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (11 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 11;
		iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 11;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 11;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 11;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 11;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 11;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 11;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 11;
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		iFrame = 7;
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (11 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 11;
		iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 11;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 11;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 11;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 11;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 11;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 11;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 11;
		break;
	default:
		switch (m_entityState.m_sOwnerType) {
		case 28: // Troll
		case 29: // Ogre
		case 30: // Liche
		case 31: // DD		// les 2 dernieres sont pas bonnes pour un mort !
		case 63: // Frost	// les 2 dernieres sont pas bonnes pour un mort !
			iFrame = 5;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			break;

		case 32: // Uni
		case 33: // WW
		case 43: // LWB
		case 44: // GHK
		case 45: // GHKABS
		case 46: // TK
		case 47: // BG
		case 48: // SK
		case 49: // HC
		case 50: // TW
		case 53: // BB
		case 54: // DE
		case 55: // Rabbit
		case 56: // Cat
		case 57: // Frog
		case 58: // MG
		case 59: // Ettin
		case 60: // Plant
		case 61: // Rudolph
		case 62: // Direboar
		case 64: // Crops  ----------- Crop ici! etonant, pourtant !
		case 65: // IceGolem
		case 70: // Dragon..........Ajouts par Snoopy
		case 71: // Centaur
		case 72: // ClawTurtle
		case 74: // GiantCrayfish
		case 75: // Gi Lizard
		case 76: // Gi Tree
		case 77: // Master Orc
		case 78: // Minaus
		case 79: // Nizie
		case 80: // Tentocle
		case 82: // Sorceress
		case 83: // ATK
		case 84: // MasterElf
		case 85: // DSK
		case 88: // Barbarian
			iFrame = 7;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			break;

		case 86: // HBT
		case 87: // CT
		case 89: // AGC
			iFrame = 7;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			break;

		case 66: // Wyvern
			iFrame = 15;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			break;

		case 73: // FireWyvern
			iFrame = 7;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			bTrans = true; // Prevents showing hugly corpse
			break;

		case 81: // Abaddon
			iFrame = 0;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
			bTrans = true; // Prevents showing hugly corpse
			break;

		case 51: // CP
			iFrame = 0;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			break;

		case 52: // GG
			iFrame = 11;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			break;

		case 91: // Gate
			iFrame = 5;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
			break;

		default: // 40*4 (10...27)
			iFrame = 3;
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			break;
		}
		iUndiesIndex = -1;
		iHairIndex = -1;
		iArmArmorIndex = -1;
		iBodyArmorIndex = -1;
		iPantsIndex = -1;
		iBootsIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	if (bTrans == false)
	{
		if (m_entityState.m_iFrame == -1)
		{
			m_entityState.m_iFrame = 7;
			if ((m_entityState.m_iStatus & 0x40) != 0)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
			else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, iFrame);

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if (iUndiesIndex != -1) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1)
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1)
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1)
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1)
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if (iMantleIndex != -1)
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
				else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}
		}
		else if ((m_entityState.m_iStatus & 0x20) != 0)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, iFrame, SpriteLib::DrawParams::TintedAlpha(-2 * m_entityState.m_iFrame + 5, -2 * m_entityState.m_iFrame - 5, -2 * m_entityState.m_iFrame - 5, 0.7f));
		else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, iFrame, SpriteLib::DrawParams::TintedAlpha(-2 * m_entityState.m_iFrame, -2 * m_entityState.m_iFrame, -2 * m_entityState.m_iFrame, 0.7f));

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}

	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearDeadChatMsg(indexX, indexY);
		}
	}
	// Snoopy: Abaddon effects
	if (m_entityState.m_sOwnerType == 81)
	{
		Abaddon_corpse(sX, sY); // By Snoopy....
	}
	else if (m_entityState.m_sOwnerType == 73)
	{	//m_pEffectSpr[35]->Draw(sX+120, sY+120, rand(), SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[35]->Draw(sX + 20, sY - 15, rand() % 10, SpriteLib::DrawParams::Alpha(0.7f));
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision)
{
	int dx, dy;
	int iBodyIndex, iHairIndex, iUndiesIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iHelmIndex, iR, iG, iB;
	int iWeaponIndex, iShieldIndex, iAdd, iMantleIndex;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};

	if (m_entityState.m_sOwnerType == 35 /* || m_entityState.m_sOwnerType == 66 || m_entityState.m_sOwnerType == 73*/)	bInv = true; //Energy-Ball, Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		if ((m_entityState.m_sAppr2 & 0xF000) != 0)
		{
			iAdd = 3;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 3 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 3;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (2 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 2;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 2;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 2;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 2;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 2;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 2;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else  iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 2 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 2;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 2;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 2;
		}
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if ((m_entityState.m_sAppr2 & 0xF000) != 0)
		{
			iAdd = 3;
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (iAdd * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + iAdd;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + iAdd;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + iAdd;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + iAdd;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 3 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 3;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + iAdd;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + iAdd;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (2 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 2;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 2;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 2;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 2;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 2;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 2;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 2 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 2;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 2;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 2;
		}
		break;

	default:
		if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
		iUndiesIndex = -1;
		iHairIndex = -1;
		iBodyArmorIndex = -1;
		iArmArmorIndex = -1;
		iBootsIndex = -1;
		iPantsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	// Use smooth interpolated motion offsets
	dx = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetX);
	dy = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetY);

	// Simple offset - no frame-based adjustments for smooth interpolation
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:

	case 28: // Troll.
	case 29: // Orge.
	case 30: // Liche
	case 31: // DD
	case 32: // Uni
	case 33: // ww

	case 43: // LWB
	case 44: // GHK
	case 45: // GHKABS
	case 46: // TK
	case 47: // BG
	case 48: // SK
	case 49: // HC
	case 50: // TW

	case 52: // GG
	case 53: // BB
	case 54: // DE
	case 55: // Rabbit
	case 56: // Cat
	case 57: // Frog
	case 58: // MG
	case 59: // Ettin
	case 60: // Plant
	case 61: // Rudolph
	case 62: // DireBoar
	case 63: // Frost

	case 65: // Ice-Golem
	case 66: // Wyvern

	case 70: // Dragon..........Ajouts par Snoopy
	case 71: // Centaur
	case 72: // ClawTurtle
	case 73: // FireWyvern
	case 74: // GiantCrayfish
	case 75: // Gi Lizard
	case 76: // Gi Tree
	case 77: // Master Orc
	case 78: // Minaus
	case 79: // Nizie
	case 80: // Tentocle
	case 81: // Abaddon
	case 82: // Sorceress
	case 83: // ATK
	case 84: // MasterElf
	case 85: // DSK
	case 86: // HBT
	case 87: // CT
	case 88: // Barbarian
	case 89: // AGC
	case 90: // Gail
		break;

	default:
		m_entityState.m_iFrame = m_entityState.m_iFrame / 2;
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(fix_x, fix_y, m_entityState.m_iFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (m_entityState.m_sOwnerType == 65) // IceGolem
	{	/*m_pEffectSpr[77]->Draw(sX+dx, sY+dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f));*/
		switch (rand() % 3) {
		case 0:	m_pEffectSpr[76]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		case 1:	m_pEffectSpr[77]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		case 2:	m_pEffectSpr[78]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		}
	}
	if (bTrans == false)
	{
		CheckActiveAura(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv) {
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(fix_x, fix_y, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else if (bInv == true)
				//m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX+dx, sY+dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
			}

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(fix_x, fix_y, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f));
			}
			else if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
			}

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
		}

		// Berserk
		if ((m_entityState.m_iStatus & 0x20) != 0)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(40 + (m_entityState.m_iDir - 1), fix_x + 20, fix_y - 20, m_entityState.m_iFrame % 4, dwTime);
		CheckActiveAura2(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(fix_x, fix_y, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(fix_x, fix_y, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}

	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = fix_x;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = fix_y;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	m_entityState.m_iMoveOffsetX = dx;
	m_entityState.m_iMoveOffsetY = dy;
	// Snoopy: Abaddon effects
	if (m_entityState.m_sOwnerType == 81)
	{
		int randFrame = m_entityState.m_iEffectFrame % 12;
		m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		switch (m_entityState.m_iDir) {
		case 1:
			m_pEffectSpr[153]->Draw(fix_x, fix_y + 108, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 50, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 2:
			m_pEffectSpr[153]->Draw(fix_x, fix_y + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 70, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 3:
			m_pEffectSpr[153]->Draw(fix_x, fix_y + 105, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 90, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 4:
			m_pEffectSpr[153]->Draw(fix_x - 35, fix_y + 100, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 80, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 5:
			m_pEffectSpr[153]->Draw(fix_x, fix_y + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 65, fix_y - 5, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 6:
			m_pEffectSpr[153]->Draw(fix_x + 45, fix_y + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 31, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 7:
			m_pEffectSpr[153]->Draw(fix_x + 40, fix_y + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 30, fix_y + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 8:
			m_pEffectSpr[153]->Draw(fix_x + 20, fix_y + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(fix_x - 20, fix_y + 16, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		}
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision)
{
	int cFrame, cDir;
	int dx, dy;
	int iBodyIndex, iHairIndex, iUndiesIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iHelmIndex, iR, iG, iB;
	int iWeaponIndex, iShieldIndex, iMantleIndex;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};

	if (m_entityState.m_sOwnerType == 67 || m_entityState.m_sOwnerType == 68 || m_entityState.m_sOwnerType == 69 || m_entityState.m_sOwnerType == 81) return invalidRect;
	if (m_entityState.m_sOwnerType == 35 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}
	cDir = m_entityState.m_iDir;
	switch (m_entityState.m_iDir) {
	case 1: m_entityState.m_iDir = 5; break;
	case 2: m_entityState.m_iDir = 6; break;
	case 3: m_entityState.m_iDir = 7; break;
	case 4: m_entityState.m_iDir = 8; break;
	case 5: m_entityState.m_iDir = 1; break;
	case 6: m_entityState.m_iDir = 2; break;
	case 7: m_entityState.m_iDir = 3; break;
	case 8: m_entityState.m_iDir = 4; break;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (10 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 10;
		iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 10;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 10;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 10;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 10;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 10;
		if ((m_entityState.m_sAppr2 & 0x000F) == 0)
			iShieldIndex = -1;
		else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 5;
		if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
			iWeaponIndex = -1;
		else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 5 + (m_entityState.m_iDir - 1);
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 10;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 10;
		break;
	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (10 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 10;
		iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 10;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 10;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 10;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 10;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 10;
		if ((m_entityState.m_sAppr2 & 0x000F) == 0)
			iShieldIndex = -1;
		else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 5;
		if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
			iWeaponIndex = -1;
		else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 5 + (m_entityState.m_iDir - 1);
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 10;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 10;
		break;
	default:
		if (m_entityState.m_sOwnerType == 66)      iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		else if (m_entityState.m_sOwnerType == 73) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		else if (m_entityState.m_sOwnerType == 86) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);
		else if (m_entityState.m_sOwnerType == 87) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);// Ne devrait pas arriver!
		else if (m_entityState.m_sOwnerType == 89) iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (2 * 8);// Ne devrait pas arriver!
		else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (3 * 8);
		iUndiesIndex = -1;
		iHairIndex = -1;
		iArmArmorIndex = -1;
		iBodyArmorIndex = -1;
		iPantsIndex = -1;
		iBootsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	// Use smooth interpolated motion offsets
	dx = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetX);
	dy = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetY);

	// Simple offset - no frame-based adjustments for smooth interpolation
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	cFrame = m_entityState.m_iFrame;
	if (m_bIsCrusadeMode) DrawObjectFOE(fix_x, fix_y, cFrame);
	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
					m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame);
				else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}

			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else {
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1)
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1)
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1)
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1) {
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + cFrame);
				else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iShieldIndex != -1)
			{
				if (iShieldColor == 0)
					m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else {
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iPantsIndex != -1)
			{
				if (iPantsColor == 0)
					m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
			}

			if (iArmArmorIndex != -1)
			{
				if (iArmColor == 0)
					m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0)) {
				if (iBootsColor == 0)
					m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
			}

			if (iBodyArmorIndex != -1)
			{
				if (iArmorColor == 0)
					m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
			}

			if (iHelmIndex != -1)
			{
				if (iHelmColor == 0)
					m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iShieldIndex != -1)
			{
				if (iShieldColor == 0)
					m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (iMantleColor == 0)
					m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame);
				else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 4 + cFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
			}

			if (iWeaponIndex != -1)
			{
				if (iWeaponColor == 0)
					m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame);
				else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) 	// Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, cFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(16 + (m_entityState.m_iDir - 1), fix_x + 20, fix_y - 20, cFrame % 4, dwTime);
		CheckActiveAura2(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(fix_x, fix_y, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(fix_x, fix_y, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}
	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = fix_x;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = fix_y;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	m_entityState.m_iMoveOffsetX = dx;
	m_entityState.m_iMoveOffsetY = dy;
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

// Equipment sprite indices for character rendering
struct MenuCharEquipment {
	int body, undies, hair, bodyArmor, armArmor, pants, boots, weapon, shield, mantle, helm;
	int weaponColor, shieldColor, armorColor, mantleColor, armColor, pantsColor, bootsColor, helmColor;
	bool skirtDraw;
};

// Calculate equipment indices for human characters (male/female) in menu
static void CalcHumanEquipment(const CEntityRenderState& state, bool isFemale, MenuCharEquipment& eq)
{
	// Sprite base IDs differ by gender
	int UNDIES  = isFemale ? DEF_SPRID_UNDIES_W    : DEF_SPRID_UNDIES_M;
	int HAIR    = isFemale ? DEF_SPRID_HAIR_W      : DEF_SPRID_HAIR_M;
	int ARMOR   = isFemale ? DEF_SPRID_BODYARMOR_W : DEF_SPRID_BODYARMOR_M;
	int BERK    = isFemale ? DEF_SPRID_BERK_W      : DEF_SPRID_BERK_M;
	int LEGG    = isFemale ? DEF_SPRID_LEGG_W      : DEF_SPRID_LEGG_M;
	int BOOT    = isFemale ? DEF_SPRID_BOOT_W      : DEF_SPRID_BOOT_M;
	int WEAPON  = isFemale ? DEF_SPRID_WEAPON_W    : DEF_SPRID_WEAPON_M;
	int SHIELD  = isFemale ? DEF_SPRID_SHIELD_W    : DEF_SPRID_SHIELD_M;
	int MANTLE  = isFemale ? DEF_SPRID_MANTLE_W    : DEF_SPRID_MANTLE_M;
	int HEAD    = isFemale ? DEF_SPRID_HEAD_W      : DEF_SPRID_HEAD_M;

	// Walking (sAppr2 & 0xF000) uses pose 3, standing uses pose 2
	bool isWalking = (state.m_sAppr2 & 0xF000) != 0;
	int pose = isWalking ? 3 : 2;

	// Extract appearance values
	int undiesType   = (state.m_sAppr1 & 0x000F);
	int hairType     = (state.m_sAppr1 & 0x0F00) >> 8;
	int armorType    = (state.m_sAppr3 & 0xF000) >> 12;
	int armType      = (state.m_sAppr3 & 0x000F);
	int pantsType    = (state.m_sAppr3 & 0x0F00) >> 8;
	int helmType     = (state.m_sAppr3 & 0x00F0) >> 4;
	int bootsType    = (state.m_sAppr4 & 0xF000) >> 12;
	int weaponType   = (state.m_sAppr2 & 0x0FF0) >> 4;
	int shieldType   = (state.m_sAppr2 & 0x000F);
	int mantleType   = (state.m_sAppr4 & 0x0F00) >> 8;
	bool hideArmor   = (state.m_sAppr4 & 0x80) != 0;

	// Body index
	eq.body = 500 + (state.m_sOwnerType - 1) * 8 * 15 + (pose * 8);

	// Equipment indices (-1 = not equipped)
	eq.undies    = UNDIES + undiesType * 15 + pose;
	eq.hair      = HAIR + hairType * 15 + pose;
	eq.bodyArmor = (!hideArmor && armorType != 0) ? ARMOR + armorType * 15 + pose : -1;
	eq.armArmor  = (armType != 0)   ? BERK + armType * 15 + pose : -1;
	eq.pants     = (pantsType != 0) ? LEGG + pantsType * 15 + pose : -1;
	eq.boots     = (bootsType != 0) ? BOOT + bootsType * 15 + pose : -1;
	eq.mantle    = (mantleType != 0) ? MANTLE + mantleType * 15 + pose : -1;
	eq.helm      = (helmType != 0)  ? HEAD + helmType * 15 + pose : -1;

	// Weapon/shield use direction in frame calculation
	eq.weapon = (weaponType != 0) ? WEAPON + weaponType * 64 + 8 * pose + (state.m_iDir - 1) : -1;
	eq.shield = (shieldType != 0) ? SHIELD + shieldType * 8 + pose : -1;

	// Female skirt check (pants type 1)
	eq.skirtDraw = isFemale && (pantsType == 1);
}

SpriteLib::BoundRect CGame::DrawObject_OnMove_ForMenu(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	// Extract equipment colors from packed appearance color
	MenuCharEquipment eq = {};
	eq.weaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
	eq.shieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
	eq.armorColor  = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
	eq.mantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
	eq.armColor    = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
	eq.pantsColor  = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
	eq.bootsColor  = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
	eq.helmColor   = (m_entityState.m_iApprColor & 0x0000000F);

	// Calculate equipment indices based on character type
	bool isMob = false;
	switch (m_entityState.m_sOwnerType) {
	case 1: case 2: case 3:  // Male
		CalcHumanEquipment(m_entityState, false, eq);
		break;
	case 4: case 5: case 6:  // Female
		CalcHumanEquipment(m_entityState, true, eq);
		break;
	default:  // Mob/NPC
		eq.body = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (1 * 8);
		eq.undies = eq.hair = eq.bodyArmor = eq.armArmor = -1;
		eq.boots = eq.pants = eq.weapon = eq.shield = eq.helm = eq.mantle = -1;
		isMob = true;
		break;
	}
	// Helper lambdas for drawing with optional color tint
	int dirFrame = (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame;
	int hairColor = (m_entityState.m_sAppr1 & 0x00F0) >> 4;
	int iR, iG, iB;

	auto drawEquipment = [&](int idx, int color) {
		if (idx == -1) return;
		if (color == 0)
			m_pSprite[idx]->Draw(sX, sY, dirFrame);
		else
			m_pSprite[idx]->Draw(sX, sY, dirFrame, SpriteLib::DrawParams::Tint(GameColors::Items[color].r - GameColors::Base.r, GameColors::Items[color].g - GameColors::Base.g, GameColors::Items[color].b - GameColors::Base.b));
	};

	auto drawWeapon = [&]() {
		if (eq.weapon == -1) return;
		if (eq.weaponColor == 0)
			m_pSprite[eq.weapon]->Draw(sX, sY, m_entityState.m_iFrame);
		else
			m_pSprite[eq.weapon]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[eq.weaponColor].r - GameColors::Base.r, GameColors::Weapons[eq.weaponColor].g - GameColors::Base.g, GameColors::Weapons[eq.weaponColor].b - GameColors::Base.b));
	};

	auto drawMantle = [&](int order) {
		if (eq.mantle != -1 && _cMantleDrawingOrder[m_entityState.m_iDir] == order)
			drawEquipment(eq.mantle, eq.mantleColor);
	};

	// Check if mob type should skip shadow
	auto shouldSkipShadow = [&]() {
		switch (m_entityState.m_sOwnerType) {
		case 10: case 35: case 50: case 51: case 60: case 65: case 81: case 91:
			return true;
		default:
			return false;
		}
	};

	// Draw body shadow
	if (!shouldSkipShadow() && ConfigManager::Get().GetDetailLevel() != 0 && !isMob)
		m_pSprite[eq.body + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());

	// Draw weapon first if drawing order is 1
	if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		drawWeapon();

	// Draw body
	if (isMob)
		m_pSprite[eq.body + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
	else
		m_pSprite[eq.body + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);

	// Draw equipment layers (back-to-front order)
	drawMantle(0);  // Mantle behind body
	drawEquipment(eq.undies, 0);

	// Hair (only if no helm)
	if (eq.hair != -1 && eq.helm == -1)
	{
		_GetHairColorRGB(hairColor, &iR, &iG, &iB);
		m_pSprite[eq.hair]->Draw(sX, sY, dirFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
	}

	// Boots before pants if wearing skirt
	if (eq.skirtDraw)
		drawEquipment(eq.boots, eq.bootsColor);

	drawEquipment(eq.pants, eq.pantsColor);
	drawEquipment(eq.armArmor, eq.armColor);

	// Boots after pants if not wearing skirt
	if (!eq.skirtDraw)
		drawEquipment(eq.boots, eq.bootsColor);

	drawEquipment(eq.bodyArmor, eq.armorColor);
	drawEquipment(eq.helm, eq.helmColor);
	drawMantle(2);  // Mantle over armor
	drawEquipment(eq.shield, eq.shieldColor);
	drawMantle(1);  // Mantle in front

	// Draw weapon last if drawing order is not 1
	if (_cDrawingOrder[m_entityState.m_iDir] != 1)
		drawWeapon();

	// Chat message
	if (m_entityState.m_iChatIndex != 0)
	{
		if (m_pChatMsgList[m_entityState.m_iChatIndex] != 0)
			DrawChatMsgBox(sX, sY, m_entityState.m_iChatIndex, false);
		else
			m_pMapData->ClearChatMsg(indexX, indexY);
	}

	m_entityState.m_iMoveOffsetX = 0;
	m_entityState.m_iMoveOffsetY = 0;
	return m_pSprite[eq.body + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CGame::DrawObject_OnStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	int iBodyIndex, iUndiesIndex, iHairIndex, iBodyArmorIndex, iArmArmorIndex, iPantsIndex, iBootsIndex, iHelmIndex, iR, iG, iB;
	int iWeaponIndex, iShieldIndex, iMantleIndex;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};

	// Apply motion offset if entity is still interpolating (animation finished before movement)
	sX += static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetX);
	sY += static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetY);

	if (m_entityState.m_sOwnerType == 35 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/ || m_entityState.m_sOwnerType == 81) bInv = true; //Energy-Ball, Wyvern
	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}

	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}

	// CLEROTH - Single-direction monsters
	switch (m_entityState.m_sOwnerType) {
	case 110: // Air Elemental
		m_entityState.m_iDir = 1; // North
		break;
	case 91: // Snoopy: Gate
		if (m_entityState.m_iDir <= 3) m_entityState.m_iDir = 3;
		else  m_entityState.m_iDir = 5;
		break;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		m_entityState.m_iFrame = m_entityState.m_iFrame / 2;
		if ((m_entityState.m_sAppr2 & 0xF000) != 0)
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (1 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 1;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 1;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 1;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 1;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 1;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 1;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 1 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 1;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 1;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 1;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (0 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15;
			iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 0 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 0;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 0;
		}
		break;

	case 4:
	case 5:
	case 6:
		m_entityState.m_iFrame = m_entityState.m_iFrame / 2;
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		if ((m_entityState.m_sAppr2 & 0xF000) != 0)
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (1 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 1;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 1;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 1;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 1;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 1;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 1;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 1 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 1;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 1;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 1;
		}
		else
		{
			iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (0 * 8);
			iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15;
			iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15;
			if ((m_entityState.m_sAppr4 & 0x80) == 0)
			{
				if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
					iBodyArmorIndex = -1;
				else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15;
			}
			if ((m_entityState.m_sAppr3 & 0x000F) == 0)
				iArmArmorIndex = -1;
			else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15;
			if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
				iPantsIndex = -1;
			else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15;
			if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
				iBootsIndex = -1;
			else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15;
			if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
				iWeaponIndex = -1;
			else iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 0 + (m_entityState.m_iDir - 1);
			if ((m_entityState.m_sAppr2 & 0x000F) == 0)
				iShieldIndex = -1;
			else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 0;
			if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
				iMantleIndex = -1;
			else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15;
			if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
				iHelmIndex = -1;
			else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 0;
		}
		break;
	default:
		if (m_entityState.m_sAppr2 != 0)
		{
			iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
			m_entityState.m_iFrame = (m_entityState.m_sAppr2 & 0x00FF) - 1;
		}
		/*	else if (m_entityState.m_sOwnerType == 66) iBodyIndex =  DEF_SPRID_MOB  +  (m_entityState.m_sOwnerType - 10 )*8*7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 73) iBodyIndex =  DEF_SPRID_MOB  +  (m_entityState.m_sOwnerType - 10 )*8*7 + (0 * 8);
			else if (m_entityState.m_sOwnerType == 81) iBodyIndex =  DEF_SPRID_MOB  +  (m_entityState.m_sOwnerType - 10 )*8*7 + (0 * 8);*/
		else iBodyIndex = DEF_SPRID_MOB + (m_entityState.m_sOwnerType - 10) * 8 * 7 + (0 * 8);
		iUndiesIndex = -1;
		iHairIndex = -1;
		iBodyArmorIndex = -1;
		iArmArmorIndex = -1;
		iBootsIndex = -1;
		iPantsIndex = -1;
		iWeaponIndex = -1;
		iShieldIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	if (m_bIsCrusadeMode) DrawObjectFOE(sX, sY, m_entityState.m_iFrame);
	switch (m_entityState.m_sOwnerType) { // hum? la lumiere en dessous ?
	case 15: // ShopKeeper
	case 19: // Gandalf
	case 20: // Howard
	case 24: // Tom
	case 25: // William
	case 26: // Kenedy
	case 51: // CP
	case 86: // HBT
	case 90: // Gail
		m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));
		break;
	}
	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(sX, sY, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}
	if (bTrans == false)
	{
		CheckActiveAura(sX, sY, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));

			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));

			}
			else if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
			}

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0)) {
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}
			if (m_entityState.m_sOwnerType == 35)
				m_pEffectSpr[0]->Draw(sX, sY, 1, SpriteLib::DrawParams::Alpha(0.5f));
			if (m_entityState.m_sOwnerType == 81) // Abaddon
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else if (bInv == true)
			{
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 0))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrder[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(sX, sY, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
		}
		if (m_entityState.m_sOwnerType == 64) // crop
		{
			switch (m_entityState.m_iFrame) {
			case 0: // color effect for crop
				m_pEffectSpr[84]->Draw(sX + 52, sY + 54, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f));
				break;
			case 1: // color effect for crop
				m_pEffectSpr[83]->Draw(sX + 53, sY + 59, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f));
				break;
			case 2: // color effect for crop
				m_pEffectSpr[82]->Draw(sX + 53, sY + 65, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f));
				break;
			}
		}
		// Berserk
		if ((m_entityState.m_iStatus & 0x20) != 0)
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX, sY, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(40 + (m_entityState.m_iDir - 1), sX + 20, sY - 20, m_entityState.m_iFrame % 4, dwTime);
		CheckActiveAura2(sX, sY, dwTime, m_entityState.m_sOwnerType);

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(sX, sY, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(sX, sY, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}

	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID)) {
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = sX;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	// Snoopy: Abaddon effects
	if (m_entityState.m_sOwnerType == 81)
	{
		int randFrame = m_entityState.m_iFrame % 12;
		m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		switch (m_entityState.m_iDir) {
		case 1:
			m_pEffectSpr[153]->Draw(sX, sY + 108, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 50, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 2:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 70, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 3:
			m_pEffectSpr[153]->Draw(sX, sY + 105, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 90, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 4:
			m_pEffectSpr[153]->Draw(sX - 35, sY + 100, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 80, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 5:
			m_pEffectSpr[153]->Draw(sX, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 65, sY - 5, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 6:
			m_pEffectSpr[153]->Draw(sX + 45, sY + 95, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 31, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 7:
			m_pEffectSpr[153]->Draw(sX + 40, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 30, sY + 10, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 8:
			m_pEffectSpr[153]->Draw(sX + 20, sY + 110, m_entityState.m_iEffectFrame % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_pEffectSpr[164]->Draw(sX - 20, sY + 16, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		}
	}
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}

void CGame::_ReadMapData(short sPivotX, short sPivotY, const char* pData)
{
	int i;
	const char* cp;
	char ucHeader, cDir, cName[12], cItemColor;
	short sTotal, sX, sY, sType, sAppr1, sAppr2, sAppr3, sAppr4, sDynamicObjectType;
	int iStatus;
	int iApprColor;
	uint16_t wObjectID;
	uint16_t wDynamicObjectID;
	short sItemID;
	uint32_t dwItemAttr;

	cp = pData;
	m_sVDL_X = sPivotX; // Valid Data Loc-X
	m_sVDL_Y = sPivotY;

	const auto* mapHeader = hb::net::PacketCast<hb::net::PacketMapDataHeader>(cp, sizeof(hb::net::PacketMapDataHeader));
	if (!mapHeader) return;
	sTotal = mapHeader->total;
	cp += sizeof(hb::net::PacketMapDataHeader);
	for (i = 1; i <= sTotal; i++)
	{
		const auto* entry = hb::net::PacketCast<hb::net::PacketMapDataEntryHeader>(cp, sizeof(hb::net::PacketMapDataEntryHeader));
		if (!entry) return;
		sX = entry->x;
		sY = entry->y;
		ucHeader = entry->flags;
		cp += sizeof(hb::net::PacketMapDataEntryHeader);
		if (ucHeader & 0x01) // object ID
		{
			const auto* objBase = hb::net::PacketCast<hb::net::PacketMapDataObjectBase>(cp, sizeof(hb::net::PacketMapDataObjectBase));
			if (!objBase) return;
			if (objBase->object_id < 10000)
			{
				const auto* obj = hb::net::PacketCast<hb::net::PacketMapDataObjectPlayer>(cp, sizeof(hb::net::PacketMapDataObjectPlayer));
				if (!obj) return;
				wObjectID = obj->base.object_id;
				sType = obj->base.type;
				cDir = static_cast<char>(obj->base.dir);
				sAppr1 = obj->appr1;
				sAppr2 = obj->appr2;
				sAppr3 = obj->appr3;
				sAppr4 = obj->appr4;
				iApprColor = obj->appr_color;
				iStatus = obj->status;
				std::memset(cName, 0, sizeof(cName));
				memcpy(cName, obj->name, sizeof(obj->name));
				cp += sizeof(hb::net::PacketMapDataObjectPlayer);
			}
			else // NPC
			{
				const auto* obj = hb::net::PacketCast<hb::net::PacketMapDataObjectNpc>(cp, sizeof(hb::net::PacketMapDataObjectNpc));
				if (!obj) return;
				wObjectID = obj->base.object_id;
				sType = obj->base.type;
				cDir = static_cast<char>(obj->base.dir);
				sAppr1 = sAppr3 = sAppr4 = 0;
				sAppr2 = obj->appr2;
				iApprColor = 0;
				iStatus = obj->status;
				std::memset(cName, 0, sizeof(cName));
				memcpy(cName, obj->name, sizeof(obj->name));
				cp += sizeof(hb::net::PacketMapDataObjectNpc);
			}
			m_pMapData->bSetOwner(wObjectID, sPivotX + sX, sPivotY + sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, DEF_OBJECTSTOP, 0, 0, 0);
		}
		if (ucHeader & 0x02) // object ID
		{
			const auto* objBase = hb::net::PacketCast<hb::net::PacketMapDataObjectBase>(cp, sizeof(hb::net::PacketMapDataObjectBase));
			if (!objBase) return;
			if (objBase->object_id < 10000)
			{
				const auto* obj = hb::net::PacketCast<hb::net::PacketMapDataObjectPlayer>(cp, sizeof(hb::net::PacketMapDataObjectPlayer));
				if (!obj) return;
				wObjectID = obj->base.object_id;
				sType = obj->base.type;
				cDir = static_cast<char>(obj->base.dir);
				sAppr1 = obj->appr1;
				sAppr2 = obj->appr2;
				sAppr3 = obj->appr3;
				sAppr4 = obj->appr4;
				iApprColor = obj->appr_color;
				iStatus = obj->status;
				std::memset(cName, 0, sizeof(cName));
				memcpy(cName, obj->name, sizeof(obj->name));
				cp += sizeof(hb::net::PacketMapDataObjectPlayer);
			}
			else // NPC
			{
				const auto* obj = hb::net::PacketCast<hb::net::PacketMapDataObjectNpc>(cp, sizeof(hb::net::PacketMapDataObjectNpc));
				if (!obj) return;
				wObjectID = obj->base.object_id;
				sType = obj->base.type;
				cDir = static_cast<char>(obj->base.dir);
				sAppr1 = sAppr3 = sAppr4 = 0;
				sAppr2 = obj->appr2;
				iApprColor = 0;
				iStatus = obj->status;
				std::memset(cName, 0, sizeof(cName));
				memcpy(cName, obj->name, sizeof(obj->name));
				cp += sizeof(hb::net::PacketMapDataObjectNpc);
			}
			m_pMapData->bSetDeadOwner(wObjectID, sPivotX + sX, sPivotY + sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName);
		}
		if (ucHeader & 0x04)
		{
			const auto* item = hb::net::PacketCast<hb::net::PacketMapDataItem>(cp, sizeof(hb::net::PacketMapDataItem));
			if (!item) return;
			sItemID = item->item_id;
			cItemColor = static_cast<char>(item->color);
			dwItemAttr = item->attribute;
			cp += sizeof(hb::net::PacketMapDataItem);
			m_pMapData->bSetItem(sPivotX + sX, sPivotY + sY, sItemID, cItemColor, dwItemAttr, false);
		}
		if (ucHeader & 0x08) // Dynamic object
		{
			const auto* dyn = hb::net::PacketCast<hb::net::PacketMapDataDynamicObject>(cp, sizeof(hb::net::PacketMapDataDynamicObject));
			if (!dyn) return;
			wDynamicObjectID = dyn->object_id;
			sDynamicObjectType = dyn->type;
			cp += sizeof(hb::net::PacketMapDataDynamicObject);
			m_pMapData->bSetDynamicObject(sPivotX + sX, sPivotY + sY, wDynamicObjectID, sDynamicObjectType, false);
		}
	}
}

void CGame::LogEventHandler(char* pData)
{
	WORD wEventType, wObjectID;
	short sX, sY, sType, sAppr1, sAppr2, sAppr3, sAppr4;
	int iStatus;
	char cDir, cName[12];
	int iApprColor = 0;

	const auto* base = hb::net::PacketCast<hb::net::PacketEventLogBase>(pData, sizeof(hb::net::PacketEventLogBase));
	if (!base) return;
	wEventType = base->header.msg_type;
	wObjectID = base->object_id;
	sX = base->x;
	sY = base->y;
	sType = base->type;
	cDir = static_cast<char>(base->dir);
	std::memset(cName, 0, sizeof(cName));
	if (wObjectID < 10000)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketEventLogPlayer>(pData, sizeof(hb::net::PacketEventLogPlayer));
		if (!pkt) return;
		memcpy(cName, pkt->name, sizeof(pkt->name));
		sAppr1 = pkt->appr1;
		sAppr2 = pkt->appr2;
		sAppr3 = pkt->appr3;
		sAppr4 = pkt->appr4;
		iApprColor = pkt->appr_color;
		iStatus = pkt->status;
	}
	else 	// NPC
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketEventLogNpc>(pData, sizeof(hb::net::PacketEventLogNpc));
		if (!pkt) return;
		memcpy(cName, pkt->name, sizeof(pkt->name));
		sAppr1 = sAppr3 = sAppr4 = 0;
		sAppr2 = pkt->appr2;
		iStatus = pkt->status;
	}

	switch (wEventType) {
	case DEF_MSGTYPE_CONFIRM:
		m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, DEF_OBJECTSTOP, 0, 0, 0);
		switch (sType) {
		case 43: // LWB
		case 44: // GHK
		case 45: // GHKABS
		case 46: // TK
		case 47: // BG
			m_pEffectManager->AddEffect(EffectType::WHITE_HALO, (sX) * 32, (sY) * 32, 0, 0, 0);
			break;
		}
		break;

	case DEF_MSGTYPE_REJECT:
		m_pMapData->bSetOwner(wObjectID, -1, -1, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, DEF_OBJECTSTOP, 0, 0, 0);
		break;
	}

	_RemoveChatMsgListByObjectID(wObjectID);
}

// MODERNIZED: No longer a window message handler - polls socket directly
// MODERNIZED: v4 Networking Architecture (Drain -> Queue -> Process) for Login Socket
void CGame::OnLogSocketEvent()
{
    if (m_pLSock == 0) return;

    // 1. Check for socket state changes (Connect, Close, Error)
    int iRet = m_pLSock->Poll();

    switch (iRet) {
    case DEF_XSOCKEVENT_SOCKETCLOSED:
        ChangeGameMode(GameMode::ConnectionLost);
        m_pLSock.reset();
        m_pLSock.reset();
        return;
    case DEF_XSOCKEVENT_SOCKETERROR:
        printf("[ERROR] Login socket error\n");
        ChangeGameMode(GameMode::ConnectionLost);
        m_pLSock.reset();
        m_pLSock.reset();
        return;

    case DEF_XSOCKEVENT_CONNECTIONESTABLISH:
        ConnectionEstablishHandler(DEF_SERVERTYPE_LOG);
        break;
    }

    // 2. Drain all available data from TCP buffer to the Queue
    // Only drain if socket is connected (m_bIsAvailable is set on FD_CONNECT)
    if (!m_pLSock->m_bIsAvailable) {
        return; // Still connecting, don't try to read yet
    }

    int iDrained = m_pLSock->DrainToQueue();

    if (iDrained < 0) {
        printf("[ERROR] Login socket DrainToQueue failed: %d\n", iDrained);
        ChangeGameMode(GameMode::ConnectionLost);
        m_pLSock.reset();
        m_pLSock.reset();
        return;
    }

    // 3. Process the queue with a Time Budget
    constexpr int MAX_PACKETS_PER_FRAME = 120; // Safety limit
    constexpr uint32_t MAX_TIME_MS = 3;        // 3ms budget for network processing

    uint32_t dwStartTime = GameClock::GetTimeMS();
    int iProcessed = 0;

    NetworkPacket packet;
    while (iProcessed < MAX_PACKETS_PER_FRAME) {

        // Check budget
        if (GameClock::GetTimeMS() - dwStartTime > MAX_TIME_MS) {
            break;
        }

        // Peek next packet
        if (!m_pLSock->PeekPacket(packet)) {
            break; // Queue empty
        }

        // Update timestamps
        m_dwLastNetRecvTime = GameClock::GetTimeMS();

        // Process (using the pointer directly from the packet vector)
        if (!packet.empty()) {
             LogRecvMsgHandler(const_cast<char*>(packet.ptr()));
        }

        // CRITICAL FIX: The handler might have closed/deleted the socket!
        if (m_pLSock == nullptr) return;

        // Pop logic (remove from queue)
        m_pLSock->PopPacket();
        iProcessed++;
    }
}

void CGame::LogResponseHandler(char* pData)
{
	WORD wResponse;
	char cCharName[12];
	int i;

	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(pData, sizeof(hb::net::PacketHeader));
	if (!header) {
		printf("[ERROR] LogResponseHandler - invalid packet header\n");
		return;
	}
	wResponse = header->msg_type;

	switch (wResponse) {
	case DEF_LOGRESMSGTYPE_CHARACTERDELETED:
	{
		const auto* list = hb::net::PacketCast<hb::net::PacketLogCharacterListHeader>(
			pData, sizeof(hb::net::PacketLogCharacterListHeader));
		if (!list) return;
		m_iTotalChar = list->total_chars;
		for (i = 0; i < 4; i++)
			if (m_pCharList[i] != 0)
			{
				m_pCharList[i].reset();
			}

		const auto* entries = reinterpret_cast<const hb::net::PacketLogCharacterEntry*>(
			pData + sizeof(hb::net::PacketLogCharacterListHeader));
		for (i = 0; i < m_iTotalChar; i++) {
			const auto& entry = entries[i];
			m_pCharList[i] = std::make_unique<CCharInfo>();
			memcpy(m_pCharList[i]->m_cName, entry.name, sizeof(entry.name));
			m_pCharList[i]->m_sAppr1 = entry.appr1;
			m_pCharList[i]->m_sAppr2 = entry.appr2;
			m_pCharList[i]->m_sAppr3 = entry.appr3;
			m_pCharList[i]->m_sAppr4 = entry.appr4;
			m_pCharList[i]->m_sSex = entry.sex;
			m_pCharList[i]->m_sSkinCol = entry.skin;
			m_pCharList[i]->m_sLevel = entry.level;
			m_pCharList[i]->m_iExp = entry.exp;
			m_pCharList[i]->m_iApprColor = entry.appr_color;
			std::memset(m_pCharList[i]->m_cMapName, 0, sizeof(m_pCharList[i]->m_cMapName));
			memcpy(m_pCharList[i]->m_cMapName, entry.map_name, sizeof(entry.map_name));
		}
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "3A");
		ChangeGameMode(GameMode::LogResMsg);
	}
	break;

	case DEF_LOGRESMSGTYPE_CONFIRM:
	{
		const auto* list = hb::net::PacketCast<hb::net::PacketLogCharacterListHeader>(
			pData, sizeof(hb::net::PacketLogCharacterListHeader));
		if (!list) {
			printf("[ERROR] LogResponseHandler - CONFIRM packet too small\n");
			return;
		}
		m_iAccntYear = 0;
		m_iAccntMonth = 0;
		m_iAccntDay = 0;
		m_iIpYear = 0;
		m_iIpMonth = 0;
		m_iIpDay = 0;
		m_iTotalChar = list->total_chars;
		for (i = 0; i < 4; i++)
			if (m_pCharList[i] != 0)
			{
				m_pCharList[i].reset();
			}

		const auto* entries = reinterpret_cast<const hb::net::PacketLogCharacterEntry*>(
			pData + sizeof(hb::net::PacketLogCharacterListHeader));
		for (i = 0; i < m_iTotalChar; i++)
		{
			const auto& entry = entries[i];
			m_pCharList[i] = std::make_unique<CCharInfo>();
			memcpy(m_pCharList[i]->m_cName, entry.name, sizeof(entry.name));
			m_pCharList[i]->m_sAppr1 = entry.appr1;
			m_pCharList[i]->m_sAppr2 = entry.appr2;
			m_pCharList[i]->m_sAppr3 = entry.appr3;
			m_pCharList[i]->m_sAppr4 = entry.appr4;
			m_pCharList[i]->m_sSex = entry.sex;
			m_pCharList[i]->m_sSkinCol = entry.skin;
			m_pCharList[i]->m_sLevel = entry.level;
			m_pCharList[i]->m_iExp = entry.exp;
			m_pCharList[i]->m_iApprColor = entry.appr_color;
			std::memset(m_pCharList[i]->m_cMapName, 0, sizeof(m_pCharList[i]->m_cMapName));
			memcpy(m_pCharList[i]->m_cMapName, entry.map_name, sizeof(entry.map_name));
		}
		ChangeGameMode(GameMode::SelectCharacter);
	}
	break;

	case DEF_LOGRESMSGTYPE_REJECT:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketLogResponseReject>(pData, sizeof(hb::net::PacketLogResponseReject));
		if (!pkt) return;
		m_iBlockYear = pkt->block_year;
		m_iBlockMonth = pkt->block_month;
		m_iBlockDay = pkt->block_day;

		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "7H");
		ChangeGameMode(GameMode::LogResMsg);
	}
	break;

	case DEF_LOGRESMSGTYPE_NOTENOUGHPOINT:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "7I");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_ACCOUNTLOCKED:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "7K");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_SERVICENOTAVAILABLE:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "7L");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "6B");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "6C");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_PASSWORDMISMATCH:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "11");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "12");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "54");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "05");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_ALREADYEXISTINGACCOUNT:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "06");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "Not existing character!");
		ChangeGameMode(GameMode::Msg);
		break;

	case DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED:
	{
		const auto* list = hb::net::PacketCast<hb::net::PacketLogNewCharacterCreatedHeader>(
			pData, sizeof(hb::net::PacketLogNewCharacterCreatedHeader));
		if (!list) return;
		std::memset(cCharName, 0, sizeof(cCharName));
		memcpy(cCharName, list->character_name, sizeof(list->character_name));

		m_iTotalChar = list->total_chars;
		for (i = 0; i < 4; i++)
			if (m_pCharList[i] != 0) m_pCharList[i].reset();

		const auto* entries = reinterpret_cast<const hb::net::PacketLogCharacterEntry*>(
			pData + sizeof(hb::net::PacketLogNewCharacterCreatedHeader));
		for (i = 0; i < m_iTotalChar; i++) {
			const auto& entry = entries[i];
			m_pCharList[i] = std::make_unique<CCharInfo>();
			memcpy(m_pCharList[i]->m_cName, entry.name, sizeof(entry.name));
			m_pCharList[i]->m_sAppr1 = entry.appr1;
			m_pCharList[i]->m_sAppr2 = entry.appr2;
			m_pCharList[i]->m_sAppr3 = entry.appr3;
			m_pCharList[i]->m_sAppr4 = entry.appr4;
			m_pCharList[i]->m_sSex = entry.sex;
			m_pCharList[i]->m_sSkinCol = entry.skin;
			m_pCharList[i]->m_sLevel = entry.level;
			m_pCharList[i]->m_iExp = entry.exp;
			m_pCharList[i]->m_iApprColor = entry.appr_color;
			std::memset(m_pCharList[i]->m_cMapName, 0, sizeof(m_pCharList[i]->m_cMapName));
			memcpy(m_pCharList[i]->m_cMapName, entry.map_name, sizeof(entry.map_name));
		}
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "47");
		ChangeGameMode(GameMode::LogResMsg);
	}
	break;

	case DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "28");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "29");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_ENTERGAMERESTYPE_PLAYING:
		ChangeGameMode(GameMode::QueryForceLogin);
		break;

	case DEF_ENTERGAMERESTYPE_CONFIRM:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketLogEnterGameConfirm>(
			pData, sizeof(hb::net::PacketLogEnterGameConfirm));
		if (!pkt) return;
		int iGameServerPort = pkt->game_server_port;
		char cGameServerAddr[16];
		std::memset(cGameServerAddr, 0, sizeof(cGameServerAddr));
		memcpy(cGameServerAddr, pkt->game_server_addr, sizeof(pkt->game_server_addr));
		std::memset(m_cGameServerName, 0, sizeof(m_cGameServerName));
		memcpy(m_cGameServerName, pkt->game_server_name, sizeof(pkt->game_server_name));
		(void)iGameServerPort;

		m_pGSock = std::make_unique<XSocket>(DEF_SOCKETBLOCKLIMIT);
		m_pGSock->bConnect(m_cLogServerAddr, m_iGameServerPort);
		m_pGSock->bInitBufferSize(30000);
	}
	break;

	case DEF_ENTERGAMERESTYPE_REJECT:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketLogResponseCode>(pData, sizeof(hb::net::PacketLogResponseCode));
		if (!pkt) return;
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		switch (pkt->code) {
		case 1:	strcpy(m_cMsg, "3E"); break;
		case 2:	strcpy(m_cMsg, "3F"); break;
		case 3:	strcpy(m_cMsg, "33"); break;
		case 4: strcpy(m_cMsg, "3D"); break;
		case 5: strcpy(m_cMsg, "3G"); break;
		case 6: strcpy(m_cMsg, "3Z"); break;
		case 7: strcpy(m_cMsg, "3J"); break;
		}
		ChangeGameMode(GameMode::LogResMsg);
	}
	break;

	case DEF_ENTERGAMERESTYPE_FORCEDISCONN:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "3X");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "1Y");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_INPUTKEYCODE:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketLogResponseCode>(pData, sizeof(hb::net::PacketLogResponseCode));
		if (!pkt) return;
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		switch (pkt->code) {
		case 1:	strcpy(m_cMsg, "8U"); break; //MainMenu, Keycode registration success
		case 2:	strcpy(m_cMsg, "82"); break; //MainMenu, Not existing Account
		case 3:	strcpy(m_cMsg, "81"); break; //MainMenu, Password wrong
		case 4: strcpy(m_cMsg, "8V"); break; //MainMenu, Invalid Keycode
		case 5: strcpy(m_cMsg, "8W"); break; //MainMenu, Already Used Keycode
		}
		ChangeGameMode(GameMode::LogResMsg);
	}
	break;

	case DEF_LOGRESMSGTYPE_FORCECHANGEPASSWORD:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "6M");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_INVALIDKOREANSSN:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "1a");
		ChangeGameMode(GameMode::LogResMsg);
		break;

	case DEF_LOGRESMSGTYPE_LESSTHENFIFTEEN:
		std::memset(m_cMsg, 0, sizeof(m_cMsg));
		strcpy(m_cMsg, "1b");
		ChangeGameMode(GameMode::LogResMsg);
		break;
	}
	m_pLSock.reset();
	m_pLSock.reset();
}

void CGame::LogRecvMsgHandler(char* pData)
{
	LogResponseHandler(pData);
}

void CGame::ChangeGameMode(GameMode mode)
{
	// Determine if this mode change should be instant (no fade-out)
	// Instant transitions are used for:
	// - Error states that need immediate feedback
	// - Transitions FROM loading/waiting states that don't need fade-outs
	bool instant = false;

	// Check if TARGET mode should be instant (error states)
	switch (mode)
	{
	case GameMode::ConnectionLost:  // Error - show immediately
	case GameMode::VersionNotMatch: // Error - show immediately
	case GameMode::Msg:             // Error/info - show immediately
	case GameMode::Quit:            // Already fading out visually
		instant = true;
		break;
	default:
		break;
	}

	// Check if CURRENT mode shouldn't have fade-out (loading/waiting states)
	// Use manager's current mode as source of truth
	if (!instant)
	{
		switch (GameModeManager::GetMode())
		{
		case GameMode::Loading:         // Loading screen - no fade needed
		case GameMode::Connecting:      // Waiting screen - no fade needed
		case GameMode::WaitingInitData: // Waiting screen - no fade needed
		case GameMode::WaitingResponse: // Waiting screen - no fade needed
			instant = true;
			break;
		default:
			break;
		}
	}

	// Route to new Screen system or legacy system
	switch (mode) {
	// Full screens
	case GameMode::MainMenu:
		GameModeManager::set_screen<Screen_MainMenu>();
		break;
	case GameMode::Login:
		GameModeManager::set_screen<Screen_Login>();
		break;
	case GameMode::SelectCharacter:
		GameModeManager::set_screen<Screen_SelectCharacter>();
		break;
	case GameMode::CreateNewCharacter:
		GameModeManager::set_screen<Screen_CreateNewCharacter>();
		break;
	case GameMode::CreateNewAccount:
		GameModeManager::set_screen<Screen_CreateAccount>();
		break;
	case GameMode::Quit:
		GameModeManager::set_screen<Screen_Quit>();
		break;
	case GameMode::Loading:
		GameModeManager::set_screen<Screen_Loading>();
		break;

	// Overlays - displayed on top of current base screen
	case GameMode::Connecting:
		GameModeManager::set_overlay<Overlay_Connecting>();
		break;
	case GameMode::WaitingResponse:
		GameModeManager::set_overlay<Overlay_WaitingResponse>();
		break;
	case GameMode::QueryForceLogin:
		GameModeManager::set_overlay<Overlay_QueryForceLogin>();
		break;
	case GameMode::QueryDeleteCharacter:
		GameModeManager::set_overlay<Overlay_QueryDeleteCharacter>();
		break;
	case GameMode::LogResMsg:
		GameModeManager::set_overlay<Overlay_LogResMsg>();
		break;
	case GameMode::ChangePassword:
		GameModeManager::set_overlay<Overlay_ChangePassword>();
		break;
	case GameMode::VersionNotMatch:
		GameModeManager::set_overlay<Overlay_VersionNotMatch>();
		break;
	case GameMode::ConnectionLost:
		GameModeManager::set_overlay<Overlay_ConnectionLost>();
		break;
	case GameMode::Msg:
		GameModeManager::set_overlay<Overlay_Msg>();
		break;
	case GameMode::WaitingInitData:
		GameModeManager::set_overlay<Overlay_WaitInitData>();
		break;

	case GameMode::MainGame:
		GameModeManager::set_screen<Screen_OnGame>();
		break;

	case GameMode::Null:
		// Null mode signals application exit - just set mode, no screen needed
		GameModeManager::SetCurrentMode(GameMode::Null);
		break;

	default:
		// Unhandled modes - log warning (Introduction, Agreement, InputKeyCode are unused)
		break;
	}
}

void CGame::ReleaseUnusedSprites()
{
	for (auto& [idx, spr] : m_pSprite)
	{
		if (spr->IsLoaded() && !spr->IsInUse())
		{
			if (GameClock::GetTimeMS() - spr->GetLastAccessTime() > 60000)
				spr->Unload();
		}
	}
	for(auto& [idx, spr] : m_pTileSpr)
	{
		if (spr->IsLoaded() && !spr->IsInUse())
		{
			if (GameClock::GetTimeMS() - spr->GetLastAccessTime() > 60000)
				spr->Unload();
		}
	}
	for (auto& [idx, spr] : m_pEffectSpr)
	{
		if (spr->IsLoaded() && !spr->IsInUse())
		{
			if (GameClock::GetTimeMS() - spr->GetLastAccessTime() > 60000)
				spr->Unload();
		}
	}

	// Stale sound buffer release is now handled by AudioManager::Update()
	AudioManager::Get().Update(GameClock::GetTimeMS());
}

void CGame::PutChatScrollList(char* pMsg, char cType)
{
	if (m_pChatScrollList[DEF_MAXCHATSCROLLMSGS - 1] != 0)
	{
		m_pChatScrollList[DEF_MAXCHATSCROLLMSGS - 1].reset();
	}
	for (int i = DEF_MAXCHATSCROLLMSGS - 2; i >= 0; i--)
	{
		m_pChatScrollList[i + 1] = std::move(m_pChatScrollList[i]);
	}
	m_pChatScrollList[0] = std::make_unique<CMsg>(1, pMsg, cType);
}

void CGame::ChatMsgHandler(char* pData)
{
	int i, iObjectID, iLoc;
	short sX, sY;
	char cMsgType, cName[21], cTemp[100], cMsg[100], cTxt1[100], cTxt2[100];
	uint32_t dwTime;
	bool bFlag;

	char cHeadMsg[200];

	dwTime = m_dwCurTime;

	std::memset(cTxt1, 0, sizeof(cTxt1));
	std::memset(cTxt2, 0, sizeof(cTxt2));
	std::memset(cMsg, 0, sizeof(cMsg));

	const auto* pkt = hb::net::PacketCast<hb::net::PacketCommandChatMsgHeader>(
		pData, sizeof(hb::net::PacketCommandChatMsgHeader));
	if (!pkt) return;
	iObjectID = static_cast<int>(pkt->header.msg_type);
	sX = pkt->x;
	sY = pkt->y;
	std::memset(cName, 0, sizeof(cName));
	memcpy(cName, pkt->name, sizeof(pkt->name));
	cMsgType = static_cast<char>(pkt->chat_type);

	if (bCheckExID(cName) == true) return;

	std::memset(cTemp, 0, sizeof(cTemp));
	strcpy(cTemp, pData + sizeof(hb::net::PacketCommandChatMsgHeader));

	if ((cMsgType == 0) || (cMsgType == 2) || (cMsgType == 3))
	{
		if (CMisc::bCheckIMEString(cTemp) == false) return;
	}
	if (!m_bWhisper)
	{
		if (cMsgType == 20) return;
	}
	if (!m_bShout)
	{
		if (cMsgType == 2 || cMsgType == 3) return;
	}

	std::memset(cMsg, 0, sizeof(cMsg));
	wsprintf(cMsg, "%s: %s", cName, cTemp);
	m_Renderer->BeginTextBatch();
	bFlag = false;
	short sCheckByte = 0;
	while (bFlag == false)
	{
		int msgLen = static_cast<int>(strlen(cMsg));
		iLoc = m_Renderer->GetTextLength(cMsg, 305);

		// GetTextLength returns how many chars fit; if all fit, no wrapping needed
		if (iLoc >= msgLen)
		{
			PutChatScrollList(cMsg, cMsgType);
			bFlag = true;
		}
		else if (iLoc > 0)
		{
			// Count double-byte characters for proper splitting
			for (int i = 0; i < iLoc; i++) if (cMsg[i] < 0) sCheckByte++;

			if ((sCheckByte % 2) == 0)
			{
				std::memset(cTemp, 0, sizeof(cTemp));
				memcpy(cTemp, cMsg, iLoc);
				PutChatScrollList(cTemp, cMsgType);
				std::memset(cTemp, 0, sizeof(cTemp));
				strcpy(cTemp, cMsg + iLoc);
				std::memset(cMsg, 0, sizeof(cMsg));
				strcpy(cMsg, " ");
				strcat(cMsg, cTemp);
			}
			else
			{
				std::memset(cTemp, 0, sizeof(cTemp));
				memcpy(cTemp, cMsg, iLoc + 1);
				PutChatScrollList(cTemp, cMsgType);
				std::memset(cTemp, 0, sizeof(cTemp));
				strcpy(cTemp, cMsg + iLoc + 1);
				std::memset(cMsg, 0, sizeof(cMsg));
				strcpy(cMsg, " ");
				strcat(cMsg, cTemp);
			}
		}
		else
		{
			// Edge case: even a single char doesn't fit, add anyway to avoid infinite loop
			PutChatScrollList(cMsg, cMsgType);
			bFlag = true;
		}
	}

	m_Renderer->EndTextBatch();

	_RemoveChatMsgListByObjectID(iObjectID);

	const char* cp = pData + sizeof(hb::net::PacketCommandChatMsgHeader);
	for (i = 1; i < DEF_MAXCHATMSGS; i++)
		if (m_pChatMsgList[i] == 0) {
			m_pChatMsgList[i] = std::make_unique<CMsg>(1, (char*)(cp), dwTime);
			m_pChatMsgList[i]->m_iObjectID = iObjectID;

			if (m_pMapData->bSetChatMsgOwner(iObjectID, sX, sY, i) == false) {
				m_pChatMsgList[i].reset();
			}

			if ((cMsgType != 0) && (m_dialogBoxManager.IsEnabled(DialogBoxId::ChatHistory) != true)) {
				std::memset(cHeadMsg, 0, sizeof(cHeadMsg));
				wsprintf(cHeadMsg, "%s:%s", cName, cp);
				AddEventList(cHeadMsg, cMsgType);
			}
			return;
		}
}

void CGame::ReleaseTimeoverChatMsg()
{
	uint32_t dwTime = GameClock::GetTimeMS();
	for (int i = 1; i < DEF_MAXCHATMSGS; i++)
		if (m_pChatMsgList[i] != 0) {

			if ((m_pChatMsgList[i]->m_cType >= 1) && (m_pChatMsgList[i]->m_cType <= 20)) {
				if ((dwTime - m_pChatMsgList[i]->m_dwTime) > DEF_CHATTIMEOUT_A) {
					m_pChatMsgList[i].reset();
				}
			}
			else
				if ((m_pChatMsgList[i]->m_cType >= 21) && (m_pChatMsgList[i]->m_cType <= 40)) {
					if ((dwTime - m_pChatMsgList[i]->m_dwTime) > DEF_CHATTIMEOUT_B) {
						m_pChatMsgList[i].reset();
					}
				}
				else
					if ((m_pChatMsgList[i]->m_cType >= 41) && (m_pChatMsgList[i]->m_cType <= 60)) {
						if ((dwTime - m_pChatMsgList[i]->m_dwTime) > DEF_CHATTIMEOUT_C) {
							m_pChatMsgList[i].reset();
						}
					}
					else if ((dwTime - m_pChatMsgList[i]->m_dwTime) > DEF_CHATTIMEOUT_A) {
						m_pChatMsgList[i].reset();
					}
		}
}

void CGame::DrawBackground(short sDivX, short sModX, short sDivY, short sModY)
{
	if (sDivX < 0 || sDivY < 0) return;

	// Tile-based loop constants
	constexpr int TILE_SIZE = 32;
	const int visibleTilesX = (LOGICAL_WIDTH / TILE_SIZE) + 2;   // +2 for partial tiles on edges
	const int visibleTilesY = (LOGICAL_HEIGHT / TILE_SIZE) + 2;

	// Map pivot for tile array access
	const short pivotX = m_pMapData->m_sPivotX;
	const short pivotY = m_pMapData->m_sPivotY;

	// Draw tiles directly to backbuffer (no caching)
	for (int tileY = 0; tileY < visibleTilesY; tileY++)
	{
		int indexY = sDivY + pivotY + tileY;
		int iy = tileY * TILE_SIZE - sModY;

		for (int tileX = 0; tileX < visibleTilesX; tileX++)
		{
			int indexX = sDivX + pivotX + tileX;
			int ix = tileX * TILE_SIZE - sModX;

			// Bounds check for tile array (752x752)
			if (indexX >= 0 && indexX < 752 && indexY >= 0 && indexY < 752)
			{
				short sSpr = m_pMapData->m_tile[indexX][indexY].m_sTileSprite;
				short sSprFrame = m_pMapData->m_tile[indexX][indexY].m_sTileSpriteFrame;
				m_pTileSpr[sSpr]->Draw(ix - 16, iy - 16, sSprFrame, SpriteLib::DrawParams::NoColorKey());
			}
		}
	}

	if (m_bIsCrusadeMode)
	{
		if (m_pPlayer->m_iConstructLocX != -1) DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_CRUSADE, m_pPlayer->m_iConstructLocX * 32 - m_Camera.GetX(), m_pPlayer->m_iConstructLocY * 32 - m_Camera.GetY(), 41);
		if (m_iTeleportLocX != -1) DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_CRUSADE, m_iTeleportLocX * 32 - m_Camera.GetX(), m_iTeleportLocY * 32 - m_Camera.GetY(), 42);
	}
}

SpriteLib::BoundRect CGame::DrawObject_OnRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision)
{
	int dx, dy;
	int iBodyIndex, iHairIndex, iUndiesIndex, iArmArmorIndex, iBodyArmorIndex, iPantsIndex, iBootsIndex, iWeaponIndex, iShieldIndex, iHelmIndex, iR, iG, iB, iMantleIndex;
	bool bInv = false;
	int iWeaponGlare, iShieldGlare;
	int iWeaponColor, iShieldColor, iArmorColor, iMantleColor, iArmColor, iPantsColor, iBootsColor, iHelmColor;
	int iSkirtDraw = 0;
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};

	if (m_entityState.m_sOwnerType == 35 /*|| m_entityState.m_sOwnerType == 73 || m_entityState.m_sOwnerType == 66*/) bInv = true; //Energy-Ball,Wyvern

	if (ConfigManager::Get().GetDetailLevel() == 0)
	{
		iWeaponColor = 0;
		iShieldColor = 0;
		iArmorColor = 0;
		iMantleColor = 0;
		iArmColor = 0;
		iPantsColor = 0;
		iBootsColor = 0;
		iHelmColor = 0;
	}
	else
	{
		iWeaponColor = (m_entityState.m_iApprColor & 0xF0000000) >> 28;
		iShieldColor = (m_entityState.m_iApprColor & 0x0F000000) >> 24;
		iArmorColor = (m_entityState.m_iApprColor & 0x00F00000) >> 20;
		iMantleColor = (m_entityState.m_iApprColor & 0x000F0000) >> 16;
		iArmColor = (m_entityState.m_iApprColor & 0x0000F000) >> 12;
		iPantsColor = (m_entityState.m_iApprColor & 0x00000F00) >> 8;
		iBootsColor = (m_entityState.m_iApprColor & 0x000000F0) >> 4;
		iHelmColor = (m_entityState.m_iApprColor & 0x0000000F);
	}
	iWeaponGlare = (m_entityState.m_sAppr4 & 0x000C) >> 2;
	iShieldGlare = (m_entityState.m_sAppr4 & 0x0003);
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if (memcmp(m_pPlayer->m_cPlayerName, m_entityState.m_cName.data(), 10) == 0) bInv = true;
		else if (_iGetFOE(m_entityState.m_iStatus) == 1) bInv = true;
		else return invalidRect;
	}

	switch (m_entityState.m_sOwnerType) {
	case 1:
	case 2:
	case 3:
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (4 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_M + (m_entityState.m_sAppr1 & 0x000F) * 15 + 4;
		iHairIndex = DEF_SPRID_HAIR_M + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 4;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_M + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 4;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_M + (m_entityState.m_sAppr3 & 0x000F) * 15 + 4;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_M + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 4;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_M + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 4;
		if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
			iWeaponIndex = -1;
		else
		{
			iWeaponIndex = DEF_SPRID_WEAPON_M + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 6 + (m_entityState.m_iDir - 1);
		}
		if ((m_entityState.m_sAppr2 & 0x000F) == 0)
			iShieldIndex = -1;
		else iShieldIndex = DEF_SPRID_SHIELD_M + (m_entityState.m_sAppr2 & 0x000F) * 8 + 6;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_M + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 4;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_M + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 4;
		break;

	case 4:
	case 5:
	case 6:
		if (((m_entityState.m_sAppr3 & 0x0F00) >> 8) == 1) iSkirtDraw = 1;
		iBodyIndex = 500 + (m_entityState.m_sOwnerType - 1) * 8 * 15 + (4 * 8);
		iUndiesIndex = DEF_SPRID_UNDIES_W + (m_entityState.m_sAppr1 & 0x000F) * 15 + 4;
		iHairIndex = DEF_SPRID_HAIR_W + ((m_entityState.m_sAppr1 & 0x0F00) >> 8) * 15 + 4;
		if ((m_entityState.m_sAppr4 & 0x80) == 0)
		{
			if (((m_entityState.m_sAppr3 & 0xF000) >> 12) == 0)
				iBodyArmorIndex = -1;
			else iBodyArmorIndex = DEF_SPRID_BODYARMOR_W + ((m_entityState.m_sAppr3 & 0xF000) >> 12) * 15 + 4;
		}
		if ((m_entityState.m_sAppr3 & 0x000F) == 0)
			iArmArmorIndex = -1;
		else iArmArmorIndex = DEF_SPRID_BERK_W + (m_entityState.m_sAppr3 & 0x000F) * 15 + 4;
		if ((m_entityState.m_sAppr3 & 0x0F00) == 0)
			iPantsIndex = -1;
		else iPantsIndex = DEF_SPRID_LEGG_W + ((m_entityState.m_sAppr3 & 0x0F00) >> 8) * 15 + 4;
		if (((m_entityState.m_sAppr4 & 0xF000) >> 12) == 0)
			iBootsIndex = -1;
		else iBootsIndex = DEF_SPRID_BOOT_W + ((m_entityState.m_sAppr4 & 0xF000) >> 12) * 15 + 4;
		if (((m_entityState.m_sAppr2 & 0x0FF0) >> 4) == 0)
			iWeaponIndex = -1;
		else
		{
			iWeaponIndex = DEF_SPRID_WEAPON_W + ((m_entityState.m_sAppr2 & 0x0FF0) >> 4) * 64 + 8 * 6 + (m_entityState.m_iDir - 1);
		}
		if ((m_entityState.m_sAppr2 & 0x000F) == 0)
			iShieldIndex = -1;
		else iShieldIndex = DEF_SPRID_SHIELD_W + (m_entityState.m_sAppr2 & 0x000F) * 8 + 6;
		if ((m_entityState.m_sAppr4 & 0x0F00) == 0)
			iMantleIndex = -1;
		else iMantleIndex = DEF_SPRID_MANTLE_W + ((m_entityState.m_sAppr4 & 0x0F00) >> 8) * 15 + 4;
		if ((m_entityState.m_sAppr3 & 0x00F0) == 0)
			iHelmIndex = -1;
		else iHelmIndex = DEF_SPRID_HEAD_W + ((m_entityState.m_sAppr3 & 0x00F0) >> 4) * 15 + 4;
		break;

	default:
		iUndiesIndex = -1;
		iHairIndex = -1;
		iArmArmorIndex = -1;
		iBodyArmorIndex = -1;
		iPantsIndex = -1;
		iBootsIndex = -1;
		iMantleIndex = -1;
		iHelmIndex = -1;
		break;
	}
	// Use smooth interpolated motion offsets
	dx = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetX);
	dy = static_cast<int>(m_pMapData->m_pData[m_entityState.m_iDataX][m_entityState.m_iDataY].m_motion.fCurrentOffsetY);

	// Simple offset - no frame-based adjustments for smooth interpolation
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	if (m_bIsCrusadeMode) DrawObjectFOE(fix_x, fix_y, m_entityState.m_iFrame);

	if (m_entityState.m_iEffectType != 0)
	{
		switch (m_entityState.m_iEffectType) {
		case 1: m_pEffectSpr[26]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Attack Effect
		case 2: m_pEffectSpr[27]->Draw(fix_x, fix_y, m_entityState.m_iEffectFrame, SpriteLib::DrawParams::Alpha(0.5f)); break; // Special Ability: Protect Effect
		}
	}

	if (bTrans == false)
	{
		CheckActiveAura(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);
		if (_cDrawingOrder[m_entityState.m_iDir] == 1)
		{
			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}

			if (bInv == true)
				//m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(sX+dx, sY+dy, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.5f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
			}
			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 0))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f)); // GM effect
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}
		}
		else
		{
			switch (m_entityState.m_sOwnerType) { // Pas d'ombre pour ces mobs
			case 10: // Slime
			case 35: // Energy Sphere
			case 50: // TW
			case 51: // CP
			case 60: // Plant
			case 65: // IceGolem
				//case 66: // Wyvern
				//case 73: // Fire Wyvern
			case 81: // Abaddon
			case 91: // Gate
				break;
			default:
				if (ConfigManager::Get().GetDetailLevel() != 0 && !bInv)
				{
					if (sX < 50)
						m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
					else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Shadow());
				}
				break;
			}

			if (bInv == true)
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
			else
			{
				if ((m_entityState.m_iStatus & 0x40) != 0)
					m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::BlueTintHalf.r, GameColors::BlueTintHalf.g, GameColors::BlueTintHalf.b));
				else m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
			}

			SetRect(&m_rcBodyRect, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().left, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().top,
				m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().right, m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect().bottom);

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 0))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iUndiesIndex != -1)
			{
				if (bInv) m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				m_pSprite[iUndiesIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
			}

			if ((iHairIndex != -1) && (iHelmIndex == -1))
			{
				_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
				m_pSprite[iHairIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(iR, iG, iB));
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 1))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iPantsIndex != -1)
			{
				if (bInv) m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iPantsColor == 0)
						m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iPantsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iPantsColor].r - GameColors::Base.r, GameColors::Items[iPantsColor].g - GameColors::Base.g, GameColors::Items[iPantsColor].b - GameColors::Base.b));
				}
			}

			if (iArmArmorIndex != -1)
			{
				if (bInv) m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmColor == 0)
						m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iArmArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmColor].r - GameColors::Base.r, GameColors::Items[iArmColor].g - GameColors::Base.g, GameColors::Items[iArmColor].b - GameColors::Base.b));
				}
			}

			if ((iBootsIndex != -1) && (iSkirtDraw == 0))
			{
				if (bInv) m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iBootsColor == 0)
						m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBootsIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iBootsColor].r - GameColors::Base.r, GameColors::Items[iBootsColor].g - GameColors::Base.g, GameColors::Items[iBootsColor].b - GameColors::Base.b));
				}
			}

			if (iBodyArmorIndex != -1)
			{
				if (bInv) m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iArmorColor == 0)
						m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iBodyArmorIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iArmorColor].r - GameColors::Base.r, GameColors::Items[iArmorColor].g - GameColors::Base.g, GameColors::Items[iArmorColor].b - GameColors::Base.b));
				}
			}

			if (iHelmIndex != -1)
			{
				if (bInv) m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iHelmColor == 0)
						m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iHelmIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iHelmColor].r - GameColors::Base.r, GameColors::Items[iHelmColor].g - GameColors::Base.g, GameColors::Items[iHelmColor].b - GameColors::Base.b));
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 2))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iShieldIndex != -1)
			{
				if (bInv) m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iShieldColor == 0)
						m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iShieldColor].r - GameColors::Base.r, GameColors::Items[iShieldColor].g - GameColors::Base.g, GameColors::Items[iShieldColor].b - GameColors::Base.b));
				}
				switch (iShieldGlare) {
				case 0: break;
					//case 1: m_pSprite[iShieldIndex]->Draw(sX, sY, (m_entityState.m_iDir-1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 1: m_pEffectSpr[45]->Draw(fix_x - 13, fix_y - 34, 0, SpriteLib::DrawParams::Alpha(0.5f));
				case 2: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iShieldIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}

			if ((iMantleIndex != -1) && (_cMantleDrawingOrderOnRun[m_entityState.m_iDir] == 1))
			{
				if (bInv) m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iMantleColor == 0)
						m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame);
					else m_pSprite[iMantleIndex]->Draw(fix_x, fix_y, (m_entityState.m_iDir - 1) * 8 + m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Items[iMantleColor].r - GameColors::Base.r, GameColors::Items[iMantleColor].g - GameColors::Base.g, GameColors::Items[iMantleColor].b - GameColors::Base.b));
				}
			}

			if (iWeaponIndex != -1)
			{
				if (bInv) m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Alpha(0.25f));
				else
				{
					if (iWeaponColor == 0)
						m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame);
					else m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::Tint(GameColors::Weapons[iWeaponColor].r - GameColors::Base.r, GameColors::Weapons[iWeaponColor].g - GameColors::Base.g, GameColors::Weapons[iWeaponColor].b - GameColors::Base.b));
				}
				DKGlare(iWeaponColor, iWeaponIndex, &iWeaponGlare);
				switch (iWeaponGlare) {
				case 0: break;
				case 1: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(m_iDrawFlag, 0, 0, 0.7f)); break; // Red Glare
				case 2: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, m_iDrawFlag, 0, 0.7f)); break; // Green Glare
				case 3: m_pSprite[iWeaponIndex]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, 0, m_iDrawFlag, 0.7f)); break; // Blue Glare
				}
			}
		}

		if ((m_entityState.m_iStatus & 0x20) != 0) 	// Berserk
			m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(0, -5, -5, 0.7f));
		DrawAngel(40 + (m_entityState.m_iDir - 1), fix_x + 20, fix_y - 20, m_entityState.m_iFrame % 4, dwTime);
		CheckActiveAura2(fix_x, fix_y, dwTime, m_entityState.m_sOwnerType);

		// Centuu : Haste effect
		if ((m_entityState.m_iStatus & 0x40000) != 0) {
			for (int i = 1; i <= 5; i++)
			{
				switch (m_entityState.m_iDir) {
				case 1: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y + (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 2: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x - (i * 5), fix_y + (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 3: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x - (i * 5), fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 4: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x - (i * 5), fix_y - (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 5: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x, fix_y - (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 6: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x + (i * 5), fix_y - (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 7: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x + (i * 5), fix_y, m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				case 8: m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->Draw(fix_x + (i * 5), fix_y + (i * 5), m_entityState.m_iFrame, SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f)); break;
				}
			}
		}

	}
	else if (strlen(m_entityState.m_cName.data()) > 0)
	{
		if ((m_entityState.m_sOwnerType >= 1) && (m_entityState.m_sOwnerType <= 6)) DrawObjectName(fix_x, fix_y, m_entityState.m_cName.data(), m_entityState.m_iStatus);
		else DrawNpcName(fix_x, fix_y, m_entityState.m_sOwnerType, m_entityState.m_iStatus);
	}

	if (m_entityState.m_iChatIndex != 0)
	{
		if ((m_pChatMsgList[m_entityState.m_iChatIndex] != 0) && (m_pChatMsgList[m_entityState.m_iChatIndex]->m_iObjectID == m_entityState.m_wObjectID))
		{
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sX = fix_x;
			m_pChatMsgList[m_entityState.m_iChatIndex]->m_sY = fix_y;
		}
		else
		{
			m_pMapData->ClearChatMsg(indexX, indexY);
		}
	}
	m_entityState.m_iMoveOffsetX = dx;
	m_entityState.m_iMoveOffsetY = dy;
	return m_pSprite[iBodyIndex + (m_entityState.m_iDir - 1)]->GetBoundRect();
}


void CGame::InitItemList(char* pData)
{
	int     i, iAngelValue;
	uint8_t cTotalItems;
	const char* cp;

	for (i = 0; i < DEF_MAXITEMS; i++)
		m_cItemOrder[i] = -1;

	for (i = 0; i < DEF_MAXITEMEQUIPPOS; i++)
		m_sItemEquipmentStatus[i] = -1;

	for (i = 0; i < DEF_MAXITEMS; i++)
		m_bIsItemDisabled[i] = false;

	const auto* header = hb::net::PacketCast<hb::net::PacketResponseItemListHeader>(
		pData, sizeof(hb::net::PacketResponseItemListHeader));
	if (!header) return;
	cTotalItems = header->item_count;
	cp = reinterpret_cast<const char*>(pData) + sizeof(hb::net::PacketResponseItemListHeader);

	for (i = 0; i < DEF_MAXITEMS; i++)
		if (m_pItemList[i] != 0)
		{
			m_pItemList[i].reset();
		}

	for (i = 0; i < DEF_MAXBANKITEMS; i++)
		if (m_pBankList[i] != 0)
		{
			m_pBankList[i].reset();
		}

	const auto* itemEntries = reinterpret_cast<const hb::net::PacketResponseItemListEntry*>(cp);
	for (i = 0; i < cTotalItems; i++)
	{
		const auto& entry = itemEntries[i];
		m_pItemList[i] = std::make_unique<CItem>();
		memcpy(m_pItemList[i]->m_cName, entry.name, 20);
		m_pItemList[i]->m_dwCount = entry.count;
		m_pItemList[i]->m_sX = 40;
		m_pItemList[i]->m_sY = 30;
		m_pItemList[i]->m_cItemType = entry.item_type;
		m_pItemList[i]->m_cEquipPos = entry.equip_pos;
		if (entry.is_equipped == 0) m_bIsItemEquipped[i] = false;
		else m_bIsItemEquipped[i] = true;
		if (m_bIsItemEquipped[i] == true)
		{
			m_sItemEquipmentStatus[m_pItemList[i]->m_cEquipPos] = i;
		}
		m_pItemList[i]->m_sLevelLimit = entry.level_limit;
		m_pItemList[i]->m_cGenderLimit = entry.gender_limit;
		m_pItemList[i]->m_wCurLifeSpan = entry.cur_lifespan;
		m_pItemList[i]->m_wWeight = entry.weight;
		m_pItemList[i]->m_sSprite = entry.sprite;
		m_pItemList[i]->m_sSpriteFrame = entry.sprite_frame;
		m_pItemList[i]->m_cItemColor = entry.item_color;
		m_pItemList[i]->m_sItemSpecEffectValue2 = static_cast<short>(entry.spec_value2); // v1.41
		m_pItemList[i]->m_dwAttribute = entry.attribute;
		m_pItemList[i]->m_sIDnum = entry.item_id;
		m_pItemList[i]->m_wMaxLifeSpan = entry.max_lifespan;
		/*
		m_pItemList[i]->m_bIsCustomMade = (bool)*cp;
		cp++;
		*/
		// Populate display name from the shared item display name map
		m_pItemList[i]->PopulateDisplayName();
		m_cItemOrder[i] = i;
		// Snoopy: Add Angelic Stats
		if ((m_pItemList[i]->m_cItemType == 1)
			&& (m_bIsItemEquipped[i] == true)
			&& (m_pItemList[i]->m_cEquipPos >= 11))
		{
			if (memcmp(m_pItemList[i]->m_cName, "AngelicPandent(STR)", 19) == 0)
			{
				iAngelValue = (m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pPlayer->m_iAngelicStr = 1 + iAngelValue;
			}
			else if (memcmp(m_pItemList[i]->m_cName, "AngelicPandent(DEX)", 19) == 0)
			{
				iAngelValue = (m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pPlayer->m_iAngelicDex = 1 + iAngelValue;
			}
			else if (memcmp(m_pItemList[i]->m_cName, "AngelicPandent(INT)", 19) == 0)
			{
				iAngelValue = (m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pPlayer->m_iAngelicInt = 1 + iAngelValue;
			}
			else if (memcmp(m_pItemList[i]->m_cName, "AngelicPandent(MAG)", 19) == 0)
			{
				iAngelValue = (m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pPlayer->m_iAngelicMag = 1 + iAngelValue;
			}
		}
	}

	cp = reinterpret_cast<const char*>(itemEntries + cTotalItems);
	cTotalItems = static_cast<uint8_t>(*cp);
	cp++;

	for (i = 0; i < DEF_MAXBANKITEMS; i++)
		if (m_pBankList[i] != 0)
		{
			m_pBankList[i].reset();
		}

	const auto* bankEntries = reinterpret_cast<const hb::net::PacketResponseBankItemEntry*>(cp);
	for (i = 0; i < cTotalItems; i++)
	{
		const auto& entry = bankEntries[i];
		m_pBankList[i] = std::make_unique<CItem>();
		memcpy(m_pBankList[i]->m_cName, entry.name, 20);
		m_pBankList[i]->m_dwCount = entry.count;

		m_pBankList[i]->m_sX = 40;
		m_pBankList[i]->m_sY = 30;

		m_pBankList[i]->m_cItemType = entry.item_type;
		m_pBankList[i]->m_cEquipPos = entry.equip_pos;
		m_pBankList[i]->m_sLevelLimit = entry.level_limit;
		m_pBankList[i]->m_cGenderLimit = entry.gender_limit;
		m_pBankList[i]->m_wCurLifeSpan = entry.cur_lifespan;
		m_pBankList[i]->m_wWeight = entry.weight;
		m_pBankList[i]->m_sSprite = entry.sprite;
		m_pBankList[i]->m_sSpriteFrame = entry.sprite_frame;
		m_pBankList[i]->m_cItemColor = entry.item_color;
		m_pBankList[i]->m_sItemSpecEffectValue2 = static_cast<short>(entry.spec_value2); // v1.41
		m_pBankList[i]->m_dwAttribute = entry.attribute;
		m_pBankList[i]->m_sIDnum = entry.item_id;
		m_pBankList[i]->m_wMaxLifeSpan = entry.max_lifespan;
		/*
		m_pBankList[i]->m_bIsCustomMade = (bool)*cp;
		cp++;
		*/
		// Populate display name from the shared item display name map
		m_pBankList[i]->PopulateDisplayName();
	}

	cp = reinterpret_cast<const char*>(bankEntries + cTotalItems);
	// Magic, Skill Mastery
	for (i = 0; i < DEF_MAXMAGICTYPE; i++)
	{
		m_pPlayer->m_iMagicMastery[i] = *cp;
		cp++;
	}

	for (i = 0; i < DEF_MAXSKILLTYPE; i++)
	{
		m_pPlayer->m_iSkillMastery[i] = (unsigned char)*cp;
		if (m_pSkillCfgList[i] != 0)
			m_pSkillCfgList[i]->m_iLevel = (int)*cp;
		cp++;
	}
}

void CGame::DrawDialogBoxs(short msX, short msY, short msZ, char cLB)
{
	int i;
	if (m_bIsObserverMode == true) return;
	// Note: Dialogs that handle scroll should read Input::GetMouseWheelDelta() and clear it after processing
	//Snoopy: 41->61
	bool bIconPanelDrawn = false;
	for (i = 0; i < 61; i++)
		if (m_dialogBoxManager.OrderAt(i) != 0)
		{
			switch (m_dialogBoxManager.OrderAt(i)) {
			case DialogBoxId::CharacterInfo:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CharacterInfo))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Inventory:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Inventory))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Magic:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Magic))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ItemDropConfirm:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ItemDropConfirm))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::WarningBattleArea:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::WarningBattleArea))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::GuildMenu:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::GuildMenu))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::GuildOperation:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::GuildOperation))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::GuideMap:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::GuideMap))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ChatHistory:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ChatHistory))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::SaleMenu:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::SaleMenu))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::LevelUpSetting:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::LevelUpSetting))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::CityHallMenu:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CityHallMenu))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Bank:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Bank))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Skill:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Skill))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::MagicShop:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::MagicShop))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ItemDropExternal:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ItemDropExternal))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Text:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Text))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::SystemMenu:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::SystemMenu))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::NpcActionQuery:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::NpcActionQuery))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::NpcTalk:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::NpcTalk))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Map:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Map))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::SellOrRepair:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::SellOrRepair))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Fishing:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Fishing))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Noticement:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Noticement))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Manufacture:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Manufacture))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Exchange:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Exchange))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Quest:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Quest))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::HudPanel:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::HudPanel))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				bIconPanelDrawn = true;
				break;
			case DialogBoxId::SellList:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::SellList))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Party:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Party))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::CrusadeJob:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CrusadeJob))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ItemUpgrade:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ItemUpgrade))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Help:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Help))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::CrusadeCommander:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CrusadeCommander))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::CrusadeConstructor:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CrusadeConstructor))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::CrusadeSoldier:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::CrusadeSoldier))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Slates:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Slates))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ConfirmExchange:	//Snoopy: Confirmation Exchange
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ConfirmExchange))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::ChangeStatsMajestic:
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::ChangeStatsMajestic))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::Resurrect: // Snoopy: Resurection?
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::Resurrect))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::GuildHallMenu: // Gail
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::GuildHallMenu))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			case DialogBoxId::RepairAll: //50Cent - Repair All
				if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::RepairAll))
					pDlg->OnDraw(msX, msY, msZ, cLB);
				break;
			}
		}
	if (bIconPanelDrawn == false)
	{
		if (auto* pDlg = m_dialogBoxManager.GetDialogBox(DialogBoxId::HudPanel))
			pDlg->OnDraw(msX, msY, msZ, cLB);
	}
	int resy = 0;
	int resx = 0;
	short iconX = m_dialogBoxManager.Info(DialogBoxId::HudPanel).sX;
	short iconY = m_dialogBoxManager.Info(DialogBoxId::HudPanel).sY;
	if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100)
	{
		if (m_pPlayer->m_iSuperAttackLeft > 0)
		{
			if (Input::IsAltDown())
				m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL]->Draw(iconX + 368 + resx + 7, iconY + 440 + resy, 3, SpriteLib::DrawParams::Alpha(0.5f));
			wsprintf(G_cTxt, "%d", m_pPlayer->m_iSuperAttackLeft);
			TextLib::DrawText(GameFont::Bitmap1, iconX + 380 + resx + 10 - 5, iconY + 454 + resy, G_cTxt, TextLib::TextStyle::WithIntegratedShadow(255, 255, 255));
		}
	}
	else
	{
		if (m_pPlayer->m_iSuperAttackLeft > 0)
		{
			wsprintf(G_cTxt, "%d", m_pPlayer->m_iSuperAttackLeft);
			TextLib::DrawText(GameFont::Bitmap1, iconX + 380 + resx + 10 - 5, iconY + 454 + resy, G_cTxt, TextLib::TextStyle::WithHighlight(GameColors::BmpBtnActive.r, GameColors::BmpBtnActive.g, GameColors::BmpBtnActive.b));
		}
	}
}

void CGame::_Draw_CharacterBody(short sX, short sY, short sType)
{
	uint32_t dwTime = m_dwCurTime;
	int  iR, iG, iB;

	if (sType <= 3)
	{
		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0]->Draw(sX, sY, sType - 1);
		_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18]->Draw(sX, sY, (m_entityState.m_sAppr1 & 0x0F00) >> 8, SpriteLib::DrawParams::Tint(iR, iG, iB));

		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19]->Draw(sX, sY, (m_entityState.m_sAppr1 & 0x000F));
	}
	else
	{
		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40]->Draw(sX, sY, sType - 4);
		_GetHairColorRGB(((m_entityState.m_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18 + 40]->Draw(sX, sY, (m_entityState.m_sAppr1 & 0x0F00) >> 8, SpriteLib::DrawParams::Tint(iR, iG, iB));
		m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19 + 40]->Draw(sX, sY, (m_entityState.m_sAppr1 & 0x000F));
	}
}

void CGame::EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char* pString)
{
	int i;
	short sX, sY;

	switch (iBoxID) {
	case DialogBoxId::RepairAll: //50Cent - Repair all
		m_dialogBoxManager.Info(DialogBoxId::RepairAll).cMode = cType;
		break;
	case DialogBoxId::SaleMenu:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::SaleMenu) == false)
		{
			switch (cType) {
			case 0:
				break;
			default:
				// Check if shop items are already loaded (called from ResponseShopContentsHandler)
				if (m_pItemForSaleList[0] != nullptr) {
					// Items already loaded - just show the dialog
					m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV1 = cType;
					m_dialogBoxManager.Info(DialogBoxId::SaleMenu).cMode = 0;
					m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sView = 0;
					m_dialogBoxManager.Info(DialogBoxId::SaleMenu).bFlag = true;
					m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 = 1;
				} else {
					// Request contents from server - dialog will be shown when response arrives
					m_sPendingShopType = cType;
					_LoadShopMenuContents(cType);
				}
				break;
			}
		}
		break;

	case DialogBoxId::LevelUpSetting: // levelup diag
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::LevelUpSetting) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::LevelUpSetting).sX = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sX + 20;
			m_dialogBoxManager.Info(DialogBoxId::LevelUpSetting).sY = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sY + 20;
			m_dialogBoxManager.Info(DialogBoxId::LevelUpSetting).sV1 = m_pPlayer->m_iLU_Point;
		}
		break;

	case DialogBoxId::Magic: // Magic Dialog
		break;

	case DialogBoxId::ItemDropConfirm:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm) == false) {
			m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sView = cType;
		}
		break;

	case DialogBoxId::WarningBattleArea:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::WarningBattleArea) == false) {
			m_dialogBoxManager.Info(DialogBoxId::WarningBattleArea).sView = cType;
		}
		break;

	case DialogBoxId::GuildMenu:
		if (m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1) {
			sX = m_dialogBoxManager.Info(DialogBoxId::GuildMenu).sX;
			sY = m_dialogBoxManager.Info(DialogBoxId::GuildMenu).sY;
			EndInputString();
			StartInputString(sX + 75, sY + 140, 21, m_pPlayer->m_cGuildName);
		}
		break;

	case DialogBoxId::ItemDropExternal: // demande quantit�
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == false)
		{
			m_dialogBoxManager.Info(iBoxID).cMode = 1;
			m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView = cType;
			EndInputString();
			std::memset(m_cAmountString, 0, sizeof(m_cAmountString));
			wsprintf(m_cAmountString, "%d", sV1);
			sX = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sX;
			sY = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY;
			StartInputString(sX + 40, sY + 57, 11, m_cAmountString, false);
		}
		else
		{
			if (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1)
			{
				sX = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sX;
				sY = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY;
				EndInputString();
				StartInputString(sX + 40, sY + 57, 11, m_cAmountString, false);
			}
		}
		break;

	case DialogBoxId::Text:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Text) == false)
		{
			switch (cType) {
			case 0:
				m_dialogBoxManager.Info(DialogBoxId::Text).cMode = 0;
				m_dialogBoxManager.Info(DialogBoxId::Text).sView = 0;
				break;
			default:
				_LoadTextDlgContents(cType);
				m_dialogBoxManager.Info(DialogBoxId::Text).cMode = 0;
				m_dialogBoxManager.Info(DialogBoxId::Text).sView = 0;
				break;
			}
		}
		break;

	case DialogBoxId::SystemMenu:
		break;

	case DialogBoxId::NpcActionQuery: // Talk to npc or unicorn
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV1] = false;
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::NpcActionQuery) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV1 = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV2 = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV3 =
				m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV4 = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV5 = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sV6 = 0;
			m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sView = 0;
			m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV1 = sV1;
			m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV2 = sV2;
		}
		break;

	case DialogBoxId::NpcTalk:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::NpcTalk) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::NpcTalk).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sView = 0;
			m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sV1 = _iLoadTextDlgContents2(sV1 + 20);
			m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sV2 = sV1 + 20;
		}
		break;

	case DialogBoxId::Map:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Map) == false) {
			m_dialogBoxManager.Info(DialogBoxId::Map).sV1 = sV1;
			m_dialogBoxManager.Info(DialogBoxId::Map).sV2 = sV2;

			m_dialogBoxManager.Info(DialogBoxId::Map).sSizeX = 290;
			m_dialogBoxManager.Info(DialogBoxId::Map).sSizeY = 290;
		}
		break;

	case DialogBoxId::SellOrRepair:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) == false) {
			m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sV1 = sV1;		// ItemID
			m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sV2 = sV2;
			if (cType == 2)
			{
				m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sX = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sX;
				m_dialogBoxManager.Info(DialogBoxId::SellOrRepair).sY = m_dialogBoxManager.Info(DialogBoxId::SaleMenu).sY;
			}
		}
		break;

	case DialogBoxId::Skill:
		break;

	case DialogBoxId::Fishing:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Fishing) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::Fishing).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::Fishing).sV1 = sV1;
			m_dialogBoxManager.Info(DialogBoxId::Fishing).sV2 = sV2;
			m_bSkillUsingStatus = true;
		}
		break;

	case DialogBoxId::Noticement:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Noticement) == false) {
			m_dialogBoxManager.Info(DialogBoxId::Noticement).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::Noticement).sV1 = sV1;
			m_dialogBoxManager.Info(DialogBoxId::Noticement).sV2 = sV2;
		}
		break;

	case DialogBoxId::Manufacture:
		switch (cType) {
		case DialogBoxId::CharacterInfo:
		case DialogBoxId::Inventory: //
			if (m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == false)
			{
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cMode = cType;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[0] = 0;
				m_bSkillUsingStatus = true;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeX = 195;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeY = 215;
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			}
			break;

		case DialogBoxId::Magic:	//
			if (m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == false)
			{
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sView = 0;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cMode = cType;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[0] = 0;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[1] = 0;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[4] = 0;
				m_bSkillUsingStatus = true;
				_bCheckBuildItemStatus();
				//m_dialogBoxManager.Info(DialogBoxId::Manufacture).sX = 0;
				//m_dialogBoxManager.Info(DialogBoxId::Manufacture).sY = 0;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeX = 270;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeY = 381;
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			}
			break;

		case DialogBoxId::WarningBattleArea:
			if (m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == false)
			{
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cMode = cType;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[2] = sV1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[3] = sV2;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeX = 270;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeY = 381;
				m_bSkillUsingStatus = true;
				_bCheckBuildItemStatus();
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			}
			break;
			// Crafting
		case DialogBoxId::GuildMenu:
		case DialogBoxId::GuildOperation:
			if (m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == false)
			{
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cMode = cType;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6 = -1;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[0] = 0;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[1] = 0;
				m_bSkillUsingStatus = true;
				//_bCheckCraftItemStatus();
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeX = 195;
				m_dialogBoxManager.Info(DialogBoxId::Manufacture).sSizeY = 215;
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			}
			break;
		}
		break;

	case DialogBoxId::Exchange: // Snoopy: 7 mar 06 (multitrade) case rewriten
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Exchange) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::Exchange).cMode = cType;
			for (i = 0; i < 8; i++)
			{
				std::memset(m_stDialogBoxExchangeInfo[i].cStr1, 0, sizeof(m_stDialogBoxExchangeInfo[i].cStr1));
				std::memset(m_stDialogBoxExchangeInfo[i].cStr2, 0, sizeof(m_stDialogBoxExchangeInfo[i].cStr2));
				m_stDialogBoxExchangeInfo[i].sV1 = -1;
				m_stDialogBoxExchangeInfo[i].sV2 = -1;
				m_stDialogBoxExchangeInfo[i].sV3 = -1;
				m_stDialogBoxExchangeInfo[i].sV4 = -1;
				m_stDialogBoxExchangeInfo[i].sV5 = -1;
				m_stDialogBoxExchangeInfo[i].sV6 = -1;
				m_stDialogBoxExchangeInfo[i].sV7 = -1;
				m_stDialogBoxExchangeInfo[i].dwV1 = 0;
			}
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::Manufacture);
		}
		break;

	case DialogBoxId::ConfirmExchange: // Snoopy: 7 mar 06 (MultiTrade) Confirmation dialog
		break;

	case DialogBoxId::Quest:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Quest) == false) {
			m_dialogBoxManager.Info(DialogBoxId::Quest).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::Quest).sX = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sX + 20;
			m_dialogBoxManager.Info(DialogBoxId::Quest).sY = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sY + 20;
		}
		break;

	case DialogBoxId::Party:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Party) == false) {
			m_dialogBoxManager.Info(DialogBoxId::Party).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::Party).sX = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sX + 20;
			m_dialogBoxManager.Info(DialogBoxId::Party).sY = m_dialogBoxManager.Info(DialogBoxId::CharacterInfo).sY + 20;
		}
		break;

	case DialogBoxId::CrusadeJob:
		if ((m_pPlayer->m_iHP <= 0) || (m_pPlayer->m_bCitizen == false)) return;
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::CrusadeJob) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::CrusadeJob).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::CrusadeJob).sX = 360 + SCREENX;
			m_dialogBoxManager.Info(DialogBoxId::CrusadeJob).sY = 65 + SCREENY;
			m_dialogBoxManager.Info(DialogBoxId::CrusadeJob).sV1 = sV1;
		}
		break;

	case DialogBoxId::ItemUpgrade:
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1 = -1;
			m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = 0;
		}
		else if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemUpgrade) == false)
		{
			int iSoX, iSoM;
			iSoX = iSoM = 0;
			for (i = 0; i < DEF_MAXITEMS; i++)
				if (m_pItemList[i] != 0)
				{
					if ((m_pItemList[i]->m_sSprite == 6) && (m_pItemList[i]->m_sSpriteFrame == 128)) iSoX++;
					if ((m_pItemList[i]->m_sSprite == 6) && (m_pItemList[i]->m_sSpriteFrame == 129)) iSoM++;
				}
			if ((iSoX > 0) || (iSoM > 0))
			{
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 6; // Stone upgrade
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV2 = iSoX;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV3 = iSoM;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1 = -1;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = 0;
			}
			else if (m_iGizonItemUpgradeLeft > 0)
			{
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).cMode = 1;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV2 = -1;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV3 = -1;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1 = -1;
				m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).dwV1 = 0;
			}
			else
			{
				AddEventList(DRAW_DIALOGBOX_ITEMUPGRADE30, 10); // "Stone of Xelima or Merien is not present."
				return;
			}
		}
		break;

	case DialogBoxId::MagicShop:
		if (m_dialogBoxManager.IsEnabled(iBoxID) == false) {
			if (m_pPlayer->m_iSkillMastery[4] == 0) {
				m_dialogBoxManager.DisableDialogBox(DialogBoxId::MagicShop);
				m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcTalk, 0, 480, 0);
				return;
			}
			else {
				m_dialogBoxManager.Info(iBoxID).cMode = 0;
				m_dialogBoxManager.Info(iBoxID).sView = 0;
			}
		}
		break;

	case DialogBoxId::Bank:
		EndInputString();
		if (m_dialogBoxManager.IsEnabled(iBoxID) == false) {
			m_dialogBoxManager.Info(iBoxID).cMode = 0;
			m_dialogBoxManager.Info(iBoxID).sView = 0;
			m_dialogBoxManager.EnableDialogBox(DialogBoxId::Inventory, 0, 0, 0);
		}
		break;

	case DialogBoxId::Slates: // Slates
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Slates) == false) {
			m_dialogBoxManager.Info(DialogBoxId::Slates).sView = 0;
			m_dialogBoxManager.Info(DialogBoxId::Slates).cMode = cType;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV1 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV2 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV3 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV4 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV5 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sV6 = -1;
			m_dialogBoxManager.Info(DialogBoxId::Slates).cStr[0] = 0;
			m_dialogBoxManager.Info(DialogBoxId::Slates).cStr[1] = 0;
			m_dialogBoxManager.Info(DialogBoxId::Slates).cStr[4] = 0;

			m_dialogBoxManager.Info(DialogBoxId::Slates).sSizeX = 180;
			m_dialogBoxManager.Info(DialogBoxId::Slates).sSizeY = 183;

			m_dialogBoxManager.DisableDialogBox(DialogBoxId::ItemDropExternal);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
			m_dialogBoxManager.DisableDialogBox(DialogBoxId::Manufacture);
		}
		break;
	case DialogBoxId::ChangeStatsMajestic: // Diuuude: Change stats window
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::ChangeStatsMajestic) == false) {
			m_dialogBoxManager.Info(DialogBoxId::ChangeStatsMajestic).sX = m_dialogBoxManager.Info(DialogBoxId::LevelUpSetting).sX + 10;
			m_dialogBoxManager.Info(DialogBoxId::ChangeStatsMajestic).sY = m_dialogBoxManager.Info(DialogBoxId::LevelUpSetting).sY + 10;
			m_dialogBoxManager.Info(DialogBoxId::ChangeStatsMajestic).cMode = 0;
			m_dialogBoxManager.Info(DialogBoxId::ChangeStatsMajestic).sView = 0;
			m_bSkillUsingStatus = false;
		}
		break;
	case DialogBoxId::Resurrect: // Snoopy: Resurection
		if (m_dialogBoxManager.IsEnabled(DialogBoxId::Resurrect) == false)
		{
			m_dialogBoxManager.Info(DialogBoxId::Resurrect).sX = 185;
			m_dialogBoxManager.Info(DialogBoxId::Resurrect).sY = 100;
			m_dialogBoxManager.Info(DialogBoxId::Resurrect).cMode = 0;
			m_dialogBoxManager.Info(DialogBoxId::Resurrect).sView = 0;
			m_bSkillUsingStatus = false;
		}
		break;

	default:
		EndInputString();
		if (m_dialogBoxManager.IsEnabled(iBoxID) == false) {
			m_dialogBoxManager.Info(iBoxID).cMode = 0;
			m_dialogBoxManager.Info(iBoxID).sView = 0;
		}
		break;
	}
	if (iBoxID != 30)
	{
		if (m_dialogBoxManager.IsEnabled(iBoxID) == false)
		{
			int tmpx = 720;
			int tmpy = 520;
			if (m_dialogBoxManager.Info(iBoxID).sY > tmpy) m_dialogBoxManager.Info(iBoxID).sY = tmpy + 10;
			if (m_dialogBoxManager.Info(iBoxID).sX > tmpx) m_dialogBoxManager.Info(iBoxID).sX = tmpx;
			if ((m_dialogBoxManager.Info(iBoxID).sX + m_dialogBoxManager.Info(iBoxID).sSizeX) < 10) m_dialogBoxManager.Info(iBoxID).sX += 20;
			if ((m_dialogBoxManager.Info(iBoxID).sY + m_dialogBoxManager.Info(iBoxID).sSizeY) < 10) m_dialogBoxManager.Info(iBoxID).sY += 20;
		}
	}
	m_dialogBoxManager.SetEnabled(iBoxID, true);
	if (pString != 0) strcpy(m_dialogBoxManager.Info(iBoxID).cStr, pString);
	//Snoopy: 39->59
	for (i = 0; i < 59; i++)
		if (m_dialogBoxManager.OrderAt(i) == iBoxID) m_dialogBoxManager.SetOrderAt(i, 0);
	//Snoopy: 39->59
	for (i = 1; i < 59; i++)
		if ((m_dialogBoxManager.OrderAt(i - 1) == 0) && (m_dialogBoxManager.OrderAt(i) != 0)) {
			m_dialogBoxManager.SetOrderAt(i - 1, m_dialogBoxManager.OrderAt(i));
			m_dialogBoxManager.SetOrderAt(i, 0);
		}
	//Snoopy: 39->59
	for (i = 0; i < 59; i++)
		if (m_dialogBoxManager.OrderAt(i) == 0) {
			m_dialogBoxManager.SetOrderAt(i, static_cast<char>(iBoxID));
			return;
		}
}

void CGame::DisableDialogBox(int iBoxID)
{
	int i;

	switch (iBoxID) {
	case DialogBoxId::ItemDropConfirm:
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::ItemDropConfirm).sView] = false;
		break;

	case DialogBoxId::WarningBattleArea:
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::WarningBattleArea).sView] = false;
		break;

	case DialogBoxId::GuildMenu:
		if (m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1)
			EndInputString();
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 0;
		break;

	case DialogBoxId::SaleMenu:
		for (i = 0; i < DEF_MAXMENUITEMS; i++)
			if (m_pItemForSaleList[i] != 0) {
				m_pItemForSaleList[i].reset();
			}
		m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 = 0;
		m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4 = 0; // v1.4
		m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV5 = 0;
		m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV6 = 0;
		break;

	case DialogBoxId::Bank:
		if (m_dialogBoxManager.Info(DialogBoxId::Bank).cMode < 0) return;
		break;

	case DialogBoxId::ItemDropExternal:
		if (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1) {
			EndInputString();
			m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sView] = false;
		}
		break;

	case DialogBoxId::NpcActionQuery: // v1.4
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV1] = false;
		break;

	case DialogBoxId::NpcTalk:
		if (m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sV2 == 500)
		{
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_GETMAGICABILITY, 0, 0, 0, 0, 0);
		}
		break;

	case DialogBoxId::Fishing:
		m_bSkillUsingStatus = false;
		break;

	case DialogBoxId::Manufacture:
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1] = false;
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2] = false;
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3] = false;
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4] = false;
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5] = false;
		if (m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6 != -1) m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6] = false;
		m_bSkillUsingStatus = false;
		break;

	case DialogBoxId::Exchange: //Snoopy: 7 mar 06 (multiTrade) case rewriten
		for (i = 0; i < 8; i++)
		{
			std::memset(m_stDialogBoxExchangeInfo[i].cStr1, 0, sizeof(m_stDialogBoxExchangeInfo[i].cStr1));
			std::memset(m_stDialogBoxExchangeInfo[i].cStr2, 0, sizeof(m_stDialogBoxExchangeInfo[i].cStr2));
			m_stDialogBoxExchangeInfo[i].sV1 = -1;
			m_stDialogBoxExchangeInfo[i].sV2 = -1;
			m_stDialogBoxExchangeInfo[i].sV3 = -1;
			m_stDialogBoxExchangeInfo[i].sV4 = -1;
			m_stDialogBoxExchangeInfo[i].sV5 = -1;
			m_stDialogBoxExchangeInfo[i].sV6 = -1;
			m_stDialogBoxExchangeInfo[i].sV7 = -1;
			m_stDialogBoxExchangeInfo[i].dwV1 = 0;
			if (m_bIsItemDisabled[m_stDialogBoxExchangeInfo[i].sItemID] == true)
				m_bIsItemDisabled[m_stDialogBoxExchangeInfo[i].sItemID] = false;
		}
		break;

	case DialogBoxId::SellList:
		for (i = 0; i < DEF_MAXSELLLIST; i++)
		{
			if (m_stSellItemList[i].iIndex != -1) m_bIsItemDisabled[m_stSellItemList[i].iIndex] = false;
			m_stSellItemList[i].iIndex = -1;
			m_stSellItemList[i].iAmount = 0;
		}
		break;

	case DialogBoxId::ItemUpgrade:
		if (m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1 != -1)
			m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::ItemUpgrade).sV1] = false;
		break;

	case DialogBoxId::Slates:
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Slates).sV1] = false;
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Slates).sV2] = false;
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Slates).sV3] = false;
		m_bIsItemDisabled[m_dialogBoxManager.Info(DialogBoxId::Slates).sV4] = false;

		std::memset(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr));
		std::memset(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr2, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr2));
		std::memset(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr3, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr3));
		std::memset(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr4, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::Slates).cStr4));
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV1 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV2 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV3 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV4 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV5 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV6 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV9 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV10 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV11 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV12 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV13 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).sV14 = -1;
		m_dialogBoxManager.Info(DialogBoxId::Slates).dwV1 = 0;
		m_dialogBoxManager.Info(DialogBoxId::Slates).dwV2 = 0;
		break;

	case DialogBoxId::ChangeStatsMajestic:
		cStateChange1 = 0;
		cStateChange2 = 0;
		cStateChange3 = 0;
		/*	m_pPlayer->m_wLU_Str = 0;
			m_pPlayer->m_wLU_Vit = 0;
			m_pPlayer->m_wLU_Dex = 0;
			m_pPlayer->m_wLU_Int = 0;
			m_pPlayer->m_wLU_Mag = 0;
			m_pPlayer->m_wLU_Char = 0;*/
		break;

	}

	// Call OnDisable for migrated dialogs
	if (auto* pDlg = m_dialogBoxManager.GetDialogBox(iBoxID))
		pDlg->OnDisable();

	m_dialogBoxManager.SetEnabled(iBoxID, false);
	// Snoopy: 39->59
	for (i = 0; i < 59; i++)
		if (m_dialogBoxManager.OrderAt(i) == iBoxID)
			m_dialogBoxManager.SetOrderAt(i, 0);

	// Snoopy: 39->59
	for (i = 1; i < 59; i++)
		if ((m_dialogBoxManager.OrderAt(i - 1) == 0) && (m_dialogBoxManager.OrderAt(i) != 0))
		{
			m_dialogBoxManager.SetOrderAt(i - 1, m_dialogBoxManager.OrderAt(i));
			m_dialogBoxManager.SetOrderAt(i, 0);
		}
}

int CGame::iGetTopDialogBoxIndex()
{
	int i;
	//Snoopy: 38->58
	for (i = 58; i >= 0; i--)
		if (m_dialogBoxManager.OrderAt(i) != 0)
			return m_dialogBoxManager.OrderAt(i);

	return 0;
}

void CGame::DrawChatMsgs(short sX, short sY, short dX, short dY)
{
	int i;

	for (i = 0; i < DEF_MAXCHATMSGS; i++)
		if (m_pChatMsgList[i] != 0)
			if ((m_pChatMsgList[i]->m_sX >= sX) && (m_pChatMsgList[i]->m_sX <= dX) &&
				(m_pChatMsgList[i]->m_sY >= sY) && (m_pChatMsgList[i]->m_sY <= dY)) {

				switch (m_pChatMsgList[i]->m_cType) {
				case 41:
				case 42:
				case 21:
				case 22:
				case 23:
					DrawChatMsgBox(m_pChatMsgList[i]->m_sX, m_pChatMsgList[i]->m_sY, i, false);
					break;
				}
			}

	m_Renderer->BeginTextBatch();
	for (i = 0; i < DEF_MAXCHATMSGS; i++)
		if (m_pChatMsgList[i] != 0)
			if ((m_pChatMsgList[i]->m_sX >= sX) && (m_pChatMsgList[i]->m_sX <= dX) &&
				(m_pChatMsgList[i]->m_sY >= sY) && (m_pChatMsgList[i]->m_sY <= dY)) {

				switch (m_pChatMsgList[i]->m_cType) {
				case 41:
				case 42:
				case 21:
				case 22:
				case 23:
					break;

				case 20:
				default:
					DrawChatMsgBox(m_pChatMsgList[i]->m_sX, m_pChatMsgList[i]->m_sY, i, true);
					break;
				}
			}
	m_Renderer->EndTextBatch();
}

void CGame::_LoadTextDlgContents(int cType)
{
	char* pContents, * token, cTemp[120], cFileName[120];
	char   seps[] = "\n";
	int    iIndex = 0, i;
	uint32_t dwFileSize;
	HANDLE hFile;
	FILE* pFile;
	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();
	}
	// cType
	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFileName, 0, sizeof(cFileName));

	wsprintf(cTemp, "contents%d", cType);
	strcat(cFileName, "contents");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cTemp);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	pFile = fopen(cFileName, "rt");
	if (pFile == 0) return;
	else
	{
		pContents = new char[dwFileSize + 1];
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
	}
	fclose(pFile);
	token = strtok(pContents, seps);
	while (token != 0)
	{
		m_pMsgTextList[iIndex] = std::make_unique<CMsg>(0, token, 0);
		token = strtok(NULL, seps);
		iIndex++;
	}
	delete[] pContents;
}

int CGame::_iLoadTextDlgContents2(int iType)
{
	char* pContents, * token, cTemp[120], cFileName[120];
	char   seps[] = "\n";
	int    iIndex = 0, i;
	uint32_t dwFileSize;
	HANDLE hFile;
	FILE* pFile;
	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList2[i] != 0)
			m_pMsgTextList2[i].reset();
	}
	// cType
	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFileName, 0, sizeof(cFileName));

	wsprintf(cTemp, "contents%d", iType);

	strcat(cFileName, "contents");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cTemp);
	strcat(cFileName, ".txt");
	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	pFile = fopen(cFileName, "rt");
	if (pFile == 0) return -1;
	else
	{
		pContents = new char[dwFileSize + 1];
		if (pContents == 0) return -1;
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
	}
	fclose(pFile);
	token = strtok(pContents, seps);
	while (token != 0)
	{
		m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, token, 0);
		token = strtok(NULL, seps);
		iIndex++;
	}
	delete[] pContents;
	return iIndex;
}

void CGame::_LoadGameMsgTextContents()
{
	char* pContents, * token, cTemp[120], cFileName[120];
	char   seps[] = ";\n";
	int    iIndex = 0, i;
	uint32_t dwFileSize;
	HANDLE hFile;
	FILE* pFile;

	for (i = 0; i < DEF_MAXGAMEMSGS; i++) {
		if (m_pGameMsgList[i] != 0)
			m_pGameMsgList[i].reset();
	}

	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFileName, 0, sizeof(cFileName));

	strcpy(cTemp, "GameMsgList");

	strcat(cFileName, "contents");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cTemp);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	pFile = fopen(cFileName, "rt");
	if (pFile == 0) return;
	else {
		pContents = new char[dwFileSize + 1];
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
	}

	fclose(pFile);

	token = strtok(pContents, seps);
	while (token != 0) {
		m_pGameMsgList[iIndex] = std::make_unique<CMsg>(0, token, 0);
		token = strtok(NULL, seps);
		iIndex++;
	}

	delete[] pContents;
}

void CGame::_RequestMapStatus(char* pMapName, int iMode)
{
	bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_MAPSTATUS, 0, iMode, 0, 0, pMapName);
}

void CGame::AddMapStatusInfo(char* pData, bool bIsLastData)
{
	char cTotal;
	short sIndex;
	int i;

	std::memset(m_cStatusMapName, 0, sizeof(m_cStatusMapName));

	const auto* header = hb::net::PacketCast<hb::net::PacketNotifyMapStatusHeader>(
		pData, sizeof(hb::net::PacketNotifyMapStatusHeader));
	if (!header) return;
	memcpy(m_cStatusMapName, header->map_name, sizeof(header->map_name));
	sIndex = header->index;
	cTotal = header->total;

	const auto* entries = reinterpret_cast<const hb::net::PacketNotifyMapStatusEntry*>(header + 1);

	for (i = 1; i <= cTotal; i++) {
		m_stCrusadeStructureInfo[sIndex].cType = entries->type;
		m_stCrusadeStructureInfo[sIndex].sX = entries->x;
		m_stCrusadeStructureInfo[sIndex].sY = entries->y;
		m_stCrusadeStructureInfo[sIndex].cSide = entries->side;
		entries++;

		sIndex++;
	}

	if (bIsLastData == true) {
		while (sIndex < DEF_MAXCRUSADESTRUCTURES) {
			m_stCrusadeStructureInfo[sIndex].cType = 0;
			m_stCrusadeStructureInfo[sIndex].sX = 0;
			m_stCrusadeStructureInfo[sIndex].sY = 0;
			m_stCrusadeStructureInfo[sIndex].cSide = 0;
			sIndex++;
		}
	}
}

bool CGame::GetText(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	int len;
	HIMC hIMC = 0;
	if (m_pInputBuffer == 0) return false;
	switch (msg) {
	case WM_IME_COMPOSITION:
		std::memset(m_cEdit, 0, sizeof(m_cEdit));
		if (lparam & GCS_RESULTSTR)
		{
			hIMC = ImmGetContext(hWnd);
			len = ImmGetCompositionString(hIMC, GCS_RESULTSTR, 0, 0);
			if (len > 4) len = 4;
			ImmGetCompositionString(hIMC, GCS_RESULTSTR, m_cEdit, len);
			ImmReleaseContext(hWnd, hIMC);
			len = strlen(m_pInputBuffer) + strlen(m_cEdit);
			if (len < m_cInputMaxLen) strcpy(m_pInputBuffer + strlen(m_pInputBuffer), m_cEdit);
			std::memset(m_cEdit, 0, sizeof(m_cEdit));
		}
		else if (lparam & GCS_COMPSTR)
		{
			hIMC = ImmGetContext(hWnd);
			len = ImmGetCompositionString(hIMC, GCS_COMPSTR, 0, 0);
			if (len > 4) len = 4;
			ImmGetCompositionString(hIMC, GCS_COMPSTR, m_cEdit, len);
			ImmReleaseContext(hWnd, hIMC);
			len = strlen(m_pInputBuffer) + strlen(m_cEdit);
			if (len >= m_cInputMaxLen) std::memset(m_cEdit, 0, sizeof(m_cEdit));
		}
		return true;

	case WM_CHAR:
		if (wparam == 8)
		{
			if (strlen(m_pInputBuffer) > 0)
			{
				len = strlen(m_pInputBuffer);
				switch (GetCharKind(m_pInputBuffer, len - 1)) {
				case 1:
					m_pInputBuffer[len - 1] = 0;
					break;
				case 2:
				case 3:
					m_pInputBuffer[len - 2] = 0;
					m_pInputBuffer[len - 1] = 0;
					break;
				}
				std::memset(m_cEdit, 0, sizeof(m_cEdit));
			}
		}
		else if ((wparam != 9) && (wparam != 13) && (wparam != 27))
		{
			len = strlen(m_pInputBuffer);
			if (len >= m_cInputMaxLen - 1) return false;
			m_pInputBuffer[len] = wparam & 0xff;
			m_pInputBuffer[len + 1] = 0;
		}
		return true;
	}
	return false;
}

int CGame::GetCharKind(char* str, int index)
{
	int kind = 1;
	do
	{
		if (kind == 2) kind = 3;
		else
		{
			if ((unsigned char)*str < 128) kind = 1;
			else kind = 2;
		}
		str++;
		index--;
	} while (index >= 0);
	return kind;
}

void CGame::ShowReceivedString(bool bIsHide)
{
	// Safety check: m_pInputBuffer may not be initialized yet
	if (m_pInputBuffer == nullptr) return;

	std::memset(G_cTxt, 0, sizeof(G_cTxt));

	strcpy(G_cTxt, m_pInputBuffer);
	if ((m_cEdit[0] != 0) && (strlen(m_pInputBuffer) + strlen(m_cEdit) + 1 <= m_cInputMaxLen))
	{
		strcpy(G_cTxt + strlen(m_pInputBuffer), m_cEdit);
	}

	if (bIsHide == true)
	{
		for (unsigned char i = 0; i < strlen(G_cTxt); i++)
			if (G_cTxt[i] != 0) G_cTxt[i] = '*';
	}

	if ((GameClock::GetTimeMS() % 400) < 210) G_cTxt[strlen(G_cTxt)] = '_';

	TextLib::DrawText(GameFont::Default, m_iInputX + 1, m_iInputY + 1, G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIBlack.ToColorRef()));
	TextLib::DrawText(GameFont::Default, m_iInputX, m_iInputY + 1, G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIBlack.ToColorRef()));
	TextLib::DrawText(GameFont::Default, m_iInputX + 1, m_iInputY, G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIBlack.ToColorRef()));
	TextLib::DrawText(GameFont::Default, m_iInputX, m_iInputY, G_cTxt, TextLib::TextStyle::FromColorRef(GameColors::UIWhite.ToColorRef()));
}

void CGame::ClearInputString()
{
	if (m_pInputBuffer != 0)	std::memset(m_pInputBuffer, 0, sizeof(m_pInputBuffer));
	std::memset(m_cEdit, 0, sizeof(m_cEdit));
}

void CGame::StartInputString(int sX, int sY, unsigned char iLen, char* pBuffer, bool bIsHide)
{
	m_bInputStatus = true;
	m_iInputX = sX;
	m_iInputY = sY;
	m_pInputBuffer = pBuffer;
	std::memset(m_cEdit, 0, sizeof(m_cEdit));
	m_cInputMaxLen = iLen;
}

void CGame::EndInputString()
{
	m_bInputStatus = false;
	int len = strlen(m_cEdit);
	if (len > 0)
	{
		m_cEdit[len] = 0;
		strcpy(m_pInputBuffer + strlen(m_pInputBuffer), m_cEdit);
		std::memset(m_cEdit, 0, sizeof(m_cEdit));
	}
}

void CGame::ReceiveString(char* pString)
{
	strcpy(pString, m_pInputBuffer);
}

void CGame::DrawNewDialogBox(char cType, int sX, int sY, int iFrame, bool bIsNoColorKey, bool bIsTrans)
{
	if (m_pSprite[cType] == 0) return;
	if (bIsNoColorKey == false)
	{
		if (bIsTrans == true)
			m_pSprite[cType]->Draw(sX, sY, iFrame, SpriteLib::DrawParams::Alpha(0.25f));
		else m_pSprite[cType]->Draw(sX, sY, iFrame);
	}
	else m_pSprite[cType]->Draw(sX, sY, iFrame, SpriteLib::DrawParams::NoColorKey());
}

void CGame::SetCameraShakingEffect(short sDist, int iMul)
{
	int iDegree = 5 - sDist;
	if (iDegree <= 0) iDegree = 0;
	iDegree *= 2;

	if (iMul != 0) iDegree *= iMul;

	if (iDegree <= 2) return;

	m_Camera.SetShake(iDegree);
}

void CGame::MeteorStrikeComing(int iCode)
{
	switch (iCode) {
	case 1: //
		SetTopMsg(m_pGameMsgList[0]->m_pMsg, 5);
		break;
	case 2: //
		SetTopMsg(m_pGameMsgList[10]->m_pMsg, 10);
		break;
	case 3: //
		SetTopMsg(m_pGameMsgList[91]->m_pMsg, 5);
		break;
	case 4: //
		SetTopMsg(m_pGameMsgList[11]->m_pMsg, 10);
		break;
	}
}

void CGame::DrawObjectFOE(int ix, int iy, int iFrame)
{
	if (_iGetFOE(m_entityState.m_iStatus) < 0) // red crusade circle
	{
		if (iFrame <= 4) m_pEffectSpr[38]->Draw(ix, iy, iFrame, SpriteLib::DrawParams::Alpha(0.5f));
	}
}

void CGame::SetTopMsg(char* pString, unsigned char iLastSec)
{
	std::memset(m_cTopMsg, 0, sizeof(m_cTopMsg));
	strcpy(m_cTopMsg, pString);

	m_iTopMsgLastSec = iLastSec;
	m_dwTopMsgTime = GameClock::GetTimeMS();
}

void CGame::DrawTopMsg()
{
	if (strlen(m_cTopMsg) == 0) return;
	m_Renderer->DrawShadowBox(0, 0, LOGICAL_MAX_X, 30);

	if ((((GameClock::GetTimeMS() - m_dwTopMsgTime) / 250) % 2) == 0)
		TextLib::DrawTextAligned(GameFont::Default, 0, 10, LOGICAL_MAX_X, 15, m_cTopMsg, TextLib::TextStyle::Color(GameColors::UITopMsgYellow.r, GameColors::UITopMsgYellow.g, GameColors::UITopMsgYellow.b), TextLib::Align::TopCenter);

	if (GameClock::GetTimeMS() > (m_iTopMsgLastSec * 1000 + m_dwTopMsgTime)) {
		std::memset(m_cTopMsg, 0, sizeof(m_cTopMsg));
	}
}

void CGame::CannotConstruct(int iCode)
{
	switch (iCode) {
	case 1: //
		SetTopMsg(m_pGameMsgList[18]->m_pMsg, 5);
		break;

	case 2: //
		wsprintf(G_cTxt, "%s XY(%d, %d)", m_pGameMsgList[19]->m_pMsg, m_pPlayer->m_iConstructLocX, m_pPlayer->m_iConstructLocY);
		SetTopMsg(G_cTxt, 5);
		break;

	case 3: //
		SetTopMsg(m_pGameMsgList[20]->m_pMsg, 5);
		break;
	case 4: //
		SetTopMsg(m_pGameMsgList[20]->m_pMsg, 5);
		break;

	}
}

void CGame::FormatCommaNumber(uint32_t value, char* buffer, size_t bufSize)
{
	if (buffer == nullptr || bufSize == 0) return;

	char numStr[20];
	_ultoa(value, numStr, 10);

#ifdef DEF_COMMA_GOLD
	int srcLen = static_cast<int>(strlen(numStr));
	int destIdx = 0;
	int digitCount = 0;

	// Build result backwards with commas every 3 digits
	for (int i = srcLen - 1; i >= 0 && destIdx < static_cast<int>(bufSize) - 1; i--) {
		if (digitCount > 0 && digitCount % 3 == 0 && destIdx < static_cast<int>(bufSize) - 1) {
			buffer[destIdx++] = ',';
		}
		buffer[destIdx++] = numStr[i];
		digitCount++;
	}
	buffer[destIdx] = '\0';

	// Reverse the string
	for (int i = 0, j = destIdx - 1; i < j; i++, j--) {
		char tmp = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = tmp;
	}
#else
	strncpy(buffer, numStr, bufSize - 1);
	buffer[bufSize - 1] = '\0';
#endif
}


void CGame::CrusadeContributionResult(int iWarContribution)
{
	int i;
	char cTemp[120];
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Text);
	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();
	}
	if (iWarContribution > 0)
	{
		PlaySound('E', 23, 0, 0);
		PlaySound('C', 21, 0, 0);
		PlaySound('C', 22, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[22]->m_pMsg, 0); // Congratulations! Your nation
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[23]->m_pMsg, 0); // was victory in the battle!
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, m_pGameMsgList[24]->m_pMsg, 0); // As a victorious citizen
		m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[25]->m_pMsg, 0); // You will receive
		m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[26]->m_pMsg, 0); // a prize
		m_pMsgTextList[6] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[27]->m_pMsg, 0); // Experience point of the battle contribution:
		std::memset(cTemp, 0, sizeof(cTemp));											//
		wsprintf(cTemp, "+%dExp Points!", iWarContribution);
		m_pMsgTextList[8] = std::make_unique<CMsg>(0, cTemp, 0);
		for (i = 9; i < 18; i++)
			m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);

	}
	else if (iWarContribution < 0)
	{
		PlaySound('E', 24, 0, 0);
		PlaySound('C', 12, 0, 0);
		PlaySound('C', 13, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[28]->m_pMsg, 0); // Unfortunately! Your country
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[29]->m_pMsg, 0); // have lost the all out war.
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, m_pGameMsgList[30]->m_pMsg, 0); // As a losser citizen;
		m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[31]->m_pMsg, 0); // the prize that accomplishes
		m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[32]->m_pMsg, 0); // will not be given.
		m_pMsgTextList[6] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[33]->m_pMsg, 0); // I hope you to win
		m_pMsgTextList[8] = std::make_unique<CMsg>(0, m_pGameMsgList[34]->m_pMsg, 0); // in the next battle
		for (i = 9; i < 18; i++)
			m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
	}
	else if (iWarContribution == 0)
	{
		PlaySound('E', 25, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[50]->m_pMsg, 0); // The battle that you have participated
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[51]->m_pMsg, 0); // is already finished;
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[52]->m_pMsg, 0); //
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[53]->m_pMsg, 0); // You must connect after finishing
		m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[54]->m_pMsg, 0); // the previous and before starting
		m_pMsgTextList[6] = std::make_unique<CMsg>(0, m_pGameMsgList[55]->m_pMsg, 0); // the next battle so you can receive
		m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[56]->m_pMsg, 0); // the prize
		for (i = 8; i < 18; i++)
			m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
	}
	m_dialogBoxManager.EnableDialogBox(DialogBoxId::Text, 0, 0, 0);
}

void CGame::CrusadeWarResult(int iWinnerSide)
{
	int i, iPlayerSide;
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Text);
	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();
	}
	if (m_pPlayer->m_bCitizen == false) iPlayerSide = 0;
	else if (m_pPlayer->m_bAresden == true) iPlayerSide = 1;
	else if (m_pPlayer->m_bAresden == false) iPlayerSide = 2;
	if (iPlayerSide == 0)
	{
		switch (iWinnerSide) {
		case 0:
			PlaySound('E', 25, 0, 0);
			m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
			m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[36]->m_pMsg, 0); // There was a draw in the
			m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[37]->m_pMsg, 0); // battle
			m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
			break;
		case 1:
			PlaySound('E', 25, 0, 0);
			m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
			m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[38]->m_pMsg, 0); // Aresden was victorious
			m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[39]->m_pMsg, 0); // and put an end to the war
			m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
			break;
		case 2:
			PlaySound('E', 25, 0, 0);
			m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
			m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[40]->m_pMsg, 0); // Elvine was victorious
			m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[41]->m_pMsg, 0); // and put an end to the war
			m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
			break;
		}
		for (i = 4; i < 18; i++)
			m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
	}
	else
	{
		if (iWinnerSide == 0)
		{
			PlaySound('E', 25, 0, 0);
			m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
			m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[36]->m_pMsg, 0); // There was a draw in the
			m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[37]->m_pMsg, 0); // battle
			m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
			for (i = 4; i < 18; i++)
				m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
		}
		else
		{
			if (iWinnerSide == iPlayerSide)
			{
				PlaySound('E', 23, 0, 0);
				PlaySound('C', 21, 0, 0);
				PlaySound('C', 22, 0, 0);
				switch (iWinnerSide) {
				case 1:
					m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!;
					m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[38]->m_pMsg, 0); // Aresden was victorious;
					m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[39]->m_pMsg, 0); // and put an end to the war
					m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
					m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[42]->m_pMsg, 0); // Congratulations!
					m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[43]->m_pMsg, 0); // As a victorious citizen
					m_pMsgTextList[6] = std::make_unique<CMsg>(0, m_pGameMsgList[44]->m_pMsg, 0); // You will receive
					m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[45]->m_pMsg, 0); // a prize
					break;
				case 2:
					m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
					m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[40]->m_pMsg, 0); // Elvine was victorious
					m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[41]->m_pMsg, 0); // and put an end to the war
					m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
					m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[42]->m_pMsg, 0); // Congratulations!
					m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[43]->m_pMsg, 0); // As a victorious citizen
					m_pMsgTextList[6] = std::make_unique<CMsg>(0, m_pGameMsgList[44]->m_pMsg, 0); // You will receive
					m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[45]->m_pMsg, 0); // a prize
					break;
				}
				for (i = 8; i < 18; i++)
					m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
			else if (iWinnerSide != iPlayerSide)
			{
				PlaySound('E', 24, 0, 0);
				PlaySound('C', 12, 0, 0);
				PlaySound('C', 13, 0, 0);
				switch (iWinnerSide) {
				case 1:
					m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
					m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[38]->m_pMsg, 0); // Aresden was victorious;
					m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[39]->m_pMsg, 0); // and put an end to the war
					m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
					m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[46]->m_pMsg, 0); // Unfortunately,
					m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[47]->m_pMsg, 0); // As a losser citizen
					m_pMsgTextList[6] = std::make_unique<CMsg>(0, m_pGameMsgList[48]->m_pMsg, 0); // the prize that accomplishes
					m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[49]->m_pMsg, 0); // will not be given.
					break;
				case 2:
					m_pMsgTextList[0] = std::make_unique<CMsg>(0, m_pGameMsgList[35]->m_pMsg, 0); // All out war finished!
					m_pMsgTextList[1] = std::make_unique<CMsg>(0, m_pGameMsgList[40]->m_pMsg, 0); // Elvine was victorious
					m_pMsgTextList[2] = std::make_unique<CMsg>(0, m_pGameMsgList[41]->m_pMsg, 0); // and put an end to the war
					m_pMsgTextList[3] = std::make_unique<CMsg>(0, " ", 0);
					m_pMsgTextList[4] = std::make_unique<CMsg>(0, m_pGameMsgList[46]->m_pMsg, 0); // Unfortunately,
					m_pMsgTextList[5] = std::make_unique<CMsg>(0, m_pGameMsgList[47]->m_pMsg, 0); // As a losser citizen
					m_pMsgTextList[6] = std::make_unique<CMsg>(0, m_pGameMsgList[48]->m_pMsg, 0); // the prize that accomplishes
					m_pMsgTextList[7] = std::make_unique<CMsg>(0, m_pGameMsgList[49]->m_pMsg, 0); // will not be given.
					break;
				}
				for (i = 8; i < 18; i++)
					m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
		}
	}
	m_dialogBoxManager.EnableDialogBox(DialogBoxId::Text, 0, 0, 0);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeCommander);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeConstructor);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeSoldier);
}


// _Draw_UpdateScreen_OnCreateNewAccount removed - migrated to Screen_CreateAccount

void CGame::DrawChatMsgBox(short sX, short sY, int iChatIndex, bool bIsPreDC)
{
	char cMsg[100], cMsgA[22], cMsgB[22], cMsgC[22], * cp;
	int  iRet, iLines, i, iSize, iSize2, iLoc, iFontSize;
	uint32_t dwTime;
	COLORREF rgb;
	bool bIsTrans;
	RECT rcRect;
	SIZE Size;

	std::memset(cMsg, 0, sizeof(cMsg));
	std::memset(cMsgA, 0, sizeof(cMsgA));
	std::memset(cMsgB, 0, sizeof(cMsgB));
	std::memset(cMsgC, 0, sizeof(cMsgC));

	dwTime = m_pChatMsgList[iChatIndex]->m_dwTime;
	strcpy(cMsg, m_pChatMsgList[iChatIndex]->m_pMsg);
	cp = (char*)cMsg;
	iLines = 0;

	rgb = GameColors::UIWhite.ToColorRef();
	switch (m_pChatMsgList[iChatIndex]->m_cType) {
	case 1:
		rgb = GameColors::UIWhite.ToColorRef();
		break;
	case 20:
		rgb = GameColors::UIDmgYellow.ToColorRef();
		// �޽��� ǥ�ÿ� �����̰� �ɸ���.
		if ((m_dwCurTime - dwTime) < 650) return;
		else dwTime += 650;
		break;
	case 41:
		rgb = GameColors::UIDmgRed.ToColorRef();
		break;

	case 42:
		rgb = GameColors::UIDmgRed.ToColorRef();
		if ((m_dwCurTime - dwTime) < 650) return;
		else dwTime += 650;
		break;
	}

	if (strlen(cp) != 0) {
		memcpy(cMsgA, cp, 20);

		iRet = GetCharKind(cMsgA, 19);
		if (iRet == CODE_HAN1) {
			cMsgA[20] = cp[20];
			cp++;
		}
		cp += 20;
		iLines = 1;
	}

	if (strlen(cp) != 0) {
		memcpy(cMsgB, cp, 20);

		iRet = GetCharKind(cMsgB, 19);
		if (iRet == CODE_HAN1) {
			cMsgB[20] = cp[20];
			cp++;
		}
		cp += 20;
		iLines = 2;
	}

	if (strlen(cp) != 0) {
		memcpy(cMsgC, cp, 20);

		iRet = GetCharKind(cMsgC, 19);
		if (iRet == CODE_HAN1) {
			cMsgC[20] = cp[20];
			cp++;
		}
		cp += 20;
		iLines = 3;
	}

	iSize = 0;
	for (i = 0; i < 20; i++)
		if (cMsgA[i] != 0)

			if ((unsigned char)cMsgA[i] >= 128) {
				iSize += 5;	//6
				i++;
			}
			else iSize += 4;

	iLoc = m_dwCurTime - dwTime;
	switch (m_pChatMsgList[iChatIndex]->m_cType) {
	case 21:
	case 22:
	case 23://...
		if (iLoc > 80) iLoc = 10;
		else iLoc = iLoc >> 3;
		break;
	default://
		if (iLoc > 352) iLoc = 9;
		else if (iLoc > 320) iLoc = 10;
		else iLoc = iLoc >> 5;
		break;
	}

	if (ConfigManager::Get().GetDetailLevel() == 0)
		bIsTrans = false;
	else bIsTrans = true;

	switch (m_pChatMsgList[iChatIndex]->m_cType) {
	case 41:
	case 42:
		iSize2 = 0;
		for (i = 0; i < 100; i++)
			if (cMsg[i] != 0)
				if ((unsigned char)cMsg[i] >= 128) {
					iSize2 += 5;
					i++;
				}
				else iSize2 += 4;
		if (CMisc::bCheckIMEString(cMsg) == false)
		{
			TextLib::DrawText(GameFont::Default, sX - iSize2, sY - 65 - iLoc, cMsg, TextLib::TextStyle::FromColorRef(GameColors::UIBuildRed.ToColorRef()));
			TextLib::DrawText(GameFont::Default, sX - iSize2 + 1, sY - 65 - iLoc, cMsg, TextLib::TextStyle::FromColorRef(GameColors::UIBuildRed.ToColorRef()));
		}
		else TextLib::DrawText(GameFont::SprFont3_0, sX - iSize2, sY - 65 - iLoc, cMsg, TextLib::TextStyle::WithTwoPointShadow(GameColors::Red4x.r, GameColors::Red4x.g, GameColors::Red4x.b));
		break;

	case 21:
	case 22:
	case 23:
		iFontSize = 23 - (int)m_pChatMsgList[iChatIndex]->m_cType;
		switch (iLines) {
		case 1:
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 65 - iLoc, cMsgA, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			break;
		case 2:
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 81 - iLoc, cMsgA, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 65 - iLoc, cMsgB, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			break;
		case 3:
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 97 - iLoc, cMsgA, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 81 - iLoc, cMsgB, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			TextLib::DrawText(GameFont::SprFont3_0 + iFontSize, sX - iSize, sY - 65 - iLoc, cMsgC, bIsTrans ? TextLib::TextStyle::Color(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b).WithAlpha(0.7f) : TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
			break;
		}
		break;

	case 20:
	default:
		if (bIsPreDC == false)
			m_Renderer->BeginTextBatch();

		int iTextWidth = m_Renderer->GetTextWidth(cMsg);

		switch (iTextWidth / 160) {
		case 0:
			SetRect(&rcRect, sX - 80 + 1, sY - 65 - iLoc, sX + 80 + 1, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 65 - iLoc + 1, sX + 80, sY - iLoc + 1);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 65 - iLoc, sX + 80, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, rgb);
			break;

		case 1:
			SetRect(&rcRect, sX - 80 + 1, sY - 83 - iLoc, sX + 80 + 1, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 83 - iLoc + 1, sX + 80, sY - iLoc + 1);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 83 - iLoc, sX + 80, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, rgb);
			break;

		case 2:
			SetRect(&rcRect, sX - 80 + 1, sY - 101 - iLoc, sX + 80 + 1, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 101 - iLoc + 1, sX + 80, sY - iLoc + 1);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 101 - iLoc, sX + 80, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, rgb);
			break;

		case 3:
			SetRect(&rcRect, sX - 80 + 1, sY - 119 - iLoc, sX + 80 + 1, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 119 - iLoc + 1, sX + 80, sY - iLoc + 1);
			m_Renderer->DrawTextRect(&rcRect, cMsg, GameColors::UIBlack.ToColorRef());

			SetRect(&rcRect, sX - 80, sY - 119 - iLoc, sX + 80, sY - iLoc);
			m_Renderer->DrawTextRect(&rcRect, cMsg, rgb);
			break;
		}

		if (bIsPreDC == false)
			m_Renderer->EndTextBatch();
		break;
	}
}

void CGame::CivilRightAdmissionHandler(char* pData)
{
	uint16_t wResult;
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	wResult = header->msg_type;

	switch (wResult) {
	case 0:
		m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 4;
		break;

	case 1:
		m_dialogBoxManager.Info(DialogBoxId::CityHallMenu).cMode = 3;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseCivilRight>(
			pData, sizeof(hb::net::PacketResponseCivilRight));
		if (!pkt) return;
		std::memset(m_cLocation, 0, sizeof(m_cLocation));
		memcpy(m_cLocation, pkt->location, 10);
		if (memcmp(m_cLocation, "aresden", 7) == 0)
		{
			m_pPlayer->m_bAresden = true;
			m_pPlayer->m_bCitizen = true;
			m_pPlayer->m_bHunter = false;
		}
		else if (memcmp(m_cLocation, "arehunter", 9) == 0)
		{
			m_pPlayer->m_bAresden = true;
			m_pPlayer->m_bCitizen = true;
			m_pPlayer->m_bHunter = true;
		}
		else if (memcmp(m_cLocation, "elvine", 6) == 0)
		{
			m_pPlayer->m_bAresden = false;
			m_pPlayer->m_bCitizen = true;
			m_pPlayer->m_bHunter = false;
		}
		else if (memcmp(m_cLocation, "elvhunter", 9) == 0)
		{
			m_pPlayer->m_bAresden = false;
			m_pPlayer->m_bCitizen = true;
			m_pPlayer->m_bHunter = true;
		}
		else
		{
			m_pPlayer->m_bAresden = true;
			m_pPlayer->m_bCitizen = false;
			m_pPlayer->m_bHunter = true;
		}
		break;
	}
}

void CGame::_RemoveChatMsgListByObjectID(int iObjectID)
{
	int i;

	for (i = 1; i < DEF_MAXCHATMSGS; i++)
		if ((m_pChatMsgList[i] != 0) && (m_pChatMsgList[i]->m_iObjectID == iObjectID)) {
			m_pChatMsgList[i].reset();
		}
}

void CGame::PlaySound(char cType, int iNum, int iDist, long lPan)
{
	// Forward to AudioManager
	SoundType type;
	switch (cType)
	{
	case 'C':
		type = SoundType::Character;
		break;
	case 'M':
		type = SoundType::Monster;
		break;
	case 'E':
		type = SoundType::Effect;
		break;
	default:
		return;
	}
	AudioManager::Get().PlaySound(type, iNum, iDist, static_cast<int>(lPan));
}

bool CGame::_bCheckItemByType(char cType)
{
	int i;

	for (i = 0; i < DEF_MAXITEMS; i++)
		if ((m_pItemList[i] != 0) && (m_pItemList[i]->m_cItemType == cType)) return true;

	return false;
}

void CGame::DynamicObjectHandler(char* pData)
{
	short sX, sY, sV1, sV2, sV3;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseDynamicObject>(
		pData, sizeof(hb::net::PacketResponseDynamicObject));
	if (!pkt) return;
	sX = pkt->x;
	sY = pkt->y;
	sV1 = pkt->v1;
	sV2 = pkt->v2;
	sV3 = pkt->v3;

	switch (pkt->header.msg_type) {
	case DEF_MSGTYPE_CONFIRM:// Dynamic Object
		m_pMapData->bSetDynamicObject(sX, sY, sV2, sV1, true);
		break;

	case DEF_MSGTYPE_REJECT:// Dynamic object
		m_pMapData->bSetDynamicObject(sX, sY, sV2, 0, true);
		break;
	}
}

bool CGame::_bIsItemOnHand() // Snoopy: Fixed to remove ShieldCast
{
	int i;
	uint16_t wWeaponType;
	for (i = 0; i < DEF_MAXITEMS; i++)
		if ((m_pItemList[i] != 0) && (m_bIsItemEquipped[i] == true))
		{
			if ((m_pItemList[i]->m_cEquipPos == DEF_EQUIPPOS_LHAND)
				|| (m_pItemList[i]->m_cEquipPos == DEF_EQUIPPOS_TWOHAND))
				return true;
		}
	for (i = 0; i < DEF_MAXITEMS; i++)
		if ((m_pItemList[i] != 0) && (m_bIsItemEquipped[i] == true))
		{
			if (m_pItemList[i]->m_cEquipPos == DEF_EQUIPPOS_RHAND)
			{
				wWeaponType = ((m_pPlayer->m_sPlayerAppr2 & 0x0FF0) >> 4);
				// Snoopy 34 for all wands.
				if ((wWeaponType >= 34) && (wWeaponType < 40)) return false;
				//else if( wWeaponType == 27 ) return false; // Farming's hoe !
				else return true;
			}
		}
	return false;
}

int CGame::_iCalcTotalWeight()
{
	int i, iWeight, iCnt, iTemp;
	iCnt = 0;
	iWeight = 0;
	for (i = 0; i < DEF_MAXITEMS; i++)
		if (m_pItemList[i] != 0)
		{
			if ((m_pItemList[i]->m_cItemType == DEF_ITEMTYPE_CONSUME)
				|| (m_pItemList[i]->m_cItemType == DEF_ITEMTYPE_ARROW))
			{
				iTemp = m_pItemList[i]->m_wWeight * m_pItemList[i]->m_dwCount;
				if (m_pItemList[i]->m_sIDnum == hb::item::ItemId::Gold) iTemp = iTemp / 20;
				iWeight += iTemp;
			}
			else iWeight += m_pItemList[i]->m_wWeight;
			iCnt++;
		}

	return iWeight;
}
uint32_t CGame::iGetLevelExp(int iLevel)
{
	return CalculateLevelExp(iLevel);
}

int CGame::_iGetTotalItemNum()
{
	int i, iCnt;
	iCnt = 0;
	for (i = 0; i < DEF_MAXITEMS; i++)
		if (m_pItemList[i] != 0) iCnt++;
	return iCnt;
}

bool CGame::bCheckExID(char* pName)
{
	if (m_pExID == 0) return false;
	if (memcmp(m_pPlayer->m_cPlayerName, pName, 10) == 0) return false;
	char cTxt[12];
	std::memset(cTxt, 0, sizeof(cTxt));
	memcpy(cTxt, m_pExID->m_pMsg, strlen(m_pExID->m_pMsg));
	if (memcmp(cTxt, pName, 10) == 0) return true;
	else return false;
}

void CGame::DrawWhetherEffects()
{
#define MAXNUM 1000
	static int ix1[MAXNUM];
	static int iy2[MAXNUM];
	static int iFrame[MAXNUM];
	static int iNum = 0;
	int i;
	short dX, dY, sCnt;
	char cTempFrame;
	uint32_t dwTime = m_dwCurTime;

	switch (m_cWhetherEffectType) {
	case 1:
	case 2:
	case 3: // rain
		switch (m_cWhetherEffectType) {
		case 1: sCnt = DEF_MAXWHETHEROBJECTS / 5; break;
		case 2:	sCnt = DEF_MAXWHETHEROBJECTS / 2; break;
		case 3:	sCnt = DEF_MAXWHETHEROBJECTS;     break;
		}

		for (i = 0; i < sCnt; i++)
		{
			if ((m_stWhetherObject[i].cStep >= 0) && (m_stWhetherObject[i].cStep < 20) && (m_stWhetherObject[i].sX != 0))
			{
				dX = m_stWhetherObject[i].sX - m_Camera.GetX();
				dY = m_stWhetherObject[i].sY - m_Camera.GetY();
				cTempFrame = 16 + (m_stWhetherObject[i].cStep / 6);
				m_pEffectSpr[11]->Draw(dX, dY, cTempFrame, SpriteLib::DrawParams::Alpha(0.5f));
			}
			else if ((m_stWhetherObject[i].cStep >= 20) && (m_stWhetherObject[i].cStep < 25) && (m_stWhetherObject[i].sX != 0))
			{
				dX = m_stWhetherObject[i].sX - m_Camera.GetX();
				dY = m_stWhetherObject[i].sY - m_Camera.GetY();
				m_pEffectSpr[11]->Draw(dX, dY, m_stWhetherObject[i].cStep, SpriteLib::DrawParams::Alpha(0.5f));
			}
		}
		break;

	case 4:
	case 5:
	case 6: // Snow
		switch (m_cWhetherEffectType) {
		case 4: sCnt = DEF_MAXWHETHEROBJECTS / 5; break;
		case 5:	sCnt = DEF_MAXWHETHEROBJECTS / 2; break;
		case 6:	sCnt = DEF_MAXWHETHEROBJECTS;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			if ((m_stWhetherObject[i].cStep >= 0) && (m_stWhetherObject[i].cStep < 80))
			{
				dX = m_stWhetherObject[i].sX - m_Camera.GetX();
				dY = m_stWhetherObject[i].sY - m_Camera.GetY();

				// Snoopy: Snow on lower bar
				if (dY >= 460)
				{
					cTempFrame = 39 + (m_stWhetherObject[i].cStep / 20) * 3;
					dX = m_stWhetherObject[i].sBX;
					dY = 426;
				}
				else cTempFrame = 39 + (m_stWhetherObject[i].cStep / 20) * 3 + (rand() % 3);

				m_pEffectSpr[11]->Draw(dX, dY, cTempFrame, SpriteLib::DrawParams::Alpha(0.5f));

				if (m_bIsXmas == true)
				{
					if (dY == 478 - 53)
					{
						ix1[iNum] = dX;
						iy2[iNum] = dY + (rand() % 5);
						iFrame[iNum] = cTempFrame;
						iNum++;
					}
					if (iNum >= MAXNUM) iNum = 0;
				}
			}
		}
		if (m_bIsXmas == true)
		{
			for (i = 0; i <= MAXNUM; i++)
			{
				if (iy2[i] > 10) m_pEffectSpr[11]->Draw(ix1[i], iy2[i], iFrame[i], SpriteLib::DrawParams::Alpha(0.5f));
			}
		}
		break;
	}
}

void CGame::WhetherObjectFrameCounter()
{
	int i;
	short sCnt;
	char  cAdd;
	uint32_t dwTime = m_dwCurTime;

	if ((dwTime - m_dwWOFtime) < 30) return;
	m_dwWOFtime = dwTime;

	switch (m_cWhetherEffectType) {
	case 1:
	case 2:
	case 3: // Rain
		switch (m_cWhetherEffectType) {
		case 1: sCnt = DEF_MAXWHETHEROBJECTS / 5; break;
		case 2:	sCnt = DEF_MAXWHETHEROBJECTS / 2; break;
		case 3:	sCnt = DEF_MAXWHETHEROBJECTS;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			m_stWhetherObject[i].cStep++;
			if ((m_stWhetherObject[i].cStep >= 0) && (m_stWhetherObject[i].cStep < 20))
			{
				cAdd = (40 - m_stWhetherObject[i].cStep);
				if (cAdd < 0) cAdd = 0;
				m_stWhetherObject[i].sY = m_stWhetherObject[i].sY + cAdd;
				if (cAdd != 0)
					m_stWhetherObject[i].sX = m_stWhetherObject[i].sX - 1;
			}
			else if (m_stWhetherObject[i].cStep >= 25)
			{
				if (m_bIsWhetherEffect == false)
				{
					m_stWhetherObject[i].sX = 0;
					m_stWhetherObject[i].sY = 0;
					m_stWhetherObject[i].cStep = 30;
				}
				else
				{
					m_stWhetherObject[i].sX = (m_pMapData->m_sPivotX * 32) + ((rand() % 940) - 200) + 300;
					m_stWhetherObject[i].sY = (m_pMapData->m_sPivotY * 32) + ((rand() % LOGICAL_WIDTH) - LOGICAL_HEIGHT) + 240;
					m_stWhetherObject[i].cStep = -1 * (rand() % 10);
				}
			}
		}
		break;

	case 4:
	case 5:
	case 6:
		switch (m_cWhetherEffectType) {
		case 4: sCnt = DEF_MAXWHETHEROBJECTS / 5; break;
		case 5:	sCnt = DEF_MAXWHETHEROBJECTS / 2; break;
		case 6:	sCnt = DEF_MAXWHETHEROBJECTS;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			m_stWhetherObject[i].cStep++;
			if ((m_stWhetherObject[i].cStep >= 0) && (m_stWhetherObject[i].cStep < 80))
			{
				cAdd = (80 - m_stWhetherObject[i].cStep) / 10;
				if (cAdd < 0) cAdd = 0;
				m_stWhetherObject[i].sY = m_stWhetherObject[i].sY + cAdd;

				//Snoopy: Snow on lower bar
				if (m_stWhetherObject[i].sY > (426 + m_Camera.GetY()))
				{
					m_stWhetherObject[i].sY = 470 + m_Camera.GetY();
					if ((rand() % 10) != 2) m_stWhetherObject[i].cStep--;
					if (m_stWhetherObject[i].sBX == 0) m_stWhetherObject[i].sBX = m_stWhetherObject[i].sX - m_Camera.GetX();

				}
				else m_stWhetherObject[i].sX += 1 - (rand() % 3);
			}
			else if (m_stWhetherObject[i].cStep >= 80)
			{
				if (m_bIsWhetherEffect == false)
				{
					m_stWhetherObject[i].sX = 0;
					m_stWhetherObject[i].sY = 0;
					m_stWhetherObject[i].sBX = 0;
					m_stWhetherObject[i].cStep = 80;
				}
				else
				{
					m_stWhetherObject[i].sX = (m_pMapData->m_sPivotX * 32) + ((rand() % 940) - 200) + 300;
					m_stWhetherObject[i].sY = (m_pMapData->m_sPivotY * 32) + ((rand() % LOGICAL_WIDTH) - LOGICAL_HEIGHT) + LOGICAL_HEIGHT;
					m_stWhetherObject[i].cStep = -1 * (rand() % 10);
					m_stWhetherObject[i].sBX = 0;
				}
			}
		}
		break;
	}
}

void CGame::SetWhetherStatus(bool bStart, char cType)
{
	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);

	// Always stop weather sounds first when changing weather
	AudioManager::Get().StopSound(SoundType::Effect, 38);

	if (bStart == true)
	{
		m_bIsWhetherEffect = true;
		m_cWhetherEffectType = cType;

		// Rain sound (types 1-3)
		if (AudioManager::Get().IsSoundEnabled() && (cType >= 1) && (cType <= 3))
			AudioManager::Get().PlaySoundLoop(SoundType::Effect, 38);

		for (int i = 0; i < DEF_MAXWHETHEROBJECTS; i++)
		{
			m_stWhetherObject[i].sX = 1;
			m_stWhetherObject[i].sBX = 1;
			m_stWhetherObject[i].sY = 1;
			m_stWhetherObject[i].cStep = -1 * (rand() % 40);
		}

		// Snow BGM (types 4-6)
		if (cType >= 4 && cType <= 6)
		{
			if (AudioManager::Get().IsMusicEnabled()) StartBGM();
		}
	}
	else
	{
		m_bIsWhetherEffect = false;
		m_cWhetherEffectType = 0;
	}
}

void CGame::_DrawThunderEffect(int sX, int sY, int dX, int dY, int rX, int rY, char cType)
{
	int j, iErr, pX1, pY1, iX1, iY1, tX, tY;
	char cDir;
	uint32_t dwTime;
	uint16_t wR1, wG1, wB1, wR2, wG2, wB2, wR3, wG3, wB3, wR4, wG4, wB4;
	dwTime = m_dwCurTime;
	sX = pX1 = iX1 = tX = sX;
	sY = pY1 = iY1 = tY = sY;
	CMisc::ColorTransfer(m_Renderer->GetPixelFormat(), GameColors::NightBlueMid.ToColorRef(), &wR1, &wG1, &wB1);
	CMisc::ColorTransfer(m_Renderer->GetPixelFormat(), GameColors::NightBlueDark.ToColorRef(), &wR2, &wG2, &wB2);
	CMisc::ColorTransfer(m_Renderer->GetPixelFormat(), GameColors::NightBlueDeep.ToColorRef(), &wR3, &wG3, &wB3);
	CMisc::ColorTransfer(m_Renderer->GetPixelFormat(), GameColors::NightBlueBright.ToColorRef(), &wR4, &wG4, &wB4);

	for (j = 0; j < 100; j++)
	{
		switch (cType) {
		case 1:
			m_Renderer->DrawLine(pX1, pY1, iX1, iY1, 15, 15, 20);
			m_Renderer->DrawLine(pX1 - 1, pY1, iX1 - 1, iY1, wR1, wG1, wB1);
			m_Renderer->DrawLine(pX1 + 1, pY1, iX1 + 1, iY1, wR1, wG1, wB1);
			m_Renderer->DrawLine(pX1, pY1 - 1, iX1, iY1 - 1, wR1, wG1, wB1);
			m_Renderer->DrawLine(pX1, pY1 + 1, iX1, iY1 + 1, wR1, wG1, wB1);

			m_Renderer->DrawLine(pX1 - 2, pY1, iX1 - 2, iY1, wR2, wG2, wB2);
			m_Renderer->DrawLine(pX1 + 2, pY1, iX1 + 2, iY1, wR2, wG2, wB2);
			m_Renderer->DrawLine(pX1, pY1 - 2, iX1, iY1 - 2, wR2, wG2, wB2);
			m_Renderer->DrawLine(pX1, pY1 + 2, iX1, iY1 + 2, wR2, wG2, wB2);

			m_Renderer->DrawLine(pX1 - 1, pY1 - 1, iX1 - 1, iY1 - 1, wR3, wG3, wB3);
			m_Renderer->DrawLine(pX1 + 1, pY1 - 1, iX1 + 1, iY1 - 1, wR3, wG3, wB3);
			m_Renderer->DrawLine(pX1 + 1, pY1 - 1, iX1 + 1, iY1 - 1, wR3, wG3, wB3);
			m_Renderer->DrawLine(pX1 - 1, pY1 + 1, iX1 - 1, iY1 + 1, wR3, wG3, wB3);
			break;

		case 2:
			m_Renderer->DrawLine(pX1, pY1, iX1, iY1, wR4, wG4, wB4, 0.5f);
			break;
		}
		iErr = 0;
		CMisc::GetPoint(sX, sY, dX, dY, &tX, &tY, &iErr, j * 10);
		pX1 = iX1;
		pY1 = iY1;
		cDir = CMisc::cGetNextMoveDir(iX1, iY1, tX, tY);
		switch (cDir) {
		case 1:	rY -= 5; break;
		case 2: rY -= 5; rX += 5; break;
		case 3:	rX += 5; break;
		case 4: rX += 5; rY += 5; break;
		case 5: rY += 5; break;
		case 6: rX -= 5; rY += 5; break;
		case 7: rX -= 5; break;
		case 8: rX -= 5; rY -= 5; break;
		}
		if (rX < -20) rX = -20;
		if (rX > 20) rX = 20;
		if (rY < -20) rY = -20;
		if (rY > 20) rY = 20;
		iX1 = iX1 + rX;
		iY1 = iY1 + rY;
		if ((abs(tX - dX) < 5) && (abs(tY - dY) < 5)) break;
	}
	switch (cType) {
	case 1:
		m_pEffectSpr[6]->Draw(iX1, iY1, (rand() % 2), SpriteLib::DrawParams::Alpha(0.75f));
		break;
	}
}

// Snoopy: added StormBlade
int CGame::_iGetAttackType()
{
	uint16_t wWeaponType;
	wWeaponType = ((m_pPlayer->m_sPlayerAppr2 & 0x0FF0) >> 4);
	if (wWeaponType == 0)
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[5] >= 100)) return 20;
		else return 1;		// Boxe
	}
	else if ((wWeaponType >= 1) && (wWeaponType <= 2))
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[7] >= 100)) return 21;
		else return 1;		//Dag, SS
	}
	else if ((wWeaponType > 2) && (wWeaponType < 20))
	{
		if ((wWeaponType == 7) || (wWeaponType == 18)) // Added Kloness Esterk
		{
			if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[9] >= 100)) return 22;
			else return 1;  // Esterk
		}
		else if (wWeaponType == 15)
		{
			if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[8] >= 100)) return 30;
			else return 5;  // StormBlade
		}
		else
		{
			if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[8] >= 100)) return 23;
			else return 1;	// LongSwords
		}
	}
	else if ((wWeaponType >= 20) && (wWeaponType < 29))
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[10] >= 100)) return 24;
		else return 1;		// Haches
	}
	else if ((wWeaponType >= 30) && (wWeaponType < 33))
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[14] >= 100)) return 26;
		else return 1;		// Hammers
	}
	else if ((wWeaponType >= 34) && (wWeaponType < 40))
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[21] >= 100)) return 27;
		else return 1;		// Wands
	}
	else if (wWeaponType >= 40)
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[6] >= 100)) return 25;
		else return 2;		// Bows
	}
	else if ((wWeaponType == 29) || (wWeaponType == 33))
	{
		if ((m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true) && (m_pPlayer->m_iSkillMastery[8] >= 100)) return 23;
		else return 1;		// LS
	}
	return 0;
}

int CGame::_iGetWeaponSkillType()
{
	uint16_t wWeaponType;
	wWeaponType = ((m_pPlayer->m_sPlayerAppr2 & 0x0FF0) >> 4);
	if (wWeaponType == 0)
	{
		return 5; // Openhand
	}
	else if ((wWeaponType >= 1) && (wWeaponType < 3))
	{
		return 7; // SS
	}
	else if ((wWeaponType >= 3) && (wWeaponType < 20))
	{
		if ((wWeaponType == 7) || (wWeaponType == 18)) // Esterk or KlonessEsterk
			return 9; // Fencing
		else return 8; // LS
	}
	else if ((wWeaponType >= 20) && (wWeaponType < 29))
	{
		return 10; // Axe (20..28)
	}
	else if ((wWeaponType >= 30) && (wWeaponType < 33))
	{
		return 14; // Hammer (30,31,32)
	}
	else if ((wWeaponType >= 34) && (wWeaponType < 40))
	{
		return 21; // Wand
	}
	else if (wWeaponType >= 40)
	{
		return 6;  // Bow
	}
	else if ((wWeaponType == 29) || (wWeaponType == 33))
	{
		return 8;  // LS LightingBlade || BlackShadow
	}
	return 1; // Fishing !
}

int CGame::_iGetBankItemCount()
{
	int i, iCnt;

	iCnt = 0;
	for (i = 0; i < DEF_MAXBANKITEMS; i++)
		if (m_pBankList[i] != 0) iCnt++;

	return iCnt;
}

bool CGame::_bDecodeBuildItemContents()
{
	char cFileName[255], cTemp[255];
	HANDLE hFile;
	FILE* pFile;
	uint32_t dwFileSize;
	char* pBuffer;
	bool   bRet;
	int    i;

	for (i = 0; i < DEF_MAXBUILDITEMS; i++)
		if (m_pBuildItemList[i] != 0)
		{
			m_pBuildItemList[i].reset();
		}

	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFileName, 0, sizeof(cFileName));

	strcpy(cTemp, "BItemcfg");
	strcat(cFileName, "contents");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cTemp);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	pFile = fopen(cFileName, "rt");
	if (pFile == 0) return false;
	else
	{
		pBuffer = new char[dwFileSize + 1];
		std::memset(pBuffer, 0, dwFileSize + 1);
		fread(pBuffer, dwFileSize, 1, pFile);
		bRet = __bDecodeBuildItemContents(pBuffer);
		delete[] pBuffer;
	}
	fclose(pFile);
	return bRet;
}

bool CGame::_bCheckBuildItemStatus()
{
	int iIndex, i, j, iMatch, iCount;
	char cTempName[21];
	int  iItemCount[DEF_MAXITEMS];

	for (i = 0; i < DEF_MAXBUILDITEMS; i++)
		if (m_pDispBuildItemList[i] != 0)
		{
			m_pDispBuildItemList[i].reset();
		}
	iIndex = 0;
	for (i = 0; i < DEF_MAXBUILDITEMS; i++)
		if (m_pBuildItemList[i] != 0)
		{	// Skill-Limit
			if (m_pPlayer->m_iSkillMastery[13] >= m_pBuildItemList[i]->m_iSkillLimit)
			{
				iMatch = 0;
				m_pDispBuildItemList[iIndex] = std::make_unique<CBuildItem>();
				memcpy(m_pDispBuildItemList[iIndex]->m_cName, m_pBuildItemList[i]->m_cName, 20);

				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName1, m_pBuildItemList[i]->m_cElementName1, 20);
				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName2, m_pBuildItemList[i]->m_cElementName2, 20);
				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName3, m_pBuildItemList[i]->m_cElementName3, 20);
				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName4, m_pBuildItemList[i]->m_cElementName4, 20);
				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName5, m_pBuildItemList[i]->m_cElementName5, 20);
				memcpy(m_pDispBuildItemList[iIndex]->m_cElementName6, m_pBuildItemList[i]->m_cElementName6, 20);

				m_pDispBuildItemList[iIndex]->m_iElementCount[1] = m_pBuildItemList[i]->m_iElementCount[1];
				m_pDispBuildItemList[iIndex]->m_iElementCount[2] = m_pBuildItemList[i]->m_iElementCount[2];
				m_pDispBuildItemList[iIndex]->m_iElementCount[3] = m_pBuildItemList[i]->m_iElementCount[3];
				m_pDispBuildItemList[iIndex]->m_iElementCount[4] = m_pBuildItemList[i]->m_iElementCount[4];
				m_pDispBuildItemList[iIndex]->m_iElementCount[5] = m_pBuildItemList[i]->m_iElementCount[5];
				m_pDispBuildItemList[iIndex]->m_iElementCount[6] = m_pBuildItemList[i]->m_iElementCount[6];

				m_pDispBuildItemList[iIndex]->m_iSprH = m_pBuildItemList[i]->m_iSprH;
				m_pDispBuildItemList[iIndex]->m_iSprFrame = m_pBuildItemList[i]->m_iSprFrame;
				m_pDispBuildItemList[iIndex]->m_iMaxSkill = m_pBuildItemList[i]->m_iMaxSkill;
				m_pDispBuildItemList[iIndex]->m_iSkillLimit = m_pBuildItemList[i]->m_iSkillLimit;

				// ItemCount
				for (j = 0; j < DEF_MAXITEMS; j++)
					if (m_pItemList[j] != 0)
						iItemCount[j] = m_pItemList[j]->m_dwCount;
					else iItemCount[j] = 0;

				// Element1
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName1, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[1];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0) {
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[1] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP2;
							}
						}
				}

			CBIS_STEP2:;
				// Element2
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName2, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[2];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0)
						{
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[2] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP3;
							}
						}
				}

			CBIS_STEP3:;
				// Element3
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName3, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[3];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0)
						{
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[3] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP4;
							}
						}
				}

			CBIS_STEP4:;
				// Element4 �˻�
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName4, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[4];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0)
						{
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[4] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP5;
							}
						}
				}

			CBIS_STEP5:;

				// Element5
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName5, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[5];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0)
						{
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[5] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP6;
							}
						}
				}

			CBIS_STEP6:;

				// Element6
				std::memset(cTempName, 0, sizeof(cTempName));
				memcpy(cTempName, m_pBuildItemList[i]->m_cElementName6, 20);
				iCount = m_pBuildItemList[i]->m_iElementCount[6];
				if (iCount == 0) iMatch++;
				else
				{
					for (j = 0; j < DEF_MAXITEMS; j++)
						if (m_pItemList[j] != 0)
						{
							if ((memcmp(m_pItemList[j]->m_cName, cTempName, 20) == 0) && (m_pItemList[j]->m_dwCount >= (DWORD)(iCount)) &&
								(iItemCount[j] > 0))
							{
								iMatch++;
								m_pDispBuildItemList[iIndex]->m_bElementFlag[6] = true;
								iItemCount[j] -= iCount;
								goto CBIS_STEP7;
							}
						}
				}

			CBIS_STEP7:;

				if (iMatch == 6) m_pDispBuildItemList[iIndex]->m_bBuildEnabled = true;
				iIndex++;
			}
		}
	return true;
}

bool CGame::_ItemDropHistory(char* ItemName)
{
	bool bFlag = false;
	if (m_iItemDropCnt == 0)
	{
		strcpy(m_cItemDrop[m_iItemDropCnt], ItemName);
		m_iItemDropCnt++;
		return true;
	}
	if ((1 <= m_iItemDropCnt) && (20 >= m_iItemDropCnt))
	{
		for (int i = 0; i < m_iItemDropCnt; i++)
		{
			if (strcmp(m_cItemDrop[i], ItemName) == 0)
			{
				bFlag = true;
				break;
			}
		}
		if (bFlag)
		{
			if (m_bItemDrop)
				return false;
			else
				return true;
		}

		if (20 < m_iItemDropCnt)
		{
			for (int i = 0; i < m_iItemDropCnt; i++)
				strcpy(m_cItemDrop[i - 1], ItemName);
			strcpy(m_cItemDrop[20], ItemName);
			m_iItemDropCnt = 21;
		}
		else
		{
			strcpy(m_cItemDrop[m_iItemDropCnt], ItemName);
			m_iItemDropCnt++;
		}
	}
	return true;
}

bool CGame::__bDecodeBuildItemContents(char* pBuffer)
{
	char* pContents, * token;
	char seps[] = "= ,\t\n";
	char cReadModeA = 0;
	char cReadModeB = 0;
	int  iIndex = 0;
	pContents = pBuffer;
	token = strtok(pContents, seps);

	while (token != 0)
	{
		if (cReadModeA != 0)
		{
			switch (cReadModeA) {
			case 1:
				switch (cReadModeB) {
				case 1:
					std::memset(m_pBuildItemList[iIndex]->m_cName, 0, sizeof(m_pBuildItemList[iIndex]->m_cName));
					memcpy(m_pBuildItemList[iIndex]->m_cName, token, strlen(token));
					cReadModeB = 2;
					break;
				case 2:
					m_pBuildItemList[iIndex]->m_iSkillLimit = atoi(token);
					cReadModeB = 3;
					break;
				case 3: // m_cElementName1
					std::memset(m_pBuildItemList[iIndex]->m_cElementName1, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName1));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName1, token, strlen(token));
					cReadModeB = 4;
					break;
				case 4: // m_iElementCount1
					m_pBuildItemList[iIndex]->m_iElementCount[1] = atoi(token);
					cReadModeB = 5;
					break;
				case 5: // m_cElementName2
					std::memset(m_pBuildItemList[iIndex]->m_cElementName2, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName2));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName2, token, strlen(token));
					cReadModeB = 6;
					break;
				case 6: // m_iElementCount2
					m_pBuildItemList[iIndex]->m_iElementCount[2] = atoi(token);
					cReadModeB = 7;
					break;
				case 7: // m_cElementName3
					std::memset(m_pBuildItemList[iIndex]->m_cElementName3, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName3));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName3, token, strlen(token));
					cReadModeB = 8;
					break;
				case 8: // m_iElementCount3
					m_pBuildItemList[iIndex]->m_iElementCount[3] = atoi(token);
					cReadModeB = 9;
					break;
				case 9: // m_cElementName4
					std::memset(m_pBuildItemList[iIndex]->m_cElementName4, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName4));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName4, token, strlen(token));
					cReadModeB = 10;
					break;
				case 10: // m_iElementCount4
					m_pBuildItemList[iIndex]->m_iElementCount[4] = atoi(token);
					cReadModeB = 11;
					break;
				case 11: // m_cElementName5
					std::memset(m_pBuildItemList[iIndex]->m_cElementName5, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName5));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName5, token, strlen(token));
					cReadModeB = 12;
					break;
				case 12: // m_iElementCount5
					m_pBuildItemList[iIndex]->m_iElementCount[5] = atoi(token);
					cReadModeB = 13;
					break;
				case 13: // m_cElementName6
					std::memset(m_pBuildItemList[iIndex]->m_cElementName6, 0, sizeof(m_pBuildItemList[iIndex]->m_cElementName6));
					memcpy(m_pBuildItemList[iIndex]->m_cElementName6, token, strlen(token));
					cReadModeB = 14;
					break;
				case 14: // m_iElementCount6
					m_pBuildItemList[iIndex]->m_iElementCount[6] = atoi(token);
					cReadModeB = 15;
					break;

				case 15:
					m_pBuildItemList[iIndex]->m_iSprH = atoi(token);
					cReadModeB = 16;
					break;

				case 16:
					m_pBuildItemList[iIndex]->m_iSprFrame = atoi(token);
					cReadModeB = 17;
					break;

				case 17:
					m_pBuildItemList[iIndex]->m_iMaxSkill = atoi(token);

					cReadModeA = 0;
					cReadModeB = 0;
					iIndex++;
					break;
				}
				break;

			default:
				break;
			}
		}
		else
		{
			if (memcmp(token, "BuildItem", 9) == 0)
			{
				cReadModeA = 1;
				cReadModeB = 1;
				m_pBuildItemList[iIndex] = std::make_unique<CBuildItem>();
			}
		}
		token = strtok(NULL, seps);
	}
	if ((cReadModeA != 0) || (cReadModeB != 0)) return false;
	return true;
}

bool CGame::_bCheckCurrentBuildItemStatus()
{
	int i, iCount2, iMatch, iIndex, iItemIndex[7];
	int iCount;
	int iItemCount[7];
	char cTempName[21];
	bool bItemFlag[7];

	iIndex = m_dialogBoxManager.Info(DialogBoxId::Manufacture).cStr[0];

	if (m_pBuildItemList[iIndex] == 0) return false;

	iItemIndex[1] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV1;
	iItemIndex[2] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV2;
	iItemIndex[3] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV3;
	iItemIndex[4] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV4;
	iItemIndex[5] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV5;
	iItemIndex[6] = m_dialogBoxManager.Info(DialogBoxId::Manufacture).sV6;

	for (i = 1; i <= 6; i++)
		if (iItemIndex[i] != -1)
			iItemCount[i] = m_pItemList[iItemIndex[i]]->m_dwCount;
		else iItemCount[i] = 0;
	iMatch = 0;
	for (i = 1; i <= 6; i++) bItemFlag[i] = false;

	// Element1
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName1, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[1];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP2;
			}
		}
	}

CCBIS_STEP2:;

	// Element2
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName2, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[2];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP3;
			}
		}
	}

CCBIS_STEP3:;

	// Element3
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName3, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[3];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP4;
			}
		}
	}

CCBIS_STEP4:;

	// Element4
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName4, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[4];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP5;
			}
		}
	}

CCBIS_STEP5:;

	// Element5
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName5, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[5];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP6;
			}
		}
	}

CCBIS_STEP6:;

	// Element6
	std::memset(cTempName, 0, sizeof(cTempName));
	memcpy(cTempName, m_pDispBuildItemList[iIndex]->m_cElementName6, 20);
	iCount = m_pDispBuildItemList[iIndex]->m_iElementCount[6];
	if (iCount == 0) iMatch++;
	else
	{
		for (i = 1; i <= 6; i++)
		{
			if ((iItemIndex[i] != -1) && (memcmp(m_pItemList[iItemIndex[i]]->m_cName, cTempName, 20) == 0) &&
				(m_pItemList[iItemIndex[i]]->m_dwCount >= (DWORD)(iCount)) &&
				(iItemCount[i] > 0) && (bItemFlag[i] == false))
			{
				iMatch++;
				iItemCount[i] -= iCount;
				bItemFlag[i] = true;
				goto CCBIS_STEP7;
			}
		}
	}

CCBIS_STEP7:;

	iCount = 0;
	for (i = 1; i <= 6; i++)
		if (m_pDispBuildItemList[iIndex]->m_iElementCount[i] != 0) iCount++;
	iCount2 = 0;
	for (i = 1; i <= 6; i++)
		if (iItemIndex[i] != -1) iCount2++;
	if ((iMatch == 6) && (iCount == iCount2)) return true;
	return false;
}

void CGame::GetItemName(CItem* pItem, char* pStr1, char* pStr2, char* pStr3)
{
	char cTxt[256], cTxt2[256];
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	m_bIsSpecial = false;
	std::memset(pStr1, 0, 64);
	std::memset(pStr2, 0, 64);
	std::memset(pStr3, 0, 64);

	// Use the display name from the item (populated from ItemDisplayNames.h)
	const char* cName = pItem->GetDisplayName();

	//auto it = std::find_if(m_pItemConfigList.begin(), m_pItemConfigList.end(), [&](const auto& configItem) {
	//		return configItem != nullptr && configItem->m_sIDnum == pItem->m_sIDnum;
	//	});
	//if (it == m_pItemConfigList.end())
	//{
	//	throw std::runtime_error("Item not found for ID: " + std::to_string(pItem->m_sIDnum));
	//}

	if (0 == memcmp(pItem->m_cName, "AcientTablet", 12)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "NecklaceOf", 10)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "DarkElfBow", 10)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "DarkExecutor", 12)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "The_Devastator", 14)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "DemonSlayer", 10)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "LightingBlade", 12)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "5thAnniversary", 13)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "RubyRing", 8)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "SapphireRing", 12)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Ringof", 6)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "MagicNecklace", 13)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "MagicWand(M.Shield)", 19)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "MagicWand(MS30-LLF)", 19)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Merien", 6)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "BerserkWand", 11)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "ResurWand", 9)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Blood", 5)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Swordof", 7)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "StoneOf", 7)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "ZemstoneofSacrifice", 19)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "StormBringer", 12)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Aresden", 7)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Elvine", 6)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "EmeraldRing", 11)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Excaliber", 9)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Xelima", 6)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "Kloness", 7)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "aHeroOf", 7)) m_bIsSpecial = true;
	else if (0 == memcmp(pItem->m_cName, "eHeroOf", 7)) m_bIsSpecial = true;

	if ((pItem->m_dwAttribute & 0x00000001) != 0)
	{
		m_bIsSpecial = true;
		strcpy(pStr1, cName);
		if (pItem->m_cItemType == DEF_ITEMTYPE_MATERIAL)
			wsprintf(pStr2, GET_ITEM_NAME1, pItem->m_sItemSpecEffectValue2);
		else
		{
			if (pItem->m_cEquipPos == DEF_EQUIPPOS_LFINGER)
			{
				wsprintf(pStr2, GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2);
			}
			else
			{
				wsprintf(pStr2, GET_ITEM_NAME2, pItem->m_sItemSpecEffectValue2 + 100);
			}
		}
	}
	else
	{
		if (pItem->m_dwCount == 1)
			wsprintf(G_cTxt, "%s", cName);
		else wsprintf(G_cTxt, DRAW_DIALOGBOX_SELLOR_REPAIR_ITEM1, pItem->m_dwCount, cName);
		strcpy(pStr1, G_cTxt);
	}

	if ((pItem->m_dwAttribute & 0x00F0F000) != 0)
	{
		m_bIsSpecial = true;
		dwType1 = (pItem->m_dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (pItem->m_dwAttribute & 0x000F0000) >> 16;
		dwType2 = (pItem->m_dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (pItem->m_dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: strcpy(cTxt, GET_ITEM_NAME3);   break;
			case 2: strcpy(cTxt, GET_ITEM_NAME4);   break;
			case 3: strcpy(cTxt, GET_ITEM_NAME5);   break;
			case 4: break;
			case 5: strcpy(cTxt, GET_ITEM_NAME6);   break;
			case 6: strcpy(cTxt, GET_ITEM_NAME7);   break;
			case 7: strcpy(cTxt, GET_ITEM_NAME8);   break;
			case 8: strcpy(cTxt, GET_ITEM_NAME9);   break;
			case 9: strcpy(cTxt, GET_ITEM_NAME10);  break;
			case 10: strcpy(cTxt, GET_ITEM_NAME11); break;
			case 11: strcpy(cTxt, GET_ITEM_NAME12); break;
			case 12: strcpy(cTxt, GET_ITEM_NAME13); break;
			}
			strcat(cTxt, pStr1);
			std::memset(pStr1, 0, 64);
			strcpy(pStr1, cTxt);

			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: wsprintf(cTxt, GET_ITEM_NAME14, dwValue1); break;
			case 2: wsprintf(cTxt, GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: strcpy(cTxt, GET_ITEM_NAME16); break;
			case 6: wsprintf(cTxt, GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: strcpy(cTxt, GET_ITEM_NAME18); break;
			case 8: wsprintf(cTxt, GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: strcpy(cTxt, GET_ITEM_NAME20); break;
			case 10: wsprintf(cTxt, GET_ITEM_NAME21, dwValue1 * 3); break;
			case 11: wsprintf(cTxt, GET_ITEM_NAME22, dwValue1); break;
			case 12: wsprintf(cTxt, GET_ITEM_NAME23, dwValue1); break;
			}
			strcat(pStr2, cTxt);

			if (dwType2 != 0) {
				std::memset(cTxt, 0, sizeof(cTxt));
				switch (dwType2) {
				case 1:  wsprintf(cTxt, GET_ITEM_NAME24, dwValue2 * 7); break;
				case 2:  wsprintf(cTxt, GET_ITEM_NAME25, dwValue2 * 7); break;
				case 3:  wsprintf(cTxt, GET_ITEM_NAME26, dwValue2 * 7); break;
				case 4:  wsprintf(cTxt, GET_ITEM_NAME27, dwValue2 * 7); break;
				case 5:  wsprintf(cTxt, GET_ITEM_NAME28, dwValue2 * 7); break;
				case 6:  wsprintf(cTxt, GET_ITEM_NAME29, dwValue2 * 7); break;
				case 7:  wsprintf(cTxt, GET_ITEM_NAME30, dwValue2 * 7); break;
				case 8:  wsprintf(cTxt, GET_ITEM_NAME31, dwValue2 * 3); break;
				case 9:  wsprintf(cTxt, GET_ITEM_NAME32, dwValue2 * 3); break;
				case 10: wsprintf(cTxt, GET_ITEM_NAME33, dwValue2);   break;
				case 11: wsprintf(cTxt, GET_ITEM_NAME34, dwValue2 * 10); break;
				case 12: wsprintf(cTxt, GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				strcpy(pStr3, cTxt);
			}
		}
	}

	dwValue3 = (pItem->m_dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (pStr1[strlen(pStr1) - 2] == '+')
		{
			dwValue3 = atoi((char*)(pStr1 + strlen(pStr1) - 1)) + dwValue3;
			std::memset(cTxt, 0, sizeof(cTxt));
			memcpy(cTxt, pStr1, strlen(pStr1) - 2);
			std::memset(cTxt2, 0, sizeof(cTxt2));
			wsprintf(cTxt2, "%s+%d", cTxt, dwValue3);
			std::memset(pStr1, 0, 64);
			strcpy(pStr1, cTxt2);
		}
		else
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			wsprintf(cTxt, "+%d", dwValue3);
			strcat(pStr1, cTxt);
		}
	}

	// Display mana save effect if present
	auto effectType = pItem->GetItemEffectType();
	int iManaSaveValue = 0;
	if (effectType == hb::item::ItemEffectType::AttackManaSave)
	{
		iManaSaveValue = pItem->m_sItemEffectValue4;
	}
	else if (effectType == hb::item::ItemEffectType::AddEffect &&
	         pItem->m_sItemEffectValue1 == hb::item::ToInt(hb::item::AddEffectType::ManaSave))
	{
		iManaSaveValue = pItem->m_sItemEffectValue2;
	}

	if (iManaSaveValue > 0)
	{
		m_bIsSpecial = true;
		std::memset(cTxt, 0, sizeof(cTxt));
		wsprintf(cTxt, "Mana Save +%d%%", iManaSaveValue);
		// Add to pStr2 if empty, otherwise pStr3
		if (pStr2[0] == '\0')
			strcpy(pStr2, cTxt);
		else if (pStr3[0] == '\0')
			strcpy(pStr3, cTxt);
	}
}

void CGame::GetItemName(short sItemId, uint32_t dwAttribute, char* pStr1, char* pStr2, char* pStr3)
{
	char cTxt[256], cTxt2[256];
	uint32_t dwType1, dwType2, dwValue1, dwValue2, dwValue3;

	m_bIsSpecial = false;
	std::memset(pStr1, 0, 64);
	std::memset(pStr2, 0, 64);
	std::memset(pStr3, 0, 64);

	// Look up item config by ID to get display name (m_cName now contains display names)
	const char* cName = nullptr;
	if (sItemId > 0 && sItemId < 5000 && m_pItemConfigList[sItemId] != nullptr) {
		cName = m_pItemConfigList[sItemId]->m_cName;
	}
	if (cName == nullptr || cName[0] == '\0') {
		// Fallback to "Unknown Item" if no display name found
		strcpy(pStr1, "Unknown Item");
		return;
	}
	strcpy(pStr1, cName);

	if ((dwAttribute & 0x00F0F000) != 0)
	{
		m_bIsSpecial = true;
		dwType1 = (dwAttribute & 0x00F00000) >> 20;
		dwValue1 = (dwAttribute & 0x000F0000) >> 16;
		dwType2 = (dwAttribute & 0x0000F000) >> 12;
		dwValue2 = (dwAttribute & 0x00000F00) >> 8;
		if (dwType1 != 0)
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: strcpy(cTxt, GET_ITEM_NAME3); break;
			case 2: strcpy(cTxt, GET_ITEM_NAME4); break;
			case 3: strcpy(cTxt, GET_ITEM_NAME5); break;
			case 4: break;
			case 5: strcpy(cTxt, GET_ITEM_NAME6); break;
			case 6: strcpy(cTxt, GET_ITEM_NAME7); break;
			case 7: strcpy(cTxt, GET_ITEM_NAME8); break;
			case 8: strcpy(cTxt, GET_ITEM_NAME9); break;
			case 9: strcpy(cTxt, GET_ITEM_NAME10); break;
			case 10: strcpy(cTxt, GET_ITEM_NAME11); break;
			case 11: strcpy(cTxt, GET_ITEM_NAME12); break;
			case 12: strcpy(cTxt, GET_ITEM_NAME13); break;
			}
			strcat(cTxt, pStr1);
			std::memset(pStr1, 0, 64);
			strcpy(pStr1, cTxt);

			std::memset(cTxt, 0, sizeof(cTxt));
			switch (dwType1) {
			case 1: wsprintf(cTxt, GET_ITEM_NAME14, dwValue1); break;
			case 2: wsprintf(cTxt, GET_ITEM_NAME15, dwValue1 * 5); break;
			case 3: break;
			case 4: break;
			case 5: strcpy(cTxt, GET_ITEM_NAME16); break;
			case 6: wsprintf(cTxt, GET_ITEM_NAME17, dwValue1 * 4); break;
			case 7: strcpy(cTxt, GET_ITEM_NAME18); break;
			case 8: wsprintf(cTxt, GET_ITEM_NAME19, dwValue1 * 7); break;
			case 9: strcpy(cTxt, GET_ITEM_NAME20); break;
			case 10: wsprintf(cTxt, GET_ITEM_NAME21, dwValue1 * 3); break;
			case 11: wsprintf(cTxt, GET_ITEM_NAME22, dwValue1); break;
			case 12: wsprintf(cTxt, GET_ITEM_NAME23, dwValue1); break;
			}
			strcat(pStr2, cTxt);

			if (dwType2 != 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				switch (dwType2) {
				case 1:  wsprintf(cTxt, GET_ITEM_NAME24, dwValue2 * 7);  break;
				case 2:  wsprintf(cTxt, GET_ITEM_NAME25, dwValue2 * 7);  break;
				case 3:  wsprintf(cTxt, GET_ITEM_NAME26, dwValue2 * 7);  break;
				case 4:  wsprintf(cTxt, GET_ITEM_NAME27, dwValue2 * 7);  break;
				case 5:  wsprintf(cTxt, GET_ITEM_NAME28, dwValue2 * 7);  break;
				case 6:  wsprintf(cTxt, GET_ITEM_NAME29, dwValue2 * 7);  break;
				case 7:  wsprintf(cTxt, GET_ITEM_NAME30, dwValue2 * 7);  break;
				case 8:  wsprintf(cTxt, GET_ITEM_NAME31, dwValue2 * 3);  break;
				case 9:  wsprintf(cTxt, GET_ITEM_NAME32, dwValue2 * 3);  break;
				case 10: wsprintf(cTxt, GET_ITEM_NAME33, dwValue2);    break;
				case 11: wsprintf(cTxt, GET_ITEM_NAME34, dwValue2 * 10); break;
				case 12: wsprintf(cTxt, GET_ITEM_NAME35, dwValue2 * 10); break;
				}
				strcpy(pStr3, cTxt);
			}
		}
	}

	dwValue3 = (dwAttribute & 0xF0000000) >> 28;
	if (dwValue3 > 0)
	{
		if (pStr1[strlen(pStr1) - 2] == '+')
		{
			dwValue3 = atoi((char*)(pStr1 + strlen(pStr1) - 1)) + dwValue3;
			std::memset(cTxt, 0, sizeof(cTxt));
			memcpy(cTxt, pStr1, strlen(pStr1) - 2);
			std::memset(cTxt2, 0, sizeof(cTxt2));
			wsprintf(cTxt2, "%s+%d", cTxt, dwValue3);
			std::memset(pStr1, 0, 64);
			strcpy(pStr1, cTxt2);
		}
		else
		{
			std::memset(cTxt, 0, sizeof(cTxt));
			wsprintf(cTxt, "+%d", dwValue3);
			strcat(pStr1, cTxt);
		}
	}
}

short CGame::FindItemIdByName(const char* cItemName)
{
	if (cItemName == nullptr) return 0;
	for (int i = 1; i < 5000; i++) {
		if (m_pItemConfigList[i] != nullptr &&
			memcmp(m_pItemConfigList[i]->m_cName, cItemName, 20) == 0) {
			return static_cast<short>(i);
		}
	}
	return 0; // Not found
}

void CGame::NoticementHandler(char* pData)
{
	FILE* pFile;
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	switch (header->msg_type) {
	case DEF_MSGTYPE_CONFIRM:
	case DEF_MSGTYPE_REJECT:
		const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseNoticementText>(
			pData, sizeof(hb::net::PacketResponseNoticementText));
		if (!pkt) return;
		pFile = fopen("contents\\contents1000.txt", "wt");
		if (pFile == 0) return;
		fwrite(pkt->text, strlen(pkt->text), 1, pFile);
		fclose(pFile);
		m_dialogBoxManager.Info(DialogBoxId::Text).sX = 20;
		m_dialogBoxManager.Info(DialogBoxId::Text).sY = 65;
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::Text, 1000, 0, 0);
		break;
	}
	AddEventList("Press F1 for news and help.", 10);
	m_dialogBoxManager.EnableDialogBox(DialogBoxId::Help, 0, 0, 0);

}

int CGame::_iGetFOE(int iStatus)
{
	bool bPK, bCitizen, bAresden, bHunter;
	if (m_pPlayer->m_iPKCount != 0) return -1;
	//	CLEROTH			0x00000000 status is int NOT short ( 3.51 )
	if (iStatus & 0x80000000) bPK = true;
	else bPK = false;
	if (iStatus & 0x40000000) bCitizen = true;
	else bCitizen = false;
	if (iStatus & 0x20000000) bAresden = true;
	else bAresden = false;
	if (iStatus & 0x10000000) bHunter = true;
	else bHunter = false;
	if (bPK == true) return -2;
	if (bCitizen == false) return 0;
	if (m_pPlayer->m_bCitizen == false) return 0;
	if ((m_pPlayer->m_bAresden == true) && (bAresden == true)) return 1;
	if ((m_pPlayer->m_bAresden == false) && (bAresden == false)) return 1;
	if (m_bIsCrusadeMode == true) return -1;
	else
	{
		if ((m_pPlayer->m_bHunter == false) && (bHunter == false)) return -1;
		else return 0;
	}
}

void CGame::_SetIlusionEffect(int iOwnerH)
{
	char cDir;

	m_iIlusionOwnerH = iOwnerH;

	std::memset(m_cName_IE, 0, sizeof(m_cName_IE));
	m_pMapData->GetOwnerStatusByObjectID(iOwnerH, &m_cIlusionOwnerType, &cDir, &m_pPlayer->m_sAppr1_IE, &m_pPlayer->m_sAppr2_IE, &m_pPlayer->m_sAppr3_IE, &m_pPlayer->m_sAppr4_IE, &m_pPlayer->m_iStatus_IE, &m_pPlayer->m_iApprColor_IE, m_cName_IE);
}

void CGame::ResponsePanningHandler(char* pData)
{
	char cDir;
	short sX, sY;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponsePanningHeader>(
		pData, sizeof(hb::net::PacketResponsePanningHeader));
	if (!pkt) return;
	sX = pkt->x;
	sY = pkt->y;
	cDir = static_cast<char>(pkt->dir);

	switch (cDir) {
	case 1: m_Camera.MoveDestination(0, -32); m_pPlayer->m_sPlayerY--; break;
	case 2: m_Camera.MoveDestination(32, -32); m_pPlayer->m_sPlayerY--; m_pPlayer->m_sPlayerX++; break;
	case 3: m_Camera.MoveDestination(32, 0); m_pPlayer->m_sPlayerX++; break;
	case 4: m_Camera.MoveDestination(32, 32); m_pPlayer->m_sPlayerY++; m_pPlayer->m_sPlayerX++; break;
	case 5: m_Camera.MoveDestination(0, 32); m_pPlayer->m_sPlayerY++; break;
	case 6: m_Camera.MoveDestination(-32, 32); m_pPlayer->m_sPlayerY++; m_pPlayer->m_sPlayerX--; break;
	case 7: m_Camera.MoveDestination(-32, 0); m_pPlayer->m_sPlayerX--; break;
	case 8: m_Camera.MoveDestination(-32, -32); m_pPlayer->m_sPlayerY--; m_pPlayer->m_sPlayerX--; break;
	}

	m_pMapData->ShiftMapData(cDir);
	const char* mapData = reinterpret_cast<const char*>(pData) + sizeof(hb::net::PacketResponsePanningHeader);
	_ReadMapData(sX, sY, mapData);

	m_bIsObserverCommanded = false;
}

/*********************************************************************************************************************
** void CGame::CreateScreenShot()										(snoopy)									**
**  description			:: Fixed Screen Shots																		**
**********************************************************************************************************************/
void CGame::CreateScreenShot()
{	//HelShot20060307_173003_Warehouse000.jpg
	int i;
	FILE* pFile;
	char cFn[256];
	char LongMapName[128];
	char SStime[32];
	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);
	std::memset(LongMapName, 0, sizeof(LongMapName));
	GetOfficialMapName(m_cMapName, LongMapName);
	std::memset(SStime, 0, sizeof(SStime));
	wsprintf(SStime, "%02d:%02d - %02d:%02d:%02d"
		, SysTime.wMonth, SysTime.wDay
		, SysTime.wHour, SysTime.wMinute, SysTime.wSecond
		, LongMapName);
	TextLib::DrawTextAligned(GameFont::Default, 500 + SCREENX, 30, (650 + SCREENY) - (500 + SCREENX), 15, SStime, TextLib::TextStyle::Color(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b), TextLib::Align::TopCenter); //ScreenShot time

	for (i = 0; i < 1000; i++)
	{
		std::memset(cFn, 0, sizeof(cFn));
		wsprintf(cFn, "Save\\HelShot%04d%02d%02d_%02d%02d%02d_%s%03d.bmp"
			, SysTime.wYear, SysTime.wMonth, SysTime.wDay
			, SysTime.wHour, SysTime.wMinute, SysTime.wSecond
			, LongMapName
			, i);
		_mkdir("SAVE");
		pFile = fopen(cFn, "rb");
		if (pFile == 0)
		{
			m_Renderer->Screenshot(cFn);

			wsprintf(G_cTxt, NOTIFYMSG_CREATE_SCREENSHOT1, cFn);
			AddEventList(G_cTxt, 10);
			return;
		}
		fclose(pFile);
	}
	AddEventList(NOTIFYMSG_CREATE_SCREENSHOT2, 10);
}

// _bDraw_OnCreateNewCharacter removed - migrated to Screen_CreateNewCharacter

void CGame::_LoadAgreementTextContents(char cType)
{
	char* pContents, * token, cTemp[120], cFileName[120];
	char   seps[] = "\n";
	int    iIndex = 0, i;
	uint32_t dwFileSize;
	HANDLE hFile;
	FILE* pFile;

	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++) {
		if (m_pAgreeMsgTextList[i] != 0)
			m_pAgreeMsgTextList[i].reset();
	}

	std::memset(cTemp, 0, sizeof(cTemp));
	std::memset(cFileName, 0, sizeof(cFileName));

	wsprintf(cTemp, "contents%d", cType);

	strcat(cFileName, "contents");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cTemp);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	pFile = fopen(cFileName, "rt");
	if (pFile == 0) return;
	else {
		pContents = new char[dwFileSize + 1];
		std::memset(pContents, 0, dwFileSize + 1);
		fread(pContents, dwFileSize, 1, pFile);
	}

	fclose(pFile);
	token = strtok(pContents, seps);
	while (token != 0) {
		m_pAgreeMsgTextList[iIndex] = std::make_unique<CMsg>(0, token, 0);
		token = strtok(NULL, seps);
		iIndex++;
	}
	delete[] pContents;
}


void CGame::OnKeyUp(int key)
{
	if (HotkeyManager::Get().HandleKeyUp(static_cast<int>(key))) {
		return;
	}

	// Use KeyCode enum for engine-agnostic key handling
	KeyCode _key = static_cast<KeyCode>(key);

	// When an overlay is active, only allow certain hotkeys (like screenshot)
	// Block most hotkeys to prevent interaction with base screen
	if (GameModeManager::GetActiveOverlay() != nullptr)
	{
		// Only allow screenshot hotkey when overlay is visible
		if (_key == KeyCode::F11) {
			Hotkey_Simple_Screenshot();
		}
		return;
	}

	switch (_key) {
	case KeyCode::NumpadAdd:      // Numpad +
		Hotkey_Simple_ZoomIn();
		break;
	case KeyCode::NumpadSubtract: // Numpad -
		Hotkey_Simple_ZoomOut();
		break;

	case KeyCode::F2:
		Hotkey_Simple_UseShortcut2();
		break;

	case KeyCode::F3:
		Hotkey_Simple_UseShortcut3();
		break;

	case KeyCode::Insert:
		Hotkey_Simple_UseHealthPotion();
		break;

	case KeyCode::Delete:
		Hotkey_Simple_UseManaPotion();
		break;

	case KeyCode::End:
		Hotkey_Simple_LoadBackupChat();
		break;

	case KeyCode::F4:
		Hotkey_Simple_UseMagicShortcut();
		break;

	case KeyCode::F5:
		Hotkey_Simple_ToggleCharacterInfo();
		break;

	case KeyCode::F6:
		Hotkey_Simple_ToggleInventory();
		break;

	case KeyCode::F7:
		Hotkey_Simple_ToggleMagic();
		break;

	case KeyCode::F8:
		Hotkey_Simple_ToggleSkill();
		break;

	case KeyCode::F9:
		Hotkey_Simple_ToggleChatHistory();
		break;

	case KeyCode::F12:
		Hotkey_Simple_ToggleSystemMenu();
		break;

	case KeyCode::F1:
		Hotkey_Simple_UseShortcut1();
		break;

	case KeyCode::Up:
		Hotkey_Simple_WhisperCycleUp();
		break;

	case KeyCode::Right:
		Hotkey_Simple_ArrowRight();
		break;

	case KeyCode::Down:
		Hotkey_Simple_WhisperCycleDown();
		break;

	case KeyCode::Left:
		Hotkey_Simple_ArrowLeft();
		break;

	case KeyCode::Tab:
		Hotkey_Simple_TabToggleCombat();
		break;

	case KeyCode::Home:
		Hotkey_Simple_ToggleSafeAttack();
		break;

	case KeyCode::Escape:
		Hotkey_Simple_Escape();
		break;

	case KeyCode::PageUp:
		Hotkey_Simple_SpecialAbility();
		break;

	case KeyCode::F11:
		Hotkey_Simple_Screenshot();
		break;

	default:
		return;
	}
}

void CGame::OnKeyDown(int key)
{
	// Use KeyCode enum for engine-agnostic key handling
	KeyCode _key = static_cast<KeyCode>(key);

	// When an overlay is active, block all OnKeyDown actions
	// Overlays handle their own input in on_update()
	if (GameModeManager::GetActiveOverlay() != nullptr)
	{
		return;
	}

	// Filter out keys that should not trigger any action in OnKeyDown
	// These are handled in OnKeyUp or elsewhere
	switch (_key) {
	case KeyCode::Insert:
	case KeyCode::Delete:
	case KeyCode::Tab:
	case KeyCode::Escape:
	case KeyCode::End:
	case KeyCode::Home:
	case KeyCode::F1:
	case KeyCode::F2:
	case KeyCode::F3:
	case KeyCode::F4:
	case KeyCode::F5:
	case KeyCode::F6:
	case KeyCode::F7:
	case KeyCode::F8:
	case KeyCode::F9:
	case KeyCode::F10:
	case KeyCode::F11:
	case KeyCode::F12:
	case KeyCode::PageUp:
	case KeyCode::PageDown:
	case KeyCode::LWin:
	case KeyCode::RWin:
	case KeyCode::NumpadMultiply:
	case KeyCode::NumpadAdd:
	case KeyCode::NumpadSeparator:
	case KeyCode::NumpadSubtract:
	case KeyCode::NumpadDecimal:
	case KeyCode::NumpadDivide:
	case KeyCode::NumLock:
	case KeyCode::ScrollLock:
		return;
	default:
		break;
	}

	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		if (Input::IsCtrlDown())
		{
			// Ctrl+0-9 for magic views
			switch (_key) {
			case KeyCode::Num0: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 9; break;
			case KeyCode::Num1: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 0; break;
			case KeyCode::Num2: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 1; break;
			case KeyCode::Num3: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 2; break;
			case KeyCode::Num4: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 3; break;
			case KeyCode::Num5: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 4; break;
			case KeyCode::Num6: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 5; break;
			case KeyCode::Num7: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 6; break;
			case KeyCode::Num8: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 7; break;
			case KeyCode::Num9: m_dialogBoxManager.EnableDialogBox(DialogBoxId::Magic, 0, 0, 0); m_dialogBoxManager.Info(DialogBoxId::Magic).sView = 8; break;
			default: break;
			}
		}
		// Only Enter key activates chat input - not every key press
		else if (_key == KeyCode::Enter && (m_bInputStatus == false) && (!Input::IsAltDown()))
		{
			StartInputString(CHAT_INPUT_X, CHAT_INPUT_Y, sizeof(m_cChatMsg), m_cChatMsg);
			ClearInputString();
		}
	}
}

void CGame::ReserveFightzoneResponseHandler(char* pData)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseFightzoneReserve>(
		pData, sizeof(hb::net::PacketResponseFightzoneReserve));
	if (!pkt) return;
	switch (pkt->header.msg_type) {
	case DEF_MSGTYPE_CONFIRM:
		AddEventList(RESERVE_FIGHTZONE_RESPONSE_HANDLER1, 10);
		m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 14;
		m_iFightzoneNumber = m_iFightzoneNumberTemp;
		break;

	case DEF_MSGTYPE_REJECT:
		AddEventList(RESERVE_FIGHTZONE_RESPONSE_HANDLER2, 10);
		m_iFightzoneNumberTemp = 0;

		if (pkt->result == 0) {
			m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 15;
		}
		else if (pkt->result == -1) {
			m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 16;
		}
		else if (pkt->result == -2) {
			m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 17;
		}
		else if (pkt->result == -3) {
			m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 21;
		}
		else if (pkt->result == -4) {
			m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 22;
		}
		break;
	}
}


// UpdateScreen_OnLogResMsg removed - replaced by Overlay_LogResMsg

void CGame::RetrieveItemHandler(char* pData)
{
	char cBankItemIndex, cItemIndex, cTxt[120];
	int j;
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	if (header->msg_type != DEF_MSGTYPE_REJECT)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseRetrieveItem>(
			pData, sizeof(hb::net::PacketResponseRetrieveItem));
		if (!pkt) return;
		cBankItemIndex = static_cast<char>(pkt->bank_index);
		cItemIndex = static_cast<char>(pkt->item_index);

		if (m_pBankList[cBankItemIndex] != 0) {
			// v1.42
			char cStr1[64], cStr2[64], cStr3[64];
			GetItemName(m_pBankList[cBankItemIndex].get(), cStr1, cStr2, cStr3);

			std::memset(cTxt, 0, sizeof(cTxt));
			wsprintf(cTxt, RETIEVE_ITEM_HANDLER4, cStr1);//""You took out %s."
			AddEventList(cTxt, 10);

			if ((m_pBankList[cBankItemIndex]->m_cItemType == DEF_ITEMTYPE_CONSUME) ||
				(m_pBankList[cBankItemIndex]->m_cItemType == DEF_ITEMTYPE_ARROW))
			{
				if (m_pItemList[cItemIndex] == 0) goto RIH_STEP2;
				m_pBankList[cBankItemIndex].reset();
				for (j = 0; j <= DEF_MAXBANKITEMS - 2; j++)
				{
					if ((m_pBankList[j + 1] != 0) && (m_pBankList[j] == 0))
					{
						m_pBankList[j] = std::move(m_pBankList[j + 1]);
					}
				}
			}
			else
			{
			RIH_STEP2:;
				if (m_pItemList[cItemIndex] != 0) return;
				short nX, nY;
				nX = 40;
				nY = 30;
				for (j = 0; j < DEF_MAXITEMS; j++)
				{
					if ((m_pItemList[j] != 0) && (memcmp(m_pItemList[j]->m_cName, cStr1, 20) == 0))
					{
						nX = m_pItemList[j]->m_sX + 1;
						nY = m_pItemList[j]->m_sY + 1;
						break;
					}
				}
				m_pItemList[cItemIndex] = std::move(m_pBankList[cBankItemIndex]);
				m_pItemList[cItemIndex]->m_sX = nX;
				m_pItemList[cItemIndex]->m_sY = nY;
				bSendCommand(MSGID_REQUEST_SETITEMPOS, 0, cItemIndex, nX, nY, 0, 0);

				for (j = 0; j < DEF_MAXITEMS; j++)
					if (m_cItemOrder[j] == -1)
					{
						m_cItemOrder[j] = cItemIndex;
						break;
					}
				m_bIsItemEquipped[cItemIndex] = false;
				m_bIsItemDisabled[cItemIndex] = false;
				// m_pBankList[cBankItemIndex] already moved above
				for (j = 0; j <= DEF_MAXBANKITEMS - 2; j++)
				{
					if ((m_pBankList[j + 1] != 0) && (m_pBankList[j] == 0))
					{
						m_pBankList[j] = std::move(m_pBankList[j + 1]);
					}
				}
			}
		}
	}
	m_dialogBoxManager.Info(DialogBoxId::Bank).cMode = 0;
}

void CGame::EraseItem(char cItemID)
{
	int i;
	char cStr1[64], cStr2[64], cStr3[64];
	std::memset(cStr1, 0, sizeof(cStr1));
	std::memset(cStr2, 0, sizeof(cStr2));
	std::memset(cStr3, 0, sizeof(cStr3));
	for (i = 0; i < 6; i++)
	{
		if (m_sShortCut[i] == cItemID)
		{
			GetItemName(m_pItemList[cItemID].get(), cStr1, cStr2, cStr3);
			if (i < 3) wsprintf(G_cTxt, ERASE_ITEM, cStr1, cStr2, cStr3, i + 1);
			else wsprintf(G_cTxt, ERASE_ITEM, cStr1, cStr2, cStr3, i + 7);
			AddEventList(G_cTxt, 10);
			m_sShortCut[i] = -1;
		}
	}

	if (cItemID == m_sRecentShortCut)
		m_sRecentShortCut = -1;
	// ItemOrder
	for (i = 0; i < DEF_MAXITEMS; i++)
		if (m_cItemOrder[i] == cItemID)
			m_cItemOrder[i] = -1;
	for (i = 1; i < DEF_MAXITEMS; i++)
		if ((m_cItemOrder[i - 1] == -1) && (m_cItemOrder[i] != -1))
		{
			m_cItemOrder[i - 1] = m_cItemOrder[i];
			m_cItemOrder[i] = -1;
		}
	// ItemList
	m_pItemList[cItemID].reset();
	m_bIsItemEquipped[cItemID] = false;
	m_bIsItemDisabled[cItemID] = false;
}

void CGame::DrawNpcName(short sX, short sY, short sOwnerType, int iStatus)
{
	char cTxt[32], cTxt2[64];
	std::memset(cTxt, 0, sizeof(cTxt));
	std::memset(cTxt2, 0, sizeof(cTxt2));
	GetNpcName(sOwnerType, cTxt);
	if ((iStatus & 0x20) != 0) strcat(cTxt, DRAW_OBJECT_NAME50);//" Berserk"
	if ((iStatus & 0x40) != 0) strcat(cTxt, DRAW_OBJECT_NAME51);//" Frozen"
	TextLib::DrawText(GameFont::Default, sX, sY, cTxt, TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
	if (m_bIsObserverMode == true) TextLib::DrawText(GameFont::Default, sX, sY + 14, cTxt, TextLib::TextStyle::WithShadow(GameColors::NameNeutral.r, GameColors::NameNeutral.g, GameColors::NameNeutral.b));
	else if (m_pPlayer->m_bIsConfusion || (m_iIlusionOwnerH != 0))
	{
		std::memset(cTxt, 0, sizeof(cTxt));
		strcpy(cTxt, DRAW_OBJECT_NAME87);//"(Unknown)"
		TextLib::DrawText(GameFont::Default, sX, sY + 14, cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b)); // v2.171
	}
	else
	{
		switch (_iGetFOE(iStatus)) {
		case -2:
		case -1:
			TextLib::DrawText(GameFont::Default, sX, sY + 14, DRAW_OBJECT_NAME90, TextLib::TextStyle::WithShadow(GameColors::UIRed.r, GameColors::UIRed.g, GameColors::UIRed.b)); // "(Enemy)"
			break;
		case 0:
			TextLib::DrawText(GameFont::Default, sX, sY + 14, DRAW_OBJECT_NAME88, TextLib::TextStyle::WithShadow(GameColors::NameNeutral.r, GameColors::NameNeutral.g, GameColors::NameNeutral.b)); // "(Neutral)"
			break;
		case 1:
			TextLib::DrawText(GameFont::Default, sX, sY + 14, DRAW_OBJECT_NAME89, TextLib::TextStyle::WithShadow(GameColors::NameFriendly.r, GameColors::NameFriendly.g, GameColors::NameFriendly.b)); // "(Friendly)"
			break;
		}
	}
	switch ((iStatus & 0x0F00) >> 8) {
	case 0: break;
	case 1: strcpy(cTxt2, DRAW_OBJECT_NAME52); break;//"Clairvoyant"
	case 2: strcpy(cTxt2, DRAW_OBJECT_NAME53); break;//"Destruction of Magic Protection"
	case 3: strcpy(cTxt2, DRAW_OBJECT_NAME54); break;//"Anti-Physical Damage"
	case 4: strcpy(cTxt2, DRAW_OBJECT_NAME55); break;//"Anti-Magic Damage"
	case 5: strcpy(cTxt2, DRAW_OBJECT_NAME56); break;//"Poisonous"
	case 6: strcpy(cTxt2, DRAW_OBJECT_NAME57); break;//"Critical Poisonous"
	case 7: strcpy(cTxt2, DRAW_OBJECT_NAME58); break;//"Explosive"
	case 8: strcpy(cTxt2, DRAW_OBJECT_NAME59); break;//"Critical Explosive"
	}
	if (CMisc::bCheckIMEString(cTxt2)) TextLib::DrawText(GameFont::SprFont3_2, sX, sY + 22, cTxt2, TextLib::TextStyle::WithTwoPointShadow(GameColors::Yellow2x.r, GameColors::Yellow2x.g, GameColors::Yellow2x.b));
	else TextLib::DrawText(GameFont::Default, sX, sY + 28, cTxt2, TextLib::TextStyle::WithShadow(GameColors::ChatBrightYellow.r, GameColors::ChatBrightYellow.g, GameColors::ChatBrightYellow.b));

	// centu: no muestra la barra de hp de algunos npc
	switch (sOwnerType) {
	case 15:
	case 19:
	case 20:
	case 24:
	case 25:
	case 26:
	case 42:
	case 55:
	case 56:
	case 67:
	case 68:
	case 69:
	case 64:
	{
		switch ((m_entityState.m_sAppr2 & 0xFF00) >> 8) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		default:
			break;
		}
	}
	case 90:
		break;
	default:
		if (iNpcHP > 0)
		{
			m_pSprite[DEF_SPRID_INTERFACE_ND_PARTYSTATUS]->DrawWidth(sX, sY + 16, 18, 75);
			int iBarWidth2 = (iNpcHP * 75) / iNpcMaxHP;
			if (iBarWidth2 < 0) iBarWidth2 = 0;
			if (iBarWidth2 > 75) iBarWidth2 = 75;
			m_pSprite[DEF_SPRID_INTERFACE_ND_PARTYSTATUS]->DrawWidth(sX, sY + 16, 19, iBarWidth2); // 16
		}
		break;
	}
}

void CGame::DrawObjectName(short sX, short sY, char* pName, int iStatus)
{
	char cTxt[64], cTxt2[64];
	short sR, sG, sB;
	int i, iGuildIndex, iFOE, iAddY = 0;
	bool bPK, bCitizen, bAresden, bHunter;
	iFOE = _iGetFOE(iStatus);
	if (iFOE < 0)
	{
		sR = 255; sG = 0; sB = 0;
	}
	else if (iFOE == 0)
	{
		sR = 50; sG = 50; sB = 255;
	}
	else
	{
		sR = 30; sG = 200; sB = 30;
	}
	std::memset(cTxt, 0, sizeof(cTxt));
	std::memset(cTxt2, 0, sizeof(cTxt2));

	if (m_iIlusionOwnerH == 0)
	{
		if (m_bIsCrusadeMode == false) wsprintf(cTxt, "%s", pName);
		else
		{
			if (m_entityState.m_wObjectID >= 10000) strcpy(cTxt, NPC_NAME_MERCENARY); //"Mercenary"
			else
			{
				if (iFOE == -1) wsprintf(cTxt, "%d", m_entityState.m_wObjectID);
				else strcpy(cTxt, pName);
			}
		}
		if (m_iPartyStatus != 0)
		{
			for (i = 0; i < DEF_MAXPARTYMEMBERS; i++)
			{
				if (strcmp(m_stPartyMemberNameList[i].cName, pName) == 0)
				{
					strcat(cTxt, BGET_NPC_NAME23); // ", Party Member"
					break;
				}
			}
		}
	}
	else strcpy(cTxt, "?????");

	if ((iStatus & 0x20) != 0) strcat(cTxt, DRAW_OBJECT_NAME50);//" Berserk"
	if ((iStatus & 0x40) != 0) strcat(cTxt, DRAW_OBJECT_NAME51);//" Frozen"

	TextLib::DrawText(GameFont::Default, sX, sY, cTxt, TextLib::TextStyle::WithShadow(GameColors::UIWhite.r, GameColors::UIWhite.g, GameColors::UIWhite.b));
	std::memset(cTxt, 0, sizeof(cTxt));

	if (memcmp(m_pPlayer->m_cPlayerName, pName, 10) == 0)
	{
		if (m_pPlayer->m_iGuildRank == 0)
		{
			wsprintf(G_cTxt, DEF_MSG_GUILDMASTER, m_pPlayer->m_cGuildName);//" Guildmaster)"
			TextLib::DrawText(GameFont::Default, sX, sY + 14, G_cTxt, TextLib::TextStyle::WithShadow(GameColors::InfoGrayLight.r, GameColors::InfoGrayLight.g, GameColors::InfoGrayLight.b));
			iAddY = 14;
		}
		if (m_pPlayer->m_iGuildRank > 0)
		{
			wsprintf(G_cTxt, DEF_MSG_GUILDSMAN, m_pPlayer->m_cGuildName);//" Guildsman)"
			TextLib::DrawText(GameFont::Default, sX, sY + 14, G_cTxt, TextLib::TextStyle::WithShadow(GameColors::InfoGrayLight.r, GameColors::InfoGrayLight.g, GameColors::InfoGrayLight.b));
			iAddY = 14;
		}
		if (m_pPlayer->m_iPKCount != 0)
		{
			bPK = true;
			sR = 255; sG = 0; sB = 0;
		}
		else
		{
			bPK = false;
			sR = 30; sG = 200; sB = 30;
		}
		bCitizen = m_pPlayer->m_bCitizen;
		bAresden = m_pPlayer->m_bAresden;
		bHunter = m_pPlayer->m_bHunter;
	}
	else
	{	// CLEROTH - CRASH BUG ( STATUS )
		if (iStatus & 0x80000000) bPK = true;
		else bPK = false;
		if (iStatus & 0x40000000) bCitizen = true;
		else bCitizen = false;
		if (iStatus & 0x20000000) bAresden = true;
		else bAresden = false;
		if (iStatus & 0x10000000) bHunter = true;
		else bHunter = false;
		if (m_bIsCrusadeMode == false || iFOE >= 0)
		{
			if (FindGuildName(pName, &iGuildIndex) == true)
			{
				if (m_stGuildName[iGuildIndex].cGuildName[0] != 0)
				{
					if (strcmp(m_stGuildName[iGuildIndex].cGuildName, "NONE") != 0)
					{
						if (m_stGuildName[iGuildIndex].iGuildRank == 0)
						{
							wsprintf(G_cTxt, DEF_MSG_GUILDMASTER, m_stGuildName[iGuildIndex].cGuildName);//
							TextLib::DrawText(GameFont::Default, sX, sY + 14, G_cTxt, TextLib::TextStyle::WithShadow(GameColors::InfoGrayLight.r, GameColors::InfoGrayLight.g, GameColors::InfoGrayLight.b));
							m_stGuildName[iGuildIndex].dwRefTime = m_dwCurTime;
							iAddY = 14;
						}
						else if (m_stGuildName[iGuildIndex].iGuildRank > 0)
						{
							wsprintf(G_cTxt, DEF_MSG_GUILDSMAN, m_stGuildName[iGuildIndex].cGuildName);//"
							TextLib::DrawText(GameFont::Default, sX, sY + 14, G_cTxt, TextLib::TextStyle::WithShadow(GameColors::InfoGrayLight.r, GameColors::InfoGrayLight.g, GameColors::InfoGrayLight.b));
							m_stGuildName[iGuildIndex].dwRefTime = m_dwCurTime;
							iAddY = 14;
						}
					}
					else
					{
						m_stGuildName[iGuildIndex].dwRefTime = 0;
					}
				}
			}
			else bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQGUILDNAME, 0, m_entityState.m_wObjectID, iGuildIndex, 0, 0);
		}
	}

	if (bCitizen == false)	strcpy(cTxt, DRAW_OBJECT_NAME60);// "Traveller"
	else
	{
		if (bAresden)
		{
			if (bHunter == true) strcpy(cTxt, DEF_MSG_ARECIVIL); // "Aresden Civilian"
			else strcpy(cTxt, DEF_MSG_ARESOLDIER); // "Aresden Combatant"
		}
		else
		{
			if (bHunter == true) strcpy(cTxt, DEF_MSG_ELVCIVIL);// "Elvine Civilian"
			else strcpy(cTxt, DEF_MSG_ELVSOLDIER);	// "Elvine Combatant"
		}
	}
	if (bPK == true)
	{
		if (bCitizen == false) strcpy(cTxt, DEF_MSG_PK);	//"Criminal"
		else
		{
			if (bAresden) strcpy(cTxt, DEF_MSG_AREPK);// "Aresden Criminal"
			else strcpy(cTxt, DEF_MSG_ELVPK);  // "Elvine Criminal"
		}
	}
	TextLib::DrawText(GameFont::Default, sX, sY + 14 + iAddY, cTxt, TextLib::TextStyle::WithShadow(sR, sG, sB));
}

bool CGame::FindGuildName(char* pName, int* ipIndex)
{
	int i, iRet = 0;
	uint32_t dwTmpTime;
	for (i = 0; i < DEF_MAXGUILDNAMES; i++)
	{
		if (memcmp(m_stGuildName[i].cCharName, pName, 10) == 0)
		{
			m_stGuildName[i].dwRefTime = m_dwCurTime;
			*ipIndex = i;
			return true;
		}
	}
	dwTmpTime = m_stGuildName[0].dwRefTime;
	for (i = 0; i < DEF_MAXGUILDNAMES; i++)
	{
		if (m_stGuildName[i].dwRefTime < dwTmpTime)
		{
			iRet = i;
			dwTmpTime = m_stGuildName[i].dwRefTime;
		}
	}
	std::memset(m_stGuildName[iRet].cGuildName, 0, sizeof(m_stGuildName[iRet].cGuildName));
	memcpy(m_stGuildName[iRet].cCharName, pName, 10);
	m_stGuildName[iRet].dwRefTime = m_dwCurTime;
	m_stGuildName[iRet].iGuildRank = -1;
	*ipIndex = iRet;
	return false;
}

void CGame::DrawVersion()
{
	std::snprintf(G_cTxt, sizeof(G_cTxt), "Ver: %s", hb::version::GetDisplayString());
	TextLib::DrawText(GameFont::Default, 12 + SCREENX, (LOGICAL_HEIGHT - 12 - 14) + SCREENY, G_cTxt, TextLib::TextStyle::WithShadow(GameColors::UIDisabled.r, GameColors::UIDisabled.g, GameColors::UIDisabled.b));
}

char CGame::GetOfficialMapName(char* pMapName, char* pName)
{	// MapIndex
	if (strcmp(pMapName, "middleland") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME28);	// Middleland
		return 4;
	}
	else if (strcmp(pMapName, "huntzone3") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME31);	// Death Valley
		return 0;
	}
	else if (strcmp(pMapName, "huntzone1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME29);	// Rocky Highland
		return 1;
	}
	else if (strcmp(pMapName, "elvuni") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME57);	// Eldiniel Garden
		return 2;
	}
	else if (strcmp(pMapName, "elvine") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME24);	// Elvine City
		return 3;
	}
	else if (strcmp(pMapName, "elvfarm") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME2);	// Elvine Farm
		return 5;
	}
	else if (strcmp(pMapName, "arefarm") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME1);	// Aresden Farm
		return 6;
	}
	else if (strcmp(pMapName, "default") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME3);	// Beginner Zone
		return 7;
	}
	else if (strcmp(pMapName, "huntzone4") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME32);	// Silent Wood
		return 8;
	}
	else if (strcmp(pMapName, "huntzone2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME30);	// Eternal Field
		return 9;
	}
	else if (strcmp(pMapName, "areuni") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME56);	// Aresien Garden
		return 10;
	}
	else if (strcmp(pMapName, "aresden") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME22);	// Aresden City
		return 11;
	}
	else if (strcmp(pMapName, "dglv2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME25);	// Dungeon L2
		return 12;
	}
	else if (strcmp(pMapName, "dglv3") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME26);	// Dungeon L3
		return 13;
	}
	else if (strcmp(pMapName, "dglv4") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME53);	// Dungeon L4
		return 14;
	}
	else if (strcmp(pMapName, "elvined1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME23);	// Elvine Dungeon
		return 15;
	}
	else if (strcmp(pMapName, "aresdend1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME21);	// Aresden Dungeon
		return 16;
	}
	else if (strcmp(pMapName, "bisle") == 0) {
		strcpy(pName, GET_OFFICIAL_MAP_NAME27);	// Bleeding Island
		return 17;
	}
	else if (strcmp(pMapName, "toh1") == 0) {
		strcpy(pName, GET_OFFICIAL_MAP_NAME60);	// Tower of Hell 1
		return 18;
	}
	else if (strcmp(pMapName, "toh2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME61);	// Tower of Hell 2
		return 19;
	}
	else if (strcmp(pMapName, "toh3") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME62);	// Tower of Hell 3
		return 20;
	}
	else if (strcmp(pMapName, "middled1x") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME58);	// Middleland Mine
		return 21;
	}
	else if (strcmp(pMapName, "middled1n") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME59);	// Middleland Dungeon
		return 22;
	}
	else if (strcmp(pMapName, "2ndmiddle") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME65);	// Promiseland
		return 23;
	}
	else if (strcmp(pMapName, "icebound") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME66);	// Ice Map
		return 24;
		// Snoopy:
	}
	else if (strcmp(pMapName, "druncncity") == 0) // Snoopy: Apocalypse maps
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME70);
		return 25;
	}
	else if (strcmp(pMapName, "inferniaA") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME71);
		return 26;
	}
	else if (strcmp(pMapName, "inferniaB") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME72);
		return 27;
	}
	else if (strcmp(pMapName, "maze") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME73);
		return 28;
	}
	else if (strcmp(pMapName, "procella") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME74);
		return 29;
	}
	else if (strcmp(pMapName, "abaddon") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME75);
		return 30;
	}
	else if (strcmp(pMapName, "BtField") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME76);
		return 35;
	}
	else if (strcmp(pMapName, "GodH") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME77);
		return 36;
	}
	else if (strcmp(pMapName, "HRampart") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME78);
		return 37;
	}
	else if (strcmp(pMapName, "cityhall_1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME35);	// Aresden Cityhall
		return -1;
	}
	else if (strcmp(pMapName, "cityhall_2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME36);	// Elvine Cityhall
		return -1;
	}
	else if (strcmp(pMapName, "gldhall_1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME37);	// Aresden Guildhall
		return -1;
	}
	else if (strcmp(pMapName, "gldhall_2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME38);	// Elvine Guildhall
		return -1;
	}
	else if (memcmp(pMapName, "bsmith_1", 8) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME33);	// Aresden Blacksmith
		return -1;
	}
	else if (memcmp(pMapName, "bsmith_2", 8) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME34);	// Elvine Blacksmith
		return -1;
	}
	else if (memcmp(pMapName, "gshop_1", 7) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME39);	// Aresden Shop
		return -1;
	}
	else if (memcmp(pMapName, "gshop_2", 7) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME40);	// Elvine Shop
		return -1;
	}
	else if (memcmp(pMapName, "wrhus_1", 7) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME43);	// Aresden Warehouse
		return -1;
	}
	else if (memcmp(pMapName, "wrhus_2", 7) == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME44);	// Elvine Warehouse
		return -1;
	}
	else if (strcmp(pMapName, "arewrhus") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME45);	// Aresden Warehouse
		return -1;
	}
	else if (strcmp(pMapName, "elvwrhus") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME46);	// Elvine Warehouse
		return -1;
	}
	else if (strcmp(pMapName, "wzdtwr_1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME41);	// Magic Tower
		return -1;
	}
	else if (strcmp(pMapName, "wzdtwr_2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME42);	// Magic Tower
		return -1;
	}
	else if (strcmp(pMapName, "cath_1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME47);	// Aresien Church
		return -1;
	}
	else if (strcmp(pMapName, "cath_2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME48);	// Eldiniel Church
		return -1;
	}
	else if (strcmp(pMapName, "resurr1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME54);	// Revival Zone
		return -1;
	}
	else if (strcmp(pMapName, "resurr2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME55);	// Revival Zone
		return -1;
	}
	else if (strcmp(pMapName, "arebrk11") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME4);	// Aresden Barrack 1
		return -1;
	}
	else if (strcmp(pMapName, "arebrk12") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME5);	// Aresden Barrack 1
		return -1;
	}
	else if (strcmp(pMapName, "arebrk21") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME6);	// Aresden Barrack 2
		return -1;
	}
	else if (strcmp(pMapName, "arebrk22") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME7);	// Aresden Barrack 2
		return -1;
	}
	else if (strcmp(pMapName, "elvbrk11") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME8);	// Elvine Barrack 1
		return -1;
	}
	else if (strcmp(pMapName, "elvbrk12") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME9);	// Elvine Barrack 1
		return -1;
	}
	else if (strcmp(pMapName, "elvbrk21") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME10);	// Elvine Barrack 2
		return -1;
	}
	else if (strcmp(pMapName, "elvbrk22") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME11);	// Elvine Barrack 2
		return -1;
	}
	else if (strcmp(pMapName, "fightzone1") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME12);	// Arena 1
		return -1;
	}
	else if (strcmp(pMapName, "fightzone2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME13);	// Arena 2
		return -1;
	}
	else if (strcmp(pMapName, "fightzone3") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME14);	// Arena 3
		return -1;
	}
	else if (strcmp(pMapName, "fightzone4") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME15);	// Arena 4
		return -1;
	}
	else if (strcmp(pMapName, "fightzone5") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME16);	// Arena 5
		return -1;
	}
	else if (strcmp(pMapName, "fightzone6") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME17);	// Arena 6
		return -1;
	}
	else if (strcmp(pMapName, "fightzone7") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME18);	// Arena 7
		return -1;
	}
	else if (strcmp(pMapName, "fightzone8") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME19);	// Arena 8
		return -1;
	}
	else if (strcmp(pMapName, "fightzone9") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME20);	// Arena 9
		return -1;
	}
	else if (strcmp(pMapName, "arejail") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME63);	// Aresden Jail
		return -1;
	}
	else if (strcmp(pMapName, "elvjail") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME64);	// Elvine Jail
		return -1;
	}
	else if (strcmp(pMapName, "CmdHall_1") == 0) // Snoopy: Commander Halls
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME79);
		return -1;
	}
	else if (strcmp(pMapName, "CmdHall_2") == 0)
	{
		strcpy(pName, GET_OFFICIAL_MAP_NAME79);
		return -1;
	}
	else
	{
		strcpy(pName, pMapName);
		return -1;
	}
}

bool CGame::bCheckLocalChatCommand(char* pMsg)
{
	return ChatCommandManager::Get().ProcessCommand(pMsg);
}

bool CGame::bCheckItemOperationEnabled(char cItemID)
{
	if (m_pItemList[cItemID] == 0) return false;
	if (m_pPlayer->m_Controller.GetCommand() < 0) return false;
	if (m_bIsTeleportRequested == true) return false;
	if (m_bIsItemDisabled[cItemID] == true) return false;

	if ((m_pItemList[cItemID]->m_sSpriteFrame == 155) && (m_bUsingSlate == true))
	{
		if ((m_cMapIndex == 35) || (m_cMapIndex == 36) || (m_cMapIndex == 37))
		{
			AddEventList(DEF_MSG_NOTIFY_SLATE_WRONG_MAP, 10); // "You cannot use it right here."
			return false;
		}
		AddEventList(DEF_MSG_NOTIFY_SLATE_ALREADYUSING, 10); // Already Using Another Slate
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropExternal) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::NpcActionQuery) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::SellOrRepair) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Manufacture) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::Exchange) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::SellList) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	if (m_dialogBoxManager.IsEnabled(DialogBoxId::ItemDropConfirm) == true)
	{
		AddEventList(BCHECK_ITEM_OPERATION_ENABLE1, 10);
		return false;
	}

	return true;
}

void CGame::ClearSkillUsingStatus()
{
	if (m_bSkillUsingStatus == true)
	{
		AddEventList(CLEAR_SKILL_USING_STATUS1, 10);//"
		m_dialogBoxManager.DisableDialogBox(DialogBoxId::Fishing);
		m_dialogBoxManager.DisableDialogBox(DialogBoxId::Manufacture);
		if ((m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6)/* && ((m_pPlayer->m_sPlayerAppr2 & 0xF000) == 0)*/) {
			m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			m_pPlayer->m_Controller.SetDestination(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);
		}
	}
	m_bSkillUsingStatus = false;
}

void CGame::NpcTalkHandler(char* pData)
{
	char cRewardName[21], cTargetName[21], cTemp[21], cTxt[250];
	short sType, sResponse;
	int iAmount, iIndex, iContribution, iX, iY, iRange;
	int iTargetType, iTargetCount, iQuestionType;

	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyNpcTalk>(
		pData, sizeof(hb::net::PacketNotifyNpcTalk));
	if (!pkt) return;

	sType = pkt->type;
	sResponse = pkt->response;
	iAmount = pkt->amount;
	iContribution = pkt->contribution;
	iTargetType = pkt->target_type;
	iTargetCount = pkt->target_count;
	iX = pkt->x;
	iY = pkt->y;
	iRange = pkt->range;

	std::memset(cRewardName, 0, sizeof(cRewardName));
	memcpy(cRewardName, pkt->reward_name, 20);
	std::memset(cTargetName, 0, sizeof(cTargetName));
	memcpy(cTargetName, pkt->target_name, 20);

	m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcTalk, sResponse, sType, 0);

	if ((sType >= 1) && (sType <= 100))
	{
		iIndex = m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sV1;
		m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
		iIndex++;
		iQuestionType = 0;
		switch (sType) {
		case 1: //Monster Hunt
			std::memset(cTemp, 0, sizeof(cTemp));
			GetNpcName(iTargetType, cTemp);
			std::memset(cTxt, 0, sizeof(cTxt));
			wsprintf(cTxt, NPC_TALK_HANDLER16, iTargetCount, cTemp);
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER17);//"
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			else {
				std::memset(cTemp, 0, sizeof(cTemp));
				GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER18, cTemp);//"Map : %s"
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;

				if (iX != 0) {
					std::memset(cTxt, 0, sizeof(cTxt));
					wsprintf(cTxt, NPC_TALK_HANDLER19, iX, iY, iRange);//"Position: %d,%d within %d blocks"
					m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
					iIndex++;
				}

				std::memset(cTxt, 0, sizeof(cTxt));
				wsprintf(cTxt, NPC_TALK_HANDLER20, iContribution);//"
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			iQuestionType = 1;
			break;

		case 7: //
			std::memset(cTxt, 0, sizeof(cTxt));
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, NPC_TALK_HANDLER21, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER22);
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			else {
				std::memset(cTemp, 0, sizeof(cTemp));
				GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER23, cTemp);
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;

				if (iX != 0) {
					std::memset(cTxt, 0, sizeof(cTxt));
					wsprintf(cTxt, NPC_TALK_HANDLER24, iX, iY, iRange);
					m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
					iIndex++;
				}

				std::memset(cTxt, 0, sizeof(cTxt));
				wsprintf(cTxt, NPC_TALK_HANDLER25, iContribution);
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			iQuestionType = 1;
			break;

		case 10: // Crusade
			std::memset(cTxt, 0, sizeof(cTxt));
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, NPC_TALK_HANDLER26, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			strcpy(cTxt, NPC_TALK_HANDLER27);//"
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			strcpy(cTxt, NPC_TALK_HANDLER28);//"
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			strcpy(cTxt, NPC_TALK_HANDLER29);//"
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			strcpy(cTxt, NPC_TALK_HANDLER30);//"
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			strcpy(cTxt, " ");
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
			iIndex++;

			std::memset(cTxt, 0, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER31);//"
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			else {
				std::memset(cTemp, 0, sizeof(cTemp));
				GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER32, cTemp);//"
				m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, cTxt, 0);
				iIndex++;
			}
			iQuestionType = 2;
			break;
		}

		switch (iQuestionType) {
		case 1:
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
			iIndex++;
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, NPC_TALK_HANDLER33, 0);//"
			iIndex++;
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, NPC_TALK_HANDLER34, 0);//"
			iIndex++;
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
			iIndex++;
			break;

		case 2:
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
			iIndex++;
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, NPC_TALK_HANDLER35, 0);//"
			iIndex++;
			m_pMsgTextList2[iIndex] = std::make_unique<CMsg>(0, "  ", 0);
			iIndex++;
			break;

		default: break;
		}
	}
}

void CGame::GetNpcName(short sType, char* pName)
{
	switch (sType)
	{
	case 10: strcpy(pName, NPC_NAME_SLIME); break;
	case 11: strcpy(pName, NPC_NAME_SKELETON); break;
	case 12: strcpy(pName, NPC_NAME_STONEGOLEM); break;
	case 13: strcpy(pName, NPC_NAME_CYCLOPS); break;
	case 14: strcpy(pName, NPC_NAME_ORC); break;
	case 15: strcpy(pName, NPC_NAME_SHOP_KEEPER); break;
	case 16: strcpy(pName, NPC_NAME_GIANTANT); break;
	case 17: strcpy(pName, NPC_NAME_GIANTSCORPION); break;
	case 18: strcpy(pName, NPC_NAME_ZOMBIE); break;
	case 19: strcpy(pName, NPC_NAME_MAGICIAN); break;
	case 20: strcpy(pName, NPC_NAME_WAREHOUSE_KEEPER); break;
	case 21: strcpy(pName, NPC_NAME_GUARD); break;
	case 22: strcpy(pName, NPC_NAME_SNAKE); break;
	case 23: strcpy(pName, NPC_NAME_CLAYGOLEM); break;
	case 24: strcpy(pName, NPC_NAME_BLACKSMITH_KEEPER); break;
	case 25: strcpy(pName, NPC_NAME_CITYHALL_OFFICER); break;
	case 26: strcpy(pName, NPC_NAME_GUILDHALL_OFFICER); break;
	case 27: strcpy(pName, NPC_NAME_HELHOUND); break;
	case 28: strcpy(pName, NPC_NAME_TROLL); break;
	case 29: strcpy(pName, NPC_NAME_OGRE); break;
	case 30: strcpy(pName, NPC_NAME_LICHE); break;
	case 31: strcpy(pName, NPC_NAME_DEMON); break;
	case 32: strcpy(pName, NPC_NAME_UNICORN); break;
	case 33: strcpy(pName, NPC_NAME_WEREWOLF); break;
	case 34: strcpy(pName, NPC_NAME_DUMMY); break;
	case 35: strcpy(pName, NPC_NAME_ENERGYSPHERE); break;
	case 36:
		if (m_entityState.m_sAppr2 != 0) strcpy(pName, NPC_NAME_ARROWGUARDTOWER_CK);
		else strcpy(pName, NPC_NAME_ARROWGUARDTOWER);
		break;
	case 37:
		if (m_entityState.m_sAppr2 != 0) strcpy(pName, NPC_NAME_CANNONGUARDTOWER_CK);
		else strcpy(pName, NPC_NAME_CANNONGUARDTOWER);
		break;
	case 38:
		if (m_entityState.m_sAppr2 != 0) strcpy(pName, NPC_NAME_MANACOLLECTOR_CK);
		else strcpy(pName, NPC_NAME_MANACOLLECTOR);
		break;
	case 39:
		if (m_entityState.m_sAppr2 != 0) strcpy(pName, NPC_NAME_DETECTOR_CK);
		else strcpy(pName, NPC_NAME_DETECTOR);
		break;
	case 40: strcpy(pName, NPC_NAME_ENERGYSHIELD); break;
	case 41: strcpy(pName, NPC_NAME_GRANDMAGICGENERATOR); break;
	case 42: strcpy(pName, NPC_NAME_MANASTONE); break;
	case 43: strcpy(pName, NPC_NAME_LIGHTWARBEETLE); break;
	case 44: strcpy(pName, NPC_NAME_GODSHANDKNIGHT); break;
	case 45: strcpy(pName, NPC_NAME_GODSHANDKNIGHT_CK); break;
	case 46: strcpy(pName, NPC_NAME_TEMPLEKNIGHT); break;
	case 47: strcpy(pName, NPC_NAME_BATTLEGOLEM); break;
	case 48: strcpy(pName, NPC_NAME_STALKER); break;
	case 49: strcpy(pName, NPC_NAME_HELLCLAW); break;
	case 50: strcpy(pName, NPC_NAME_TIGERWORM); break;
	case 51: strcpy(pName, NPC_NAME_CATAPULT); break;
	case 52: strcpy(pName, NPC_NAME_GARGOYLE); break;
	case 53: strcpy(pName, NPC_NAME_BEHOLDER); break;
	case 54: strcpy(pName, NPC_NAME_DARKELF); break;
	case 55: strcpy(pName, NPC_NAME_RABBIT); break;
	case 56: strcpy(pName, NPC_NAME_CAT); break;
	case 57: strcpy(pName, NPC_NAME_FROG); break;
	case 58: strcpy(pName, NPC_NAME_MOUNTAIN_GIANT); break;
	case 59: strcpy(pName, NPC_NAME_ETTIN); break;
	case 60: strcpy(pName, NPC_NAME_CANNIBAL); break;
	case 61: strcpy(pName, NPC_NAME_RUDOLPH); break;
	case 62: strcpy(pName, NPC_NAME_DIREBOAR); break;
	case 63: strcpy(pName, NPC_NAME_FROST); break;
	case 64:
	{
		switch ((m_entityState.m_sAppr2 & 0xFF00) >> 8) {
		case 1:	strcpy(pName, NPC_NAME_WATERMELON);	break;
		case 2: strcpy(pName, NPC_NAME_PUMPKIN); break;
		case 3: strcpy(pName, NPC_NAME_GARLIC); break;
		case 4: strcpy(pName, NPC_NAME_BARLEY); break;
		case 5:	strcpy(pName, NPC_NAME_CARROT); break;
		case 6: strcpy(pName, NPC_NAME_RADISH); break;
		case 7: strcpy(pName, NPC_NAME_CORN); break;
		case 8: strcpy(pName, NPC_NAME_BFLOWER); break;
		case 9: strcpy(pName, NPC_NAME_MELON); break;
		case 10: strcpy(pName, NPC_NAME_TOMATO); break;
		case 11: strcpy(pName, NPC_NAME_GRAPPE); break;
		case 12: strcpy(pName, NPC_NAME_BLUEGRAPPE); break;
		case 13: strcpy(pName, NPC_NAME_MUSHROM); break;
		case 14: strcpy(pName, NPC_NAME_GINSENG); break;
		default: strcpy(pName, NPC_NAME_CROP); break;
		}
	}
	break;
	case 65: strcpy(pName, NPC_NAME_ICEGOLEM); break;
	case 66: strcpy(pName, NPC_NAME_WYVERN); break;
	case 67: strcpy(pName, NPC_NAME_MCGAFFIN); break;
	case 68: strcpy(pName, NPC_NAME_PERRY); break;
	case 69: strcpy(pName, NPC_NAME_DEVLIN); break;

	case 70: strcpy(pName, NPC_NAME_DRAGON); break;
	case 71: strcpy(pName, NPC_NAME_CENTAUR); break;
	case 72: strcpy(pName, NPC_NAME_CLAWTUR); break;
	case 73: strcpy(pName, NPC_NAME_FIREWYV); break;
	case 74: strcpy(pName, NPC_NAME_GICRAYF); break;
	case 75: strcpy(pName, NPC_NAME_GILIZAR); break;
	case 76: strcpy(pName, NPC_NAME_GITREE); break;
	case 77: strcpy(pName, NPC_NAME_MASTORC); break;
	case 78: strcpy(pName, NPC_NAME_MINAUS); break;
	case 79: strcpy(pName, NPC_NAME_NIZIE); break;

	case 80: strcpy(pName, NPC_NAME_TENTOCL); break;
	case 81: strcpy(pName, NPC_NAME_ABADDON); break;
	case 82: strcpy(pName, NPC_NAME_SORCERS); break;
	case 83: strcpy(pName, NPC_NAME_ATK); break;
	case 84: strcpy(pName, NPC_NAME_MASTELF); break;
	case 85: strcpy(pName, NPC_NAME_DSK); break;
	case 86: strcpy(pName, NPC_NAME_HBT); break;
	case 87: strcpy(pName, NPC_NAME_CT); break;
	case 88: strcpy(pName, NPC_NAME_BARBAR); break;
	case 89: strcpy(pName, NPC_NAME_AGC); break;
	case 90: strcpy(pName, NPC_NAME_GAIL); break;
	case 91: strcpy(pName, NPC_NAME_GATE); break;

		// CLEROTH - NEW MONSTERS
	case 110: strcpy(pName, NPC_NAME_AIRLEMENTAL); break;
	}
}

void CGame::_CalcSocketClosed()
{
	if (GameModeManager::GetMode() == GameMode::MainGame)
	{
		m_pGSock.reset();
		m_pGSock.reset();
		PlaySound('E', 14, 5);
		AudioManager::Get().StopSound(SoundType::Effect, 38);
		AudioManager::Get().StopMusic();
		ChangeGameMode(GameMode::Quit);
	}
}

void CGame::PointCommandHandler(int indexX, int indexY, char cItemID)
{
	char cTemp[31];
	if ((m_iPointCommandType >= 100) && (m_iPointCommandType < 200))
	{
		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_MAGIC, 0, indexX, indexY, m_iPointCommandType, 0);
	}
	else if ((m_iPointCommandType >= 0) && (m_iPointCommandType < 50))
	{
		bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_USEITEM, 0, m_iPointCommandType, indexX, indexY, cTemp, cItemID); // v1.4

		if (m_pItemList[m_iPointCommandType]->m_cItemType == DEF_ITEMTYPE_USE_SKILL)
			m_bSkillUsingStatus = true;
	}
	else if (m_iPointCommandType == 200) // Normal Hand
	{
		if ((strlen(m_cMCName) == 0) || (strcmp(m_cMCName, m_pPlayer->m_cPlayerName) == 0) || (m_cMCName[0] == '_'))
		{
			m_dialogBoxManager.Info(DialogBoxId::Party).cMode = 0;
			PlaySound('E', 14, 5);
			AddEventList(POINT_COMMAND_HANDLER1, 10);
		}
		else
		{
			m_dialogBoxManager.Info(DialogBoxId::Party).cMode = 3;
			PlaySound('E', 14, 5);
			std::memset(m_dialogBoxManager.Info(DialogBoxId::Party).cStr, 0, sizeof(m_dialogBoxManager.Info(DialogBoxId::Party).cStr));
			strcpy(m_dialogBoxManager.Info(DialogBoxId::Party).cStr, m_cMCName);
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQUEST_JOINPARTY, 0, 1, 0, 0, m_cMCName);
			return;
		}
	}
}


void CGame::StartBGM()
{
	// Determine track name based on current location
	const char* trackName = "MainTm";

	if ((m_bIsXmas == true) && (m_cWhetherEffectType >= 4))
	{
		trackName = "Carol";
	}
	else if (memcmp(m_cCurLocation, "aresden", 7) == 0)
	{
		trackName = "aresden";
	}
	else if (memcmp(m_cCurLocation, "elvine", 6) == 0)
	{
		trackName = "elvine";
	}
	else if (memcmp(m_cCurLocation, "dglv", 4) == 0)
	{
		trackName = "dungeon";
	}
	else if (memcmp(m_cCurLocation, "middled1", 8) == 0)
	{
		trackName = "dungeon";
	}
	else if (memcmp(m_cCurLocation, "middleland", 10) == 0)
	{
		trackName = "middleland";
	}
	else if (memcmp(m_cCurLocation, "druncncity", 10) == 0)
	{
		trackName = "druncncity";
	}
	else if (memcmp(m_cCurLocation, "inferniaA", 9) == 0)
	{
		trackName = "middleland";
	}
	else if (memcmp(m_cCurLocation, "inferniaB", 9) == 0)
	{
		trackName = "middleland";
	}
	else if (memcmp(m_cCurLocation, "maze", 4) == 0)
	{
		trackName = "dungeon";
	}
	else if (memcmp(m_cCurLocation, "abaddon", 7) == 0)
	{
		trackName = "abaddon";
	}

	// Forward to AudioManager
	AudioManager::Get().PlayMusic(trackName);
}

void CGame::MotionResponseHandler(char* pData)
{
	WORD wResponse;
	short sX, sY;
	char cDir;
	int iPreHP;
	//						          0 3        4 5						 6 7		8 9		   10	    11
	// Confirm Code(4) | MsgSize(4) | MsgID(4) | DEF_OBJECTMOVE_CONFIRM(2) | Loc-X(2) | Loc-Y(2) | Dir(1) | MapData ...
	// Confirm Code(4) | MsgSize(4) | MsgID(4) | DEF_OBJECTMOVE_REJECT(2)  | Loc-X(2) | Loc-Y(2)
	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(
		pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	wResponse = header->msg_type;

	switch (wResponse) {
	case DEF_OBJECTMOTION_CONFIRM:
		m_pPlayer->m_Controller.DecrementCommandCount();
		break;

	case DEF_OBJECTMOTION_ATTACK_CONFIRM:
		m_pPlayer->m_Controller.DecrementCommandCount();
		if ((m_wLastAttackTargetID >= 10000) && (m_wLastAttackTargetID < 30000)) {
			bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_GETNPCHP, 0, m_wLastAttackTargetID, 0, 0, 0);
		}
		break;

	case DEF_OBJECTMOTION_REJECT:
		if (m_pPlayer->m_iHP <= 0) return;
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseMotionReject>(
				pData, sizeof(hb::net::PacketResponseMotionReject));
			if (!pkt) return;
			m_pPlayer->m_sPlayerX = pkt->x;
			m_pPlayer->m_sPlayerY = pkt->y;
		}

		m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
		m_pPlayer->m_Controller.SetDestination(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);

		m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
			m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
			m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
			DEF_OBJECTSTOP, 0, 0, 0);
		m_pPlayer->m_Controller.ResetCommandCount();
		m_bIsGetPointingMode = false;
		m_Camera.SnapTo((m_pPlayer->m_sPlayerX - VIEW_CENTER_TILE_X) * 32, (m_pPlayer->m_sPlayerY - (VIEW_CENTER_TILE_Y + 1)) * 32);
		break;

	case DEF_OBJECTMOVE_CONFIRM:
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseMotionMoveConfirm>(
			pData, sizeof(hb::net::PacketResponseMotionMoveConfirm));
		if (!pkt) return;
		sX = pkt->x;
		sY = pkt->y;
		cDir = static_cast<char>(pkt->dir);
		m_pPlayer->m_iSP = m_pPlayer->m_iSP - pkt->stamina_cost;
		if (m_pPlayer->m_iSP < 0) m_pPlayer->m_iSP = 0;
		// v1.3
		//m_iOccupyStatus = (int)*cp;
		iPreHP = m_pPlayer->m_iHP;
		m_pPlayer->m_iHP = pkt->hp;

		if (m_pPlayer->m_iHP != iPreHP)
		{
			if (m_pPlayer->m_iHP < iPreHP)
			{
				wsprintf(G_cTxt, NOTIFYMSG_HP_DOWN, iPreHP - m_pPlayer->m_iHP);
				AddEventList(G_cTxt, 10);
				m_dwDamagedTime = GameClock::GetTimeMS();
				if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
				{
					m_cLogOutCount = -1;
					AddEventList(MOTION_RESPONSE_HANDLER2, 10);
				}
			}
			else
			{
				wsprintf(G_cTxt, NOTIFYMSG_HP_UP, m_pPlayer->m_iHP - iPreHP);
				AddEventList(G_cTxt, 10);
			}
		}
		m_pMapData->ShiftMapData(cDir);
		const char* mapData = reinterpret_cast<const char*>(pData) + sizeof(hb::net::PacketResponseMotionMoveConfirm);
		_ReadMapData(sX, sY, mapData);
		m_pPlayer->m_Controller.DecrementCommandCount();
	}
	break;

	case DEF_OBJECTMOVE_REJECT:
		if (m_pPlayer->m_iHP <= 0) return;
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseMotionMoveReject>(
				pData, sizeof(hb::net::PacketResponseMotionMoveReject));
			if (!pkt) return;
			if (m_pPlayer->m_sPlayerObjectID != pkt->object_id) return;
			m_pPlayer->m_sPlayerX = pkt->x;
			m_pPlayer->m_sPlayerY = pkt->y;
			m_pPlayer->m_sPlayerType = pkt->type;
			m_pPlayer->m_iPlayerDir = static_cast<char>(pkt->dir);
			m_pPlayer->m_sPlayerAppr1 = pkt->appr1;
			m_pPlayer->m_sPlayerAppr2 = pkt->appr2;
			m_pPlayer->m_sPlayerAppr3 = pkt->appr3;
			m_pPlayer->m_sPlayerAppr4 = pkt->appr4;
			m_pPlayer->m_iPlayerApprColor = pkt->appr_color;
			m_pPlayer->m_iPlayerStatus = pkt->status;
		}
		m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
		m_pPlayer->m_Controller.SetDestination(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);
		m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
			m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor, // v1.4
			m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
			DEF_OBJECTSTOP, 0, 0, 0,
			0, 7);
		m_pPlayer->m_Controller.ResetCommandCount();
		m_bIsGetPointingMode = false;
		m_Camera.SnapTo((m_pPlayer->m_sPlayerX - VIEW_CENTER_TILE_X) * 32, (m_pPlayer->m_sPlayerY - (VIEW_CENTER_TILE_Y + 1)) * 32);
		m_pPlayer->m_Controller.SetPrevMoveBlocked(true);
		switch (m_pPlayer->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			PlaySound('C', 12, 0);
			break;
		case 4:
		case 5:
		case 6:
			PlaySound('C', 13, 0);
			break;
		}
		//m_pPlayer->m_Controller.SetCommandAvailable(true);
		break;
	}
}

void CGame::CommandProcessor(short msX, short msY, short indexX, short indexY, char cLB, char cRB)
{
	char   cDir, absX, absY, cName[12];
	short  sX, sY, sObjectType, tX, tY;
	int iObjectStatus;
	int    iRet;
	uint32_t dwTime = GameClock::GetTimeMS();
	uint16_t wType = 0;
	int i;//, iFOE;
	char   cTxt[120];

	char  pDstName[21];
	short sDstOwnerType;
	int iDstOwnerStatus;

	bool  bGORet;
	// Fixed by Snoopy
	if ((m_bIsObserverCommanded == false) && (m_bIsObserverMode == true))
	{
		if ((msX == 0) && (msY == 0) && (m_Camera.GetDestinationX() > 32 * VIEW_TILE_WIDTH) && (m_Camera.GetDestinationY() > 32 * VIEW_TILE_HEIGHT))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 8, 0, 0, 0, 0);
		else if ((msX == LOGICAL_MAX_X) && (msY == 0) && (m_Camera.GetDestinationX() < 32 * m_pMapData->m_sMapSizeX - 32 * VIEW_TILE_WIDTH) && (m_Camera.GetDestinationY() > 32 * VIEW_TILE_HEIGHT))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 2, 0, 0, 0, 0);
		else if ((msX == LOGICAL_MAX_X) && (msY == LOGICAL_MAX_Y) && (m_Camera.GetDestinationX() < 32 * m_pMapData->m_sMapSizeX - 32 * VIEW_TILE_WIDTH) && (m_Camera.GetDestinationY() < 32 * m_pMapData->m_sMapSizeY - 32 * VIEW_TILE_HEIGHT))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 4, 0, 0, 0, 0);
		else if ((msX == 0) && (msY == LOGICAL_MAX_Y))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 6, 0, 0, 0, 0);
		else if ((msX == 0) && (m_Camera.GetDestinationX() > 32 * VIEW_TILE_WIDTH))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 7, 0, 0, 0, 0);
		else if ((msX == LOGICAL_MAX_X) && (m_Camera.GetDestinationX() < 32 * m_pMapData->m_sMapSizeX - 32 * VIEW_TILE_WIDTH))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 3, 0, 0, 0, 0);
		else if ((msY == 0) && (m_Camera.GetDestinationY() > 32 * VIEW_TILE_HEIGHT))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 1, 0, 0, 0, 0);
		else if ((msY == LOGICAL_MAX_Y) && (m_Camera.GetDestinationY() < 32 * m_pMapData->m_sMapSizeY - 32 * VIEW_TILE_HEIGHT))
			bSendCommand(MSGID_REQUEST_PANNING, 0, 5, 0, 0, 0, 0);
		else return;

		m_bIsObserverCommanded = true;
		m_cArrowPressed = 0;
		return;
	}

	if (m_bIsObserverMode == true) return;

	if (Input::IsAltDown()) // [ALT]
		m_pPlayer->m_bSuperAttackMode = true;
	else m_pPlayer->m_bSuperAttackMode = false;

	switch (static_cast<char>(CursorTarget::GetCursorStatus())) {
	case DEF_CURSORSTATUS_NULL:
		if (cLB != 0)
		{
			iRet = m_dialogBoxManager.HandleMouseDown(msX, msY);
			if (iRet == 1)
			{
				CursorTarget::SetCursorStatus(CursorStatus::Selected);
				return;
			}
			else if (iRet == 0)
			{
				CursorTarget::SetCursorStatus(CursorStatus::Pressed);
				// Snoopy: Added Golden LevelUp
				if ((msX > LEVELUP_TEXT_X) && (msX < (LEVELUP_TEXT_X)+75) && (msY > LEVELUP_TEXT_Y) && (msY < (LEVELUP_TEXT_Y)+21))
				{
					if (m_pPlayer->m_iHP > 0)
					{
						if ((m_dialogBoxManager.IsEnabled(DialogBoxId::LevelUpSetting) != true) && (m_pPlayer->m_iLU_Point > 0))
						{
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::LevelUpSetting, 0, 0, 0);
							PlaySound('E', 14, 5);
						}
					}
					else // Centuu : restart
					{
						if (m_cRestartCount == -1)
						{
							m_cRestartCount = 5;
							m_dwRestartCountTime = GameClock::GetTimeMS();
							wsprintf(G_cTxt, DLGBOX_CLICK_SYSMENU1, m_cRestartCount); // "Restarting game....%d"
							AddEventList(G_cTxt, 10);
							PlaySound('E', 14, 5);

						}
					}
					return;
				}
			}
			else if (iRet == -1)
			{
				// Scroll/slider claimed - set status to prevent re-processing
				CursorTarget::SetCursorStatus(CursorStatus::Selected);
				return;
			}
		}
		else if (cRB != 0)
		{
			if (m_dialogBoxManager.HandleRightClick(msX, msY, dwTime)) return;
		}
		break;
	case DEF_CURSORSTATUS_PRESSED:
		if (cLB == 0) // Normal Click
		{
			CursorTarget::SetCursorStatus(CursorStatus::Null);
		}
		break;
	case DEF_CURSORSTATUS_SELECTED:
		if (cLB == 0)
		{
			CursorTarget::SetCursorStatus(CursorStatus::Null);
			//ZeroEoyPnk - Bye delay...
			if (((m_dialogBoxManager.IsEnabled(DialogBoxId::LevelUpSetting) != true) || (CursorTarget::GetSelectedID() != 12))
				&& ((m_dialogBoxManager.IsEnabled(DialogBoxId::ChangeStatsMajestic) != true) || (CursorTarget::GetSelectedID() != 42)))
			{
				if (((dwTime - CursorTarget::GetSelectionClickTime()) < DEF_DOUBLECLICKTIME) 	// Double Click
					&& (abs(msX - CursorTarget::GetSelectionClickX()) <= DEF_DOUBLECLICKTOLERANCE)
					&& (abs(msY - CursorTarget::GetSelectionClickY()) <= DEF_DOUBLECLICKTOLERANCE))
				{
					CursorTarget::ResetSelectionClickTime(); // Reset to prevent triple-click
					m_dialogBoxManager.HandleDoubleClick(msX, msY);
				}
				else // Click
				{
					m_dialogBoxManager.HandleClick(msX, msY);
				}
			}
			else
			{
				m_dialogBoxManager.HandleClick(msX, msY);
			}
			CursorTarget::RecordSelectionClick(msX, msY, dwTime);
			if (CursorTarget::GetSelectedType() == SelectedObjectType::Item)
			{
				if (!m_dialogBoxManager.HandleDraggingItemRelease(msX, msY))
				{
					bItemDrop_ExternalScreen((char)CursorTarget::GetSelectedID(), msX, msY);
				}
			}
			// Always clear selection after click-release to prevent stale state
			CursorTarget::ClearSelection();
			return;
		}
		else 			// v2.05 01-11-30
		{
			if ((m_pMapData->bIsTeleportLoc(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY) == true) && (m_pPlayer->m_Controller.GetCommandCount() == 0)) goto CP_SKIPMOUSEBUTTONSTATUS;

			if ((CursorTarget::GetPrevX() != msX) || (CursorTarget::GetPrevY() != msY))
			{
				CursorTarget::SetCursorStatus(CursorStatus::Dragging);
				CursorTarget::SetPrevPosition(msX, msY);
				if ((CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox) &&
					(CursorTarget::GetSelectedID() == 30))
				{
				}

				if ((CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox) &&
					(CursorTarget::GetSelectedID() == 7) && (m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 1))
				{
					EndInputString();
					m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 20;
				}
				// Query Drop Item Amount
				if ((CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox) &&
					(CursorTarget::GetSelectedID() == 17) && (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 1))
				{
					EndInputString();
					m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode = 20;
				}
				return;
			}
			if ((m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTMOVE) || (m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTRUN)) goto MOTION_COMMAND_PROCESS;
			return;
		}
		break;
	case DEF_CURSORSTATUS_DRAGGING:
		if (cLB != 0)
		{
			if ((m_pMapData->bIsTeleportLoc(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY) == true) && (m_pPlayer->m_Controller.GetCommandCount() == 0)) goto CP_SKIPMOUSEBUTTONSTATUS;
			if (CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox)
			{
				// HudPanel is fixed and cannot be moved
				if (CursorTarget::GetSelectedID() != DialogBoxId::HudPanel)
				{
					m_dialogBoxManager.Info(CursorTarget::GetSelectedID()).sX = msX - CursorTarget::GetDragDistX();
					m_dialogBoxManager.Info(CursorTarget::GetSelectedID()).sY = msY - CursorTarget::GetDragDistY();
				}
			}
			CursorTarget::SetPrevPosition(msX, msY);

			if ((m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTMOVE) || (m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTRUN)) goto MOTION_COMMAND_PROCESS;
			return;
		}
		if (cLB == 0) {
			CursorTarget::SetCursorStatus(CursorStatus::Null);
			switch (static_cast<char>(CursorTarget::GetSelectedType())) {
			case static_cast<char>(SelectedObjectType::DialogBox):
				if ((CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox) &&
					(CursorTarget::GetSelectedID() == 7) && (m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode == 20))
				{
					sX = m_dialogBoxManager.Info(DialogBoxId::GuildMenu).sX;
					sY = m_dialogBoxManager.Info(DialogBoxId::GuildMenu).sY;
					StartInputString(sX + 75, sY + 140, 21, m_pPlayer->m_cGuildName);
					m_dialogBoxManager.Info(DialogBoxId::GuildMenu).cMode = 1;
				}

				if ((CursorTarget::GetSelectedType() == SelectedObjectType::DialogBox) &&
					(CursorTarget::GetSelectedID() == 17) && (m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode == 20))
				{	// Query Drop Item Amount
					sX = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sX;
					sY = m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).sY;
					StartInputString(sX + 40, sY + 57, 11, m_cAmountString);
					m_dialogBoxManager.Info(DialogBoxId::ItemDropExternal).cMode = 1;
				}

				if (CursorTarget::GetSelectedID() == 9)
				{
					{
						if (msX < 400) //LifeX Fix Map
						{
							m_dialogBoxManager.Info(DialogBoxId::GuideMap).sX = 0;
						}
						else
						{
							m_dialogBoxManager.Info(DialogBoxId::GuideMap).sX = LOGICAL_MAX_X - m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeX;
						}

						if (msY < 273)
						{
							m_dialogBoxManager.Info(DialogBoxId::GuideMap).sY = 0;
						}
						else
						{
							m_dialogBoxManager.Info(DialogBoxId::GuideMap).sY = 547 - m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeY;
						}
					}
				}

				CursorTarget::ClearSelection();
				break;

			case static_cast<char>(SelectedObjectType::Item):
				if (!m_dialogBoxManager.HandleDraggingItemRelease(msX, msY))
				{
					bItemDrop_ExternalScreen((char)CursorTarget::GetSelectedID(), msX, msY);
				}
				CursorTarget::ClearSelection();
				break;

			default:
				CursorTarget::ClearSelection();
				break;
			}
			return;
		}
		break;
	}

CP_SKIPMOUSEBUTTONSTATUS:;
	if (m_pPlayer->m_Controller.IsCommandAvailable() == false) return;
	if ((dwTime - m_pPlayer->m_Controller.GetCommandTime()) < 300)
	{
		m_pGSock.reset();
		m_pGSock.reset();
		PlaySound('E', 14, 5);
		AudioManager::Get().StopSound(SoundType::Effect, 38);
		AudioManager::Get().StopMusic();
		ChangeGameMode(GameMode::MainMenu);
		return;
	}
	if (m_pPlayer->m_iHP <= 0) return;

	if (m_pPlayer->m_sDamageMove != 0)
	{
		m_pPlayer->m_Controller.SetCommand(DEF_OBJECTDAMAGEMOVE);
		goto MOTION_COMMAND_PROCESS;
	}

	if ((m_pMapData->bIsTeleportLoc(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY) == true) && (m_pPlayer->m_Controller.GetCommandCount() == 0))
		RequestTeleportAndWaitData();

	// indexX, indexY
	if (cLB != 0) // Mouse Left button
	{
		if (m_bIsGetPointingMode == true)
		{
			if ((m_sMCX != 0) || (m_sMCY != 0))
				PointCommandHandler(m_sMCX, m_sMCY);
			else PointCommandHandler(indexX, indexY);

			m_pPlayer->m_Controller.SetCommandAvailable(false);
			m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
			m_bIsGetPointingMode = false;
			return;
		}

		m_pMapData->bGetOwner(m_sMCX, m_sMCY - 1, cName, &sObjectType, &iObjectStatus, &m_wCommObjectID); // v1.4
		//m_pMapData->m_pData[dX][dY].m_sItemSprite
		if (memcmp(m_cMCName, m_pPlayer->m_cPlayerName, 10) == 0 && (sObjectType <= 6 || (m_pMapData->m_pData[m_pPlayer->m_sPlayerX - m_pMapData->m_sPivotX][m_pPlayer->m_sPlayerY - m_pMapData->m_sPivotY].m_sItemID != 0 && m_pItemConfigList[m_pMapData->m_pData[m_pPlayer->m_sPlayerX - m_pMapData->m_sPivotX][m_pPlayer->m_sPlayerY - m_pMapData->m_sPivotY].m_sItemID]->m_sSprite != 0)))
		{//if (memcmp(m_cMCName, m_pPlayer->m_cPlayerName, 10) == 0 && ( sObjectType <= 6 || m_pMapData->m_pData[15][15].m_sItemSprite != 0 )) {
		 //if (memcmp(m_cMCName, m_pPlayer->m_cPlayerName, 10) == 0 && sObjectType <= 6){
			if ((m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6)/* && ((m_pPlayer->m_sPlayerAppr2 & 0xF000) == 0)*/)
			{
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTGETITEM);
				m_pPlayer->m_Controller.SetDestination(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);
			}
		}
		else
		{
			if (memcmp(m_cMCName, m_pPlayer->m_cPlayerName, 10) == 0) m_sMCY -= 1;
			if ((m_sMCX != 0) && (m_sMCY != 0)) // m_sMCX, m_sMCY
			{
				if (Input::IsCtrlDown() == true)
				{
					m_pMapData->bGetOwner(m_sMCX, m_sMCY, cName, &sObjectType, &iObjectStatus, &m_wCommObjectID);
					if ((iObjectStatus & 0x10) != 0) return;
					if ((sObjectType == 15) || (sObjectType == 20) || (sObjectType == 24)) return;
					absX = abs(m_pPlayer->m_sPlayerX - m_sMCX);
					absY = abs(m_pPlayer->m_sPlayerY - m_sMCY);
					if ((absX <= 1) && (absY <= 1))
					{
						wType = _iGetAttackType();
						m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
						m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
					}
					else if ((absX <= 2) && (absY <= 2) // strike on Big mobs & gate from a range
						&& ((sObjectType == 66) || (sObjectType == 73) || (sObjectType == 81) || (sObjectType == 91)))
					{
						wType = _iGetAttackType();
						m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
						m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
					}
					else // Pas au corp � corp
					{
						switch (_iGetWeaponSkillType()) {
						case 6: // Bow
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							wType = _iGetAttackType();
							break;

						case 5: // OpenHand
						case 7: // SS
							if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0)))
							{
								if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
								{
									if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100)
									{
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										wType = _iGetAttackType();
									}
									else
									{
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								}
								else
								{
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							else
							{
								if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)
									&& (m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);	// Staminar
								else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
							}
							break;

						case 8: // LS
							if ((absX <= 3) && (absY <= 3) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
								&& (_iGetAttackType() != 30)) // Crit without StormBlade
							{
								wType = _iGetAttackType();
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							}
							else if ((absX <= 5) && (absY <= 5) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
								&& (_iGetAttackType() == 30))  // Crit with StormBlade (by Snoopy)
							{
								wType = _iGetAttackType();
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							}
							else if ((absX <= 3) && (absY <= 3)
								&& (_iGetAttackType() == 5))  // Normal hit with StormBlade (by Snoopy)
							{
								wType = _iGetAttackType();
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							}
							else // Swing
							{
								if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0))
									&& (_iGetAttackType() != 5)) // no Dash possible with StormBlade
								{
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
									{
										if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100)
										{
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
											wType = _iGetAttackType();
										}
										else
										{
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
											m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
										}
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									}
									else
									{
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
								}
								else
								{
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)
										&& (m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							break;

						case 9: // Fencing
							if ((absX <= 4) && (absY <= 4) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true))
							{
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								wType = _iGetAttackType();
							}
							else {
								if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0))) {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)) {
										if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100) {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
											wType = _iGetAttackType();
										}
										else {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
											m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
										}
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									}
									else {
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
								}
								else {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
										(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							break;

						case 10: // Axe
							if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true))
							{
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								wType = _iGetAttackType();
							}
							else
							{
								if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0)))
								{
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
									{
										if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100)
										{
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
											wType = _iGetAttackType();
										}
										else
										{
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
											m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
										}
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									}
									else
									{
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
								}
								else
								{
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
										(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							break;
						case 14: // Hammer
							if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								wType = _iGetAttackType();
							}
							else {
								if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0))) {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)) {
										if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100) {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
											wType = _iGetAttackType();
										}
										else {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
											m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
										}
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									}
									else {
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
								}
								else {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
										(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							break;
						case 21: // Wand
							if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								wType = _iGetAttackType();
							}
							else {
								if (((absX == 2) && (absY == 2)) || ((absX == 0) && (absY == 2)) || ((absX == 2) && (absY == 0))) {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)) {
										if (m_pPlayer->m_iSkillMastery[_iGetWeaponSkillType()] == 100) {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
											wType = _iGetAttackType();
										}
										else {
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
											m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
										}
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									}
									else {
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
								}
								else {
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
										(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
								}
							}
							break;
						}
					}
				}
				else // CTRL not pressed
				{
					m_pMapData->bGetOwner(m_sMCX, m_sMCY, cName, &sObjectType, &iObjectStatus, &m_wCommObjectID);
					if (sObjectType >= 10 || ((sObjectType >= 1) && (sObjectType <= 6)))
					{
						switch (sObjectType) { 	// CLEROTH - NPC TALK
						case 15: // ShopKeeper-W�
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 5, 11, 1);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 15;
							/*	break;
							}*/
							break;

						case 19: // Gandlf
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 0, 16, 0);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 19;
							/*	break;
							}*/
							break;

						case 20: // Howard
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 0, 14, 0);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 20;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 = 20;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4 = m_wCommObjectID;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV5 = m_sMCX;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV6 = m_sMCY;
							/*	break;
							}*/
							break;

						case 24: // Tom
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 5, 11, 2);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 24;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV3 = 24;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV4 = m_wCommObjectID;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV5 = m_sMCX;
							m_dialogBoxManager.Info(DialogBoxId::GiveItem).sV6 = m_sMCY;
							/*	break;
							}*/
							break;

						case 25: // William
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 0, 13, 0);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 25;
							/*	break;
							}*/
							break;

						case 26: // Kennedy
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 0, 7, 0);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 26;
							/*	break;
							}*/
							break;

						case 21: // Guard
							if ((_iGetFOE(iObjectStatus) >= 0) && (!m_pPlayer->m_bIsCombatMode))
							{
								m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 4, 0, 0);
								tX = msX - 117;
								tY = msY - 50;
								if (tX < 0) tX = 0;
								if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
								if (tY < 0) tY = 0;
								if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 21;
							}
							break;
						case 67: // McGaffin
						case 68: // Perry
						case 69: // Devlin
							if (!m_pPlayer->m_bIsCombatMode)
							{
								m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 4, 0, 0);
								tX = msX - 117;
								tY = msY - 50;
								if (tX < 0) tX = 0;
								if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
								if (tY < 0) tY = 0;
								if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = sObjectType;
							}
							break;

						case 32: // Unicorn
							if (!m_pPlayer->m_bIsCombatMode)
							{
								m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 4, 0, 0);
								tX = msX - 117;
								tY = msY - 50;
								if (tX < 0) tX = 0;
								if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
								if (tY < 0) tY = 0;
								if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
								m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 32;
							}
							break;

						case 90: // Snoopy: Gail
							/*switch (cName[0]) {
							case '1':*/
							m_dialogBoxManager.EnableDialogBox(DialogBoxId::NpcActionQuery, 6, 0, 0);
							tX = msX - 117;
							tY = msY - 50;
							if (tX < 0) tX = 0;
							if ((tX + 235) > LOGICAL_MAX_X) tX = LOGICAL_MAX_X - 235;
							if (tY < 0) tY = 0;
							if ((tY + 100) > LOGICAL_MAX_Y) tY = LOGICAL_MAX_Y - 100;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sX = tX;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sY = tY;
							m_dialogBoxManager.Info(DialogBoxId::NpcActionQuery).sV3 = 90;
							/*break;
						}*/
							break;

						default: // Other mobs
							if (_iGetFOE(iObjectStatus) >= 0) break;
							if ((sObjectType >= 1) && (sObjectType <= 6) && (m_pPlayer->m_bForceAttack == false)) break;
							absX = abs(m_pPlayer->m_sPlayerX - m_sMCX);
							absY = abs(m_pPlayer->m_sPlayerY - m_sMCY);
							if ((absX <= 1) && (absY <= 1))
							{
								wType = _iGetAttackType();
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							}
							else if ((absX <= 2) && (absY <= 2) // strike on Big mobs & gate from a range
								&& ((sObjectType == 66) || (sObjectType == 73) || (sObjectType == 81) || (sObjectType == 91)))
							{
								wType = _iGetAttackType();
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							}
							else // Normal hit from a range.
							{
								switch (_iGetWeaponSkillType()) {
								case 6: // Bow
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									wType = _iGetAttackType();
									break;

								case 5: // Boxe
								case 7: // SS
									if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0)
										&& (m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
									else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									break;

								case 8: // LS
									if ((absX <= 3) && (absY <= 3) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
										&& (_iGetAttackType() != 30)) // Crit without StormBlade by Snoopy
									{
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else if ((absX <= 5) && (absY <= 5) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
										&& (_iGetAttackType() == 30)) // Crit with StormBlade by Snoopy
									{
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else if ((absX <= 3) && (absY <= 3)
										&& (_iGetAttackType() == 5)) // Normal hit with StormBlade by Snoopy
									{
										m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else
									{
										if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
											(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									break;

								case 9: // Fencing
									if ((absX <= 4) && (absY <= 4) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true))
									{
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else
									{
										if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
											(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									break;

								case 10: //
									if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else {
										if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
											(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									break;
								case 14: //
									if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else {
										if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
											(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									break;
								case 21: //
									if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
										if ((absX <= 1) && (absY <= 1) && (Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACKMOVE);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										wType = _iGetAttackType();
									}
									else {
										if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
											(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
											m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
										else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
										m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
										m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
									}
									break;
								}
							}
							break;
						}
					}
					else {
						if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
							(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
						else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
						m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
					}
				}
			}
			else
			{
				if ((Input::IsShiftDown() || ConfigManager::Get().IsRunningModeEnabled()) && (m_pPlayer->m_iSP > 0) &&
					(m_pPlayer->m_sPlayerType >= 1) && (m_pPlayer->m_sPlayerType <= 6))
					m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
				else m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
				m_pPlayer->m_Controller.SetDestination(indexX, indexY);
				m_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData.get());
			}
		}
	}
	else if (cRB != 0) // Mouse Right button
	{
		m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
		if (m_bIsGetPointingMode == true)
		{
			m_bIsGetPointingMode = false;
			AddEventList(COMMAND_PROCESSOR1, 10);
		}
		if (m_pPlayer->m_Controller.IsCommandAvailable() == false) return;
		if (m_pPlayer->m_Controller.GetCommandCount() >= 6) return;

		if ((m_sMCX != 0) && (m_sMCY != 0))
		{
			absX = abs(m_pPlayer->m_sPlayerX - m_sMCX);
			absY = abs(m_pPlayer->m_sPlayerY - m_sMCY);
			if (absX == 0 && absY == 0) return;

			if (Input::IsCtrlDown() == true)
			{
				m_pMapData->bGetOwner(m_sMCX, m_sMCY, cName, &sObjectType, &iObjectStatus, &m_wCommObjectID);
				if ((iObjectStatus & 0x10) != 0) return;
				if ((sObjectType == 15) || (sObjectType == 20) || (sObjectType == 24)) return;

				if ((absX <= 1) && (absY <= 1))
				{
					wType = _iGetAttackType();
					m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
					m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
				}
				else if ((absX <= 2) && (absY <= 2) // strike on Big mobs & gate from a range
					&& ((sObjectType == 66) || (sObjectType == 73) || (sObjectType == 81) || (sObjectType == 91)))
				{
					wType = _iGetAttackType();
					m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
					m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
				}
				else
				{
					switch (_iGetWeaponSkillType()) {
					case 6: // Bow
						m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
						m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						wType = _iGetAttackType();
						break;

					case 5: // Boxe
					case 7: // SS
						break;

					case 8: // LS
						if ((absX <= 3) && (absY <= 3) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
							&& (_iGetAttackType() != 30)) // without StormBlade by Snoopy
						{
							wType = _iGetAttackType();
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						}
						else if ((absX <= 5) && (absY <= 5) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
							&& (_iGetAttackType() == 30)) // with stormBlade crit by Snoopy
						{
							wType = _iGetAttackType();
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						}
						else if ((absX <= 3) && (absY <= 3)
							&& (_iGetAttackType() == 5)) // with stormBlade no crit by Snoopy
						{
							wType = _iGetAttackType();
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						}
						break;

					case 9: // Fencing
						if ((absX <= 4) && (absY <= 4) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							wType = _iGetAttackType();
						}
						break;

					case 10: //
						if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							wType = _iGetAttackType();
						}
						break;

					case 14: //
						if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							wType = _iGetAttackType();
						}
						break;
					case 21: //
						if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
							wType = _iGetAttackType();
						}
						break;
					}
				}
			}
			else // CTRL not pressed
			{
				absX = abs(m_pPlayer->m_sPlayerX - m_sMCX);
				absY = abs(m_pPlayer->m_sPlayerY - m_sMCY);
				m_pMapData->bGetOwner(m_sMCX, m_sMCY, cName, &sObjectType, &iObjectStatus, &m_wCommObjectID);
				if (sObjectType >= 10 || ((sObjectType >= 1) && (sObjectType <= 6))) {
					switch (sObjectType) {
					case 15:
					case 19:
					case 20:
					case 24:
					case 25:
					case 26: // npcs
						break;

					default: // All "normal mobs"
						if (_iGetFOE(iObjectStatus) >= 0) break;
						if ((sObjectType >= 1) && (sObjectType <= 6) && (m_pPlayer->m_bForceAttack == false)) break;
						if ((absX <= 1) && (absY <= 1))
						{
							wType = _iGetAttackType();
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						}
						else if ((absX <= 2) && (absY <= 2) // strike on Big mobs & gate from a range
							&& ((sObjectType == 66) || (sObjectType == 73) || (sObjectType == 81) || (sObjectType == 91)))
						{
							wType = _iGetAttackType();
							m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
							m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
						}
						else //
						{
							switch (_iGetWeaponSkillType()) {
							case 6: // Bow
								m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
								m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								wType = _iGetAttackType();
								break;

							case 5: // Boxe
							case 7: // SS
								break;

							case 8: // LS
								if ((absX <= 3) && (absY <= 3) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
									&& (_iGetAttackType() != 30)) // crit without StormBlade by Snoopy
								{
									wType = _iGetAttackType();
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								}
								else if ((absX <= 5) && (absY <= 5) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)
									&& (_iGetAttackType() == 30)) // with stormBlade crit by Snoopy
								{
									wType = _iGetAttackType();
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								}
								else if ((absX <= 3) && (absY <= 3)
									&& (_iGetAttackType() == 5)) // with stormBlade no crit by Snoopy
								{
									wType = _iGetAttackType();
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
								}
								break;

							case 9: // fencing
								if ((absX <= 4) && (absY <= 4) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									wType = _iGetAttackType();
								}
								break;

							case 10: //
								if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									wType = _iGetAttackType();
								}
								break;
							case 14: // hammer
								if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									wType = _iGetAttackType();
								}
								break;
							case 21: // wand
								if ((absX <= 2) && (absY <= 2) && (m_pPlayer->m_iSuperAttackLeft > 0) && (m_pPlayer->m_bSuperAttackMode == true)) {
									m_pPlayer->m_Controller.SetCommand(DEF_OBJECTATTACK);
									m_pPlayer->m_Controller.SetDestination(m_sMCX, m_sMCY);
									wType = _iGetAttackType();
								}
								break;
							}
						}
						break;
					}
				}
			}
		}
		else
		{
			cDir = CMisc::cGetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, indexX, indexY);
			if (m_pPlayer->m_iHP <= 0) return;
			if (cDir == 0) return;
			if (m_pPlayer->m_iPlayerDir == cDir) return;
			ClearSkillUsingStatus();
			m_pPlayer->m_iPlayerDir = cDir;
			bSendCommand(MSGID_COMMAND_MOTION, DEF_OBJECTSTOP, m_pPlayer->m_iPlayerDir, 0, 0, 0, 0);

			m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
				m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
				m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
				m_pPlayer->m_Controller.GetCommand(), 0, 0, 0, 0,
				10);
			m_pPlayer->m_Controller.SetCommandAvailable(false);
			m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
			return;
		}
	}

MOTION_COMMAND_PROCESS:;

	if (m_pPlayer->m_Controller.GetCommand() != DEF_OBJECTSTOP)
	{
		if (m_pPlayer->m_iHP <= 0) return;
		if (m_pPlayer->m_Controller.GetCommandCount() == 5) AddEventList(COMMAND_PROCESSOR2, 10, false);
		if (m_pPlayer->m_Controller.IsCommandAvailable() == false) return;
		if (m_pPlayer->m_Controller.GetCommandCount() >= 6) return;

		if ((m_pPlayer->m_sPlayerType >= 0) && (m_pPlayer->m_sPlayerType > 6))
		{
			switch (m_pPlayer->m_Controller.GetCommand()) {
			case DEF_OBJECTRUN:
			case DEF_OBJECTMAGIC:
			case DEF_OBJECTGETITEM:
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
				break;
			}
		}

		ClearSkillUsingStatus();

		if ((m_pPlayer->m_sDamageMove != 0) || (m_pPlayer->m_sDamageMoveAmount != 0))
		{
			if (m_pPlayer->m_sDamageMove != 0)
			{
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTDAMAGEMOVE);
				m_pPlayer->m_Controller.SetDestination(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);

				// mim crit fixed by kaozures tocado para ande bien by cloud :P
				if (m_bIllusionMVT == true) {
					switch (m_pPlayer->m_sDamageMove) {
					case 1: m_pPlayer->m_Controller.MoveDestination(0, 1); break;
					case 2: m_pPlayer->m_Controller.MoveDestination(-1, 1); break;
					case 3: m_pPlayer->m_Controller.MoveDestination(-1, 0); break;
					case 4: m_pPlayer->m_Controller.MoveDestination(-1, -1); break;
					case 5: m_pPlayer->m_Controller.MoveDestination(0, -1); break;
					case 6: m_pPlayer->m_Controller.MoveDestination(1, -1); break;
					case 7: m_pPlayer->m_Controller.MoveDestination(1, 0); break;
					case 8: m_pPlayer->m_Controller.MoveDestination(1, 1); break;
					}
				}
				else {
					switch (m_pPlayer->m_sDamageMove) {
					case 1: m_pPlayer->m_Controller.MoveDestination(0, -1); break;
					case 2: m_pPlayer->m_Controller.MoveDestination(1, -1); break;
					case 3: m_pPlayer->m_Controller.MoveDestination(1, 0); break;
					case 4: m_pPlayer->m_Controller.MoveDestination(1, 1); break;
					case 5: m_pPlayer->m_Controller.MoveDestination(0, 1); break;
					case 6: m_pPlayer->m_Controller.MoveDestination(-1, 1); break;
					case 7: m_pPlayer->m_Controller.MoveDestination(-1, 0); break;
					case 8: m_pPlayer->m_Controller.MoveDestination(-1, -1); break;
					}
				}
			}

			for (i = 1; i < DEF_MAXCHATMSGS; i++)
				if (m_pChatMsgList[i] == 0)
				{
					std::memset(cTxt, 0, sizeof(cTxt));
					if (m_pPlayer->m_sDamageMoveAmount > 0)
						wsprintf(cTxt, "-%dPts", m_pPlayer->m_sDamageMoveAmount); //pts
					else strcpy(cTxt, "Critical!");

					int iFontType;
					if ((m_pPlayer->m_sDamageMoveAmount >= 0) && (m_pPlayer->m_sDamageMoveAmount < 12))		iFontType = 21;
					else if ((m_pPlayer->m_sDamageMoveAmount >= 12) && (m_pPlayer->m_sDamageMoveAmount < 40)) iFontType = 22;
					else if ((m_pPlayer->m_sDamageMoveAmount >= 40) || (m_pPlayer->m_sDamageMoveAmount < 0))	iFontType = 23;

					m_pChatMsgList[i] = std::make_unique<CMsg>(iFontType, cTxt, m_dwCurTime);
					m_pChatMsgList[i]->m_iObjectID = m_pPlayer->m_sPlayerObjectID;

					if (m_pMapData->bSetChatMsgOwner(m_pPlayer->m_sPlayerObjectID, -10, -10, i) == false) {
						m_pChatMsgList[i].reset();
					}
					break;
				}
			m_pPlayer->m_sDamageMove = 0;
			m_pPlayer->m_sDamageMoveAmount = 0;
		}

		switch (m_pPlayer->m_Controller.GetCommand()) {
		case DEF_OBJECTRUN:
		case DEF_OBJECTMOVE:
		case DEF_OBJECTDAMAGEMOVE: // v1.43

			if (m_pPlayer->m_bParalyze) return;
			bGORet = m_pMapData->bGetOwner(m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), pDstName, &sDstOwnerType, &iDstOwnerStatus, &m_wCommObjectID); // v1.4

			if ((m_pPlayer->m_sPlayerX == m_pPlayer->m_Controller.GetDestinationX()) && (m_pPlayer->m_sPlayerY == m_pPlayer->m_Controller.GetDestinationY()))
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			else if ((abs(m_pPlayer->m_sPlayerX - m_pPlayer->m_Controller.GetDestinationX()) <= 1) && (abs(m_pPlayer->m_sPlayerY - m_pPlayer->m_Controller.GetDestinationY()) <= 1) &&
				(bGORet == true) && (sDstOwnerType != 0))
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			else if ((abs(m_pPlayer->m_sPlayerX - m_pPlayer->m_Controller.GetDestinationX()) <= 2) && (abs(m_pPlayer->m_sPlayerY - m_pPlayer->m_Controller.GetDestinationY()) <= 2) &&
				(m_pMapData->m_tile[m_pPlayer->m_Controller.GetDestinationX()][m_pPlayer->m_Controller.GetDestinationY()].m_bIsMoveAllowed == false))
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			else
			{
				if (m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTMOVE)
				{
					if (ConfigManager::Get().IsRunningModeEnabled() || Input::IsShiftDown()) m_pPlayer->m_Controller.SetCommand(DEF_OBJECTRUN);
				}
				if (m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTRUN)
				{
					if ((ConfigManager::Get().IsRunningModeEnabled() == false) && (Input::IsShiftDown() == false)) m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
					if (m_pPlayer->m_iSP < 1) m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMOVE);
				}

				cDir = m_pPlayer->m_Controller.GetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), m_pMapData.get(), true);
				// Snoopy: Illusion Movement
				if ((m_bIllusionMVT == true) && (m_pPlayer->m_Controller.GetCommand() != DEF_OBJECTDAMAGEMOVE))
				{
					cDir = m_pPlayer->m_Controller.GetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), m_pMapData.get(), true, true);
				}
				if (cDir != 0)
				{
					// Cancel logout countdown on movement
					if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
					{
						m_cLogOutCount = -1;
						AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
					}

					m_pPlayer->m_iPlayerDir = cDir;
					bSendCommand(MSGID_COMMAND_MOTION, m_pPlayer->m_Controller.GetCommand(), cDir, 0, 0, 0, 0);
					switch (cDir) {
					case 1:	m_pPlayer->m_sPlayerY--; break;
					case 2:	m_pPlayer->m_sPlayerY--; m_pPlayer->m_sPlayerX++;	break;
					case 3:	m_pPlayer->m_sPlayerX++; break;
					case 4:	m_pPlayer->m_sPlayerX++; m_pPlayer->m_sPlayerY++;	break;
					case 5:	m_pPlayer->m_sPlayerY++; break;
					case 6:	m_pPlayer->m_sPlayerX--; m_pPlayer->m_sPlayerY++;	break;
					case 7:	m_pPlayer->m_sPlayerX--; break;
					case 8:	m_pPlayer->m_sPlayerX--; m_pPlayer->m_sPlayerY--;	break;
					}
					m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
						m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor, // v1.4
						m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
						m_pPlayer->m_Controller.GetCommand(), 0, 0, 0);
					m_pPlayer->m_Controller.SetCommandAvailable(false);
					m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
					m_pPlayer->m_Controller.SetPrevMove(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);
				}
			}

			if (m_pPlayer->m_Controller.GetCommand() == DEF_OBJECTDAMAGEMOVE)
			{
				m_bIsGetPointingMode = false;
				m_iPointCommandType = -1;
				ClearSkillUsingStatus();
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			}
			break;

		case DEF_OBJECTATTACK:
			cDir = CMisc::cGetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY());
			// Snoopy: Illusion movement
			if (m_bIllusionMVT == true)
			{
				cDir += 4;
				if (cDir > 8) cDir -= 8;
			}
			if (cDir != 0)
			{
				// Cancel logout countdown on attack
				if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
				{
					m_cLogOutCount = -1;
					AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
				}

				if ((wType == 2) || (wType == 25))
				{
					if (_bCheckItemByType(DEF_ITEMTYPE_ARROW) == false)
						wType = 0;
				}
				if (wType >= 20)
				{
					m_pPlayer->m_iSuperAttackLeft--;
					if (m_pPlayer->m_iSuperAttackLeft < 0) m_pPlayer->m_iSuperAttackLeft = 0;
				}
				m_pPlayer->m_iPlayerDir = cDir;
				m_wLastAttackTargetID = m_wCommObjectID;
				bSendCommand(MSGID_COMMAND_MOTION, DEF_OBJECTATTACK, cDir, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), wType, 0, m_wCommObjectID);
				m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
					m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
					m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
					DEF_OBJECTATTACK,
					m_pPlayer->m_Controller.GetDestinationX() - m_pPlayer->m_sPlayerX, m_pPlayer->m_Controller.GetDestinationY() - m_pPlayer->m_sPlayerY, wType);
				m_pPlayer->m_Controller.SetCommandAvailable(false);
				m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
			}
			m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			break;

		case DEF_OBJECTATTACKMOVE:
			if (m_pPlayer->m_bParalyze) return;
			bGORet = m_pMapData->bGetOwner(m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), pDstName, &sDstOwnerType, &iDstOwnerStatus, &m_wCommObjectID);
			if ((m_pPlayer->m_sPlayerX == m_pPlayer->m_Controller.GetDestinationX()) && (m_pPlayer->m_sPlayerY == m_pPlayer->m_Controller.GetDestinationY()))
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			else if ((abs(m_pPlayer->m_sPlayerX - m_pPlayer->m_Controller.GetDestinationX()) <= 1) && (abs(m_pPlayer->m_sPlayerY - m_pPlayer->m_Controller.GetDestinationY()) <= 1) &&
				(bGORet == true) && (sDstOwnerType != 0))
				m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			else
			{
				cDir = m_pPlayer->m_Controller.GetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), m_pMapData.get(), true);
				// Snoopy: Illusion mvt
				if (m_bIllusionMVT == true)
				{
					cDir = m_pPlayer->m_Controller.GetNextMoveDir(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), m_pMapData.get(), true, true);
				}
				if (cDir != 0)
				{
					// Cancel logout countdown on attack-move
					if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
					{
						m_cLogOutCount = -1;
						AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
					}

					m_pPlayer->m_iPlayerDir = cDir;
					m_wLastAttackTargetID = m_wCommObjectID;
					bSendCommand(MSGID_COMMAND_MOTION, DEF_OBJECTATTACKMOVE, cDir, m_pPlayer->m_Controller.GetDestinationX(), m_pPlayer->m_Controller.GetDestinationY(), wType, 0, m_wCommObjectID);
					switch (cDir) {
					case 1:	m_pPlayer->m_sPlayerY--; break;
					case 2:	m_pPlayer->m_sPlayerY--; m_pPlayer->m_sPlayerX++;	break;
					case 3:	m_pPlayer->m_sPlayerX++; break;
					case 4:	m_pPlayer->m_sPlayerX++; m_pPlayer->m_sPlayerY++;	break;
					case 5:	m_pPlayer->m_sPlayerY++; break;
					case 6:	m_pPlayer->m_sPlayerX--; m_pPlayer->m_sPlayerY++;	break;
					case 7:	m_pPlayer->m_sPlayerX--; break;
					case 8:	m_pPlayer->m_sPlayerX--; m_pPlayer->m_sPlayerY--;	break;
					}

					m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
						m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
						m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
						m_pPlayer->m_Controller.GetCommand(), m_pPlayer->m_Controller.GetDestinationX() - m_pPlayer->m_sPlayerX, m_pPlayer->m_Controller.GetDestinationY() - m_pPlayer->m_sPlayerY, wType);
					m_pPlayer->m_Controller.SetCommandAvailable(false);
					m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
					m_pPlayer->m_Controller.SetPrevMove(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY);
				}
			}
			m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			break;

		case DEF_OBJECTGETITEM:
			// Cancel logout countdown on get item
			if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
			{
				m_cLogOutCount = -1;
				AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
			}

			bSendCommand(MSGID_COMMAND_MOTION, DEF_OBJECTGETITEM, m_pPlayer->m_iPlayerDir, 0, 0, 0, 0);
			m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
				m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
				m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
				DEF_OBJECTGETITEM, 0, 0, 0);
			m_pPlayer->m_Controller.SetCommandAvailable(false);
			m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			break;

		case DEF_OBJECTMAGIC:
			// Cancel logout countdown on magic cast
			if ((m_cLogOutCount > 0) && (m_bForceDisconn == false))
			{
				m_cLogOutCount = -1;
				AddEventList(DLGBOX_CLICK_SYSMENU2, 10);
			}

			bSendCommand(MSGID_COMMAND_MOTION, DEF_OBJECTMAGIC, m_pPlayer->m_iPlayerDir, m_iCastingMagicType, 0, 0, 0);
			m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
				m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor,
				m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
				DEF_OBJECTMAGIC, m_iCastingMagicType, 0, 0);
			m_pPlayer->m_Controller.SetCommandAvailable(false);
			m_pPlayer->m_Controller.SetCommandTime(GameClock::GetTimeMS());
			m_bIsGetPointingMode = true;
			m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
			_RemoveChatMsgListByObjectID(m_pPlayer->m_sPlayerObjectID);
			for (i = 1; i < DEF_MAXCHATMSGS; i++)
				if (m_pChatMsgList[i] == 0)
				{
					std::memset(cTxt, 0, sizeof(cTxt));
					wsprintf(cTxt, "%s!", m_pMagicCfgList[m_iCastingMagicType]->m_cName);
					m_pChatMsgList[i] = std::make_unique<CMsg>(41, cTxt, GameClock::GetTimeMS());
					m_pChatMsgList[i]->m_iObjectID = m_pPlayer->m_sPlayerObjectID;
					m_pMapData->bSetChatMsgOwner(m_pPlayer->m_sPlayerObjectID, -10, -10, i);
					return;
				}
			break;

		default:
			break;
		}
	}
}

void CGame::ResponseTeleportList(char* pData)
{
	int i;
#ifdef _DEBUG
	AddEventList("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		pData, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		pData + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_iTeleportMapCount = header->count;
	for (i = 0; i < m_iTeleportMapCount; i++)
	{
		m_stTeleportList[i].iIndex = entries[i].index;
		std::memset(m_stTeleportList[i].mapname, 0, sizeof(m_stTeleportList[i].mapname));
		memcpy(m_stTeleportList[i].mapname, entries[i].map_name, 10);
		m_stTeleportList[i].iX = entries[i].x;
		m_stTeleportList[i].iY = entries[i].y;
		m_stTeleportList[i].iCost = entries[i].cost;
	}
}

void CGame::ResponseChargedTeleport(char* pData)
{
	short sRejectReason = 0;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseChargedTeleport>(
		pData, sizeof(hb::net::PacketResponseChargedTeleport));
	if (!pkt) return;
	sRejectReason = pkt->reason;

#ifdef _DEBUG
	AddEventList("charged teleport ?", 10);
#endif

	switch (sRejectReason) {
	case 1:
		AddEventList(RESPONSE_CHARGED_TELEPORT1, 10);
		break;
	case 2:
		AddEventList(RESPONSE_CHARGED_TELEPORT2, 10);
		break;
	case 3:
		AddEventList(RESPONSE_CHARGED_TELEPORT3, 10);
		break;
	case 4:
		AddEventList(RESPONSE_CHARGED_TELEPORT4, 10);
		break;
	case 5:
		AddEventList(RESPONSE_CHARGED_TELEPORT5, 10);
		break;
	case 6:
		AddEventList(RESPONSE_CHARGED_TELEPORT6, 10);
		break;
	default:
		AddEventList(RESPONSE_CHARGED_TELEPORT7, 10);
	}
}




// _Draw_OnLogin removed - migrated to Screen_Login

void CGame::ShowEventList(uint32_t dwTime)
{
	int i;
	int baseY = EVENTLIST2_BASE_Y;
	m_Renderer->BeginTextBatch();
	for (i = 0; i < 6; i++)
		if ((dwTime - m_stEventHistory[i].dwTime) < 5000)
		{
			switch (m_stEventHistory[i].cColor) {
			case 0:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::UINearWhiteAlt.r, GameColors::UINearWhiteAlt.g, GameColors::UINearWhiteAlt.b));
				break;
			case 1:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatEventGreen.r, GameColors::ChatEventGreen.g, GameColors::ChatEventGreen.b));
				break;
			case 2:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::UIWorldChat.r, GameColors::UIWorldChat.g, GameColors::UIWorldChat.b));
				break;
			case 3:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatBlue.r, GameColors::ChatBlue.g, GameColors::ChatBlue.b));
				break;
			case 4:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatYellow.r, GameColors::ChatYellow.g, GameColors::ChatYellow.b));
				break;
			case 10:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatLightGreen.r, GameColors::ChatLightGreen.g, GameColors::ChatLightGreen.b));
				break;
			case 20:
				TextLib::DrawText(GameFont::Default, 10, 10 + i * 15, m_stEventHistory[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatGray.r, GameColors::ChatGray.g, GameColors::ChatGray.b));
				break;
			}
		}

	for (i = 0; i < 6; i++)
		if ((dwTime - m_stEventHistory2[i].dwTime) < 5000)
		{
			switch (m_stEventHistory2[i].cColor) {
			case 0:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::UINearWhiteAlt.r, GameColors::UINearWhiteAlt.g, GameColors::UINearWhiteAlt.b));
				break;
			case 1:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatEventGreen.r, GameColors::ChatEventGreen.g, GameColors::ChatEventGreen.b));
				break;
			case 2:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::UIWorldChat.r, GameColors::UIWorldChat.g, GameColors::UIWorldChat.b));
				break;
			case 3:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatBlue.r, GameColors::ChatBlue.g, GameColors::ChatBlue.b));
				break;
			case 4:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatYellow.r, GameColors::ChatYellow.g, GameColors::ChatYellow.b));
				break;
			case 10:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatLightGreen.r, GameColors::ChatLightGreen.g, GameColors::ChatLightGreen.b));
				break;
			case 20:
				TextLib::DrawText(GameFont::Default, 10, baseY + i * 15, m_stEventHistory2[i].cTxt, TextLib::TextStyle::WithShadow(GameColors::ChatGray.r, GameColors::ChatGray.g, GameColors::ChatGray.b));
				break;
			}
		}
	if (m_bSkillUsingStatus == true)
	{
		TextLib::DrawText(GameFont::Default, 440 - 29, 440 - 52, SHOW_EVENT_LIST1, TextLib::TextStyle::WithShadow(GameColors::UINearWhiteAlt2.r, GameColors::UINearWhiteAlt2.g, GameColors::UINearWhiteAlt2.b));
	}
	m_Renderer->EndTextBatch();
}

void CGame::RequestTeleportAndWaitData()
{	// Snoopy: removed that, Noob Dungeon is now at farm...
	/*if (strcmp(m_cMapName, "aresden") == 0)
	{	if ( ((m_pPlayer->m_sPlayerX == 188) && (m_pPlayer->m_sPlayerY == 105))  ||
			 ((m_pPlayer->m_sPlayerX == 187) && (m_pPlayer->m_sPlayerY == 105))  ||
			 ((m_pPlayer->m_sPlayerX == 187) && (m_pPlayer->m_sPlayerY == 106))  ||
			 ((m_pPlayer->m_sPlayerX == 186) && (m_pPlayer->m_sPlayerY == 106))  ||
			 ((m_pPlayer->m_sPlayerX == 186) && (m_pPlayer->m_sPlayerY == 107))  )
		{	if ( (m_pPlayer->m_iLevel < 30) || (m_pPlayer->m_iLevel>80) )
			{	AddEventList(REQUEST_TELEPORT_AND_WAIT_DATA1, 10);
				return;
	}	}	}
	if (strcmp(m_cMapName, "elvine") == 0)
	{	if ( ((m_pPlayer->m_sPlayerX == 218) && (m_pPlayer->m_sPlayerY == 109))  ||
			 ((m_pPlayer->m_sPlayerX == 217) && (m_pPlayer->m_sPlayerY == 109))  ||
			 ((m_pPlayer->m_sPlayerX == 217) && (m_pPlayer->m_sPlayerY == 110))  ||
			 ((m_pPlayer->m_sPlayerX == 216) && (m_pPlayer->m_sPlayerY == 110))  ||
			 ((m_pPlayer->m_sPlayerX == 216) && (m_pPlayer->m_sPlayerY == 111))  )
		{	if ( (m_pPlayer->m_iLevel < 30) || (m_pPlayer->m_iLevel>80) )
			{	AddEventList(REQUEST_TELEPORT_AND_WAIT_DATA1, 10);
				return;
	}	}	}*/
	bSendCommand(MSGID_REQUEST_TELEPORT, 0, 0, 0, 0, 0, 0);
	ChangeGameMode(GameMode::WaitingInitData);
}

void CGame::InitDataResponseHandler(char* pData)
{
	int i;
	short sX, sY;
	const char* cp;
	char cMapFileName[32], cTxt[120], cPreCurLocation[12];
	bool  bIsObserverMode;
	HANDLE hFile;
	uint32_t dwFileSize;

	std::memset(cPreCurLocation, 0, sizeof(cPreCurLocation));
	m_pPlayer->m_bParalyze = false;
	m_pMapData->Init();

	m_sMonsterID = 0;
	m_dwMonsterEventTime = 0;

	m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuildMenu);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::SaleMenu);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CityHallMenu);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Bank);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::MagicShop);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Map);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcActionQuery);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcTalk);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::SellOrRepair);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuildHallMenu); // Gail's diag

	m_pPlayer->m_Controller.SetCommand(DEF_OBJECTSTOP);
	//m_pPlayer->m_Controller.SetCommandAvailable(true);
	m_pPlayer->m_Controller.ResetCommandCount();
	m_bIsGetPointingMode = false;
	m_iPointCommandType = -1;
	m_iIlusionOwnerH = 0;
	m_cIlusionOwnerType = 0;
	m_bIsTeleportRequested = false;
	m_pPlayer->m_bIsConfusion = false;
	m_bSkillUsingStatus = false;

	m_bItemUsingStatus = false;

	m_cRestartCount = -1;
	m_dwRestartCountTime = 0;

	if (m_pEffectManager) m_pEffectManager->ClearAllEffects();

	for (i = 0; i < DEF_MAXWHETHEROBJECTS; i++)
	{
		m_stWhetherObject[i].sX = 0;
		m_stWhetherObject[i].sBX = 0;
		m_stWhetherObject[i].sY = 0;
		m_stWhetherObject[i].cStep = 0;
	}

	for (i = 0; i < DEF_MAXGUILDNAMES; i++)
	{
		m_stGuildName[i].dwRefTime = 0;
		m_stGuildName[i].iGuildRank = -1;
		std::memset(m_stGuildName[i].cCharName, 0, sizeof(m_stGuildName[i].cCharName));
		std::memset(m_stGuildName[i].cGuildName, 0, sizeof(m_stGuildName[i].cGuildName));
	}

	for (i = 0; i < DEF_MAXCHATMSGS; i++) {
		if (m_pChatMsgList[i] != 0) m_pChatMsgList[i].reset();
	}

	const auto* pkt = hb::net::PacketCast<hb::net::PacketResponseInitDataHeader>(
		pData, sizeof(hb::net::PacketResponseInitDataHeader));
	if (!pkt) return;
	m_pPlayer->m_sPlayerObjectID = pkt->player_object_id;
	sX = pkt->pivot_x;
	sY = pkt->pivot_y;
	m_pPlayer->m_sPlayerType = pkt->player_type;
	m_pPlayer->m_sPlayerAppr1 = pkt->appr1;
	m_pPlayer->m_sPlayerAppr2 = pkt->appr2;
	m_pPlayer->m_sPlayerAppr3 = pkt->appr3;
	m_pPlayer->m_sPlayerAppr4 = pkt->appr4;
	m_pPlayer->m_iPlayerApprColor = pkt->appr_color;
	m_pPlayer->m_iPlayerStatus = pkt->status;

	//Snoopy MIM fix
	if ((m_pPlayer->m_iPlayerStatus & 0x00200000) == 0x00200000)
	{
		m_bIllusionMVT = true;
	}
	else
	{
		m_bIllusionMVT = false;
	}
	std::memset(m_cMapName, 0, sizeof(m_cMapName));
	std::memset(m_cMapMessage, 0, sizeof(m_cMapMessage));
	memcpy(m_cMapName, pkt->map_name, sizeof(pkt->map_name));
	m_cMapIndex = GetOfficialMapName(m_cMapName, m_cMapMessage);
	if (m_cMapIndex < 0)
	{
		m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeX = -1;
		m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeY = -1;
	}
	else
	{
		m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeX = 128;
		m_dialogBoxManager.Info(DialogBoxId::GuideMap).sSizeY = 128;
	}

	strcpy(cPreCurLocation, m_cCurLocation);
	std::memset(m_cCurLocation, 0, sizeof(m_cCurLocation));
	memcpy(m_cCurLocation, pkt->cur_location, sizeof(pkt->cur_location));

	G_cSpriteAlphaDegree = static_cast<char>(pkt->sprite_alpha);

	m_cWhetherStatus = static_cast<char>(pkt->weather_status);
	switch (G_cSpriteAlphaDegree) { //Snoopy:  Xmas bulbs
		// Will be sent by server if DayTime is 3 (and a snowy weather)
	case 1:	m_bIsXmas = false; break;
	case 2: m_bIsXmas = false; break;
	case 3: // Snoopy Special night with chrismas bulbs
		if (m_cWhetherStatus > 3) m_bIsXmas = true;
		else m_bIsXmas = false;
		G_cSpriteAlphaDegree = 2;
		break;
	}
	m_pPlayer->m_iContribution = pkt->contribution;
	//	m_iContributionPrice = 0;
	bIsObserverMode = pkt->observer_mode != 0;
	//	m_iRating = pkt->rating;
	m_pPlayer->m_iHP = pkt->hp;
	m_cDiscount = static_cast<char>(pkt->discount);

	cp = reinterpret_cast<const char*>(pData) + sizeof(hb::net::PacketResponseInitDataHeader);

	if (m_cWhetherStatus != 0)
		SetWhetherStatus(true, m_cWhetherStatus);
	else SetWhetherStatus(false, m_cWhetherStatus);

	std::memset(cMapFileName, 0, sizeof(cMapFileName));
	strcat(cMapFileName, "mapdata\\");
	// CLEROTH - MW MAPS
	if (memcmp(m_cMapName, "defaultmw", 9) == 0)
	{
		strcat(cMapFileName, "mw\\defaultmw");
	}
	else
	{
		strcat(cMapFileName, m_cMapName);
	}

	strcat(cMapFileName, ".amd");
	m_pMapData->OpenMapDataFile(cMapFileName);

	m_pMapData->m_sPivotX = sX;
	m_pMapData->m_sPivotY = sY;

	m_pPlayer->m_sPlayerX = sX + 19;
	m_pPlayer->m_sPlayerY = sY + 17;

	m_pPlayer->m_iPlayerDir = 5;

	if (bIsObserverMode == false)
	{
		m_pMapData->bSetOwner(m_pPlayer->m_sPlayerObjectID, m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pPlayer->m_sPlayerType, m_pPlayer->m_iPlayerDir,
			m_pPlayer->m_sPlayerAppr1, m_pPlayer->m_sPlayerAppr2, m_pPlayer->m_sPlayerAppr3, m_pPlayer->m_sPlayerAppr4, m_pPlayer->m_iPlayerApprColor, // v1.4
			m_pPlayer->m_iPlayerStatus, m_pPlayer->m_cPlayerName,
			DEF_OBJECTSTOP, 0, 0, 0);
	}

	//m_sViewDstX = m_sViewPointX = (sX + VIEW_CENTER_TILE_X) * 32 - 16;
	//m_sViewDstY = m_sViewPointY = (sY + VIEW_CENTER_TILE_Y + 1) * 32 - 16;
	m_Camera.SnapTo((m_pPlayer->m_sPlayerX - VIEW_CENTER_TILE_X) * 32, (m_pPlayer->m_sPlayerY - (VIEW_CENTER_TILE_Y + 1)) * 32);
	_ReadMapData(sX + 7, sY + 8, cp);
	// ------------------------------------------------------------------------+
	wsprintf(cTxt, INITDATA_RESPONSE_HANDLER1, m_cMapMessage);
	AddEventList(cTxt, 10);

	m_dialogBoxManager.Info(DialogBoxId::WarningBattleArea).sX = 150;
	m_dialogBoxManager.Info(DialogBoxId::WarningBattleArea).sY = 130;

	if ((memcmp(m_cCurLocation, "middleland", 10) == 0)
		|| (memcmp(m_cCurLocation, "dglv2", 5) == 0)
		|| (memcmp(m_cCurLocation, "middled1n", 9) == 0))
		m_dialogBoxManager.EnableDialogBox(DialogBoxId::WarningBattleArea, 0, 0, 0);

	// Snoopy: removed for v351 compatibility. Maybe usefull later...
	/*	bool bPrevSafe, bNowSafe;
		if( memcmp( cPreCurLocation, m_cLocation, 3 ) == 0 )
			bPrevSafe = true;
		else bPrevSafe = false;

		if( memcmp( m_cCurLocation, m_cLocation, 3 ) == 0 )
			bNowSafe = true;
		else bNowSafe = false;

		if( memcmp( m_cCurLocation, "2nd", 3 ) == 0 ) bNowSafe = true;
		if( m_pPlayer->m_iPKCount != 0 ) bNowSafe = false;

		if( bPrevSafe )
		{	if( bNowSafe == false ) SetTopMsg(DEF_MSG_DANGERZONE, 5);
		}else
		{	if( bNowSafe ) SetTopMsg(DEF_MSG_SAFEZONE, 5);
		}*/

		// ------------------------------------------------------------------------+

	GameModeManager::set_screen<Screen_OnGame>();

	//v1.41
	if ((m_pPlayer->m_sPlayerAppr2 & 0xF000) != 0)
		m_pPlayer->m_bIsCombatMode = true;
	else m_pPlayer->m_bIsCombatMode = false;

	//v1.42
	if (m_bIsFirstConn == true)
	{
		m_bIsFirstConn = false;
		hFile = CreateFile("contents\\contents1000.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE)
			dwFileSize = 0;
		else
		{
			dwFileSize = GetFileSize(hFile, 0);
			CloseHandle(hFile);
		}
		bSendCommand(MSGID_REQUEST_NOTICEMENT, 0, 0, (int)dwFileSize, 0, 0, 0);
	}
	//cp += 2;
}

void CGame::MotionEventHandler(char* pData)
{
	WORD wEventType, wObjectID;
	short sX, sY, sType, sAppr1, sAppr2, sAppr3, sAppr4, sV1, sV2, sV3, sPrevAppr2;
	int iStatus;
	char cDir, cName[12];
	int iApprColor, iLoc;
	char    cTxt[120];
	int i;
	std::memset(cName, 0, sizeof(cName));
	sX = -1;
	sY = -1;
	sV1 = sV2 = sV3 = 0;
	iApprColor = 0;
	iStatus = 0;
	iLoc = 0;
	cDir = 0;

	const auto* header = hb::net::PacketCast<hb::net::PacketHeader>(pData, sizeof(hb::net::PacketHeader));
	if (!header) return;
	wEventType = header->msg_type;

	const auto* baseId = hb::net::PacketCast<hb::net::PacketEventMotionBaseId>(pData, sizeof(hb::net::PacketEventMotionBaseId));
	if (!baseId) return;
	wObjectID = baseId->object_id;

	if (wObjectID >= 10000 && wObjectID < 30000) {
		m_dwLastNpcEventTime = GameClock::GetTimeMS();
	}

	if (wObjectID < 30000)
	{
		if (wObjectID < 10000) 	// Player
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionPlayer>(pData, sizeof(hb::net::PacketEventMotionPlayer));
			if (!pkt) return;
			sX = pkt->x;
			sY = pkt->y;
			sType = pkt->type;
			cDir = static_cast<char>(pkt->dir);
			memcpy(cName, pkt->name, sizeof(pkt->name));
			sAppr1 = pkt->appr1;
			sAppr2 = pkt->appr2;
			sAppr3 = pkt->appr3;
			sAppr4 = pkt->appr4;
			iApprColor = pkt->appr_color;
			iStatus = pkt->status;
			iLoc = pkt->loc;
		}
		else 	// Npc or mob
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionNpc>(pData, sizeof(hb::net::PacketEventMotionNpc));
			if (!pkt) return;
			sX = pkt->x;
			sY = pkt->y;
			sType = pkt->type;
			cDir = static_cast<char>(pkt->dir);
			memcpy(cName, pkt->name, sizeof(pkt->name));
			sAppr1 = sAppr3 = sAppr4 = 0;
			sAppr2 = pkt->appr2;
			iStatus = pkt->status;
			iLoc = pkt->loc;
		}
	}
	else
	{
		switch (wEventType) {
		case DEF_OBJECTMOVE:
		case DEF_OBJECTRUN:
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionDirOnly>(pData, sizeof(hb::net::PacketEventMotionDirOnly));
			if (!pkt) return;
			cDir = static_cast<char>(pkt->dir);
			sX = -1;
			sY = -1;
		}
		break;

		case DEF_OBJECTMAGIC:
		case DEF_OBJECTDAMAGE:
		case DEF_OBJECTDAMAGEMOVE:
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionShort>(pData, sizeof(hb::net::PacketEventMotionShort));
			if (!pkt) return;
			cDir = static_cast<char>(pkt->dir);
			sV1 = pkt->v1; // Damage or 0
			sV2 = pkt->v2;
			sX = -1;
			sY = -1;
		}
		break;

		case DEF_OBJECTDYING:
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionMove>(pData, sizeof(hb::net::PacketEventMotionMove));
			if (!pkt) return;
			cDir = static_cast<char>(pkt->dir);
			sV1 = pkt->v1;
			sV2 = pkt->v2;
			sX = pkt->x;
			sY = pkt->y;
		}
		break;

		case DEF_OBJECTATTACK:
		case DEF_OBJECTATTACKMOVE:
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionAttack>(pData, sizeof(hb::net::PacketEventMotionAttack));
			if (!pkt) return;
			cDir = static_cast<char>(pkt->dir);
			sV1 = pkt->v1;
			sV2 = pkt->v2;
			sV3 = pkt->v3;
		}
		break;

		default:
		{
			const auto* pkt = hb::net::PacketCast<hb::net::PacketEventMotionDirOnly>(pData, sizeof(hb::net::PacketEventMotionDirOnly));
			if (!pkt) return;
			cDir = static_cast<char>(pkt->dir);
		}
		break;
		}
	}

	if ((wEventType == DEF_OBJECTNULLACTION) && (memcmp(cName, m_pPlayer->m_cPlayerName, 10) == 0))
	{
		m_pPlayer->m_sPlayerType = sType;
		m_pPlayer->m_sPlayerAppr1 = sAppr1;
		sPrevAppr2 = m_pPlayer->m_sPlayerAppr2;
		m_pPlayer->m_sPlayerAppr2 = sAppr2;
		m_pPlayer->m_sPlayerAppr3 = sAppr3;
		m_pPlayer->m_sPlayerAppr4 = sAppr4;
		m_pPlayer->m_iPlayerApprColor = iApprColor;
		m_pPlayer->m_iPlayerStatus = iStatus;
		if ((sPrevAppr2 & 0xF000) == 0)
		{
			if ((sAppr2 & 0xF000) != 0)
			{
				AddEventList(MOTION_EVENT_HANDLER1, 10);
				m_pPlayer->m_bIsCombatMode = true;
			}
		}
		else
		{
			if ((sAppr2 & 0xF000) == 0)
			{
				AddEventList(MOTION_EVENT_HANDLER2, 10);
				m_pPlayer->m_bIsCombatMode = false;
			}
		}
		if (m_pPlayer->m_Controller.GetCommand() != DEF_OBJECTRUN && m_pPlayer->m_Controller.GetCommand() != DEF_OBJECTMOVE) m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, (char)wEventType, sV1, sV2, sV3, iLoc);
	}
	else m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, (char)wEventType, sV1, sV2, sV3, iLoc);

	switch (wEventType) {
	case DEF_OBJECTMAGIC: // Casting
		_RemoveChatMsgListByObjectID(static_cast<uint16_t>(wObjectID - 30000));

		for (i = 1; i < DEF_MAXCHATMSGS; i++)
			if (m_pChatMsgList[i] == 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				wsprintf(cTxt, "%s!", m_pMagicCfgList[sV1]->m_cName);
				m_pChatMsgList[i] = std::make_unique<CMsg>(41, cTxt, m_dwCurTime);
				m_pChatMsgList[i]->m_iObjectID = static_cast<uint16_t>(wObjectID - 30000);
				if (m_pMapData->bSetChatMsgOwner(static_cast<uint16_t>(wObjectID - 30000), -10, -10, i) == false)
				{
					m_pChatMsgList[i].reset();
				}
				return;
			}
		break;

	case DEF_OBJECTDYING:
		_RemoveChatMsgListByObjectID(static_cast<uint16_t>(wObjectID - 30000));
		for (i = 1; i < DEF_MAXCHATMSGS; i++)
			if (m_pChatMsgList[i] == 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				if (sV1 > 0)
					wsprintf(cTxt, "-%dPts!", sV1);
				else strcpy(cTxt, "Critical!");
				int iFontType;
				if ((sV1 >= 0) && (sV1 < 12))		iFontType = 21;
				else if ((sV1 >= 12) && (sV1 < 40)) iFontType = 22;
				else if ((sV1 >= 40) || (sV1 < 0))	iFontType = 23;
				m_pChatMsgList[i] = std::make_unique<CMsg>(iFontType, cTxt, m_dwCurTime);
				m_pChatMsgList[i]->m_iObjectID = static_cast<uint16_t>(wObjectID - 30000);
				if (m_pMapData->bSetChatMsgOwner(static_cast<uint16_t>(wObjectID - 30000), -10, -10, i) == false)
				{
					m_pChatMsgList[i].reset();
				}
				return;
			}
		break;

	case DEF_OBJECTDAMAGE:
	case DEF_OBJECTDAMAGEMOVE:
		if (memcmp(cName, m_pPlayer->m_cPlayerName, 10) == 0)
		{
			m_bIsGetPointingMode = false;
			m_iPointCommandType = -1;
			ClearSkillUsingStatus();
		}
		_RemoveChatMsgListByObjectID(static_cast<uint16_t>(wObjectID - 30000));

		for (i = 1; i < DEF_MAXCHATMSGS; i++)
			if (m_pChatMsgList[i] == 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				if (sV1 != 0)
				{
					if (sV1 > 0)
						wsprintf(cTxt, "-%dPts", sV1);
					else strcpy(cTxt, "Critical!");
					int iFontType;
					if ((sV1 >= 0) && (sV1 < 12))		iFontType = 21;
					else if ((sV1 >= 12) && (sV1 < 40)) iFontType = 22;
					else if ((sV1 >= 40) || (sV1 < 0))	iFontType = 23;

					m_pChatMsgList[i] = std::make_unique<CMsg>(iFontType, cTxt, m_dwCurTime);
				}
				else
				{
					strcpy(cTxt, " * Failed! *");
					m_pChatMsgList[i] = std::make_unique<CMsg>(22, cTxt, m_dwCurTime);
					PlaySound('C', 17, 0);
				}
				m_pChatMsgList[i]->m_iObjectID = static_cast<uint16_t>(wObjectID - 30000);
				if (m_pMapData->bSetChatMsgOwner(static_cast<uint16_t>(wObjectID - 30000), -10, -10, i) == false)
				{
					m_pChatMsgList[i].reset();
				}
				return;
			}
		break;

	case DEF_OBJECTATTACK:
	case DEF_OBJECTATTACKMOVE:
		if (wObjectID == m_pPlayer->m_sPlayerObjectID + 30000)
		{
			if (m_pMagicCfgList[sV3] != 0)
			{
				std::memset(cTxt, 0, sizeof(cTxt));
				wsprintf(cTxt, "%s", m_pMagicCfgList[sV3]->m_cName);
				AddEventList(cTxt, 10);
			}
		}
		break;
	}
}

void CGame::GrandMagicResult(char* pMapName, int iV1, int iV2, int iV3, int iV4, int iHP1, int iHP2, int iHP3, int iHP4)
{
	int i, iTxtIdx = 0;
	char cTemp[120];

	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();
	}

	for (i = 0; i < 92; i++)
		if (m_pGameMsgList[i] == 0) return;

	if (strcmp(pMapName, "aresden") == 0)
	{
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[2]->m_pMsg, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[3]->m_pMsg, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[4]->m_pMsg, iV1);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[5]->m_pMsg, iV2);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[6]->m_pMsg, iV3);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[58]->m_pMsg, iV4);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d %d %d %d", NOTIFY_MSG_STRUCTURE_HP, iHP1, iHP2, iHP3, iHP4);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		if (iV2 == 0) {
			if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
			{
				PlaySound('E', 25, 0, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[59]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[60]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[61]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[62]->m_pMsg, 0);
				for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
			else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
			{
				PlaySound('E', 25, 0, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[69]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[70]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[71]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[72]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[73]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[74]->m_pMsg, 0);
				for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
			else PlaySound('E', 25, 0, 0);
		}
		else
		{
			if (iV1 != 0)
			{
				if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
				{
					PlaySound('E', 23, 0, 0);
					PlaySound('C', 21, 0, 0);
					PlaySound('C', 22, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[63]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[64]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[65]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
				{
					PlaySound('E', 24, 0, 0);
					PlaySound('C', 12, 0, 0);
					PlaySound('C', 13, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[75]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[76]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[77]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[78]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[79]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[80]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[81]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[82]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else PlaySound('E', 25, 0, 0);
			}
			else
			{
				if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
				{
					PlaySound('E', 23, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[66]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[67]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[68]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
				{
					PlaySound('E', 24, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[83]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[84]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[85]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[86]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[87]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[88]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[89]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[90]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else PlaySound('E', 25, 0, 0);
			}
		}
	}
	else if (strcmp(pMapName, "elvine") == 0)
	{
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[7]->m_pMsg, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[8]->m_pMsg, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[4]->m_pMsg, iV1);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[5]->m_pMsg, iV2);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[6]->m_pMsg, iV3);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d", m_pGameMsgList[58]->m_pMsg, iV4);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		std::memset(cTemp, 0, sizeof(cTemp));
		wsprintf(cTemp, "%s %d %d %d %d", NOTIFY_MSG_STRUCTURE_HP, iHP1, iHP2, iHP3, iHP4);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, cTemp, 0);
		m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, " ", 0);

		if (iV2 == 0) {
			if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
			{
				PlaySound('E', 25, 0, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[59]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[60]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[61]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[62]->m_pMsg, 0);
				for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
			else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
			{
				PlaySound('E', 25, 0, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[69]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[70]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[71]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[72]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[73]->m_pMsg, 0);
				m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[74]->m_pMsg, 0);
				for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
			}
			else PlaySound('E', 25, 0, 0);
		}
		else
		{
			if (iV1 != 0) {
				if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
				{
					PlaySound('E', 23, 0, 0);
					PlaySound('C', 21, 0, 0);
					PlaySound('C', 22, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[63]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[64]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[65]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
				{
					PlaySound('E', 24, 0, 0);
					PlaySound('C', 12, 0, 0);
					PlaySound('C', 13, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[75]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[76]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[77]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[78]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[79]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[80]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[81]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[82]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else PlaySound('E', 25, 0, 0);
			}
			else
			{
				if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == true))
				{
					PlaySound('E', 23, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[66]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[67]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[68]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else if ((m_pPlayer->m_bCitizen == true) && (m_pPlayer->m_bAresden == false))
				{
					PlaySound('E', 24, 0, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[83]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[84]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[85]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[86]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[87]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[88]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[89]->m_pMsg, 0);
					m_pMsgTextList[iTxtIdx++] = std::make_unique<CMsg>(0, m_pGameMsgList[90]->m_pMsg, 0);
					for (i = iTxtIdx; i < 18; i++) m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
				}
				else PlaySound('E', 25, 0, 0);
			}
		}
	}

	m_dialogBoxManager.EnableDialogBox(DialogBoxId::Text, 0, 0, 0);
}

// num : 1 - F2, 2 - F3
void CGame::UseShortCut(int num)
{
	int index;
	if (num < 4) index = num;
	else index = num + 7;
	if (GameModeManager::GetMode() != GameMode::MainGame) return;
	if (Input::IsCtrlDown() == true)
	{
		if (m_sRecentShortCut == -1)
		{
			AddEventList(MSG_SHORTCUT1, 10);
			wsprintf(G_cTxt, MSG_SHORTCUT2, index);// [F%d]
			AddEventList(G_cTxt, 10);
			wsprintf(G_cTxt, MSG_SHORTCUT3, index);// [Control]-[F%d]
			AddEventList(G_cTxt, 10);
		}
		else
		{
			m_sShortCut[num] = m_sRecentShortCut;
			if (m_sShortCut[num] < 100)
			{
				if (m_pItemList[m_sShortCut[num]] == 0)
				{
					m_sShortCut[num] = -1;
					m_sRecentShortCut = -1;
					return;
				}
				char cStr1[64], cStr2[64], cStr3[64];
				std::memset(cStr1, 0, sizeof(cStr1));
				std::memset(cStr2, 0, sizeof(cStr2));
				std::memset(cStr3, 0, sizeof(cStr3));

				GetItemName(m_pItemList[m_sShortCut[num]].get(), cStr1, cStr2, cStr3);
				wsprintf(G_cTxt, MSG_SHORTCUT4, cStr1, cStr2, cStr3, index);// (%s %s %s) [F%d]
				AddEventList(G_cTxt, 10);
			}
			else if (m_sShortCut[num] >= 100)
			{
				if (m_pMagicCfgList[m_sShortCut[num] - 100] == 0)
				{
					m_sShortCut[num] = -1;
					m_sRecentShortCut = -1;
					return;
				}
				wsprintf(G_cTxt, MSG_SHORTCUT5, m_pMagicCfgList[m_sShortCut[num] - 100]->m_cName, index);// %s) [F%d]
				AddEventList(G_cTxt, 10);
			}
		}
	}
	else
	{
		if (m_sShortCut[num] == -1)
		{
			AddEventList(MSG_SHORTCUT1, 10);
			wsprintf(G_cTxt, MSG_SHORTCUT2, index);// [F%d]
			AddEventList(G_cTxt, 10);
			wsprintf(G_cTxt, MSG_SHORTCUT3, index);// [Control]-[F%d]
			AddEventList(G_cTxt, 10);
		}
		else if (m_sShortCut[num] < 100)
		{
			ItemEquipHandler((char)m_sShortCut[num]);
		}
		else if (m_sShortCut[num] >= 100) UseMagic(m_sShortCut[num] - 100);
	}
}

int CGame::iGetManaCost(int iMagicNo)
{
	int i, iManaSave, iManaCost;
	iManaSave = 0;
	if (iMagicNo < 0 || iMagicNo >= 100) return 1;
	for (i = 0; i < DEF_MAXITEMS; i++)
	{
		if (m_pItemList[i] == 0) continue;
		if (m_bIsItemEquipped[i] == true)
		{
			// Data-driven mana save calculation using ItemEffectType
			auto effectType = m_pItemList[i]->GetItemEffectType();
			switch (effectType)
			{
			case hb::item::ItemEffectType::AttackManaSave:
				// Weapons with mana save: value stored in m_sItemEffectValue4
				iManaSave += m_pItemList[i]->m_sItemEffectValue4;
				break;

			case hb::item::ItemEffectType::AddEffect:
				// AddEffect with sub-type ManaSave (necklaces, etc.)
				if (m_pItemList[i]->m_sItemEffectValue1 == hb::item::ToInt(hb::item::AddEffectType::ManaSave))
				{
					iManaSave += m_pItemList[i]->m_sItemEffectValue2;
				}
				break;

			default:
				break;
			}
		}
	}
	// Mana save max = 80%
	if (iManaSave > 80) iManaSave = 80;
	iManaCost = m_pMagicCfgList[iMagicNo]->m_sValue1;
	if (m_pPlayer->m_bIsSafeAttackMode) iManaCost += (iManaCost / 2) - (iManaCost / 10);
	if (iManaSave > 0)
	{
		double dV1 = (double)iManaSave;
		double dV2 = (double)(dV1 / 100.0f);
		double dV3 = (double)iManaCost;
		dV1 = dV2 * dV3;
		dV2 = dV3 - dV1;
		iManaCost = (int)dV2;
	}
	if (iManaCost < 1) iManaCost = 1;
	return iManaCost;
}

void CGame::UseMagic(int iMagicNo)
{
	if (iMagicNo < 0 || iMagicNo >= 100) return;
	if ((m_pPlayer->m_iMagicMastery[iMagicNo] == 0) || (m_pMagicCfgList[iMagicNo] == 0)) return;

	// Casting
	if (m_pPlayer->m_iHP <= 0) return;
	if (m_bIsGetPointingMode == true) return;
	if (iGetManaCost(iMagicNo) > m_pPlayer->m_iMP) return;
	if (_bIsItemOnHand() == true)
	{
		AddEventList(DLGBOX_CLICK_MAGIC1, 10);
		return;
	}
	if (m_bSkillUsingStatus == true)
	{
		AddEventList(DLGBOX_CLICK_MAGIC2, 10);
		return;
	}
	if ((m_pPlayer->m_sPlayerAppr2 & 0xF000) == 0) bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_TOGGLECOMBATMODE, 0, 0, 0, 0, 0);
	m_pPlayer->m_Controller.SetCommand(DEF_OBJECTMAGIC);
	m_iCastingMagicType = iMagicNo;
	m_sMagicShortCut = iMagicNo;
	m_sRecentShortCut = iMagicNo + 100;
	m_iPointCommandType = iMagicNo + 100;
	//m_bIsGetPointingMode = true;
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Magic);
}

void CGame::ReleaseEquipHandler(char cEquipPos)
{
	char cStr1[64], cStr2[64], cStr3[64];
	if (m_sItemEquipmentStatus[cEquipPos] < 0) return;
	// Remove Angelic Stats
	if ((cEquipPos >= 11)
		&& (m_pItemList[m_sItemEquipmentStatus[cEquipPos]]->m_cItemType == 1))
	{
		char cItemID = m_sItemEquipmentStatus[cEquipPos];
		if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(STR)", 19) == 0)
		{
			m_pPlayer->m_iAngelicStr = 0;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(DEX)", 19) == 0)
		{
			m_pPlayer->m_iAngelicDex = 0;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(INT)", 19) == 0)
		{
			m_pPlayer->m_iAngelicInt = 0;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(MAG)", 19) == 0)
		{
			m_pPlayer->m_iAngelicMag = 0;
		}
	}

	GetItemName(m_pItemList[m_sItemEquipmentStatus[cEquipPos]].get(), cStr1, cStr2, cStr3);
	wsprintf(G_cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
	AddEventList(G_cTxt, 10);
	m_bIsItemEquipped[m_sItemEquipmentStatus[cEquipPos]] = false;
	m_sItemEquipmentStatus[cEquipPos] = -1;
}

void CGame::ItemEquipHandler(char cItemID)
{
	if (bCheckItemOperationEnabled(cItemID) == false) return;
	if (m_bIsItemEquipped[cItemID] == true) return;
	if (m_pItemList[cItemID]->m_cEquipPos == DEF_EQUIPPOS_NONE)
	{
		AddEventList(BITEMDROP_CHARACTER3, 10);//"The item is not available."
		return;
	}
	if (m_pItemList[cItemID]->m_wCurLifeSpan == 0)
	{
		AddEventList(BITEMDROP_CHARACTER1, 10); //"The item is exhausted. Fix it to use it."
		return;
	}
	if (m_pItemList[cItemID]->m_wWeight / 100 > m_pPlayer->m_iStr + m_pPlayer->m_iAngelicStr)
	{
		AddEventList(BITEMDROP_CHARACTER2, 10);
		return;
	}
	if (((m_pItemList[cItemID]->m_dwAttribute & 0x00000001) == 0) && (m_pItemList[cItemID]->m_sLevelLimit > m_pPlayer->m_iLevel))
	{
		AddEventList(BITEMDROP_CHARACTER4, 10);
		return;
	}
	if (m_bSkillUsingStatus == true)
	{
		AddEventList(BITEMDROP_CHARACTER5, 10);
		return;
	}
	if (m_pItemList[cItemID]->m_cGenderLimit != 0)
	{
		switch (m_pPlayer->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			if (m_pItemList[cItemID]->m_cGenderLimit != 1)
			{
				AddEventList(BITEMDROP_CHARACTER6, 10);
				return;
			}
			break;
		case 4:
		case 5:
		case 6:
			if (m_pItemList[cItemID]->m_cGenderLimit != 2)
			{
				AddEventList(BITEMDROP_CHARACTER7, 10);
				return;
			}
			break;
		}
	}

	bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_EQUIPITEM, 0, cItemID, 0, 0, 0);
	m_sRecentShortCut = cItemID;
	ReleaseEquipHandler(m_pItemList[cItemID]->m_cEquipPos);
	switch (m_pItemList[cItemID]->m_cEquipPos) {
	case DEF_EQUIPPOS_HEAD:
	case DEF_EQUIPPOS_BODY:
	case DEF_EQUIPPOS_ARMS:
	case DEF_EQUIPPOS_PANTS:
	case DEF_EQUIPPOS_BOOTS:
	case DEF_EQUIPPOS_BACK:
		ReleaseEquipHandler(DEF_EQUIPPOS_FULLBODY);
		break;
	case DEF_EQUIPPOS_FULLBODY:
		ReleaseEquipHandler(DEF_EQUIPPOS_HEAD);
		ReleaseEquipHandler(DEF_EQUIPPOS_BODY);
		ReleaseEquipHandler(DEF_EQUIPPOS_ARMS);
		ReleaseEquipHandler(DEF_EQUIPPOS_PANTS);
		ReleaseEquipHandler(DEF_EQUIPPOS_BOOTS);
		ReleaseEquipHandler(DEF_EQUIPPOS_BACK);
		break;
	case DEF_EQUIPPOS_LHAND:
	case DEF_EQUIPPOS_RHAND:
		ReleaseEquipHandler(DEF_EQUIPPOS_TWOHAND);
		break;
	case DEF_EQUIPPOS_TWOHAND:
		ReleaseEquipHandler(DEF_EQUIPPOS_RHAND);
		ReleaseEquipHandler(DEF_EQUIPPOS_LHAND);
		break;
	}

	m_sItemEquipmentStatus[m_pItemList[cItemID]->m_cEquipPos] = cItemID;
	m_bIsItemEquipped[cItemID] = true;

	// Add Angelic Stats
	if ((m_pItemList[cItemID]->m_cItemType == 1)
		&& (m_pItemList[cItemID]->m_cEquipPos >= 11))
	{
		int iAngelValue = 0;
		if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(STR)", 19) == 0)
		{
			iAngelValue = (m_pItemList[cItemID]->m_dwAttribute & 0xF0000000) >> 28;
			m_pPlayer->m_iAngelicStr = 1 + iAngelValue;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(DEX)", 19) == 0)
		{
			iAngelValue = (m_pItemList[cItemID]->m_dwAttribute & 0xF0000000) >> 28;
			m_pPlayer->m_iAngelicDex = 1 + iAngelValue;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(INT)", 19) == 0)
		{
			iAngelValue = (m_pItemList[cItemID]->m_dwAttribute & 0xF0000000) >> 28;
			m_pPlayer->m_iAngelicInt = 1 + iAngelValue;
		}
		else if (memcmp(m_pItemList[cItemID]->m_cName, "AngelicPandent(MAG)", 19) == 0)
		{
			iAngelValue = (m_pItemList[cItemID]->m_dwAttribute & 0xF0000000) >> 28;
			m_pPlayer->m_iAngelicMag = 1 + iAngelValue;
		}
	}

	char cStr1[64], cStr2[64], cStr3[64];
	GetItemName(m_pItemList[cItemID].get(), cStr1, cStr2, cStr3);
	wsprintf(G_cTxt, BITEMDROP_CHARACTER9, cStr1);
	AddEventList(G_cTxt, 10);
	PlaySound('E', 28, 0);
}

/*********************************************************************************************************************
**  void CheckActiveAura(short sX, short sY, DWORD dwTime, short sOwnerType)( initially Cleroth fixed by Snoopy )	**
**  description			: Generates special auras around players													**
**						: v351 implements this in each drawn function,beter to regroup in single function.			**
**********************************************************************************************************************/
void CGame::CheckActiveAura(short sX, short sY, uint32_t dwTime, short sOwnerType)
{	// Used at the beginning of character drawing
	// DefenseShield
	if ((m_entityState.m_iStatus & 0x02000000) != 0)
		//m_pEffectSpr[80]->Draw(sX+75, sY+107, m_entityState.m_iEffectFrame%17, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[80]->Draw(sX + 75, sY + 107, m_entityState.m_iEffectFrame % 17, SpriteLib::DrawParams::Alpha(0.5f));

	// Protection From Magic
	if ((m_entityState.m_iStatus & 0x04000000) != 0)
		//m_pEffectSpr[79]->Draw(sX+101, sY+135, m_entityState.m_iEffectFrame%15, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[79]->Draw(sX + 101, sY + 135, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));

	// Protection From Arrow
	if ((m_entityState.m_iStatus & 0x08000000) != 0)
		//m_pEffectSpr[72]->Draw(sX, sY+35, m_entityState.m_iEffectFrame%30, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[72]->Draw(sX, sY + 35, m_entityState.m_iEffectFrame % 30, SpriteLib::DrawParams::Alpha(0.7f));

	// Illusion
	if ((m_entityState.m_iStatus & 0x01000000) != 0)
		//m_pEffectSpr[73]->Draw(sX+125, sY+95, m_entityState.m_iEffectFrame%24, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[73]->Draw(sX + 125, sY + 130 - _iAttackerHeight[sOwnerType], m_entityState.m_iEffectFrame % 24, SpriteLib::DrawParams::Alpha(0.7f));

	// Illusion movement
	if ((m_entityState.m_iStatus & 0x00200000) != 0)
		//m_pEffectSpr[151]->Draw(sX+90, sY+55, m_entityState.m_iEffectFrame%24, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[151]->Draw(sX + 90, sY + 90 - _iAttackerHeight[sOwnerType], m_entityState.m_iEffectFrame % 24, SpriteLib::DrawParams::Alpha(0.7f));

	// Slate red  (HP)  Flame au sol
	if ((m_entityState.m_iStatus & 0x00400000) != 0)
		//m_pEffectSpr[149]->Draw(sX+90, sY+120, m_entityState.m_iEffectFrame%15, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[149]->Draw(sX + 90, sY + 120, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));

	// Slate Blue (Mana) Bleu au sol
	if ((m_entityState.m_iStatus & 0x00800000) != 0)
		//m_pEffectSpr[150]->Draw(sX+1, sY+26, m_entityState.m_iEffectFrame%15, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[150]->Draw(sX + 1, sY + 26, m_entityState.m_iEffectFrame % 15, SpriteLib::DrawParams::Alpha(0.7f));

	// Slate Green (XP) Mauve au sol
	if ((m_entityState.m_iStatus & 0x00010000) != 0)
		//m_pEffectSpr[148]->Draw(sX, sY+32, m_entityState.m_iEffectFrame%23, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[148]->Draw(sX, sY + 32, m_entityState.m_iEffectFrame % 23, SpriteLib::DrawParams::Alpha(0.7f));

	// Hero Flag (Heldenian)  Flameches d'entangle
	if ((m_entityState.m_iStatus & 0x00020000) != 0)
		//m_pEffectSpr[87]->Draw(sX+53, sY+54, m_entityState.m_iEffectFrame%29, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[87]->Draw(sX + 53, sY + 54, m_entityState.m_iEffectFrame % 29, SpriteLib::DrawParams::Alpha(0.7f));
}

/*********************************************************************************************************************
**  void CheckActiveAura2(short sX, short sY, DWORD dwTime,  m_entityState.m_sOwnerType) ( initially Cleroth fixed by Snoopy )	**
**  description			: Generates poison aura around players. This one should be use later...						**
**						: v351 implements this in each drawn function,beter to regroup in single function.			**
**********************************************************************************************************************/
void CGame::CheckActiveAura2(short sX, short sY, uint32_t dwTime, short sOwnerType)
{	// Poison
	if ((m_entityState.m_iStatus & 0x80) != 0)
		//m_pEffectSpr[81]->Draw(sX+115, sY+85, m_entityState.m_iEffectFrame%21, SpriteLib::DrawParams::Alpha(0.5f));
		m_pEffectSpr[81]->Draw(sX + 115, sY + 120 - _iAttackerHeight[sOwnerType], m_entityState.m_iEffectFrame % 21, SpriteLib::DrawParams::Alpha(0.7f));
	//	_iAttackerHeight[]
}

void CGame::DrawAngel(int iSprite, short sX, short sY, char cFrame, uint32_t dwTime)
{
	switch (m_entityState.m_iDir)
	{
	case 1:
	case 2:
	case 7:
	case 8:
		sX -= 30;
		break;
	}
	if ((m_entityState.m_iStatus & 0x10) != 0)
	{
		if ((m_entityState.m_iStatus & 0x1000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + iSprite]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));  //AngelicPendant(STR)
		else if ((m_entityState.m_iStatus & 0x2000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 1) + iSprite]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f)); //AngelicPendant(DEX)
		else if ((m_entityState.m_iStatus & 0x4000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 2) + iSprite]->Draw(sX, sY - 15, cFrame, SpriteLib::DrawParams::Alpha(0.5f));//AngelicPendant(INT)
		else if ((m_entityState.m_iStatus & 0x8000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 3) + iSprite]->Draw(sX, sY - 15, cFrame, SpriteLib::DrawParams::Alpha(0.5f));//AngelicPendant(MAG)
	}
	else
	{
		if ((m_entityState.m_iStatus & 0x1000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + iSprite]->Draw(sX, sY, cFrame);  //AngelicPendant(STR)
		else if ((m_entityState.m_iStatus & 0x2000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 1) + iSprite]->Draw(sX, sY, cFrame); //AngelicPendant(DEX)
		else if ((m_entityState.m_iStatus & 0x4000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 2) + iSprite]->Draw(sX, sY - 15, cFrame);//AngelicPendant(INT)
		else if ((m_entityState.m_iStatus & 0x8000) != 0)
			m_pSprite[DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + (50 * 3) + iSprite]->Draw(sX, sY - 15, cFrame);//AngelicPendant(MAG)
	}

}
/*********************************************************************************************************************
**  int CGame::bHasHeroSet( short m_sAppr3, short m_sAppr3, char OwnerType)		( Snoopy )							**
**  description			:: check weather the object (is character) is using a hero set (1:war, 2:mage)				**
**********************************************************************************************************************/
int CGame::bHasHeroSet(short Appr3, short Appr4, char OwnerType)
{
	char cArmor, cLeg, cBerk, cHat;
	cArmor = (Appr3 & 0xF000) >> 12;
	cLeg = (Appr3 & 0x0F00) >> 8;
	cHat = (Appr3 & 0x00F0) >> 4;
	cBerk = Appr3 & 0x000F;
	switch (OwnerType) {
	case 1:
	case 2:
	case 3:
		if ((cArmor == 8) && (cLeg == 5) && (cHat == 9) && (cBerk == 3)) return (1); // Warr elv M
		if ((cArmor == 9) && (cLeg == 6) && (cHat == 10) && (cBerk == 4)) return (1); // Warr ares M
		if ((cArmor == 10) && (cLeg == 5) && (cHat == 11) && (cBerk == 3)) return (2); // Mage elv M
		if ((cArmor == 11) && (cLeg == 6) && (cHat == 12) && (cBerk == 4)) return (2); // Mage ares M
		break;
	case 4:
	case 5:
	case 6: // fixed
		if ((cArmor == 9) && (cLeg == 6) && (cHat == 9) && (cBerk == 4)) return (1); //warr elv W
		if ((cArmor == 10) && (cLeg == 7) && (cHat == 10) && (cBerk == 5)) return (1); //warr ares W
		if ((cArmor == 11) && (cLeg == 6) && (cHat == 11) && (cBerk == 4)) return (2); //mage elv W
		if ((cArmor == 12) && (cLeg == 7) && (cHat == 12) && (cBerk == 5)) return (2); //mage ares W
		break;
	}
	return 0;
}
/*********************************************************************************************************************
**  void ShowHeldenianVictory( short sSide)				( Snoopy )													**
**  description			: Shows the Heldenian's End window															**
**********************************************************************************************************************/
void CGame::ShowHeldenianVictory(short sSide)
{
	int i, iPlayerSide;
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::Text);
	for (i = 0; i < DEF_TEXTDLGMAXLINES; i++)
	{
		if (m_pMsgTextList[i] != 0)
			m_pMsgTextList[i].reset();
	}
	if (m_pPlayer->m_bCitizen == false) iPlayerSide = 0;
	else if (m_pPlayer->m_bAresden == true) iPlayerSide = 1;
	else if (m_pPlayer->m_bAresden == false) iPlayerSide = 2;
	switch (sSide) {
	case 0:
		PlaySound('E', 25, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, "Heldenian holy war has been closed!", 0);
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, "Heldenian Holy war ended", 0);
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, "in a tie.", 0);
		break;
	case 1:
		PlaySound('E', 25, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, "Heldenian holy war has been closed!", 0);
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, "Heldenian Holy war ended", 0);
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, "in favor of Aresden.", 0);
		break;
	case 2:
		PlaySound('E', 25, 0, 0);
		m_pMsgTextList[0] = std::make_unique<CMsg>(0, "Heldenian holy war has been closed!", 0);
		m_pMsgTextList[1] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[2] = std::make_unique<CMsg>(0, "Heldenian Holy war ended", 0);
		m_pMsgTextList[3] = std::make_unique<CMsg>(0, "in favor of Elvine.", 0);
		break;
	}
	m_pMsgTextList[4] = std::make_unique<CMsg>(0, " ", 0);

	if (((iPlayerSide != 1) && (iPlayerSide != 2))   // Player not a normal citizen
		|| (sSide == 0))								// or no winner
	{
		PlaySound('E', 25, 0, 0);
		m_pMsgTextList[5] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[6] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[7] = std::make_unique<CMsg>(0, " ", 0);
		m_pMsgTextList[8] = std::make_unique<CMsg>(0, " ", 0);
	}
	else
	{
		if (sSide == iPlayerSide)
		{
			PlaySound('E', 23, 0, 0);
			PlaySound('C', 21, 0, 0);
			PlaySound('C', 22, 0, 0);
			m_pMsgTextList[5] = std::make_unique<CMsg>(0, "Congratulation.", 0);
			m_pMsgTextList[6] = std::make_unique<CMsg>(0, "As cityzen of victory,", 0);
			m_pMsgTextList[7] = std::make_unique<CMsg>(0, "You will recieve a reward.", 0);
			m_pMsgTextList[8] = std::make_unique<CMsg>(0, "      ", 0);
		}
		else
		{
			PlaySound('E', 24, 0, 0);
			PlaySound('C', 12, 0, 0);
			PlaySound('C', 13, 0, 0);
			m_pMsgTextList[5] = std::make_unique<CMsg>(0, "To our regret", 0);
			m_pMsgTextList[6] = std::make_unique<CMsg>(0, "As cityzen of defeat,", 0);
			m_pMsgTextList[7] = std::make_unique<CMsg>(0, "You cannot recieve any reward.", 0);
			m_pMsgTextList[8] = std::make_unique<CMsg>(0, "     ", 0);
		}
	}
	for (i = 9; i < 18; i++)
		m_pMsgTextList[i] = std::make_unique<CMsg>(0, " ", 0);
	m_dialogBoxManager.EnableDialogBox(DialogBoxId::Text, 0, 0, 0);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeCommander);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeConstructor);
	m_dialogBoxManager.DisableDialogBox(DialogBoxId::CrusadeSoldier);
}

/*********************************************************************************************************************
**  void 	ResponseHeldenianTeleportList(char *pData)									(  Snoopy )					**
**  description			: Gail's TP																					**
**********************************************************************************************************************/
void CGame::ResponseHeldenianTeleportList(char* pData)
{
	int i;
#ifdef _DEBUG
	AddEventList("Teleport ???", 10);
#endif
	const auto* header = hb::net::PacketCast<hb::net::PacketResponseTeleportListHeader>(
		pData, sizeof(hb::net::PacketResponseTeleportListHeader));
	if (!header) return;
	const auto* entries = reinterpret_cast<const hb::net::PacketResponseTeleportListEntry*>(
		pData + sizeof(hb::net::PacketResponseTeleportListHeader));
	m_iTeleportMapCount = header->count;
	for (i = 0; i < m_iTeleportMapCount; i++)
	{
		m_stTeleportList[i].iIndex = entries[i].index;
		std::memset(m_stTeleportList[i].mapname, 0, sizeof(m_stTeleportList[i].mapname));
		memcpy(m_stTeleportList[i].mapname, entries[i].map_name, 10);
		m_stTeleportList[i].iX = entries[i].x;
		m_stTeleportList[i].iY = entries[i].y;
		m_stTeleportList[i].iCost = entries[i].cost;
	}
}
/*********************************************************************************************************************
**  bool DKGlare(int iWeaponIndex, int iWeaponIndex, int *iWeaponGlare)	( Snoopy )									**
**  description			: test glowing condition for DK set															**
**********************************************************************************************************************/
void CGame::DKGlare(int iWeaponColor, int iWeaponIndex, int* iWeaponGlare)
{
	if (iWeaponColor != 9) return;
	if (((iWeaponIndex >= DEF_SPRID_WEAPON_M + 64 * 14) && (iWeaponIndex < DEF_SPRID_WEAPON_M + 64 * 14 + 56)) //msw3
		|| ((iWeaponIndex >= DEF_SPRID_WEAPON_W + 64 * 14) && (iWeaponIndex < DEF_SPRID_WEAPON_W + 64 * 14 + 56))) //wsw3
	{
		*iWeaponGlare = 3;
	}
	else if (((iWeaponIndex >= DEF_SPRID_WEAPON_M + 64 * 37) && (iWeaponIndex < DEF_SPRID_WEAPON_M + 64 * 37 + 56)) //MStaff3
		|| ((iWeaponIndex >= DEF_SPRID_WEAPON_W + 64 * 37) && (iWeaponIndex < DEF_SPRID_WEAPON_W + 64 * 37 + 56)))//WStaff3
	{
		*iWeaponGlare = 2;
	}
}
/*********************************************************************************************************************
**  void CGame::Abaddon_corpse(int sX, int sY);		( Snoopy )														**
**  description			: Placeholder for abaddon's death lightnings												**
**********************************************************************************************************************/
void CGame::Abaddon_corpse(int sX, int sY)
{
	int ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX + 30, 0, sX + 30, sY - 10, ir, ir, 1);
	_DrawThunderEffect(sX + 30, 0, sX + 30, sY - 10, ir + 2, ir, 2);
	_DrawThunderEffect(sX + 30, 0, sX + 30, sY - 10, ir - 2, ir, 2);
	ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX - 20, 0, sX - 20, sY - 35, ir, ir, 1);
	_DrawThunderEffect(sX - 20, 0, sX - 20, sY - 35, ir + 2, ir, 2);
	_DrawThunderEffect(sX - 20, 0, sX - 20, sY - 35, ir - 2, ir, 2);
	ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX - 10, 0, sX - 10, sY + 30, ir, ir, 1);
	_DrawThunderEffect(sX - 10, 0, sX - 10, sY + 30, ir + 2, ir + 2, 2);
	_DrawThunderEffect(sX - 10, 0, sX - 10, sY + 30, ir - 2, ir + 2, 2);
	ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX + 50, 0, sX + 50, sY + 35, ir, ir, 1);
	_DrawThunderEffect(sX + 50, 0, sX + 50, sY + 35, ir + 2, ir + 2, 2);
	_DrawThunderEffect(sX + 50, 0, sX + 50, sY + 35, ir - 2, ir + 2, 2);
	ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX + 65, 0, sX + 65, sY - 5, ir, ir, 1);
	_DrawThunderEffect(sX + 65, 0, sX + 65, sY - 5, ir + 2, ir + 2, 2);
	_DrawThunderEffect(sX + 65, 0, sX + 65, sY - 5, ir - 2, ir + 2, 2);
	ir = (rand() % 20) - 10;
	_DrawThunderEffect(sX + 45, 0, sX + 45, sY - 50, ir, ir, 1);
	_DrawThunderEffect(sX + 45, 0, sX + 45, sY - 50, ir + 2, ir + 2, 2);
	_DrawThunderEffect(sX + 45, 0, sX + 45, sY - 50, ir - 2, ir + 2, 2);

	for (int x = sX - 50; x <= sX + 100; x += rand() % 35)
	{
		for (int y = sY - 30; y <= sY + 50; y += rand() % 45)
		{
			ir = (rand() % 20) - 10;
			_DrawThunderEffect(x, 0, x, y, ir, ir, 2);
		}
	}
}