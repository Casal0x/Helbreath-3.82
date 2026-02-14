#!/usr/bin/env python3
"""Agent 1: WeatherManager + BuildItemManager + CBuildItem snake_case conversion.

Covers:
  - WeatherManager.h / WeatherManager.cpp
  - BuildItemManager.h / BuildItemManager.cpp
  - BuildItem.h

All replacements use word-boundary regex and skip #include lines for class renames.

SAFETY NOTES:
  - Initialize, Shutdown, Draw, Update, Get are EXCLUDED because their definitions
    live in SFMLEngine/Shared (IRenderer, ISprite, Application, etc.) which this
    script does not process. Renaming call sites without definitions would break
    the build.
  - m_cName is EXCLUDED because CItem::m_cName is defined in Shared/Item/Item.h
    (outside Client scope). A global rename would break CItem member accesses.
  - iIndex is EXCLUDED because QueuedMsg::iIndex is a struct member defined in
    Shared/Net/ConcurrentMsgQueue.h. A global rename would break struct accesses.
"""
import glob, os, re

# ---------------------------------------------------------------------------
# Replacements that must SKIP #include lines (class/struct name renames that
# appear in filenames). We process these line-by-line.
# ---------------------------------------------------------------------------
INCLUDE_SAFE_REPLACEMENTS = [
    # Class renames — skip #include directives to preserve filenames
    (r'\bWeatherManager\b', 'weather_manager', 'WeatherManager → weather_manager (class)'),
    (r'\bWeatherParticle\b', 'weather_particle', 'WeatherParticle → weather_particle (struct)'),
    (r'\bBuildItemManager\b', 'build_item_manager', 'BuildItemManager → build_item_manager (class)'),
    (r'\bCBuildItem\b', 'build_item', 'CBuildItem → build_item (class, remove C prefix)'),
]

# ---------------------------------------------------------------------------
# Replacements safe to apply globally (no filename collision risk).
# Only includes identifiers whose definitions are within Client/*.h / *.cpp,
# or which are purely local variables/parameters.
# ---------------------------------------------------------------------------
REPLACEMENTS = [
    # ===================================================================
    # WeatherManager methods (PascalCase → snake_case)
    # NOTE: Initialize, Shutdown, Draw, Update, Get are EXCLUDED — their
    # definitions live in SFMLEngine/Shared headers outside Client scope.
    # ===================================================================
    (r'\bSetWeather\b', 'set_weather', 'WeatherManager::SetWeather → set_weather'),
    (r'\bDrawThunderEffect\b', 'draw_thunder_effect', 'WeatherManager::DrawThunderEffect → draw_thunder_effect'),
    (r'\bResetParticles\b', 'reset_particles', 'WeatherManager::ResetParticles → reset_particles'),
    (r'\bSetDependencies\b', 'set_dependencies', 'WeatherManager::SetDependencies → set_dependencies'),
    (r'\bSetMapData\b', 'set_map_data', 'WeatherManager::SetMapData → set_map_data'),
    (r'\bSetXmas\b', 'set_xmas', 'WeatherManager::SetXmas → set_xmas'),
    (r'\bGetEffectType\b', 'get_effect_type', 'WeatherManager::GetEffectType → get_effect_type'),
    (r'\bGetWeatherStatus\b', 'get_weather_status', 'WeatherManager::GetWeatherStatus → get_weather_status'),
    (r'\bSetWeatherStatus\b', 'set_weather_status', 'WeatherManager::SetWeatherStatus → set_weather_status'),
    (r'\bIsActive\b', 'is_active', 'IsActive → is_active (WeatherManager + TextInputManager, both in Client)'),
    (r'\bIsRaining\b', 'is_raining', 'WeatherManager::IsRaining → is_raining'),
    (r'\bIsSnowing\b', 'is_snowing', 'WeatherManager::IsSnowing → is_snowing'),
    (r'\bSetAmbientLight\b', 'set_ambient_light', 'WeatherManager::SetAmbientLight → set_ambient_light'),
    (r'\bGetAmbientLight\b', 'get_ambient_light', 'WeatherManager::GetAmbientLight → get_ambient_light'),
    (r'\bIsNight\b', 'is_night', 'WeatherManager::IsNight → is_night'),

    # ===================================================================
    # BuildItemManager methods (PascalCase → snake_case)
    # NOTE: SetGame is shared across many managers, all defined in Client.
    # ===================================================================
    (r'\bSetGame\b', 'set_game', 'SetGame → set_game (method, shared across Client managers)'),
    (r'\bLoadRecipes\b', 'load_recipes', 'BuildItemManager::LoadRecipes → load_recipes'),
    (r'\bUpdateAvailableRecipes\b', 'update_available_recipes', 'BuildItemManager::UpdateAvailableRecipes → update_available_recipes'),
    (r'\bValidateCurrentRecipe\b', 'validate_current_recipe', 'BuildItemManager::ValidateCurrentRecipe → validate_current_recipe'),
    (r'\bGetDisplayList\b', 'get_display_list', 'BuildItemManager::GetDisplayList → get_display_list'),
    (r'\bParseRecipeFile\b', 'parse_recipe_file', 'BuildItemManager::ParseRecipeFile → parse_recipe_file'),

    # ===================================================================
    # WeatherParticle struct fields (plain struct, no m_ prefix)
    # sBX and cStep are unique to WeatherParticle.
    # sX and sY are shared across many Client structs (DialogBoxInfo, etc.)
    # but ALL definitions are in Client headers — safe to rename globally.
    # ===================================================================
    (r'\bsBX\b', 'base_x', 'WeatherParticle::sBX → base_x (unique to WeatherParticle)'),
    (r'\bcStep\b', 'step', 'WeatherParticle::cStep → step (unique to WeatherParticle)'),
    (r'\bsX\b', 'x', 'sX → x (struct field / param / local, remove Hungarian s prefix)'),
    (r'\bsY\b', 'y', 'sY → y (struct field / param / local, remove Hungarian s prefix)'),

    # ===================================================================
    # CBuildItem members (Hungarian → snake_case)
    # All defined in Client/BuildItem.h. Server has a separate CBuildItem.
    # NOTE: m_cName is EXCLUDED — CItem::m_cName defined in Shared/Item/Item.h.
    # ===================================================================
    (r'\bm_bBuildEnabled\b', 'm_build_enabled', 'CBuildItem::m_bBuildEnabled → m_build_enabled'),
    (r'\bm_iSkillLimit\b', 'm_skill_limit', 'CBuildItem::m_iSkillLimit → m_skill_limit'),
    (r'\bm_iMaxSkill\b', 'm_max_skill', 'CBuildItem::m_iMaxSkill → m_max_skill'),
    (r'\bm_iSprH\b', 'm_sprite_handle', 'CBuildItem::m_iSprH → m_sprite_handle'),
    (r'\bm_iSprFrame\b', 'm_sprite_frame', 'CBuildItem::m_iSprFrame → m_sprite_frame'),
    (r'\bm_cElementName1\b', 'm_element_name_1', 'CBuildItem::m_cElementName1 → m_element_name_1'),
    (r'\bm_cElementName2\b', 'm_element_name_2', 'CBuildItem::m_cElementName2 → m_element_name_2'),
    (r'\bm_cElementName3\b', 'm_element_name_3', 'CBuildItem::m_cElementName3 → m_element_name_3'),
    (r'\bm_cElementName4\b', 'm_element_name_4', 'CBuildItem::m_cElementName4 → m_element_name_4'),
    (r'\bm_cElementName5\b', 'm_element_name_5', 'CBuildItem::m_cElementName5 → m_element_name_5'),
    (r'\bm_cElementName6\b', 'm_element_name_6', 'CBuildItem::m_cElementName6 → m_element_name_6'),
    (r'\bm_iElementCount\b', 'm_element_count', 'CBuildItem::m_iElementCount → m_element_count'),
    (r'\bm_bElementFlag\b', 'm_element_flag', 'CBuildItem::m_bElementFlag → m_element_flag'),

    # ===================================================================
    # Parameter names (Hungarian → snake_case)
    # ===================================================================
    (r'\bpGame\b', 'game', 'parameter pGame → game (shared across Client managers)'),

    # ===================================================================
    # Local variables — unique or safely-scoped names
    # NOTE: iIndex is EXCLUDED — QueuedMsg::iIndex is a Shared struct member.
    # ===================================================================

    # WeatherManager.cpp locals (unique or safely local-only)
    (r'\bMaxSnowAccum\b', 'max_snow_accum', 'constexpr MaxSnowAccum → max_snow_accum (WeatherManager.cpp)'),
    (r'\bsCnt\b', 'count', 'local sCnt → count (WeatherManager.cpp only)'),
    (r'\bcTempFrame\b', 'temp_frame', 'local cTempFrame → temp_frame (WeatherManager + Effect files)'),
    (r'\bcAdd\b', 'offset', 'local cAdd → offset (WeatherManager.cpp only, Y-position offset)'),
    (r'\bpX1\b', 'prev_x1', 'local pX1 → prev_x1 (WeatherManager.cpp only)'),
    (r'\bpY1\b', 'prev_y1', 'local pY1 → prev_y1 (WeatherManager.cpp only)'),
    (r'\biY1\b', 'y1', 'local iY1 → y1 (WeatherManager.cpp only)'),
    (r'\bcDir\b', 'direction', 'local cDir → direction (Client files + Shared/DirectionHelpers param)'),

    # BuildItemManager.cpp locals (unique to this file)
    (r'\bcReadModeA\b', 'read_mode_a', 'local cReadModeA → read_mode_a (BuildItemManager.cpp only)'),
    (r'\bcReadModeB\b', 'read_mode_b', 'local cReadModeB → read_mode_b (BuildItemManager.cpp only)'),
    (r'\biMatch\b', 'match_count', 'local iMatch → match_count (BuildItemManager.cpp only)'),
    (r'\biCount2\b', 'count2', 'local iCount2 → count2 (BuildItemManager.cpp only)'),
    (r'\bcTempName\b', 'temp_name', 'local cTempName → temp_name (BuildItemManager.cpp only)'),
    (r'\bbItemFlag\b', 'item_flag', 'local bItemFlag → item_flag (BuildItemManager.cpp only)'),
    (r'\biItemCount\b', 'item_count', 'local iItemCount → item_count (BuildItemManager.cpp only)'),
    (r'\bpCfgJ\b', 'cfg_item', 'local pCfgJ → cfg_item (BuildItemManager.cpp only)'),
    (r'\bpCfgBI\b', 'cfg_build_item', 'local pCfgBI → cfg_build_item (BuildItemManager.cpp only)'),
    (r'\bcopyToCharArray\b', 'copy_to_char_array', 'lambda copyToCharArray → copy_to_char_array (BuildItemManager.cpp only)'),

    # Shared local variable names — verified no struct member collisions
    (r'\biCount\b', 'count', 'local iCount → count (verified: no struct member .iCount in codebase)'),
    (r'\biItemIndex\b', 'item_index', 'local iItemIndex → item_index (verified: no struct member collision)'),
    (r'\biErr\b', 'error', 'local iErr → error (verified: no struct member collision)'),
    (r'\biX1\b', 'x1', 'local/param iX1 → x1 (WeatherManager + IGameScreen + IDialogBox)'),
    (r'\biFrame\b', 'frame', 'local iFrame → frame (verified: no struct member .iFrame in codebase)'),
]


def apply_include_safe(content: str, replacements: list) -> str:
    """Apply replacements line-by-line, skipping #include lines."""
    lines = content.split('\n')
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith('#include'):
            continue
        # Also skip .vcxproj-style Include= lines (shouldn't appear in .h/.cpp but be safe)
        if 'Include="' in line:
            continue
        for pattern, replacement, desc in replacements:
            lines[i] = re.sub(pattern, replacement, lines[i])
    return '\n'.join(lines)


files = sorted(
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
)

total_changes = 0
for f in files:
    # Skip .bak files
    if '.bak_' in os.path.basename(f):
        continue

    with open(f, 'r', encoding='utf-8', errors='replace') as fh:
        content = fh.read()
    original = content

    # Phase 1: Include-safe replacements (class/struct renames, skip #include lines)
    content = apply_include_safe(content, INCLUDE_SAFE_REPLACEMENTS)

    # Phase 2: Global replacements (methods, members, params, locals)
    for pattern, replacement, desc in REPLACEMENTS:
        content = re.sub(pattern, replacement, content)

    if content != original:
        with open(f, 'w', encoding='utf-8', newline='') as fh:
            fh.write(content)
        total_changes += 1
        print(f'  Updated: {os.path.basename(f)}')

print(f'\nAgent 1 done. {total_changes} file(s) updated.')
print(f'  {len(INCLUDE_SAFE_REPLACEMENTS)} include-safe replacements (class/struct renames)')
print(f'  {len(REPLACEMENTS)} global replacements (methods, members, params, locals)')
print(f'  {len(INCLUDE_SAFE_REPLACEMENTS) + len(REPLACEMENTS)} total replacement rules')
