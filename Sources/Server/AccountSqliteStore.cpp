#include "AccountSqliteStore.h"

#define _WINSOCKAPI_
#include <windows.h>
#include <direct.h>
#include <cstdio>

#include "Client.h"
#include "sqlite3.h"

extern void PutLogList(char* cMsg);

namespace
{
    void FormatTimestamp(const SYSTEMTIME& sysTime, char* outBuffer, size_t outBufferSize)
    {
        std::snprintf(outBuffer, outBufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
            sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    }

    bool ExecSql(sqlite3* db, const char* sql)
    {
        char* err = nullptr;
        int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            char logMsg[512] = {};
            std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Exec failed: %s", err ? err : "unknown");
            PutLogList(logMsg);
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    bool PrepareAndBindText(sqlite3_stmt** stmt, int idx, const char* value)
    {
        return sqlite3_bind_text(*stmt, idx, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
    }

    bool PrepareAndBindText(sqlite3_stmt* stmt, int idx, const char* value)
    {
        return sqlite3_bind_text(stmt, idx, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
    }
}

bool EnsureAccountDatabase(const char* accountName, sqlite3** outDb, std::string& outPath)
{
    if (outDb == nullptr || accountName == nullptr || accountName[0] == 0) {
        return false;
    }

    _mkdir("Accounts");

    char dbPath[MAX_PATH] = {};
    std::snprintf(dbPath, sizeof(dbPath), "Accounts\\%s.db", accountName);
    outPath = dbPath;

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath, &db);
    if (rc != SQLITE_OK) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Open failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_busy_timeout(db, 1000);
    if (!ExecSql(db, "PRAGMA foreign_keys = ON;")) {
        sqlite3_close(db);
        return false;
    }

    const char* schemaSql =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS meta ("
        " key TEXT PRIMARY KEY,"
        " value TEXT NOT NULL"
        ");"
        "INSERT OR REPLACE INTO meta(key, value) VALUES('schema_version','2');"
        "CREATE TABLE IF NOT EXISTS accounts ("
        " account_name TEXT PRIMARY KEY,"
        " password TEXT NOT NULL,"
        " email TEXT NOT NULL,"
        " quiz TEXT NOT NULL,"
        " answer TEXT NOT NULL,"
        " created_at TEXT NOT NULL,"
        " password_changed_at TEXT NOT NULL,"
        " last_ip TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS characters ("
        " character_name TEXT PRIMARY KEY,"
        " account_name TEXT NOT NULL,"
        " created_at TEXT NOT NULL,"
        " appr1 INTEGER NOT NULL,"
        " appr2 INTEGER NOT NULL,"
        " appr3 INTEGER NOT NULL,"
        " appr4 INTEGER NOT NULL,"
        " appr_color INTEGER NOT NULL,"
        " level INTEGER NOT NULL,"
        " exp INTEGER NOT NULL,"
        " map_name TEXT NOT NULL,"
        " map_x INTEGER NOT NULL,"
        " map_y INTEGER NOT NULL,"
        " hp INTEGER NOT NULL,"
        " mp INTEGER NOT NULL,"
        " sp INTEGER NOT NULL,"
        " str INTEGER NOT NULL,"
        " vit INTEGER NOT NULL,"
        " dex INTEGER NOT NULL,"
        " intl INTEGER NOT NULL,"
        " mag INTEGER NOT NULL,"
        " chr INTEGER NOT NULL,"
        " gender INTEGER NOT NULL,"
        " skin INTEGER NOT NULL,"
        " hairstyle INTEGER NOT NULL,"
        " haircolor INTEGER NOT NULL,"
        " underwear INTEGER NOT NULL,"
        " FOREIGN KEY(account_name) REFERENCES accounts(account_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_items ("
        " character_name TEXT NOT NULL,"
        " slot INTEGER NOT NULL,"
        " item_name TEXT NOT NULL,"
        " count INTEGER NOT NULL,"
        " touch_effect_type INTEGER NOT NULL,"
        " touch_effect_value1 INTEGER NOT NULL,"
        " touch_effect_value2 INTEGER NOT NULL,"
        " touch_effect_value3 INTEGER NOT NULL,"
        " item_color INTEGER NOT NULL,"
        " spec_effect_value1 INTEGER NOT NULL,"
        " spec_effect_value2 INTEGER NOT NULL,"
        " spec_effect_value3 INTEGER NOT NULL,"
        " cur_lifespan INTEGER NOT NULL,"
        " attribute INTEGER NOT NULL,"
        " pos_x INTEGER NOT NULL,"
        " pos_y INTEGER NOT NULL,"
        " is_equipped INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, slot),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_bank_items ("
        " character_name TEXT NOT NULL,"
        " slot INTEGER NOT NULL,"
        " item_name TEXT NOT NULL,"
        " count INTEGER NOT NULL,"
        " touch_effect_type INTEGER NOT NULL,"
        " touch_effect_value1 INTEGER NOT NULL,"
        " touch_effect_value2 INTEGER NOT NULL,"
        " touch_effect_value3 INTEGER NOT NULL,"
        " item_color INTEGER NOT NULL,"
        " spec_effect_value1 INTEGER NOT NULL,"
        " spec_effect_value2 INTEGER NOT NULL,"
        " spec_effect_value3 INTEGER NOT NULL,"
        " cur_lifespan INTEGER NOT NULL,"
        " attribute INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, slot),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_item_positions ("
        " character_name TEXT NOT NULL,"
        " slot INTEGER NOT NULL,"
        " pos_x INTEGER NOT NULL,"
        " pos_y INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, slot),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_item_equips ("
        " character_name TEXT NOT NULL,"
        " slot INTEGER NOT NULL,"
        " is_equipped INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, slot),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_magic_mastery ("
        " character_name TEXT NOT NULL,"
        " magic_index INTEGER NOT NULL,"
        " mastery_value INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, magic_index),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_skill_mastery ("
        " character_name TEXT NOT NULL,"
        " skill_index INTEGER NOT NULL,"
        " mastery_value INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, skill_index),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS character_skill_ssn ("
        " character_name TEXT NOT NULL,"
        " skill_index INTEGER NOT NULL,"
        " ssn_value INTEGER NOT NULL,"
        " PRIMARY KEY(character_name, skill_index),"
        " FOREIGN KEY(character_name) REFERENCES characters(character_name) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_characters_account ON characters(account_name);"
        "COMMIT;";

    if (!ExecSql(db, schemaSql)) {
        sqlite3_close(db);
        return false;
    }

    *outDb = db;
    return true;
}

bool InsertAccountRecord(sqlite3* db, const AccountDbAccountData& data)
{
    if (db == nullptr) {
        return false;
    }

    const char* sql =
        "INSERT INTO accounts("
        " account_name, password, email, quiz, answer, created_at, password_changed_at, last_ip"
        ") VALUES(?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Prepare account insert failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
        return false;
    }

    bool ok =
        PrepareAndBindText(&stmt, 1, data.name) &&
        PrepareAndBindText(&stmt, 2, data.password) &&
        PrepareAndBindText(&stmt, 3, data.email) &&
        PrepareAndBindText(&stmt, 4, data.quiz) &&
        PrepareAndBindText(&stmt, 5, data.answer) &&
        PrepareAndBindText(&stmt, 6, data.createdAt) &&
        PrepareAndBindText(&stmt, 7, data.passwordChangedAt) &&
        PrepareAndBindText(&stmt, 8, data.lastIp);

    if (ok) {
        ok = sqlite3_step(stmt) == SQLITE_DONE;
    }

    if (!ok) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Insert account failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool InsertCharacterRecord(sqlite3* db, const AccountDbCharacterData& data)
{
    if (db == nullptr) {
        return false;
    }

    const char* sql =
        "INSERT INTO characters("
        " character_name, account_name, created_at, appr1, appr2, appr3, appr4, appr_color,"
        " level, exp, map_name, map_x, map_y, hp, mp, sp, str, vit, dex, intl, mag, chr,"
        " gender, skin, hairstyle, haircolor, underwear"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Prepare character insert failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
        return false;
    }

    int idx = 1;
    bool ok = true;
    ok &= PrepareAndBindText(&stmt, idx++, data.characterName);
    ok &= PrepareAndBindText(&stmt, idx++, data.accountName);
    ok &= PrepareAndBindText(&stmt, idx++, data.createdAt);
    ok &= (sqlite3_bind_int(stmt, idx++, data.appr1) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.appr2) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.appr3) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.appr4) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, static_cast<int>(data.apprColor)) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.level) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, static_cast<int>(data.exp)) == SQLITE_OK);
    ok &= PrepareAndBindText(&stmt, idx++, data.mapName);
    ok &= (sqlite3_bind_int(stmt, idx++, data.mapX) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.mapY) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.hp) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.mp) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.sp) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.str) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.vit) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.dex) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.intl) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.mag) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.chr) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.gender) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.skin) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.hairStyle) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.hairColor) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, data.underwear) == SQLITE_OK);

    if (ok) {
        ok = sqlite3_step(stmt) == SQLITE_DONE;
    }

    if (!ok) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Insert character failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
    }

    sqlite3_finalize(stmt);
    return ok;
}

void CloseAccountDatabase(sqlite3* db)
{
    if (db != nullptr) {
        sqlite3_close(db);
    }
}

bool SaveCharacterSnapshot(sqlite3* db, const CClient* client)
{
    if (db == nullptr || client == nullptr) {
        return false;
    }

    if (!ExecSql(db, "BEGIN;")) {
        return false;
    }

    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    char timestamp[32] = {};
    FormatTimestamp(sysTime, timestamp, sizeof(timestamp));

    const char* upsertSql =
        "INSERT OR REPLACE INTO characters("
        " character_name, account_name, created_at, appr1, appr2, appr3, appr4, appr_color,"
        " level, exp, map_name, map_x, map_y, hp, mp, sp, str, vit, dex, intl, mag, chr,"
        " gender, skin, hairstyle, haircolor, underwear"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, upsertSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    int idx = 1;
    bool ok = true;
    ok &= PrepareAndBindText(stmt, idx++, client->m_cCharName);
    ok &= PrepareAndBindText(stmt, idx++, client->m_cAccountName);
    ok &= PrepareAndBindText(stmt, idx++, timestamp);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sAppr1) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sAppr2) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sAppr3) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sAppr4) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iApprColor) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iLevel) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, static_cast<int>(client->m_iExp)) == SQLITE_OK);
    ok &= PrepareAndBindText(stmt, idx++, client->m_cMapName);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sX) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_sY) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iHP) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iMP) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iSP) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iStr) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iVit) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iDex) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iInt) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iMag) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_iCharisma) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_cSex) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_cSkin) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_cHairStyle) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_cHairColor) == SQLITE_OK);
    ok &= (sqlite3_bind_int(stmt, idx++, client->m_cUnderwear) == SQLITE_OK);

    if (ok) {
        ok = sqlite3_step(stmt) == SQLITE_DONE;
    }
    sqlite3_finalize(stmt);
    if (!ok) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    const char* deleteItemsSql = "DELETE FROM character_items WHERE character_name = ?;";
    const char* deleteBankSql = "DELETE FROM character_bank_items WHERE character_name = ?;";
    const char* deletePosSql = "DELETE FROM character_item_positions WHERE character_name = ?;";
    const char* deleteEquipSql = "DELETE FROM character_item_equips WHERE character_name = ?;";
    const char* deleteMagicSql = "DELETE FROM character_magic_mastery WHERE character_name = ?;";
    const char* deleteSkillSql = "DELETE FROM character_skill_mastery WHERE character_name = ?;";
    const char* deleteSsnSql = "DELETE FROM character_skill_ssn WHERE character_name = ?;";

    const char* deleteStatements[] = {
        deleteItemsSql, deleteBankSql, deletePosSql, deleteEquipSql, deleteMagicSql, deleteSkillSql, deleteSsnSql
    };

    for (const char* deleteSql : deleteStatements) {
        if (sqlite3_prepare_v2(db, deleteSql, -1, &stmt, nullptr) != SQLITE_OK) {
            ExecSql(db, "ROLLBACK;");
            return false;
        }
        PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        if (!ok) {
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }

    const char* insertItemSql =
        "INSERT INTO character_items("
        " character_name, slot, item_name, count, touch_effect_type, touch_effect_value1, touch_effect_value2,"
        " touch_effect_value3, item_color, spec_effect_value1, spec_effect_value2, spec_effect_value3,"
        " cur_lifespan, attribute, pos_x, pos_y, is_equipped"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    if (sqlite3_prepare_v2(db, insertItemSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    for (int i = 0; i < DEF_MAXITEMS; i++) {
        if (client->m_pItemList[i] == nullptr) {
            continue;
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        ok = true;
        ok &= PrepareAndBindText(stmt, col++, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, client->m_pItemList[i]->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(client->m_pItemList[i]->m_dwCount)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sTouchEffectType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sTouchEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sTouchEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sTouchEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_cItemColor) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sItemSpecEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sItemSpecEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_sItemSpecEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemList[i]->m_wCurLifeSpan) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(client->m_pItemList[i]->m_dwAttribute)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_ItemPosList[i].x) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_ItemPosList[i].y) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_bIsItemEquipped[i] ? 1 : 0) == SQLITE_OK);

        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertBankSql =
        "INSERT INTO character_bank_items("
        " character_name, slot, item_name, count, touch_effect_type, touch_effect_value1, touch_effect_value2,"
        " touch_effect_value3, item_color, spec_effect_value1, spec_effect_value2, spec_effect_value3,"
        " cur_lifespan, attribute"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    if (sqlite3_prepare_v2(db, insertBankSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    for (int i = 0; i < DEF_MAXBANKITEMS; i++) {
        if (client->m_pItemInBankList[i] == nullptr) {
            continue;
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        ok = true;
        ok &= PrepareAndBindText(stmt, col++, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, client->m_pItemInBankList[i]->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(client->m_pItemInBankList[i]->m_dwCount)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sTouchEffectType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sTouchEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sTouchEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sTouchEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_cItemColor) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sItemSpecEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sItemSpecEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_sItemSpecEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, client->m_pItemInBankList[i]->m_wCurLifeSpan) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(client->m_pItemInBankList[i]->m_dwAttribute)) == SQLITE_OK);

        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertPosSql =
        "INSERT INTO character_item_positions(character_name, slot, pos_x, pos_y)"
        " VALUES(?,?,?,?);";
    if (sqlite3_prepare_v2(db, insertPosSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = true;
        ok &= PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, 2, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 3, client->m_ItemPosList[i].x) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 4, client->m_ItemPosList[i].y) == SQLITE_OK);
        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertEquipSql =
        "INSERT INTO character_item_equips(character_name, slot, is_equipped)"
        " VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, insertEquipSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    for (int i = 0; i < DEF_MAXITEMS; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = true;
        ok &= PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, 2, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 3, client->m_bIsItemEquipped[i] ? 1 : 0) == SQLITE_OK);
        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertMagicSql =
        "INSERT INTO character_magic_mastery(character_name, magic_index, mastery_value)"
        " VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, insertMagicSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    for (int i = 0; i < DEF_MAXMAGICTYPE; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = true;
        ok &= PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, 2, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 3, client->m_cMagicMastery[i]) == SQLITE_OK);
        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertSkillSql =
        "INSERT INTO character_skill_mastery(character_name, skill_index, mastery_value)"
        " VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, insertSkillSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    for (int i = 0; i < DEF_MAXSKILLTYPE; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = true;
        ok &= PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, 2, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 3, client->m_cSkillMastery[i]) == SQLITE_OK);
        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    const char* insertSsnSql =
        "INSERT INTO character_skill_ssn(character_name, skill_index, ssn_value)"
        " VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, insertSsnSql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    for (int i = 0; i < DEF_MAXSKILLTYPE; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = true;
        ok &= PrepareAndBindText(stmt, 1, client->m_cCharName);
        ok &= (sqlite3_bind_int(stmt, 2, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 3, client->m_iSkillSSN[i]) == SQLITE_OK);
        if (ok) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
        }
        if (!ok) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(stmt);

    if (!ExecSql(db, "COMMIT;")) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    return true;
}
