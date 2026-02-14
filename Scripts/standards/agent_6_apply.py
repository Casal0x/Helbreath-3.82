#!/usr/bin/env python3
"""Agent 6: CombatSystem + MagicCastingSystem + SpellAoE snake_case conversion.

Converts methods, struct names, struct fields, params, and locals to snake_case
coding standard across all client files.

Safety notes:
- All patterns use \b word boundaries to prevent partial matches
- SpellAoEParams struct fields use context-aware patterns to avoid
  renaming wire protocol struct fields (PacketMagicConfigEntry) which
  share the same field names (magicType, aoeRadiusX, etc.)
- File-specific sections handle locals that collide with other files
- .bak_* files are skipped (only process .h and .cpp)
"""
import glob, os, re

# =============================================================================
# SECTION 1: Global replacements — safe across all client files
# These identifiers are unique to CombatSystem/MagicCastingSystem/SpellAoE
# or are universally needed (SetGame across all managers).
# =============================================================================

GLOBAL_REPLACEMENTS = [
    # -------------------------------------------------------------------------
    # CombatSystem methods
    # -------------------------------------------------------------------------
    (r'\bSetPlayer\b', 'set_player', 'CombatSystem::SetPlayer -> set_player'),
    (r'\bGetAttackType\b', 'get_attack_type', 'CombatSystem::GetAttackType -> get_attack_type'),
    (r'\bGetWeaponSkillType\b', 'get_weapon_skill_type', 'CombatSystem::GetWeaponSkillType -> get_weapon_skill_type'),
    (r'\bCanSuperAttack\b', 'can_super_attack', 'CombatSystem::CanSuperAttack -> can_super_attack'),

    # -------------------------------------------------------------------------
    # MagicCastingSystem methods
    # -------------------------------------------------------------------------
    (r'\bSetGame\b', 'set_game', 'All managers: SetGame -> set_game'),
    (r'\bGetManaCost\b', 'get_mana_cost', 'MagicCastingSystem::GetManaCost -> get_mana_cost'),
    (r'\bBeginCast\b', 'begin_cast', 'MagicCastingSystem::BeginCast -> begin_cast'),

    # -------------------------------------------------------------------------
    # SpellAoE struct names
    # -------------------------------------------------------------------------
    (r'\bSpellAoETile\b', 'spell_aoe_tile', 'struct SpellAoETile -> spell_aoe_tile'),
    (r'\bSpellAoEParams\b', 'spell_aoe_params', 'struct SpellAoEParams -> spell_aoe_params'),

    # -------------------------------------------------------------------------
    # SpellAoE namespace function
    # -------------------------------------------------------------------------
    (r'\bCalculateTiles\b', 'calculate_tiles', 'SpellAoE::CalculateTiles -> calculate_tiles'),

    # -------------------------------------------------------------------------
    # SpellAoE static helper functions (only in SpellAoE.cpp)
    # -------------------------------------------------------------------------
    (r'\bAddUnique\b', 'add_unique', 'static AddUnique -> add_unique'),
    (r'\bAddArea\b', 'add_area', 'static AddArea -> add_area'),
    (r'\bGetMoveDir\b', 'get_move_dir', 'static GetMoveDir -> get_move_dir'),
    (r'\bGetWallOffset\b', 'get_wall_offset', 'static GetWallOffset -> get_wall_offset'),

    # -------------------------------------------------------------------------
    # SpellAoE params/locals — unique to SpellAoE + Screen_OnGame
    # -------------------------------------------------------------------------
    (r'\boutTiles\b', 'out_tiles', 'param outTiles -> out_tiles'),
    (r'\boutMax\b', 'out_max', 'param outMax -> out_max'),
    (r'\bcasterX\b', 'caster_x', 'param/local casterX -> caster_x'),
    (r'\bcasterY\b', 'caster_y', 'param/local casterY -> caster_y'),
    (r'\btargetX\b', 'target_x', 'param/local targetX -> target_x'),
    (r'\btargetY\b', 'target_y', 'param/local targetY -> target_y'),
    (r'\bmaxCount\b', 'max_count', 'param maxCount -> max_count'),
    (r'\bradiusX\b', 'radius_x', 'local radiusX -> radius_x'),
    (r'\bradiusY\b', 'radius_y', 'local radiusY -> radius_y'),
    (r'\bdynRadius\b', 'dyn_radius', 'local dynRadius -> dyn_radius'),
    (r'\btileCount\b', 'tile_count', 'local tileCount -> tile_count'),
    (r'\bcRet\b', 'result', 'local cRet -> result'),

    # -------------------------------------------------------------------------
    # SpellAoEParams struct field accesses via params. prefix
    # (avoids collision with wire protocol entry.magicType etc.)
    # -------------------------------------------------------------------------
    (r'\bparams\.magicType\b', 'params.magic_type', 'params.magicType -> params.magic_type'),
    (r'\bparams\.aoeRadiusX\b', 'params.aoe_radius_x', 'params.aoeRadiusX -> params.aoe_radius_x'),
    (r'\bparams\.aoeRadiusY\b', 'params.aoe_radius_y', 'params.aoeRadiusY -> params.aoe_radius_y'),
    (r'\bparams\.dynamicPattern\b', 'params.dynamic_pattern', 'params.dynamicPattern -> params.dynamic_pattern'),
    (r'\bparams\.dynamicRadius\b', 'params.dynamic_radius', 'params.dynamicRadius -> params.dynamic_radius'),

    # -------------------------------------------------------------------------
    # SpellAoEParams struct field DEFINITIONS (int fieldName pattern)
    # These only appear in SpellAoE.h within the struct definition
    # -------------------------------------------------------------------------
    (r'\bint magicType\b', 'int magic_type', 'struct field: int magicType -> int magic_type'),
    (r'\bint aoeRadiusX\b', 'int aoe_radius_x', 'struct field: int aoeRadiusX -> int aoe_radius_x'),
    (r'\bint aoeRadiusY\b', 'int aoe_radius_y', 'struct field: int aoeRadiusY -> int aoe_radius_y'),
    (r'\bint dynamicPattern\b', 'int dynamic_pattern', 'struct field: int dynamicPattern -> int dynamic_pattern'),
    (r'\bint dynamicRadius\b', 'int dynamic_radius', 'struct field: int dynamicRadius -> int dynamic_radius'),

    # -------------------------------------------------------------------------
    # CombatSystem local: wWeaponType (in CombatSystem.cpp and Game.cpp)
    # -------------------------------------------------------------------------
    (r'\bwWeaponType\b', 'weapon_type', 'local wWeaponType -> weapon_type'),

    # -------------------------------------------------------------------------
    # MagicCastingSystem param/locals — unique to MagicCastingSystem files
    # -------------------------------------------------------------------------
    (r'\biMagicNo\b', 'magic_number', 'param iMagicNo -> magic_number'),
    (r'\biManaSave\b', 'mana_save', 'local iManaSave -> mana_save'),
    (r'\biManaCost\b', 'mana_cost', 'local iManaCost -> mana_cost'),
]

# =============================================================================
# SECTION 2: File-specific replacements — for locals that collide with
# identifiers in unrelated files (wire protocol, other systems)
# =============================================================================

# MagicCastingSystem.cpp: rename dV1/dV2/dV3 and effectType
# (dV1/dV2/dV3 also exist in DialogBox_Commander/Constructor/Soldier/Map/Magic)
# (effectType also exists in Game.cpp wire access and ItemNameFormatter.cpp)
FILE_SPECIFIC = {
    'MagicCastingSystem.cpp': [
        (r'\bdV1\b', 'save_amount', 'local dV1 -> save_amount'),
        (r'\bdV2\b', 'save_factor', 'local dV2 -> save_factor'),
        (r'\bdV3\b', 'base_cost_f', 'local dV3 -> base_cost_f'),
        (r'\beffectType\b', 'effect_type', 'local effectType -> effect_type'),
    ],
    # SpellAoE.cpp: rename tX/tY and iErr which are too common for global rename
    # (tX/tY exist in Effect_Draw, DialogBox_Commander/Constructor/Soldier/Map etc.)
    # (iErr exists in Effect_Draw, WeatherManager)
    'SpellAoE.cpp': [
        (r'\btX\b', 'trace_x', 'local tX -> trace_x'),
        (r'\btY\b', 'trace_y', 'local tY -> trace_y'),
        (r'\biErr\b', 'error', 'local iErr -> error'),
    ],
}

# =============================================================================
# Process all client .h and .cpp files (skip .bak_* files)
# =============================================================================

client_dir = 'Z:/Helbreath-3.82/Sources/Client'
files = sorted(
    f for f in
    glob.glob(os.path.join(client_dir, '*.h')) +
    glob.glob(os.path.join(client_dir, '*.cpp'))
    if '.bak_' not in f
)

total_changes = 0

for f in files:
    with open(f, 'r', encoding='utf-8', errors='replace') as fh:
        content = fh.read()
    original = content
    basename = os.path.basename(f)

    # Apply global replacements
    for pattern, replacement, desc in GLOBAL_REPLACEMENTS:
        content = re.sub(pattern, replacement, content)

    # Apply file-specific replacements
    if basename in FILE_SPECIFIC:
        for pattern, replacement, desc in FILE_SPECIFIC[basename]:
            content = re.sub(pattern, replacement, content)

    if content != original:
        with open(f, 'w', encoding='utf-8', newline='') as fh:
            fh.write(content)
        total_changes += 1
        print(f'  Updated: {basename}')

total_replacements = len(GLOBAL_REPLACEMENTS) + sum(len(v) for v in FILE_SPECIFIC.values())
print(f'\nAgent 6 done. {total_changes} file(s) updated, {total_replacements} replacement rules applied.')
