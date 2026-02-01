#pragma once

#include "PacketCommon.h"
#include "PacketHeaders.h"

#include <cstdint>

#define MSGID_RESPONSE_CONFIGCACHESTATUS	0x0FA314E2

namespace hb {
namespace net {
	HB_PACK_BEGIN
	struct HB_PACKED PacketRequestInitDataEx {
		PacketHeader header;
		char player[10];
		char account[10];
		char password[10];
		uint8_t is_observer;
		char server[20];
		uint8_t padding;
		// --- Cache extension ---
		uint32_t itemConfigHash;
		uint32_t magicConfigHash;
		uint32_t skillConfigHash;
	};

	struct HB_PACKED PacketResponseConfigCacheStatus {
		PacketHeader header;
		uint8_t itemCacheValid;
		uint8_t magicCacheValid;
		uint8_t skillCacheValid;
	};
	HB_PACK_END
}
}
