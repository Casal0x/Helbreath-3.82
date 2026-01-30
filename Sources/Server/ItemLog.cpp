#include "ItemLog.h"
#include "Item.h"
#include "Item/ItemEnums.h"
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ctime>

extern void PutLogList(char* cMsg);

static const char* const ITEMLOG_DIR = "GameLogs";

static const char* const LOG_MONSTER_DROPS = "GameLogs/monster_drops.log";
static const char* const LOG_PLAYER_TRADE = "GameLogs/player_trade.log";
static const char* const LOG_SHOP = "GameLogs/shop.log";
static const char* const LOG_CRAFTING = "GameLogs/crafting.log";
static const char* const LOG_UPGRADES = "GameLogs/upgrades.log";
static const char* const LOG_BANK = "GameLogs/bank.log";
static const char* const LOG_MISC = "GameLogs/misc.log";

static const char* const ALL_LOG_FILES[] = {
	LOG_MONSTER_DROPS, LOG_PLAYER_TRADE, LOG_SHOP,
	LOG_CRAFTING, LOG_UPGRADES, LOG_BANK, LOG_MISC
};

ItemLog& ItemLog::Get()
{
	static ItemLog instance;
	return instance;
}

void ItemLog::Initialize()
{
	if (m_bInitialized)
		return;

	std::filesystem::create_directories(ITEMLOG_DIR);

	for (const char* path : ALL_LOG_FILES)
	{
		std::ofstream ofs(path, std::ios::trunc);
		ofs.close();
	}

	PutLogList((char*)"(!) Item log initialized: GameLogs/ (split log files)");
	m_bInitialized = true;
}

void ItemLog::WriteToFile(const char* filename, const char* line)
{
	if (!m_bInitialized)
		return;

	std::ofstream ofs(filename, std::ios::app);
	if (!ofs.is_open())
		return;

	auto now = std::chrono::system_clock::now();
	std::time_t tt = std::chrono::system_clock::to_time_t(now);
	struct tm tmLocal;
#ifdef _WIN32
	localtime_s(&tmLocal, &tt);
#else
	localtime_r(&tt, &tmLocal);
#endif

	char timestamp[16];
	std::snprintf(timestamp, sizeof(timestamp), "[%02d:%02d:%02d] ",
		tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec);

	ofs << timestamp << line << '\n';
}

void ItemLog::FormatItemInfo(char* buf, size_t bufSize, CItem* pItem)
{
	if (pItem == nullptr)
	{
		std::snprintf(buf, bufSize, "(null)");
		return;
	}
	std::snprintf(buf, bufSize, "%s(count=%u attr=0x%08X touch=%d:%d:%d:%d)",
		pItem->m_cName,
		pItem->m_dwCount,
		pItem->m_dwAttribute,
		pItem->m_sTouchEffectType,
		pItem->m_sTouchEffectValue1,
		pItem->m_sTouchEffectValue2,
		pItem->m_sTouchEffectValue3);
}

bool ItemLog::IsItemSuspicious(CItem* pItem)
{
	if (pItem == nullptr)
		return false;

	// Gold (ID 90), arrows, and basic consumables are not suspicious
	if (pItem->m_sIDnum == 90)
		return false;

	// If the item has attributes but no server-assigned touch ID, it's suspicious
	if (pItem->m_dwAttribute != 0 && pItem->m_sTouchEffectType != DEF_ITET_ID)
		return true;

	// High-value equipment with no touch effect at all is suspicious
	if (pItem->m_sTouchEffectType == 0 && pItem->m_dwAttribute != 0)
		return true;

	return false;
}

void ItemLog::LogDrop(const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) Drop %s at %s(%d,%d)",
		playerName, ip, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_MONSTER_DROPS, line);
}

void ItemLog::LogPickup(const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) Get %s at %s(%d,%d)",
		playerName, ip, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_MONSTER_DROPS, line);
}

void ItemLog::LogTrade(const char* giverName, const char* giverIP, const char* receiverName, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	const char* suspicious = IsItemSuspicious(pItem) ? "[SUSPICIOUS] " : "";
	std::snprintf(line, sizeof(line), "%s%s IP(%s) Give %s at %s(%d,%d) -> %s",
		suspicious, giverName, giverIP, itemInfo, mapName, sX, sY, receiverName);
	WriteToFile(LOG_PLAYER_TRADE, line);
}

void ItemLog::LogExchange(const char* giverName, const char* giverIP, const char* receiverName, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	const char* suspicious = IsItemSuspicious(pItem) ? "[SUSPICIOUS] " : "";
	std::snprintf(line, sizeof(line), "%s%s IP(%s) Exchange %s at %s(%d,%d) -> %s",
		suspicious, giverName, giverIP, itemInfo, mapName, sX, sY, receiverName);
	WriteToFile(LOG_PLAYER_TRADE, line);
}

void ItemLog::LogShop(const char* action, const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) %s %s at %s(%d,%d)",
		playerName, ip, action, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_SHOP, line);
}

void ItemLog::LogCraft(const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) Make %s at %s(%d,%d)",
		playerName, ip, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_CRAFTING, line);
}

void ItemLog::LogUpgrade(bool success, const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) Upgrade %s %s at %s(%d,%d)",
		playerName, ip, success ? "Success" : "Fail", itemInfo, mapName, sX, sY);
	WriteToFile(LOG_UPGRADES, line);
}

void ItemLog::LogBank(const char* action, const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) %s %s at %s(%d,%d)",
		playerName, ip, action, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_BANK, line);
}

void ItemLog::LogMisc(const char* action, const char* playerName, const char* ip, const char* mapName, int sX, int sY, CItem* pItem)
{
	char itemInfo[256], line[512];
	FormatItemInfo(itemInfo, sizeof(itemInfo), pItem);
	std::snprintf(line, sizeof(line), "%s IP(%s) %s %s at %s(%d,%d)",
		playerName, ip, action, itemInfo, mapName, sX, sY);
	WriteToFile(LOG_MISC, line);
}
