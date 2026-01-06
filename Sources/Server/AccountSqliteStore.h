#pragma once

#include <string>

struct sqlite3;

struct AccountDbAccountData
{
    char name[11];
    char password[11];
    char email[52];
    char quiz[47];
    char answer[27];
    char createdAt[32];
    char passwordChangedAt[32];
    char lastIp[32];
};

struct AccountDbCharacterData
{
    char accountName[11];
    char characterName[11];
    char createdAt[32];
    short appr1;
    short appr2;
    short appr3;
    short appr4;
    uint32_t apprColor;
    int level;
    uint32_t exp;
    char mapName[11];
    int mapX;
    int mapY;
    int hp;
    int mp;
    int sp;
    int str;
    int vit;
    int dex;
    int intl;
    int mag;
    int chr;
    int gender;
    int skin;
    int hairStyle;
    int hairColor;
    int underwear;
};

class CClient;

bool EnsureAccountDatabase(const char* accountName, sqlite3** outDb, std::string& outPath);
bool InsertAccountRecord(sqlite3* db, const AccountDbAccountData& data);
bool InsertCharacterRecord(sqlite3* db, const AccountDbCharacterData& data);
bool SaveCharacterSnapshot(sqlite3* db, const CClient* client);
void CloseAccountDatabase(sqlite3* db);
