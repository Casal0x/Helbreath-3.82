// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NativeTypes.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <memory>
#include <fstream>
#include <iostream>
#include <iosfwd>
#include <vector>
#include <sstream>
#include <array>
#include <format>

#include "GlobalDef.h"
#include "GameGeometry.h"
#include "IRenderer.h"
#include "RendererFactory.h"
#include "ISprite.h"
#include "ISpriteFactory.h"
#include "SpriteCollection.h"
#include "ITextRenderer.h"
#include "IBitmapFont.h"
#include "BitmapFontFactory.h"
#include "IInput.h"
#include "ASIOSocket.h"
#include "IOServicePool.h"
#include "SpriteID.h"
#include "Misc.h"
#include "ChatMsg.h"
#include "Effect.h"
#include "MapData.h"
#include "ActionID.h"
#include "ActionID_Client.h"
#include "NetMessages.h"
#include "ClientMessages.h"
#include "CharInfo.h"
#include "Item/Item.h"
#include "Magic.h"
#include "Skill.h"
#include "DynamicObjectID.h"
#include "BuildItem.h"
#include "DialogBoxManager.h"
#include "EffectManager.h"
#include "NetworkMessageManager.h"
#include "CursorTarget.h"
#include "Player.h"
#include "EntityRenderState.h"
#include "Camera.h"
#include "GameModeManager.h"
#include "PlayerRenderer.h"
#include "NpcRenderer.h"
#include "GameTimer.h"
#include "FloatingTextManager.h"
#include "FishingManager.h"
#include "CraftingManager.h"
#include "QuestManager.h"
#include "GuildManager.h"

#include "GameConstants.h"
#include "Application.h"
#include "GameEvents.h"

// Overlay types for popup screens that render over base screens
enum class OverlayType {
	None = 0,
	Connecting,
	WaitingResponse,
	LogResMsg,
	QueryForceLogin,
	QueryDeleteCharacter
};

class CGame : public hb::shared::render::application
{
public:
	CGame(hb::shared::types::NativeInstance native_instance, int icon_resource_id);
	~CGame() override;

	// --- application overrides ---
	bool on_initialize() override;
	bool on_start() override;
	void on_uninitialize() override;
	void on_run() override;
	void on_event(const hb::shared::render::event& e) override;
	bool on_native_message(uint32_t message, uintptr_t wparam, intptr_t lparam) override;
	void on_key_event(KeyCode key, bool pressed) override;
	bool on_text_input(hb::shared::types::NativeWindowHandle hwnd,
	                   uint32_t message, uintptr_t wparam, intptr_t lparam) override;
	// CLEROTH - AURAS
	void CheckActiveAura(short sX, short sY, uint32_t dwTime, short sOwnerType);
	void CheckActiveAura2(short sX, short sY, uint32_t dwTime, short sOwnerType);

	// Camera for viewport management
	CCamera m_Camera;



	void ReadSettings();
	void WriteSettings();

	bool FindGuildName(const char* pName, int* ipIndex);
	void bItemDrop_ExternalScreen(char item_id, short mouse_x, short mouse_y);
	void CreateScreenShot();
	void CrusadeWarResult(int winner_side);
	void CrusadeContributionResult(int war_contribution);
	void CannotConstruct(int iCode);
	void DrawTopMsg();
	void SetTopMsg(const char* pString, unsigned char iLastSec);
	void DrawObjectFOE(int ix, int iy, int iFrame);
	void GrandMagicResult(const char* map_name, int ares_crusade_points, int elv_crusade_points, int ares_industry_points, int elv_industry_points, int ares_crusade_casualties, int ares_industry_casualties, int elv_crusade_casualties, int elv_industry_casualties);
	void MeteorStrikeComing(int iCode);

	void DrawNewDialogBox(char cType, int sX, int sY, int iFrame, bool bIsNoColorKey = false, bool bIsTrans = false);
	void AddMapStatusInfo(const char* pData, bool bIsLastData);
	void _RequestMapStatus(const char* pMapName, int iMode);
	void DrawDialogBoxs(short mouse_x, short mouse_y, short mouse_z, char left_button);
	std::string FormatCommaNumber(uint32_t value);

	void ResponsePanningHandler(char * pData);
	void _SetIlusionEffect(int iOwnerH);
	void NoticementHandler(char * pData);
	CItem* GetItemConfig(int iItemID) const;
	short FindItemIdByName(const char* cItemName);
	void _LoadGameMsgTextContents();
	const char* GetNpcConfigName(short sType) const;
	const char* GetNpcConfigNameById(short npcConfigId) const;
	short ResolveNpcType(short npcConfigId) const;

	void UseShortCut( int num );
	void DrawCursor();
	void on_update();   // Logic update: audio, timers, network, game state
	void on_render();   // Render only: clear backbuffer -> draw -> flip

	void NpcTalkHandler(char * packet_data);
	void SetCameraShakingEffect(short sDist, int iMul = 0);
	void ClearSkillUsingStatus();
	bool bCheckExID(const char* pName);
	bool bCheckLocalChatCommand(const char* pMsg);
	char GetOfficialMapName(const char* pMapName, char* pName);
	uint32_t iGetLevelExp(int iLevel);
	void DrawVersion();
	bool _bIsItemOnHand();
	void DynamicObjectHandler(char * pData);
	bool _bCheckItemByType(hb::shared::item::ItemType type);
	void DrawNpcName(   short screen_x, short screen_y, short owner_type, const hb::shared::entity::PlayerStatus& status, short npc_config_id = -1);
	void DrawObjectName(short screen_x, short screen_y, const char* name, const hb::shared::entity::PlayerStatus& status, uint16_t object_id);
	void PlayGameSound(char cType, int iNum, int iDist, long lPan = 0);  // Forwards to AudioManager
	void _LoadTextDlgContents(int cType);
	int  _iLoadTextDlgContents2(int iType);
	void RequestFullObjectData(uint16_t wObjectID);
	void RetrieveItemHandler(char * pData);
	void CivilRightAdmissionHandler(char * pData);
	void _Draw_CharacterBody(short sX, short sY, short sType);
	void RequestTeleportAndWaitData();
	void PointCommandHandler(int indexX, int indexY, char cItemID = -1);
	void AddEventList(const char* pTxt, char cColor = 0, bool bDupAllow = true);
	void _ShiftGuildOperationList();
	void _PutGuildOperationList(char * pName, char cOpMode);
	void DisbandGuildResponseHandler(char * pData);
	void InitPlayerCharacteristics(char * pData);
	void CreateNewGuildResponseHandler(char * pData);
	void InitGameSettings();
	void CommonEventHandler(char * pData);
	int iGetTopDialogBoxIndex();
	void DisableDialogBox(int iBoxID);
	void EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char * pString = 0);
	void InitItemList(char * packet_data);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnMove_ForMenu(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect __fastcall DrawObject_OnGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	void ClearGuildNameList();
	void DrawBackground(short sDivX, short sModX, short sDivY, short sModY);
	void ChatMsgHandler(char * packet_data);
	void ReleaseUnusedSprites();
	void OnKeyUp(KeyCode key);
	void ChangeGameMode(GameMode mode);
	void LogRecvMsgHandler(char * pData);
	void LogResponseHandler(char * packet_data);
	void OnLogSocketEvent();  // MODERNIZED: Polls socket instead of handling window messages
	void OnTimer();
	void LogEventHandler(char * pData);
	void _ReadMapData(short pivot_x, short pivot_y, const char* packet_data);
	void MotionEventHandler(char * packet_data);
	void InitDataResponseHandler(char * packet_data);
	void InitPlayerResponseHandler(char * pData);
	void ConnectionEstablishHandler(char cWhere);
	void MotionResponseHandler(char * packet_data);
	void GameRecvMsgHandler(uint32_t dwMsgSize, char * pData);
	void DrawObjects(short sPivotX, short sPivotY, short sDivX, short sDivY, short sModX, short sModY, short msX, short msY);
	bool bSendCommand(uint32_t message_id, uint16_t command, char direction, int value1, int value2, int value3, const char* text, int value4 = 0); // v1.4
	void RestoreSprites();
	void CommandProcessor(short mouse_x, short mouse_y, short tile_x, short tile_y, char left_button, char right_button);
	bool ProcessLeftClick(short mouse_x, short mouse_y, short tile_x, short tile_y, uint32_t current_time, uint16_t& action_type);
	bool ProcessRightClick(short mouse_x, short mouse_y, short tile_x, short tile_y, uint32_t current_time, uint16_t& action_type);
	void ProcessMotionCommands(uint16_t action_type);
	void OnGameSocketEvent();  // MODERNIZED: Polls socket instead of handling window messages
	void OnKeyDown(KeyCode key);
	void RegisterHotkeys();
	void Hotkey_ToggleForceAttack();
	void Hotkey_CycleDetailLevel();
	void Hotkey_ToggleHelp();
	void Hotkey_ToggleDialogTransparency();
	void Hotkey_ToggleSystemMenu();
	void Hotkey_ToggleGuideMap();
	void Hotkey_ToggleRunningMode();
	void Hotkey_ToggleSoundAndMusic();
	void Hotkey_WhisperTarget();
	void Hotkey_Simple_UseHealthPotion();
	void Hotkey_Simple_UseManaPotion();
	void Hotkey_Simple_LoadBackupChat();
	void Hotkey_Simple_UseMagicShortcut();
	void Hotkey_Simple_ToggleCharacterInfo();
	void Hotkey_Simple_ToggleInventory();
	void Hotkey_Simple_ToggleMagic();
	void Hotkey_Simple_ToggleSkill();
	void Hotkey_Simple_ToggleChatHistory();
	void Hotkey_Simple_ToggleSystemMenu();
	void Hotkey_Simple_UseShortcut1();
	void Hotkey_Simple_UseShortcut2();
	void Hotkey_Simple_UseShortcut3();
	void Hotkey_Simple_WhisperCycleUp();
	void Hotkey_Simple_WhisperCycleDown();
	void Hotkey_Simple_ArrowLeft();
	void Hotkey_Simple_ArrowRight();
	void Hotkey_Simple_Screenshot();
	void Hotkey_Simple_TabToggleCombat();
	void Hotkey_Simple_ToggleSafeAttack();
	void Hotkey_Simple_Escape();
	void Hotkey_Simple_SpecialAbility();
	void Hotkey_Simple_ZoomIn();
	void Hotkey_Simple_ZoomOut();
	// Platform specifics (passed from main, used in on_initialize)
	hb::shared::types::NativeInstance m_native_instance;
	int m_icon_resource_id;

	void ReserveFightzoneResponseHandler(char * pData);
	void StartBGM();  // Forwards to AudioManager based on current location

	int bHasHeroSet(const hb::shared::entity::PlayerAppearance& appr, short OwnerType);
	void ShowHeldenianVictory(short side);
	void DKGlare(int iWeaponColor, int iWeaponIndex, int *iWeaponGlare);
	void Abaddon_corpse(int sX, int sY);
	void DrawAngel(int iSprite, short sX, short sY, char cFrame, uint32_t dwTime);

	//50Cent - Repair All
	short totalItemRepair;
	int totalPrice;
	struct
	{
		char index;
		short price;
	} m_stRepairAll[hb::shared::limits::MaxItems];

	bool _ItemDropHistory(short sItemID);

	GameTimer m_game_timer;
	DialogBoxManager m_dialogBoxManager;
	FishingManager m_fishingManager;
	CraftingManager m_craftingManager;
	QuestManager m_questManager;
	GuildManager m_guildManager;
	struct {
		int   sV1, sV2, sV3, sV4, sV5, sV6, sV7, sItemID;
		uint32_t dwV1;
		std::string cStr1;
		std::string cStr2;
	} m_stDialogBoxExchangeInfo[8];
	struct {
		int iIndex;
		int iAmount;
	} m_stSellItemList[game_limits::max_sell_list];

	struct {
		std::string cName;
		char cOpMode;
	} m_stGuildOpList[100];

	struct {
		bool bIsQuestCompleted;
		short sWho, sQuestType, sContribution, sTargetType, sTargetCount, sX, sY, sRange;
		short sCurrentCount;
		std::string cTargetName;
	} m_stQuest;

	struct {
		char cStatus;
		std::string cName;
	} m_stPartyMember[hb::shared::limits::MaxPartyMembers];

	struct {
		short sX, sY;
		char cType;
		char cSide;
	} m_stCrusadeStructureInfo[hb::shared::limits::MaxCrusadeStructures];

	struct {
		std::string cName;
	} m_stPartyMemberNameList[hb::shared::limits::MaxPartyMembers+1];

	// v2.171 2002-6-14
	struct {
		uint32_t dwRefTime;
		int iGuildRank;
		std::string cCharName;
		std::string cGuildName;
	} m_stGuildName[game_limits::max_guild_names];


	class hb::shared::render::IRenderer* m_Renderer;  // Abstract renderer interface
	std::unique_ptr<hb::shared::sprite::ISpriteFactory> m_pSpriteFactory;  // Sprite factory for creating sprites
	hb::shared::sprite::SpriteCollection m_pSprite;
	hb::shared::sprite::SpriteCollection m_pTileSpr;
	hb::shared::sprite::SpriteCollection m_pEffectSpr;

	std::unique_ptr<CPlayer> m_pPlayer;  // Main player data
	std::unique_ptr<class CMapData> m_pMapData;
	std::unique_ptr<hb::shared::net::IOServicePool> m_pIOPool;  // 0 threads = manual poll mode for client
	std::unique_ptr<class hb::shared::net::ASIOSocket> m_pGSock;
	std::unique_ptr<class hb::shared::net::ASIOSocket> m_pLSock;
	CFloatingTextManager m_floatingText;
	std::unique_ptr<EffectManager> m_pEffectManager;
	std::unique_ptr<NetworkMessageManager> m_pNetworkMessageManager;
	std::array<std::unique_ptr<class CItem>, hb::shared::limits::MaxItems> m_pItemList;
	std::array<std::unique_ptr<class CItem>, hb::shared::limits::MaxBankItems> m_pBankList;
	std::array<std::unique_ptr<class CMagic>, hb::shared::limits::MaxMagicType> m_pMagicCfgList;
	std::array<std::unique_ptr<class CSkill>, hb::shared::limits::MaxSkillType> m_pSkillCfgList;
	std::array<std::unique_ptr<class CMsg>, game_limits::max_text_dlg_lines> m_pMsgTextList;
	std::array<std::unique_ptr<class CMsg>, game_limits::max_text_dlg_lines> m_pMsgTextList2;
	std::array<std::unique_ptr<class CMsg>, game_limits::max_text_dlg_lines> m_pAgreeMsgTextList;
	std::unique_ptr<class CMsg> m_pExID;

	std::array<std::unique_ptr<class CCharInfo>, 4> m_pCharList;
	std::array<std::unique_ptr<class CMsg>, game_limits::max_game_msgs> m_pGameMsgList;


	uint32_t m_dwConnectMode;
	std::vector<char> m_pendingLoginPacket;
	uint32_t m_dwTime;
	uint32_t m_dwCurTime;
	uint32_t m_dwCheckConnTime, m_dwCheckSprTime, m_dwCheckChatTime;
	uint32_t m_dwCheckConnectionTime;
	uint32_t m_logout_count_time;
	uint32_t m_dwRestartCountTime;
	uint32_t m_dwObserverCamTime;
	uint32_t m_dwDamagedTime;
	uint32_t m_dwSpecialAbilitySettingTime;
	uint32_t m_dwCommanderCommandRequestedTime;
	uint32_t m_dwTopMsgTime;
	uint32_t m_dwEnvEffectTime;

	//v2.2
	uint32_t m_dwMonsterEventTime;
	short m_sMonsterID;
	short m_sEventX, m_sEventY;

	//v2.183 Hunter Mode - Moved to CPlayer

std::array<bool, hb::shared::limits::MaxItems> m_bIsItemEquipped{};
	std::array<bool, hb::shared::limits::MaxItems> m_bIsItemDisabled{};
	bool m_bIsGetPointingMode;
	bool m_bWaitForNewClick;  // After magic cast, ignore held click until released
	uint32_t m_dwMagicCastTime;  // Timestamp when magic was cast (for post-cast delay)
	bool m_bSkillUsingStatus;
	bool m_bItemUsingStatus;
	bool m_bIsObserverMode, m_bIsObserverCommanded;
	bool m_bIsFirstConn;
	bool m_bIsServerChanging = false;
	bool m_bIsCrusadeMode;

	bool m_bIsF1HelpWindowEnabled;
	bool m_bHideLocalCursor;
	bool m_bMouseInitialized = false;

	bool m_bForceDisconn;

	uint32_t m_dwFPStime;
	int m_iLatencyMs;
	uint32_t m_dwLastNetMsgId;
	uint32_t m_dwLastNetMsgTime;
	uint32_t m_dwLastNetMsgSize;
	uint32_t m_dwLastNetRecvTime;
	uint32_t m_dwLastNpcEventTime;

	int m_iFightzoneNumber;
	int m_iFightzoneNumberTemp;

	int m_iPointCommandType;
	int m_iTotalChar;
	short m_sMagicShortCut;
	int m_iAccntYear, m_iAccntMonth, m_iAccntDay;
	int m_iIpYear, m_iIpMonth, m_iIpDay;
	int m_iDownSkillIndex;

	int m_iIlusionOwnerH;
	short m_sRecentShortCut;
	std::array<short, 6> m_sShortCut{}; // Snoopy: 6 shortcuts
	int m_iDrawFlag;

	int m_iTimeLeftSecAccount, m_iTimeLeftSecIP;
	int m_iLogServerPort, m_iGameServerPort;
	int m_iBlockYear, m_iBlockMonth, m_iBlockDay;
	unsigned char m_iTopMsgLastSec;
	int m_iNetLagCount;
	int m_iTotalPartyMember;
	int m_iPartyStatus;
	int m_iGizonItemUpgradeLeft;
	std::array<short, hb::shared::item::DEF_MAXITEMEQUIPPOS> m_sItemEquipmentStatus{};
	short m_sMCX, m_sMCY;
	int   m_iCastingMagicType;
	short m_sVDL_X, m_sVDL_Y;

	uint16_t m_wCommObjectID;
	uint16_t m_wLastAttackTargetID;
	uint16_t m_wEnterGameType;
	char m_cItemOrder[hb::shared::limits::MaxItems];
	static constexpr int AmountStringMaxLen = 12;
	std::string m_cAmountString;
	int  m_logout_count;
	int m_cRestartCount;

	// Overlay system state
	OverlayType m_activeOverlay = OverlayType::None;
	char m_cOverlayContext;      // Which background screen (replaces m_cMsg[0] for overlay)
	char m_cOverlayMessage;      // Message code (replaces m_cMsg[1] for overlay)
	uint32_t m_dwOverlayStartTime;  // When overlay was shown

	char m_cMsg[200];
	std::string m_cLocation;
	std::string m_cCurLocation;
	std::string m_cMCName;
	std::string m_cMapName;
	std::string m_cMapMessage;
	char m_cMapIndex;
	char m_cCurFocus, m_cMaxFocus;
	char m_cArrowPressed;
	std::string m_cLogServerAddr;
	static constexpr int ChatMsgMaxLen = 64;
	std::string m_cChatMsg;
	std::string m_cBackupChatMsg;

	std::string m_cWorldServerName;
	char m_cMenuDir, m_cMenuDirCnt, m_cMenuFrame;
	char m_cIlusionOwnerType;
	std::string m_cName_IE;
	char m_cLoading;
	char m_cDiscount;

	std::string m_cStatusMapName;
	std::string m_cTopMsg;
	std::string m_cConstructMapName;
	std::string m_cGameServerName; //  Gateway

	std::array<std::unique_ptr<class CItem>, 5000> m_pItemConfigList;
	bool _bDecodeItemConfigFileContents(char* pData, uint32_t dwMsgSize);
	bool _bDecodeMagicConfigFileContents(char* pData, uint32_t dwMsgSize);
	bool _bDecodeSkillConfigFileContents(char* pData, uint32_t dwMsgSize);
	bool _bDecodeNpcConfigFileContents(char* pData, uint32_t dwMsgSize);

	struct NpcConfig { short npcType = 0; std::string name; bool valid = false; };
	std::array<NpcConfig, hb::shared::limits::MaxNpcConfigs> m_npcConfigList{};   // indexed by npc_id
	std::array<std::string, 120> m_cNpcNameByType;                    // type->name reverse map (last config wins)
	int m_iNpcConfigsReceived = 0;

	enum class ConfigRetryLevel : uint8_t { None = 0, CacheTried = 1, ServerRequested = 2, Failed = 3 };
	ConfigRetryLevel m_eConfigRetry[4]{};  // indexed by ConfigCacheType (Items=0, Magic=1, Skills=2, Npcs=3)
	uint32_t m_dwConfigRequestTime = 0;
	static constexpr uint32_t CONFIG_REQUEST_TIMEOUT_MS = 10000;

	bool m_bInitDataReady = false;      // RESPONSE_INITDATA received, waiting for configs
	bool m_bConfigsReady = false;       // All configs loaded, safe to enter game

	bool _EnsureConfigLoaded(int type);
	bool _TryReplayCacheForConfig(int type);
	void _RequestConfigsFromServer(bool bItems, bool bMagic, bool bSkills, bool bNpcs = false);
	void _CheckConfigsReadyAndEnterGame();

	bool EnsureItemConfigsLoaded()  { return _EnsureConfigLoaded(0); }
	bool EnsureMagicConfigsLoaded() { return _EnsureConfigLoaded(1); }
	bool EnsureSkillConfigsLoaded() { return _EnsureConfigLoaded(2); }
	bool EnsureNpcConfigsLoaded()   { return _EnsureConfigLoaded(3); }

	short m_sItemDropID[25];

	hb::shared::geometry::GameRectangle m_rcPlayerRect, m_rcBodyRect;


	bool m_bItemDrop;
	int  m_iItemDropCnt;

	std::string m_cGateMapName;
	int  m_iGatePositX, m_iGatePositY;
	int m_iHeldenianAresdenLeftTower;
	int m_iHeldenianElvineLeftTower;
	int m_iHeldenianAresdenFlags;
	int m_iHeldenianElvineFlags;
	bool m_bIllusionMVT;
	bool m_bIsXmas;
	bool m_bUsingSlate;

	// Entity render state (temporary state for currently rendered entity)
	CEntityRenderState m_entityState;

	// hb::shared::render::Renderer classes for entity drawing
	CPlayerRenderer m_playerRenderer;
	CNpcRenderer m_npcRenderer;


	int   m_iContributionPrice;


	short iMaxStats;
	int iMaxLevel;
	int iMaxBankItems;
};

