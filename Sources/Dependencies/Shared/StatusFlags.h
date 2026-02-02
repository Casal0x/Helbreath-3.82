// StatusFlags.h: Named constants for entity m_iStatus bit flags
//
// Shared between client and server. These replace the inline hex
// literals previously scattered across both codebases.
//////////////////////////////////////////////////////////////////////

#pragma once

namespace hb {
namespace status {

//------------------------------------------------------------------------
// Single-bit status effect flags
//------------------------------------------------------------------------
constexpr int Invisibility        = 0x00000010;
constexpr int Berserk             = 0x00000020;
constexpr int Frozen              = 0x00000040;
constexpr int Poisoned            = 0x00000080;

constexpr int AngelSTR            = 0x00001000;
constexpr int AngelDEX            = 0x00002000;
constexpr int AngelINT            = 0x00004000;
constexpr int AngelMAG            = 0x00008000;

constexpr int SlateExp            = 0x00010000;
constexpr int Hero                = 0x00020000;
constexpr int Haste               = 0x00040000;

constexpr int InhibitionCasting   = 0x00100000;
constexpr int IllusionMovement    = 0x00200000;
constexpr int SlateInvincible     = 0x00400000;
constexpr int SlateMana           = 0x00800000;

constexpr int Illusion            = 0x01000000;
constexpr int DefenseShield       = 0x02000000;
constexpr int MagicProtection     = 0x04000000;
constexpr int ProtectionFromArrow = 0x08000000;

constexpr int Hunter              = 0x10000000;
constexpr int Aresden             = 0x20000000;
constexpr int Citizen             = 0x40000000;
constexpr int PK                  = static_cast<int>(0x80000000);

//------------------------------------------------------------------------
// Multi-bit field masks
//------------------------------------------------------------------------
constexpr int AttackDelayMask     = 0x0000000F;  // bits 0-3
constexpr int AngelPercentMask    = 0x00000F00;  // bits 8-11
constexpr int AngelTypeMask       = 0x0000F000;  // bits 12-15

} // namespace status
} // namespace hb
