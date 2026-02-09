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

	HB_PACK_BEGIN
	// Enter game response: server IP + port + server name (sent to login client)
	struct HB_PACKED EnterGameResponseData {
		char server_ip[16];
		std::uint16_t server_port;
		char server_name[20];
	};
	HB_PACK_END

	HB_PACK_BEGIN
	// Test log / account verify payload (after PacketHeader)
	struct HB_PACKED TestLogPayload {
		char account_name[DEF_ACCOUNT_NAME - 1];
		char account_password[DEF_ACCOUNT_PASS - 1];
		std::int32_t level;
	};
	HB_PACK_END
}
}
