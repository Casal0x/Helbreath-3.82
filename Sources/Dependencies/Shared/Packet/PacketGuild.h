#pragma once

#include "PacketCommon.h"

#include <cstdint>

namespace hb {
namespace net {
	HB_PACK_BEGIN

	// Guild creation request payload (passed to RequestCreateNewGuild)
	struct HB_PACKED GuildCreatePayload {
		char char_name[10];
		char guild_name[20];
		char location[10];
		std::uint32_t guild_guid;
	};

	// Guild disband request payload (passed to RequestDisbandGuild)
	struct HB_PACKED GuildDisbandPayload {
		char char_name[10];
		char guild_name[20];
	};

	HB_PACK_END
}
}
