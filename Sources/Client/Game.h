// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)
#define _WINSOCKAPI_

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mmsystem.h>
#include <winbase.h>

// Undefine Windows PlaySound macro (from mmsystem.h) to prevent name collision
// with CGame::PlaySound method. We use AudioManager for audio, not Windows API.
#ifdef PlaySound
#undef PlaySound
#endif
#include <memory>
#include <string.h>
#include <process.h>
#include <direct.h>
#include <tlhelp32.h>
#include <fstream>
#include <iostream>
#include <iosfwd>
#include <vector>
#include <sstream>
#include <array>

#include "GlobalDef.h"
#include "IRenderer.h"
#include "RendererFactory.h"
#include "ISprite.h"
#include "ISpriteFactory.h"
#include "SpriteCollection.h"
#include "ITextRenderer.h"
#include "IBitmapFont.h"
#include "BitmapFontFactory.h"
#include "IInput.h"
#include "XSocket.h"
#include "SpriteID.h"
#include "Misc.h"
#include "ChatMsg.h"
#include "Effect.h"
#include "MapData.h"
#include "ActionID.h"
#include "ActionID_Client.h"
#include "NetMessages.h"
#include "ClientMessages.h"
#include "MouseInterface.h"
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

//v2.18
#define DEF_BTNSZX				74
#define DEF_BTNSZY				20
#define DEF_LBTNPOSX			30
#define DEF_RBTNPOSX			154
#define DEF_BTNPOSY				292

#define DEF_INDEX4_MSGID		0
#define DEF_INDEX2_MSGTYPE		4

#define DEF_SOCKETBLOCKLIMIT	300

#define DEF_MAXSPRITES			25000	// 20000 // Snoopy: Adjusted!
#define DEF_MAXTILES			1000	// 800 // Snoopy: Adjusted!
#define DEF_MAXEFFECTSPR		300
#define DEF_MAXSOUNDEFFECTS		200		// 110   //Snoopy: Adjusted!
#define DEF_MAXCHATMSGS			500
#define DEF_MAXWHISPERMSG		5
#define DEF_MAXCHATSCROLLMSGS	80
#define DEF_MAXEFFECTS			300	//600 <- original
#define DEF_CHATTIMEOUT_A		4000
#define DEF_CHATTIMEOUT_B		500
#define DEF_CHATTIMEOUT_C		2000
#define DEF_MAXITEMS			50
#define DEF_MAXBANKITEMS		1000 // Hard cap - actual soft limit received from server
#define DEF_MAXGUILDSMAN		32
#define DEF_MAXMENUITEMS		140  //v2.15  120 ->140
#define DEF_TEXTDLGMAXLINES		300 //v2.18 3000->300

#define DEF_MAXMAGICTYPE		100
#define DEF_MAXSKILLTYPE		60
#define DEF_MAXWHETHEROBJECTS	600
#define DEF_MAXBUILDITEMS		100
#define DEF_MAXGAMEMSGS			300
#define DEF_MAXGUILDNAMES		100
#define DEF_MAXSELLLIST			12

#define WM_USER_GAMESOCKETEVENT	WM_USER + 2000
#define WM_USER_LOGSOCKETEVENT	WM_USER + 2001

#define DEF_SERVERTYPE_GAME			1
#define DEF_SERVERTYPE_LOG			2

#define DEF_CURSORSTATUS_NULL		0
#define DEF_CURSORSTATUS_PRESSED	1
#define DEF_CURSORSTATUS_SELECTED	2
#define DEF_CURSORSTATUS_DRAGGING	3

#define DEF_DOUBLECLICKTIME			300
#define DEF_DOUBLECLICKTOLERANCE	4
#define DEF_MAXPARTYMEMBERS			8
#define DEF_MAXCRUSADESTRUCTURES	300

// Overlay types for popup screens that render over base screens
enum class OverlayType {
	None = 0,
	Connecting,
	WaitingResponse,
	LogResMsg,
	QueryForceLogin,
	QueryDeleteCharacter
};

class CGame
{
public:
	// CLEROTH - AURAS
	void CheckActiveAura(short sX, short sY, uint32_t dwTime, short sOwnerType);
	void CheckActiveAura2(short sX, short sY, uint32_t dwTime, short sOwnerType);

	// Camera for viewport management
	CCamera m_Camera;

	// MJ Stats Change Related vars - Alastor
	char cStateChange1;
	char cStateChange2;
	char cStateChange3;

	struct {
		char cName[21], cDesc[11];
		int iCount;
		uint32_t dwType;
		uint32_t dwValue;
	} m_stShards[13][17];

	struct {
		char cName[21], cDesc[11];
		int iCount;
		uint32_t dwType;
		uint32_t dwValue;
	} m_stFragments[13][17];

	int m_iTeleportMapCount;
	void ResponseTeleportList(char * pData);
	void ResponseChargedTeleport(char * pData);

	void ItemEquipHandler(char cItemID);
	void ReleaseEquipHandler(char cEquipPos);

	void ReadSettings();
	void WriteSettings();

	int  iGetManaCost(int iMagicNo);
	void UseMagic(int iMagicNo);
	bool FindGuildName(char* pName, int* ipIndex);
	void bItemDrop_ExternalScreen(char cItemID, short msX, short msY);
	void CreateScreenShot();
	void CrusadeWarResult(int iWinnerSide);
	void CrusadeContributionResult(int iWarContribution);
	void CannotConstruct(int iCode);
	void DrawTopMsg();
	void SetTopMsg(char * pString, unsigned char iLastSec);
	void DrawObjectFOE(int ix, int iy, int iFrame);
	void GrandMagicResult(char * pMapName, int iV1, int iV2, int iV3, int iV4, int iHP1, int iHP2, int iHP3, int iHP4) ;
	void MeteorStrikeComing(int iCode);

	void DrawNewDialogBox(char cType, int sX, int sY, int iFrame, bool bIsNoColorKey = false, bool bIsTrans = false);
	void AddMapStatusInfo(char * pData, bool bIsLastData);
	void _RequestMapStatus(char * pMapName, int iMode);
	int  GetCharKind(char *str, int index);
	void ReceiveString(char * pString);
	void EndInputString();
	void ClearInputString();
	void ShowReceivedString(bool bIsHide = false);
	bool GetText(HWND hWnd,UINT msg,WPARAM wparam, LPARAM lparam);
	void DrawDialogBoxs(short msX, short msY, short msZ, char cLB);
	void FormatCommaNumber(uint32_t value, char* buffer, size_t bufSize);

	void ResponsePanningHandler(char * pData);
	void _CalcSocketClosed();
	void StartInputString(int sX, int sY, unsigned char iLen, char * pBuffer, bool bIsHide = false);
	void _SetIlusionEffect(int iOwnerH);
	int _iGetFOE(int iStatus);
	void NoticementHandler(char * pData);
	void GetItemName(short sItemId, uint32_t dwAttribute, char *pStr1, char *pStr2, char *pStr3);
	void GetItemName(class CItem * pItem, char * pStr1, char * pStr2, char * pStr3);
	short FindItemIdByName(const char* cItemName);
	void _LoadGameMsgTextContents();
	bool _bCheckCurrentBuildItemStatus();
	bool _bCheckBuildItemStatus();
	bool _bDecodeBuildItemContents();
	void GetNpcName(short sType, char * pName);

	int m_iAgreeView;
	void _LoadAgreementTextContents(char cType);

	void UseShortCut( int num );
	void DrawCursor();
	void RenderFrame();       // Clear backbuffer -> DrawScreen -> Flip (centralized)

	// Legacy screen/overlay functions removed - migrated to Screen/Overlay classes:
	// MakeSprite, MakeTileSpr, MakeEffectSpr, UpdateScreen_OnLoading, DrawScreen_OnLoadingProgress
	// Screen_Quit, Screen_Loading, Screen_CreateNewAccount, Screen_SelectCharacter,
	// Screen_CreateNewCharacter, Screen_MainMenu, Screen_Login, Screen_OnGame,
	// Overlay_WaitingResponse, Overlay_Connecting, Overlay_QueryForceLogin,
	// Overlay_QueryDeleteCharacter, Overlay_LogResMsg, Overlay_ChangePassword,
	// Overlay_VersionNotMatch, Overlay_ConnectionLost, Overlay_Msg, Overlay_WaitInitData

	void NpcTalkHandler(char * pData);
	int  _iGetWeaponSkillType();
	void SetCameraShakingEffect(short sDist, int iMul = 0);
	void ClearSkillUsingStatus();
	bool bCheckItemOperationEnabled(char cItemID);
	void _DrawThunderEffect(int sX, int sY, int dX, int dY, int rX, int rY, char cType);
	void SetWhetherStatus(bool bStart, char cType);
	void WhetherObjectFrameCounter();
	void DrawWhetherEffects();
	bool bCheckExID(char * pName);
	bool bCheckLocalChatCommand(char * pMsg);
	char GetOfficialMapName(char * pMapName, char * pName);
	uint32_t iGetLevelExp(int iLevel);
	int _iCalcTotalWeight();
	void DrawVersion();
	bool _bIsItemOnHand();
	void DynamicObjectHandler(char * pData);
	bool _bCheckItemByType(char cType);
	void DrawNpcName(   short sX, short sY, short sOwnerType, int iStatus);
	void DrawObjectName(short sX, short sY, char * pName, int iStatus);
	void PlaySound(char cType, int iNum, int iDist, long lPan = 0);  // Forwards to AudioManager
	void _RemoveChatMsgListByObjectID(int iObjectID);
	void _LoadTextDlgContents(int cType);
	int  _iLoadTextDlgContents2(int iType);
	void DrawChatMsgs(short sX, short sY, short dX, short dY);
	void RequestFullObjectData(uint16_t wObjectID);
	bool bInitSkillCfgList();
	bool bCheckImportantFile();
	void EraseItem(char cItemID);
	void RetrieveItemHandler(char * pData);
	void CivilRightAdmissionHandler(char * pData);
	void _Draw_CharacterBody(short sX, short sY, short sType);
	bool _bGetIsStringIsNumber(char * pStr);
	bool bInitMagicCfgList();
	void _LoadShopMenuContents(char cType);
	void _RequestShopContents(int16_t npcType);
	void ResponseShopContentsHandler(char* pData);
	void PutChatScrollList(char * pMsg, char cType);
	void RequestTeleportAndWaitData();
	void PointCommandHandler(int indexX, int indexY, char cItemID = -1);
	void AddEventList(char * pTxt, char cColor = 0, bool bDupAllow = true);
	void ShowEventList(uint32_t dwTime);
	void _ShiftGuildOperationList();
	void _PutGuildOperationList(char * pName, char cOpMode);
	void DisbandGuildResponseHandler(char * pData);
	void InitPlayerCharacteristics(char * pData);
	void CreateNewGuildResponseHandler(char * pData);
	void _GetHairColorRGB(int iColorType , int * pR, int * pG, int * pB);
	void InitGameSettings();
	void CommonEventHandler(char * pData);
	void _SetItemOrder(char cWhere, char cItemID);
	int iGetTopDialogBoxIndex();
	void DisableDialogBox(int iBoxID);
	void EnableDialogBox(int iBoxID, int cType, int sV1, int sV2, char * pString = 0);
	void InitItemList(char * pData);
	SpriteLib::BoundRect __fastcall DrawObject_OnDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnMove_ForMenu(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision);
	SpriteLib::BoundRect __fastcall DrawObject_OnDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision);
	SpriteLib::BoundRect __fastcall DrawObject_OnRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime, bool frame_omision);
	SpriteLib::BoundRect __fastcall DrawObject_OnDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect __fastcall DrawObject_OnGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	void ClearGuildNameList();
	void DrawBackground(short sDivX, short sModX, short sDivY, short sModY);
	void DrawChatMsgBox(short sX, short sY, int iChatIndex, bool bIsPreDC);
	void ReleaseTimeoverChatMsg();
	void ChatMsgHandler(char * pData);
	void ReleaseUnusedSprites();
	void OnKeyUp(int key);
	void ChangeGameMode(GameMode mode);
	void LogRecvMsgHandler(char * pData);
	void LogResponseHandler(char * pData);
	void OnLogSocketEvent();  // MODERNIZED: Polls socket instead of handling window messages
	void OnTimer();
	void LogEventHandler(char * pData);
	void _ReadMapData(short sPivotX, short sPivotY, const char* pData);
	void MotionEventHandler(char * pData);
	void InitDataResponseHandler(char * pData);
	void InitPlayerResponseHandler(char * pData);
	void ConnectionEstablishHandler(char cWhere);
	void MotionResponseHandler(char * pData);
	void GameRecvMsgHandler(uint32_t dwMsgSize, char * pData);
	void DrawObjects(short sPivotX, short sPivotY, short sDivX, short sDivY, short sModX, short sModY, short msX, short msY);
	bool bSendCommand(uint32_t dwMsgID, uint16_t wCommand, char cDir, int iV1, int iV2, int iV3, char * pString, int iV4 = 0); // v1.4
	void RestoreSprites();
	void CommandProcessor(short msX, short msY, short indexX, short indexY, char cLB, char cRB);
	void OnGameSocketEvent();  // MODERNIZED: Polls socket instead of handling window messages
	void OnKeyDown(int key);
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
	void Quit();
	bool bInit();

	void ReserveFightzoneResponseHandler(char * pData);
	int _iGetAttackType();
	bool __bDecodeBuildItemContents(char * pBuffer);
	int _iGetBankItemCount();
	int _iGetTotalItemNum();
	void StartBGM();  // Forwards to AudioManager based on current location

	//Snoopy: added function:
	int bHasHeroSet( short Appr3, short Appr4, char OwnerType);
	void ShowHeldenianVictory(short sSide);
	void ResponseHeldenianTeleportList(char *pData);
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
	} m_stRepairAll[DEF_MAXITEMS];

	bool _ItemDropHistory(char * ItemName);
	CGame();
	virtual ~CGame();

	DialogBoxManager m_dialogBoxManager;
//Snoopy=>>>>>>>>>>>>>>>>>>>>>
	struct {
		int   sV1, sV2, sV3, sV4, sV5, sV6, sV7, sItemID;
		uint32_t dwV1;
		char  cStr1[32], cStr2[32];
	} m_stDialogBoxExchangeInfo[8];
//Snoopy end<<<<<<<<<<<<<<<<<<
	struct {
		int iIndex;
		int iAmount;
	} m_stSellItemList[DEF_MAXSELLLIST];

	struct {
		char cName[22];
		char cOpMode;
	} m_stGuildOpList[100];

	struct {
		uint32_t dwTime;
		char  cColor;
		char  cTxt[96];
	} m_stEventHistory[6];

	struct {
		uint32_t dwTime;
		char  cColor;
		char  cTxt[96];
	} m_stEventHistory2[6];

	struct {
		short sX, sY, sBX;
		char cStep;
	} m_stWhetherObject[DEF_MAXWHETHEROBJECTS];

	struct {
		bool bIsQuestCompleted;
		short sWho, sQuestType, sContribution, sTargetType, sTargetCount, sX, sY, sRange;
		short sCurrentCount; // by Snoopy
		char cTargetName[22];
	} m_stQuest;

	struct {
		char cStatus;
		char cName[12];
	} m_stPartyMember[DEF_MAXPARTYMEMBERS];

	struct {
		short sX, sY;
		char cType;
		char cSide;
	} m_stCrusadeStructureInfo[DEF_MAXCRUSADESTRUCTURES];

	struct {
		char cName[12];
	} m_stPartyMemberNameList[DEF_MAXPARTYMEMBERS+1];

	// v2.171 2002-6-14
	struct {
		uint32_t dwRefTime;
		int iGuildRank;
		char cCharName[12];
		char cGuildName[24];
	} m_stGuildName[DEF_MAXGUILDNAMES];

	struct {
		int iIndex;
		char mapname[12];
		int iX;
		int iY;
		int iCost;
	} m_stTeleportList[20];

	class IRenderer* m_Renderer;  // Abstract renderer interface
	std::unique_ptr<SpriteLib::ISpriteFactory> m_pSpriteFactory;  // Sprite factory for creating sprites
	SpriteLib::SpriteCollection m_pSprite;
	SpriteLib::SpriteCollection m_pTileSpr;
	SpriteLib::SpriteCollection m_pEffectSpr;

	std::unique_ptr<CPlayer> m_pPlayer;  // Main player data
	std::unique_ptr<class CMapData> m_pMapData;
	std::unique_ptr<class XSocket> m_pGSock;
	std::unique_ptr<class XSocket> m_pLSock;
	std::array<std::unique_ptr<class CMsg>, DEF_MAXCHATMSGS> m_pChatMsgList;
	std::array<std::unique_ptr<class CMsg>, DEF_MAXCHATSCROLLMSGS> m_pChatScrollList;
	std::array<std::unique_ptr<class CMsg>, DEF_MAXWHISPERMSG> m_pWhisperMsg;
	std::unique_ptr<EffectManager> m_pEffectManager;
	std::unique_ptr<NetworkMessageManager> m_pNetworkMessageManager;
	std::array<std::unique_ptr<class CItem>, DEF_MAXITEMS> m_pItemList;
	std::array<std::unique_ptr<class CItem>, DEF_MAXBANKITEMS> m_pBankList;
	std::array<std::unique_ptr<class CMagic>, DEF_MAXMAGICTYPE> m_pMagicCfgList;
	std::array<std::unique_ptr<class CSkill>, DEF_MAXSKILLTYPE> m_pSkillCfgList;
	std::array<std::unique_ptr<class CMsg>, DEF_TEXTDLGMAXLINES> m_pMsgTextList;
	std::array<std::unique_ptr<class CMsg>, DEF_TEXTDLGMAXLINES> m_pMsgTextList2;
	std::array<std::unique_ptr<class CMsg>, DEF_TEXTDLGMAXLINES> m_pAgreeMsgTextList;
	std::unique_ptr<class CMsg> m_pExID;
	std::array<std::unique_ptr<class CBuildItem>, DEF_MAXBUILDITEMS> m_pBuildItemList;
	std::array<std::unique_ptr<class CBuildItem>, DEF_MAXBUILDITEMS> m_pDispBuildItemList;

	std::array<std::unique_ptr<class CItem>, DEF_MAXMENUITEMS> m_pItemForSaleList;
	int16_t m_sPendingShopType;  // Shop type awaiting response from server (0 = none)
	std::array<std::unique_ptr<class CCharInfo>, 4> m_pCharList;
	std::array<std::unique_ptr<class CMsg>, DEF_MAXGAMEMSGS> m_pGameMsgList;

	char * m_pInputBuffer;

	uint32_t m_dwConnectMode;
	uint32_t m_dwTime;
	uint32_t m_dwCurTime;
	uint32_t m_dwCheckConnTime, m_dwCheckSprTime, m_dwCheckChatTime;
	uint32_t m_dwCheckConnectionTime;
	int  m_dwLogOutCountTime;//was DWORD
	uint32_t m_dwRestartCountTime;
	uint32_t m_dwWOFtime; //v1.4
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

	bool m_bIsProgramActive;
	std::array<bool, DEF_MAXITEMS> m_bIsItemEquipped{};
	std::array<bool, DEF_MAXITEMS> m_bIsItemDisabled{};
	bool m_bIsGetPointingMode;
	bool m_bSkillUsingStatus;
	bool m_bItemUsingStatus;
	bool m_bIsWhetherEffect;
	bool m_bIsObserverMode, m_bIsObserverCommanded;
		bool m_bIsFirstConn;
	bool m_bDrawFlagDir;
	bool m_bIsCrusadeMode;
		bool m_bInputStatus;
	bool m_bIsSpecial;

	bool m_bIsF1HelpWindowEnabled;
	bool m_bIsTeleportRequested;
	bool m_bHideLocalCursor;

	bool m_bForceDisconn;

	short m_sFrameCount;
	short m_sFPS;
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
//	int m_iAccountStatus;
	short m_sMagicShortCut;
		int m_iAccntYear, m_iAccntMonth, m_iAccntDay;
	int m_iIpYear, m_iIpMonth, m_iIpDay;
	int m_iDownSkillIndex;

	int m_iIlusionOwnerH;
		int m_iInputX, m_iInputY;
	short m_sRecentShortCut;
	std::array<short, 6> m_sShortCut{}; // Snoopy: 6 shortcuts
		int m_iDrawFlag;

		int m_iTimeLeftSecAccount, m_iTimeLeftSecIP;
		int m_iLogServerPort, m_iGameServerPort;
	int m_iRating; //Rating

	int m_iBlockYear, m_iBlockMonth, m_iBlockDay;
	unsigned char m_iTopMsgLastSec;
				int m_iNetLagCount;
	int m_iTeleportLocX, m_iTeleportLocY;
	int m_iTotalPartyMember;
	int m_iPartyStatus;
	int m_iGizonItemUpgradeLeft;
	//int m_iFeedBackCardIndex; // removed by Snoopy

	std::array<short, DEF_MAXITEMEQUIPPOS> m_sItemEquipmentStatus{};
	short m_sMCX, m_sMCY;
	int   m_iCastingMagicType;
	short m_sVDL_X, m_sVDL_Y;

	uint16_t m_wCommObjectID;
	uint16_t m_wLastAttackTargetID;
	uint16_t m_wEnterGameType;
	// Color arrays for sprite tinting (RGB888 format, 0-255 range)
	std::array<int16_t, 16> m_wR{}, m_wG{}, m_wB{};
	std::array<int16_t, 16> m_wWR{}, m_wWG{}, m_wWB{};

	unsigned char m_cInputMaxLen;
	char m_cEdit[4];
	char G_cTxt[128];
	char m_cBGMmapName[12];
	char m_cItemOrder[DEF_MAXITEMS];
	char m_cAmountString[12];
	char m_cLogOutCount;
	char m_cRestartCount;

	// Overlay system state
	OverlayType m_activeOverlay = OverlayType::None;
	char m_cOverlayContext;      // Which background screen (replaces m_cMsg[0] for overlay)
	char m_cOverlayMessage;      // Message code (replaces m_cMsg[1] for overlay)
	uint32_t m_dwOverlayStartTime;  // When overlay was shown

	char m_cWhisperIndex;
			char m_cAccountAge[12];
	char m_cNewPassword[12];
	char m_cNewPassConfirm[12];
	char m_cEmailAddr[52];
	char m_cAccountQuiz[46];// Quiz
	char m_cAccountAnswer[22];
			char m_cMsg[200];
	char m_cLocation[12];
	char m_cCurLocation[12];
		char m_cMCName[12];
	char m_cMapName[12];
	char m_cMapMessage[32];
	char m_cMapIndex;
	char m_cCurFocus, m_cMaxFocus;
	char m_cArrowPressed;
	char m_cLogServerAddr[16];
	char m_cChatMsg[64];
	char m_cBackupChatMsg[64];

			char m_cWorldServerName[32];
	char m_cMenuDir, m_cMenuDirCnt, m_cMenuFrame;
	char m_cWhetherEffectType;
	char m_cWhetherStatus;
	char m_cIlusionOwnerType;
	char m_cName_IE[12];
	char m_cLoading;
	char m_cDiscount;

	char m_cStatusMapName[12];
	char m_cTopMsg[64];
	char m_cTeleportMapName[12];
	char m_cConstructMapName[12];
	char m_cGameServerName[22]; //  Gateway

	std::array<std::unique_ptr<class CItem>, 5000> m_pItemConfigList;
	bool _bDecodeItemConfigFileContents(char* pData, uint32_t dwMsgSize);

	int iNpcHP, iNpcMaxHP;

	char m_cItemDrop[25][25];

	RECT m_rcPlayerRect, m_rcBodyRect;

	bool m_bWhisper;
	bool m_bShout;

	bool m_bItemDrop;
	int  m_iItemDropCnt;

	// Snoopy: Apocalypse Gate
	char m_cGateMapName[12];
	int  m_iGatePositX, m_iGatePositY;
	int m_iHeldenianAresdenLeftTower;
	int m_iHeldenianElvineLeftTower;
	int m_iHeldenianAresdenFlags;
	int m_iHeldenianElvineFlags;
	bool m_bIllusionMVT;
	int m_iGameServerMode;
	bool m_bIsXmas;
	bool m_bUsingSlate;

	// Entity render state (temporary state for currently rendered entity)
	CEntityRenderState m_entityState;


	//Snoopy: Crafting
	//bool _bDecodeCraftItemContents();
	//bool __bDecodeCraftItemContents(char *pBuffer);
	//bool _bCheckCraftItemStatus();
	//bool _bCheckCurrentCraftItemStatus();

	int   m_iContributionPrice;

	char m_cTakeHeroItemName[100]; //Drajwer - hero item str

	short iMaxStats;
	int iMaxLevel;
	int iMaxBankItems;
};

