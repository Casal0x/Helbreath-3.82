#pragma once

#include "PacketCommon.h"
#include "PacketHeaders.h"
#include "NetConstants.h"

#include <cstdint>

#define MSGID_RESPONSE_CONFIGCACHESTATUS	0x0FA314E2

namespace hb {
namespace net {
	HB_PACK_BEGIN
	struct HB_PACKED PacketRequestInitDataEx {
		PacketHeader header;
		char player[hb::shared::limits::CharNameLen];
		char account[hb::shared::limits::AccountNameLen];
		char password[hb::shared::limits::AccountPassLen];
		uint8_t is_observer;
		char server[20];
		uint8_t padding;
		// --- Cache extension ---
		uint32_t itemConfigHash;
		uint32_t magicConfigHash;
		uint32_t skillConfigHash;
		uint32_t npcConfigHash;
	};

	struct HB_PACKED PacketResponseConfigCacheStatus {
		PacketHeader header;
		uint8_t itemCacheValid;
		uint8_t magicCacheValid;
		uint8_t skillCacheValid;
		uint8_t npcCacheValid;
	};

	struct HB_PACKED PacketNotifyConfigReload {
		PacketHeader header;
		uint8_t reloadItems;
		uint8_t reloadMagic;
		uint8_t reloadSkills;
		uint8_t reloadNpcs;
	};

	struct HB_PACKED PacketRequestConfigData {
		PacketHeader header;
		uint8_t requestItems;
		uint8_t requestMagic;
		uint8_t requestSkills;
		uint8_t requestNpcs;
	};
	HB_PACK_END
}
}
