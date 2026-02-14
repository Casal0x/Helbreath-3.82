#pragma once

#include "PacketCommon.h"
#include "NetConstants.h"

#include <cstdint>

namespace hb {
namespace net {
	HB_PACK_BEGIN

	// Guild creation request payload (passed to request_create_new_guild)
	struct HB_PACKED GuildCreatePayload {
		char char_name[hb::shared::limits::CharNameLen];
		char guild_name[hb::shared::limits::GuildNameLen];
		char location[hb::shared::limits::MapNameLen];
		std::uint32_t guild_guid;
	};

	// Guild disband request payload (passed to request_disband_guild)
	struct HB_PACKED GuildDisbandPayload {
		char char_name[hb::shared::limits::CharNameLen];
		char guild_name[hb::shared::limits::GuildNameLen];
	};

	HB_PACK_END
}
}
