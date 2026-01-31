#pragma once

#include "PacketCommon.h"
#include "PacketHeaders.h"
#include "NetConstants.h"

#include <cstdint>

namespace hb {
namespace net {
	HB_PACK_BEGIN
	struct HB_PACKED LoginRequest
	{
		PacketHeader header;
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char world_name[30];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED CreateCharacterRequest
	{
		PacketHeader header;
		char character_name[DEF_ACCOUNT_NAME - 1];
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char world_name[30];
		std::uint8_t gender;
		std::uint8_t skin;
		std::uint8_t hairstyle;
		std::uint8_t haircolor;
		std::uint8_t underware;
		std::uint8_t str;
		std::uint8_t vit;
		std::uint8_t dex;
		std::uint8_t intl;
		std::uint8_t mag;
		std::uint8_t chr;
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED DeleteCharacterRequest
	{
		PacketHeader header;
		char character_name[DEF_ACCOUNT_NAME - 1];
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char world_name[30];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED ChangePasswordRequest
	{
		PacketHeader header;
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char new_password[DEF_ACCOUNT_PASS - 1];
		char new_password_confirm[DEF_ACCOUNT_PASS - 1];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED CreateAccountRequest
	{
		PacketHeader header;
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char email[DEF_ACCOUNT_EMAIL - 1];
		char quiz[DEF_ACCOUNT_QUIZ - 1];
		char answer[DEF_ACCOUNT_ANSWER - 1];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED CreateAccountRequestFull
	{
		PacketHeader header;
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		char email[DEF_ACCOUNT_EMAIL - 1];
		char gender[10];
		char age[10];
		char padding1[4];
		char padding2[2];
		char padding3[2];
		char country[17];
		char ssn[28];
		char quiz[45];
		char answer[20];
		char cmd_line[50];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED EnterGameRequest
	{
		PacketHeader header;
		char character_name[DEF_ACCOUNT_NAME - 1];
		char map_name[DEF_ACCOUNT_NAME - 1];
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		std::int32_t level;
		char world_name[DEF_ACCOUNT_NAME - 1];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	struct HB_PACKED EnterGameRequestFull
	{
		PacketHeader header;
		char character_name[DEF_ACCOUNT_NAME - 1];
		char map_name[DEF_ACCOUNT_NAME - 1];
		char account_name[DEF_ACCOUNT_NAME - 1];
		char password[DEF_ACCOUNT_PASS - 1];
		std::int32_t level;
		char world_name[30];
	};
	HB_PACK_END
}
}
