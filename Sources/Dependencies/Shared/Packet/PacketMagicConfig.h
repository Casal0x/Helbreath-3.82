#pragma once

#include "PacketCommon.h"
#include "PacketHeaders.h"
#include <cstdint>

namespace hb {
namespace net {

HB_PACK_BEGIN
struct HB_PACKED PacketMagicConfigEntry
{
	int16_t  magicId;       // Magic ID (0-99)
	char     name[31];      // Spell display name (null-padded)
	int16_t  manaCost;      // Mana cost (m_sValue1)
	int16_t  intLimit;      // Intelligence requirement (m_sIntLimit)
	int32_t  goldCost;      // Gold cost, negative = not for sale (m_iGoldCost)
	int8_t   isVisible;     // Show in magic shop (1=yes, 0=no)
	int16_t  magicType;     // DEF_MAGICTYPE_* (m_sType)
	int16_t  aoeRadiusX;    // AoE X radius in tiles (m_sValue2)
	int16_t  aoeRadiusY;    // AoE Y radius in tiles (m_sValue3)
	int16_t  dynamicPattern; // Wall=1, Field=2 (m_sValue11), 0 if N/A
	int16_t  dynamicRadius;  // Wall length / field radius (m_sValue12)
};
HB_PACK_END

HB_PACK_BEGIN
struct HB_PACKED PacketMagicConfigHeader
{
	PacketHeader header;    // msg_id = MSGID_MAGICCONFIGURATIONCONTENTS
	uint16_t magicCount;    // Entries in this packet
	uint16_t totalMagics;   // Total across all packets
	uint16_t packetIndex;   // 0-based packet index
};
HB_PACK_END

} // namespace net
} // namespace hb
