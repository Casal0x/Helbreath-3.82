#!/usr/bin/env python3
"""Agent 8: FloatingTextManager + EffectManager + AnimationState snake_case conversion.

Converts identifiers from:
  - FloatingTextManager.h/.cpp
  - FloatingText.h
  - FloatingTextTypes.h
  - EffectManager.h/.cpp
  - Effect.h
  - EffectType.h (struct/class names only, not enum values - already SCREAMING_SNAKE)
  - AnimationState.h/.cpp

All replacements use word boundaries to avoid partial matches.
AnimationState fields use m_ prefix (struct with methods).
AnimDef fields in Player.h use no m_ prefix (plain data struct).
"""
import glob, os, re

# =============================================================================
# Phase 1: File-specific replacements for AnimationState vs AnimDef conflict
# =============================================================================
# AnimationState is a struct WITH methods -> members get m_ prefix
# AnimDef (in Player.h) is a plain data struct -> members get NO m_ prefix
# Both share field names: sMaxFrame, sFrameTime, bLoop

ANIMSTATE_MEMBER_RENAMES = [
    # AnimationState struct members -> m_ prefixed snake_case
    (r'\bcAction\b', 'm_action', 'AnimationState::cAction -> m_action'),
    (r'\bcDir\b', 'm_dir', 'AnimationState::cDir -> m_dir'),
    (r'\bsMaxFrame\b', 'm_max_frame', 'AnimationState::sMaxFrame -> m_max_frame'),
    (r'\bsFrameTime\b', 'm_frame_time', 'AnimationState::sFrameTime -> m_frame_time'),
    (r'\bbLoop\b', 'm_loop', 'AnimationState::bLoop -> m_loop'),
    (r'\bcCurrentFrame\b', 'm_current_frame', 'AnimationState::cCurrentFrame -> m_current_frame'),
    (r'\bcPreviousFrame\b', 'm_previous_frame', 'AnimationState::cPreviousFrame -> m_previous_frame'),
    (r'\bdwLastFrameTime\b', 'm_last_frame_time', 'AnimationState::dwLastFrameTime -> m_last_frame_time'),
    (r'\bbFinished\b', 'm_finished', 'AnimationState::bFinished -> m_finished'),
]

ANIMDEF_MEMBER_RENAMES = [
    # AnimDef struct members -> plain snake_case (no m_)
    (r'\bsMaxFrame\b', 'max_frame', 'AnimDef::sMaxFrame -> max_frame'),
    (r'\bsFrameTime\b', 'frame_time', 'AnimDef::sFrameTime -> frame_time'),
    (r'\bbLoop\b', 'loop', 'AnimDef::bLoop -> loop'),
]

# Files where AnimationState member names appear as STRUCT FIELDS (use m_ prefix)
ANIMSTATE_FILES = {
    'AnimationState.h',
    'AnimationState.cpp',
}

# Files where AnimDef member names appear as STRUCT FIELDS (no m_ prefix)
ANIMDEF_FILES = {
    'Player.h',
}

# Dot-accessor patterns for fields accessed from other files.
#
# Strategy for SHARED field names (sMaxFrame, sFrameTime, bLoop):
#   AnimationState is ALWAYS accessed via m_animation.fieldName
#   AnimDef is accessed via def.fieldName, PlayerAnim::Stop.fieldName, etc.
#   So we use m_animation-specific patterns for AnimationState,
#   and general dot-patterns for everything else (AnimDef).
#
# For UNIQUE AnimationState field names (cAction, cDir, cCurrentFrame, etc.):
#   These only exist in AnimationState, so general dot-patterns are safe.

ACCESSOR_PATTERNS = [
    # --- AnimationState-specific: fields accessed via m_animation. ---
    # Shared field names: must scope to m_animation to avoid AnimDef collision
    (r'm_animation\.sMaxFrame\b', 'm_animation.m_max_frame', 'm_animation.sMaxFrame -> m_animation.m_max_frame'),
    (r'm_animation\.sFrameTime\b', 'm_animation.m_frame_time', 'm_animation.sFrameTime -> m_animation.m_frame_time'),
    (r'm_animation\.bLoop\b', 'm_animation.m_loop', 'm_animation.bLoop -> m_animation.m_loop'),

    # --- AnimationState-unique fields: safe as general dot patterns ---
    (r'\.cAction\b', '.m_action', '.cAction -> .m_action (AnimationState only)'),
    (r'\.cDir\b', '.m_dir', '.cDir -> .m_dir (AnimationState only)'),
    (r'\.cCurrentFrame\b', '.m_current_frame', '.cCurrentFrame -> .m_current_frame (AnimationState only)'),
    (r'\.cPreviousFrame\b', '.m_previous_frame', '.cPreviousFrame -> .m_previous_frame (AnimationState only)'),
    (r'\.dwLastFrameTime\b', '.m_last_frame_time', '.dwLastFrameTime -> .m_last_frame_time (AnimationState only)'),
    (r'\.bFinished\b', '.m_finished', '.bFinished -> .m_finished (AnimationState only)'),

    # --- AnimDef fields: general dot-patterns (everything NOT m_animation.) ---
    # These match def.sMaxFrame, PlayerAnim::Stop.sMaxFrame, etc.
    (r'\.sMaxFrame\b', '.max_frame', '.sMaxFrame -> .max_frame (AnimDef field access)'),
    (r'\.sFrameTime\b', '.frame_time', '.sFrameTime -> .frame_time (AnimDef field access)'),
    (r'\.bLoop\b', '.loop', '.bLoop -> .loop (AnimDef field access)'),
]

# =============================================================================
# Phase 2: Global replacements (safe for all files)
# =============================================================================
GLOBAL_REPLACEMENTS = [
    # =========================================================================
    # CFloatingTextManager methods
    # =========================================================================
    (r'\bAddChatText\b', 'add_chat_text', 'CFloatingTextManager::AddChatText -> add_chat_text'),
    (r'\bAddDamageText\b', 'add_damage_text', 'CFloatingTextManager::AddDamageText -> add_damage_text'),
    (r'\bAddNotifyText\b', 'add_notify_text', 'CFloatingTextManager::AddNotifyText -> add_notify_text'),
    (r'\bAddDamageFromValue\b', 'add_damage_from_value', 'CFloatingTextManager::AddDamageFromValue -> add_damage_from_value'),
    (r'\bRemoveByObjectID\b', 'remove_by_object_id', 'CFloatingTextManager::RemoveByObjectID -> remove_by_object_id'),
    (r'\bReleaseExpired\b', 'release_expired', 'CFloatingTextManager::ReleaseExpired -> release_expired'),
    (r'\bClearAll\b', 'clear_all', 'CFloatingTextManager::ClearAll / EffectManager::ClearAllEffects base'),
    (r'\bDrawAll\b', 'draw_all', 'CFloatingTextManager::DrawAll / DialogBoxManager::DrawAll -> draw_all'),
    (r'\bDrawSingle\b', 'draw_single', 'CFloatingTextManager::DrawSingle -> draw_single'),
    (r'\bUpdatePosition\b', 'update_position', 'CFloatingTextManager::UpdatePosition -> update_position'),
    (r'\bIsValid\b', 'is_valid', 'CFloatingTextManager::IsValid -> is_valid'),
    (r'\bIsOccupied\b', 'is_occupied', 'CFloatingTextManager::IsOccupied -> is_occupied'),
    (r'\bFindFreeSlot\b', 'find_free_slot', 'CFloatingTextManager::FindFreeSlot -> find_free_slot'),
    (r'\bBindToTile\b', 'bind_to_tile', 'CFloatingTextManager::BindToTile -> bind_to_tile'),
    (r'\bDrawMessage\b', 'draw_message', 'CFloatingTextManager::DrawMessage -> draw_message'),
    (r'\bMaxMessages\b', 'max_messages', 'CFloatingTextManager::MaxMessages -> max_messages'),

    # =========================================================================
    # CFloatingText methods and members
    # =========================================================================
    (r'\bGetParams\b', 'get_params', 'CFloatingText::GetParams -> get_params'),
    (r'\bUsesSpriteFont\b', 'uses_sprite_font', 'CFloatingText::UsesSpriteFont -> uses_sprite_font'),
    (r'\bm_eCategory\b', 'm_category', 'CFloatingText::m_eCategory -> m_category'),
    (r'\bm_eChatType\b', 'm_chat_type', 'CFloatingText::m_eChatType -> m_chat_type'),
    (r'\bm_eDamageType\b', 'm_damage_type', 'CFloatingText::m_eDamageType -> m_damage_type'),
    (r'\bm_eNotifyType\b', 'm_notify_type', 'CFloatingText::m_eNotifyType -> m_notify_type'),
    (r'\bm_szText\b', 'm_text', 'CFloatingText::m_szText -> m_text'),

    # =========================================================================
    # Shared member names (CFloatingText, CEffect, CMsg, etc.)
    # These exist in multiple classes but ALL need the same rename.
    # =========================================================================
    (r'\bm_iObjectID\b', 'm_object_id', 'm_iObjectID -> m_object_id (CFloatingText, CMsg)'),
    # NOTE: m_sX, m_sY, m_dwTime are in CFloatingText, CEffect, CMsg, Game, Screen_OnGame.
    # All need the same rename. CEffect's m_sX is int, CFloatingText's is short - rename is the same.
    (r'\bm_sX\b', 'm_x', 'm_sX -> m_x (CFloatingText, CEffect, CMsg)'),
    (r'\bm_sY\b', 'm_y', 'm_sY -> m_y (CFloatingText, CEffect, CMsg)'),
    (r'\bm_dwTime\b', 'm_time', 'm_dwTime -> m_time (CFloatingText, CEffect, CMsg, Game, Screen_OnGame)'),

    # =========================================================================
    # AnimParams struct fields (plain data struct, no m_ prefix)
    # =========================================================================
    (r'\bdwLifetimeMs\b', 'lifetime_ms', 'AnimParams::dwLifetimeMs -> lifetime_ms'),
    (r'\bdwShowDelayMs\b', 'show_delay_ms', 'AnimParams::dwShowDelayMs -> show_delay_ms'),
    (r'\biStartOffsetY\b', 'start_offset_y', 'AnimParams::iStartOffsetY -> start_offset_y'),
    (r'\biRisePixels\b', 'rise_pixels', 'AnimParams::iRisePixels -> rise_pixels'),
    (r'\biRiseDurationMs\b', 'rise_duration_ms', 'AnimParams::iRiseDurationMs -> rise_duration_ms'),
    (r'\biFontOffset\b', 'font_offset', 'AnimParams::iFontOffset -> font_offset'),
    (r'\bbUseSpriteFont\b', 'use_sprite_font', 'AnimParams::bUseSpriteFont -> use_sprite_font'),

    # =========================================================================
    # FloatingTextTypes.h enum values (PascalCase -> snake_case)
    # Use scoped patterns for safety with common words like Small/Medium/Large.
    # =========================================================================
    # ChatTextType values
    (r'\bPlayerChat\b', 'player_chat', 'ChatTextType::PlayerChat -> player_chat'),

    # DamageTextType values - use scoped patterns for common words
    (r'\bDamageTextType::Small\b', 'DamageTextType::small', 'DamageTextType::Small -> small'),
    (r'\bDamageTextType::Medium\b', 'DamageTextType::medium', 'DamageTextType::Medium -> medium'),
    (r'\bDamageTextType::Large\b', 'DamageTextType::large', 'DamageTextType::Large -> large'),
    # Enum definitions (match the bare value at start of line in the enum body)
    # Pattern: tab + PascalCase + comma (specific to enum definition)
    (r'(\t)Small,(\s+//\s+<12)', r'\1small,\2', 'DamageTextType::Small enum definition'),
    (r'(\t)Medium,(\s+//\s+12-39)', r'\1medium,\2', 'DamageTextType::Medium enum definition'),
    (r'(\t)Large,(\s+//\s+40\+)', r'\1large,\2', 'DamageTextType::Large enum definition'),
    # Also catch "// Small:" in comments within FloatingTextTypes.h parameter tables
    (r'(//\s+)Small:', r'\1small:', 'Comment: Small: -> small:'),
    (r'(//\s+)Medium:', r'\1medium:', 'Comment: Medium: -> medium:'),
    (r'(//\s+)Large:', r'\1large:', 'Comment: Large: -> large:'),

    # NotifyTextType values
    (r'\bSkillChange\b', 'skill_change', 'NotifyTextType::SkillChange -> skill_change'),
    (r'\bMagicCastName\b', 'magic_cast_name', 'NotifyTextType::MagicCastName -> magic_cast_name'),
    # LevelUp - scoped to NotifyTextType:: to avoid colliding with Notify::LevelUp protocol enum
    (r'\bNotifyTextType::LevelUp\b', 'NotifyTextType::level_up', 'NotifyTextType::LevelUp -> level_up'),
    # LevelUp in enum definition (bare, in FloatingTextTypes.h only)
    (r'(\t)LevelUp,(\s+//)', r'\1level_up,\2', 'NotifyTextType::LevelUp enum definition'),
    # LevelUp in comments within FloatingTextTypes.h
    (r'(//\s+)LevelUp:', r'\1level_up:', 'Comment: LevelUp: -> level_up:'),
    (r'\bEnemyKill\b', 'enemy_kill', 'NotifyTextType::EnemyKill -> enemy_kill'),

    # =========================================================================
    # EffectManager methods
    # =========================================================================
    (r'\bSetEffectSprites\b', 'set_effect_sprites', 'EffectManager::SetEffectSprites -> set_effect_sprites'),
    (r'\bAddEffect\b', 'add_effect', 'EffectManager::AddEffect -> add_effect'),
    (r'\bDrawEffects\b', 'draw_effects', 'EffectManager::DrawEffects -> draw_effects'),
    (r'\bDrawEffectLights\b', 'draw_effect_lights', 'EffectManager::DrawEffectLights -> draw_effect_lights'),
    (r'\bClearAllEffects\b', 'clear_all_effects', 'EffectManager::ClearAllEffects -> clear_all_effects'),
    (r'\bAddEffectImpl\b', 'add_effect_impl', 'EffectManager::AddEffectImpl -> add_effect_impl'),
    (r'\bUpdateEffectsImpl\b', 'update_effects_impl', 'EffectManager::UpdateEffectsImpl -> update_effects_impl'),
    (r'\bDrawEffectsImpl\b', 'draw_effects_impl', 'EffectManager::DrawEffectsImpl -> draw_effects_impl'),
    (r'\bDrawEffectLightsImpl\b', 'draw_effect_lights_impl', 'EffectManager::DrawEffectLightsImpl -> draw_effect_lights_impl'),

    # EffectManager members
    (r'\bm_pEffectList\b', 'm_effect_list', 'EffectManager::m_pEffectList -> m_effect_list'),
    (r'\bm_pEffectSpr\b', 'm_effect_sprites', 'EffectManager::m_pEffectSpr -> m_effect_sprites'),
    # m_pGame is used in 178 files by many manager classes. All need same rename.
    (r'\bm_pGame\b', 'm_game', 'm_pGame -> m_game (all managers)'),

    # =========================================================================
    # CEffect members (class with virtual destructor -> m_ prefix)
    # =========================================================================
    (r'\bm_sType\b', 'm_type', 'CEffect::m_sType -> m_type (also Magic::m_sType)'),
    (r'\bm_cFrame\b', 'm_frame', 'CEffect::m_cFrame -> m_frame'),
    (r'\bm_cMaxFrame\b', 'm_max_frame', 'CEffect::m_cMaxFrame -> m_max_frame'),
    (r'\bm_cDir\b', 'm_dir', 'CEffect::m_cDir -> m_dir'),
    (r'\bm_dwFrameTime\b', 'm_frame_time', 'CEffect::m_dwFrameTime -> m_frame_time (also MapData)'),
    (r'\bm_dX\b', 'm_dest_x', 'CEffect::m_dX -> m_dest_x'),
    (r'\bm_dY\b', 'm_dest_y', 'CEffect::m_dY -> m_dest_y'),
    (r'\bm_mX\b', 'm_move_x', 'CEffect::m_mX -> m_move_x'),
    (r'\bm_mY\b', 'm_move_y', 'CEffect::m_mY -> m_move_y'),
    (r'\bm_mX2\b', 'm_move_x2', 'CEffect::m_mX2 -> m_move_x2'),
    (r'\bm_mY2\b', 'm_move_y2', 'CEffect::m_mY2 -> m_move_y2'),
    (r'\bm_mX3\b', 'm_move_x3', 'CEffect::m_mX3 -> m_move_x3'),
    (r'\bm_mY3\b', 'm_move_y3', 'CEffect::m_mY3 -> m_move_y3'),
    (r'\bm_iErr\b', 'm_error', 'CEffect::m_iErr -> m_error'),
    (r'\bm_rX\b', 'm_render_x', 'CEffect::m_rX -> m_render_x'),
    (r'\bm_rY\b', 'm_render_y', 'CEffect::m_rY -> m_render_y'),
    (r'\bm_iV1\b', 'm_value1', 'CEffect::m_iV1 -> m_value1'),

    # =========================================================================
    # AnimationState methods
    # =========================================================================
    (r'\bSetAction\b', 'set_action', 'AnimationState::SetAction -> set_action'),
    (r'\bSetDirection\b', 'set_direction', 'AnimationState::SetDirection -> set_direction'),
    (r'\bIsFinished\b', 'is_finished', 'AnimationState::IsFinished -> is_finished'),
    (r'\bFrameChanged\b', 'frame_changed', 'AnimationState::FrameChanged -> frame_changed'),
    (r'\bJustChangedTo\b', 'just_changed_to', 'AnimationState::JustChangedTo -> just_changed_to'),

    # =========================================================================
    # Common method names used across many classes (all need same rename)
    # =========================================================================
    (r'\bReset\b', 'reset', 'Reset -> reset (AnimationState, Camera, EntityMotion, etc.)'),
    (r'\bUpdate\b', 'update', 'Update -> update (EffectManager, AnimationState, Camera, etc.)'),
    (r'\bClear\b(?!All)', 'clear', 'Clear -> clear (FloatingTextManager, CursorTarget, etc.)'),
    # NOTE: "Clear" not followed by "All" to avoid double-renaming "ClearAll" -> "clearAll"
    # ClearAll is handled separately above.

    # =========================================================================
    # Static/free function renames
    # =========================================================================
    (r'\bGetCharKind\b', 'get_char_kind', 'GetCharKind -> get_char_kind (FloatingTextManager, TextInputManager)'),
    (r'\bCharKind_HAN1\b', 'CHAR_KIND_HAN1', 'CharKind_HAN1 -> CHAR_KIND_HAN1 (constant)'),

    # =========================================================================
    # EffectManager parameter name: effectSpr (camelCase -> snake_case)
    # =========================================================================
    (r'\beffectSpr\b', 'effect_sprites', 'effectSpr param -> effect_sprites'),

    # =========================================================================
    # camelCase parameter/local names from AnimationState::SetAction
    # =========================================================================
    (r'\bmaxFrame\b', 'max_frame', 'param maxFrame -> max_frame'),
    (r'\bframeTime\b', 'frame_time', 'param frameTime -> frame_time'),
    (r'\bstartFrame\b', 'start_frame', 'param startFrame -> start_frame'),

    # =========================================================================
    # Contained parameter/local renames (specific to assigned files)
    # =========================================================================
    (r'\bcStartFrame\b', 'start_frame', 'EffectManager param cStartFrame -> start_frame'),
    (r'\bbLastHit\b', 'last_hit', 'FloatingTextManager param bLastHit -> last_hit'),
    (r'\bsDamage\b', 'damage', 'FloatingTextManager param sDamage -> damage'),

    # =========================================================================
    # cDir as a local variable / parameter name (NOT AnimationState member)
    # Applied AFTER file-specific AnimationState handling.
    # In AnimationState files, this was already renamed to m_dir.
    # In all other files, cDir is a local/param that should become dir.
    # =========================================================================
    (r'\bcDir\b', 'dir', 'local/param cDir -> dir'),
    # cAction as a local/param (rare outside AnimationState, but be safe)
    (r'\bcAction\b', 'action', 'local/param cAction -> action'),
]


# =============================================================================
# Script execution
# =============================================================================

def apply_replacements(content, replacements):
    """Apply a list of (pattern, replacement, desc) tuples to content."""
    for pattern, replacement, desc in replacements:
        content = re.sub(pattern, replacement, content)
    return content


def main():
    files = sorted(
        glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h')
        + glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
    )
    # Exclude .bak files
    files = [f for f in files if '.bak_' not in f]

    total_changes = 0

    for f in files:
        with open(f, 'r', encoding='utf-8', errors='replace') as fh:
            content = fh.read()
        original = content
        basename = os.path.basename(f)

        # =====================================================================
        # Phase 1: File-specific AnimationState / AnimDef member renames
        # =====================================================================
        if basename in ANIMSTATE_FILES:
            # AnimationState.h/cpp: rename fields to m_ prefixed versions
            content = apply_replacements(content, ANIMSTATE_MEMBER_RENAMES)
        elif basename in ANIMDEF_FILES:
            # Player.h: rename AnimDef fields to non-m_ versions
            content = apply_replacements(content, ANIMDEF_MEMBER_RENAMES)
        else:
            # All other files: rename dot-accessor patterns.
            # Order matters: m_animation-scoped patterns FIRST (for shared
            # field names), then general dot-patterns for AnimDef accesses.
            content = apply_replacements(content, ACCESSOR_PATTERNS)

        # =====================================================================
        # Phase 2: Global replacements (all files)
        # =====================================================================
        content = apply_replacements(content, GLOBAL_REPLACEMENTS)

        # =====================================================================
        # Write if changed
        # =====================================================================
        if content != original:
            with open(f, 'w', encoding='utf-8', newline='') as fh:
                fh.write(content)
            total_changes += 1
            print(f'  Updated: {basename}')

    total_patterns = (
        len(GLOBAL_REPLACEMENTS)
        + len(ANIMSTATE_MEMBER_RENAMES)
        + len(ANIMDEF_MEMBER_RENAMES)
        + len(ACCESSOR_PATTERNS)
    )
    print(f'\nAgent 8 done. {total_changes} file(s) updated, {total_patterns} replacement patterns defined.')


if __name__ == '__main__':
    main()
