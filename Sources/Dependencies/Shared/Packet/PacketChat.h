#pragma once

#include "PacketCommon.h"
#include "PacketHeaders.h"
#include "NetConstants.h"

#include <cstdint>

namespace hb {
namespace net {
	HB_PACK_BEGIN

	// Chat message packet header (variable-length message follows)
	struct HB_PACKED PacketChatMsg {
		PacketHeader header;
		std::int16_t reserved1;
		std::int16_t reserved2;
		char name[hb::shared::limits::CharNameLen];
		std::uint8_t chat_type;
	};

	HB_PACK_END
}
}
