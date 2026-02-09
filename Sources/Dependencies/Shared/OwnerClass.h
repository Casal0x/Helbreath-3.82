#pragma once

// Owner class constants - classifies tile/object owners as Player, NPC,
// or PlayerIndirect. Used in CTile::m_cOwnerClass, CDynamicObject::m_cOwnerType,
// and function parameters (cOwnerType, cAttackerType, cTargetType).

namespace hb::ownerclass {
	constexpr char Player = 1;
	constexpr char Npc = 2;
	constexpr char PlayerIndirect = 3;
}
