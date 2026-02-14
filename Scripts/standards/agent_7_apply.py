#!/usr/bin/env python3
"""Agent 7: AudioManager + ConfigManager snake_case conversion.

Converts all PascalCase/camelCase/Hungarian identifiers in AudioManager and
ConfigManager (and their consumers across all client files) to snake_case per
the project coding standards.

Skips: wire protocol structs, #include paths, already-compliant identifiers.
"""
import glob, os, re

REPLACEMENTS = [
    # =========================================================================
    # STRUCT / ENUM TYPE NAMES (rename the types themselves)
    # =========================================================================
    # SoundType enum — qualified values FIRST (longer patterns before shorter)
    (r'\bSoundType::Character\b', 'sound_type::character', 'SoundType::Character → sound_type::character'),
    (r'\bSoundType::Monster\b', 'sound_type::monster', 'SoundType::Monster → sound_type::monster'),
    (r'\bSoundType::Effect\b', 'sound_type::effect', 'SoundType::Effect → sound_type::effect'),
    (r'\bSoundType\b', 'sound_type', 'enum class SoundType → sound_type'),
    (r'\bDecodedSound\b', 'decoded_sound', 'struct DecodedSound → decoded_sound'),
    (r'\bActiveSound\b', 'active_sound', 'struct ActiveSound → active_sound'),

    # =========================================================================
    # AUDIOMANAGER METHODS (public + private, PascalCase → snake_case)
    # Longest names first to avoid partial matches
    # =========================================================================
    (r'\bCleanupFinishedSounds\b', 'cleanup_finished_sounds', 'AudioManager::CleanupFinishedSounds'),
    (r'\bGetCurrentMusicTrack\b', 'get_current_music_track', 'AudioManager::GetCurrentMusicTrack'),
    (r'\bSetListenerPosition\b', 'set_listener_position', 'AudioManager::SetListenerPosition'),
    (r'\bIsFullscreenStretchEnabled\b', 'is_fullscreen_stretch_enabled', 'ConfigManager::IsFullscreenStretchEnabled'),
    (r'\bSetFullscreenStretchEnabled\b', 'set_fullscreen_stretch_enabled', 'ConfigManager::SetFullscreenStretchEnabled'),
    (r'\bIsDialogTransparencyEnabled\b', 'is_dialog_transparency_enabled', 'ConfigManager::IsDialogTransparencyEnabled'),
    (r'\bSetDialogTransparencyEnabled\b', 'set_dialog_transparency_enabled', 'ConfigManager::SetDialogTransparencyEnabled'),
    (r'\bIsQuickActionsEnabled\b', 'is_quick_actions_enabled', 'ConfigManager::IsQuickActionsEnabled'),
    (r'\bIsPatchingGridEnabled\b', 'is_patching_grid_enabled', 'ConfigManager::IsPatchingGridEnabled'),
    (r'\bSetPatchingGridEnabled\b', 'set_patching_grid_enabled', 'ConfigManager::SetPatchingGridEnabled'),
    (r'\bIsMouseCaptureEnabled\b', 'is_mouse_capture_enabled', 'ConfigManager::IsMouseCaptureEnabled'),
    (r'\bSetMouseCaptureEnabled\b', 'set_mouse_capture_enabled', 'ConfigManager::SetMouseCaptureEnabled'),
    (r'\bIsShowLatencyEnabled\b', 'is_show_latency_enabled', 'ConfigManager::IsShowLatencyEnabled'),
    (r'\bSetShowLatencyEnabled\b', 'set_show_latency_enabled', 'ConfigManager::SetShowLatencyEnabled'),
    (r'\bIsRunningModeEnabled\b', 'is_running_mode_enabled', 'ConfigManager::IsRunningModeEnabled'),
    (r'\bSetRunningModeEnabled\b', 'set_running_mode_enabled', 'ConfigManager::SetRunningModeEnabled'),
    (r'\bIsFullscreenEnabled\b', 'is_fullscreen_enabled', 'ConfigManager::IsFullscreenEnabled'),
    (r'\bSetFullscreenEnabled\b', 'set_fullscreen_enabled', 'ConfigManager::SetFullscreenEnabled'),
    (r'\bIsBorderlessEnabled\b', 'is_borderless_enabled', 'ConfigManager::IsBorderlessEnabled'),
    (r'\bSetBorderlessEnabled\b', 'set_borderless_enabled', 'ConfigManager::SetBorderlessEnabled'),
    (r'\bIsTileGridEnabled\b', 'is_tile_grid_enabled', 'ConfigManager::IsTileGridEnabled'),
    (r'\bSetTileGridEnabled\b', 'set_tile_grid_enabled', 'ConfigManager::SetTileGridEnabled'),
    (r'\bIsZoomMapEnabled\b', 'is_zoom_map_enabled', 'ConfigManager::IsZoomMapEnabled'),
    (r'\bSetZoomMapEnabled\b', 'set_zoom_map_enabled', 'ConfigManager::SetZoomMapEnabled'),
    (r'\bIsShowFpsEnabled\b', 'is_show_fps_enabled', 'ConfigManager::IsShowFpsEnabled'),
    (r'\bSetShowFpsEnabled\b', 'set_show_fps_enabled', 'ConfigManager::SetShowFpsEnabled'),
    (r'\bGetGroupForSound\b', 'get_group_for_sound', 'AudioManager::GetGroupForSound'),
    (r'\bIsCategoryEnabled\b', 'is_category_enabled', 'AudioManager::IsCategoryEnabled'),
    (r'\bFreeDecodedSound\b', 'free_decoded_sound', 'AudioManager::FreeDecodedSound'),
    (r'\bGetDecodedSound\b', 'get_decoded_sound', 'AudioManager::GetDecodedSound'),
    (r'\bPlayGameSound\b', 'play_game_sound', 'AudioManager::PlayGameSound'),
    (r'\bPlaySoundLoop\b', 'play_sound_loop', 'AudioManager::PlaySoundLoop'),
    (r'\bStopAllSounds\b', 'stop_all_sounds', 'AudioManager::StopAllSounds'),
    (r'\bIsMusicPlaying\b', 'is_music_playing', 'AudioManager::IsMusicPlaying'),
    (r'\bVolumeToFloat\b', 'volume_to_float', 'AudioManager::VolumeToFloat'),
    (r'\bUnloadSounds\b', 'unload_sounds', 'AudioManager::UnloadSounds'),
    (r'\bSetAmbientVolume\b', 'set_ambient_volume', 'AudioManager/ConfigManager::SetAmbientVolume'),
    (r'\bSetAmbientEnabled\b', 'set_ambient_enabled', 'AudioManager/ConfigManager::SetAmbientEnabled'),
    (r'\bGetAmbientVolume\b', 'get_ambient_volume', 'AudioManager/ConfigManager::GetAmbientVolume'),
    (r'\bIsAmbientEnabled\b', 'is_ambient_enabled', 'AudioManager/ConfigManager::IsAmbientEnabled'),
    (r'\bSetMasterVolume\b', 'set_master_volume', 'AudioManager/ConfigManager::SetMasterVolume'),
    (r'\bSetMasterEnabled\b', 'set_master_enabled', 'AudioManager/ConfigManager::SetMasterEnabled'),
    (r'\bGetMasterVolume\b', 'get_master_volume', 'AudioManager/ConfigManager::GetMasterVolume'),
    (r'\bIsMasterEnabled\b', 'is_master_enabled', 'AudioManager/ConfigManager::IsMasterEnabled'),
    (r'\bSetSoundVolume\b', 'set_sound_volume', 'AudioManager/ConfigManager::SetSoundVolume'),
    (r'\bSetSoundEnabled\b', 'set_sound_enabled', 'AudioManager/ConfigManager::SetSoundEnabled'),
    (r'\bGetSoundVolume\b', 'get_sound_volume', 'AudioManager/ConfigManager::GetSoundVolume'),
    (r'\bIsSoundEnabled\b', 'is_sound_enabled', 'AudioManager/ConfigManager::IsSoundEnabled'),
    (r'\bSetMusicVolume\b', 'set_music_volume', 'AudioManager/ConfigManager::SetMusicVolume'),
    (r'\bSetMusicEnabled\b', 'set_music_enabled', 'AudioManager/ConfigManager::SetMusicEnabled'),
    (r'\bGetMusicVolume\b', 'get_music_volume', 'AudioManager/ConfigManager::GetMusicVolume'),
    (r'\bIsMusicEnabled\b', 'is_music_enabled', 'AudioManager/ConfigManager::IsMusicEnabled'),
    (r'\bIsSoundAvailable\b', 'is_sound_available', 'AudioManager::IsSoundAvailable'),
    (r'\bSetUIVolume\b', 'set_ui_volume', 'AudioManager/ConfigManager::SetUIVolume'),
    (r'\bSetUIEnabled\b', 'set_ui_enabled', 'AudioManager/ConfigManager::SetUIEnabled'),
    (r'\bGetUIVolume\b', 'get_ui_volume', 'AudioManager/ConfigManager::GetUIVolume'),
    (r'\bIsUIEnabled\b', 'is_ui_enabled', 'AudioManager/ConfigManager::IsUIEnabled'),
    (r'\bIsVSyncEnabled\b', 'is_vsync_enabled', 'ConfigManager::IsVSyncEnabled'),
    (r'\bSetVSyncEnabled\b', 'set_vsync_enabled', 'ConfigManager::SetVSyncEnabled'),
    (r'\bGetDetailLevel\b', 'get_detail_level', 'ConfigManager::GetDetailLevel'),
    (r'\bSetDetailLevel\b', 'set_detail_level', 'ConfigManager::SetDetailLevel'),
    (r'\bGetListenerX\b', 'get_listener_x', 'AudioManager::GetListenerX'),
    (r'\bGetListenerY\b', 'get_listener_y', 'AudioManager::GetListenerY'),
    (r'\bLoadSounds\b', 'load_sounds', 'AudioManager::LoadSounds'),
    (r'\bDecodeFile\b', 'decode_file', 'AudioManager::DecodeFile'),
    (r'\bStopSound\b', 'stop_sound', 'AudioManager::StopSound'),
    (r'\bPlayMusic\b', 'play_music', 'AudioManager::PlayMusic'),
    (r'\bStopMusic\b', 'stop_music', 'AudioManager::StopMusic'),

    # =========================================================================
    # CONFIGMANAGER-ONLY METHODS
    # =========================================================================
    (r'\bGetMagicShortcut\b', 'get_magic_shortcut', 'ConfigManager::GetMagicShortcut'),
    (r'\bSetMagicShortcut\b', 'set_magic_shortcut', 'ConfigManager::SetMagicShortcut'),
    (r'\bGetRecentShortcut\b', 'get_recent_shortcut', 'ConfigManager::GetRecentShortcut'),
    (r'\bSetRecentShortcut\b', 'set_recent_shortcut', 'ConfigManager::SetRecentShortcut'),
    (r'\bGetWindowWidth\b', 'get_window_width', 'ConfigManager::GetWindowWidth'),
    (r'\bGetWindowHeight\b', 'get_window_height', 'ConfigManager::GetWindowHeight'),
    (r'\bSetWindowSize\b', 'set_window_size', 'ConfigManager::SetWindowSize'),
    (r'\bGetFpsLimit\b', 'get_fps_limit', 'ConfigManager::GetFpsLimit'),
    (r'\bSetFpsLimit\b', 'set_fps_limit', 'ConfigManager::SetFpsLimit'),
    (r'\bGetShortcut\b', 'get_shortcut', 'ConfigManager::GetShortcut'),
    (r'\bSetShortcut\b', 'set_shortcut', 'ConfigManager::SetShortcut'),
    (r'\bSetDefaults\b', 'set_defaults', 'ConfigManager::SetDefaults (private)'),
    (r'\bMarkClean\b', 'mark_clean', 'ConfigManager::MarkClean'),
    (r'\bIsDirty\b', 'is_dirty', 'ConfigManager::IsDirty'),

    # =========================================================================
    # MEMBER VARIABLES — Hungarian prefix removal (m_b, m_dw, m_c, m_i)
    # Longest first to avoid partial matches
    # =========================================================================
    (r'\bm_bAmbientGroupInitialized\b', 'm_ambient_group_initialized', 'AudioManager::m_bAmbientGroupInitialized'),
    (r'\bm_bSfxGroupInitialized\b', 'm_sfx_group_initialized', 'AudioManager::m_bSfxGroupInitialized'),
    (r'\bm_bUIGroupInitialized\b', 'm_ui_group_initialized', 'AudioManager::m_bUIGroupInitialized'),
    (r'\bm_bFullscreenStretch\b', 'm_fullscreen_stretch', 'ConfigManager::m_bFullscreenStretch'),
    (r'\bm_bSoundAvailable\b', 'm_sound_available', 'AudioManager::m_bSoundAvailable'),
    (r'\bm_bMasterEnabled\b', 'm_master_enabled', 'AudioManager/ConfigManager::m_bMasterEnabled'),
    (r'\bm_bAmbientEnabled\b', 'm_ambient_enabled', 'AudioManager/ConfigManager::m_bAmbientEnabled'),
    (r'\bm_bSoundEnabled\b', 'm_sound_enabled', 'AudioManager/ConfigManager::m_bSoundEnabled'),
    (r'\bm_bMusicEnabled\b', 'm_music_enabled', 'AudioManager/ConfigManager::m_bMusicEnabled'),
    (r'\bm_bCaptureMouse\b', 'm_capture_mouse', 'ConfigManager::m_bCaptureMouse'),
    (r'\bm_bPatchingGrid\b', 'm_patching_grid', 'ConfigManager::m_bPatchingGrid'),
    (r'\bm_bShowLatency\b', 'm_show_latency', 'ConfigManager::m_bShowLatency'),
    (r'\bm_bRunningMode\b', 'm_running_mode', 'ConfigManager::m_bRunningMode'),
    (r'\bm_bDialogTrans\b', 'm_dialog_trans', 'ConfigManager::m_bDialogTrans'),
    (r'\bm_bInitialized\b', 'm_initialized', 'AudioManager/ConfigManager/ChatCommandManager::m_bInitialized'),
    (r'\bm_bFullscreen\b', 'm_fullscreen', 'ConfigManager::m_bFullscreen'),
    (r'\bm_bBorderless\b', 'm_borderless', 'ConfigManager::m_bBorderless'),
    (r'\bm_bUIEnabled\b', 'm_ui_enabled', 'AudioManager/ConfigManager::m_bUIEnabled'),
    (r'\bm_bBgmLoaded\b', 'm_bgm_loaded', 'AudioManager::m_bBgmLoaded'),
    (r'\bm_dwSoundOrder\b', 'm_sound_order', 'AudioManager::m_dwSoundOrder'),
    (r'\bm_cDetailLevel\b', 'm_detail_level', 'ConfigManager::m_cDetailLevel'),
    (r'\bm_bTileGrid\b', 'm_tile_grid', 'ConfigManager::m_bTileGrid'),
    (r'\bm_bShowFPS\b', 'm_show_fps', 'ConfigManager::m_bShowFPS'),
    (r'\bm_bZoomMap\b', 'm_zoom_map', 'ConfigManager::m_bZoomMap'),
    (r'\bm_iFpsLimit\b', 'm_fps_limit', 'ConfigManager::m_iFpsLimit'),
    (r'\bm_bDirty\b', 'm_dirty', 'ConfigManager::m_bDirty'),
    (r'\bm_bVSync\b', 'm_vsync', 'ConfigManager::m_bVSync'),

    # =========================================================================
    # MEMBER VARIABLES — camelCase → snake_case (no Hungarian prefix)
    # =========================================================================
    (r'\bm_currentMusicTrack\b', 'm_current_music_track', 'AudioManager::m_currentMusicTrack'),
    (r'\bm_characterSounds\b', 'm_character_sounds', 'AudioManager::m_characterSounds'),
    (r'\bm_monsterSounds\b', 'm_monster_sounds', 'AudioManager::m_monsterSounds'),
    (r'\bm_effectSounds\b', 'm_effect_sounds', 'AudioManager::m_effectSounds'),
    (r'\bm_activeSounds\b', 'm_active_sounds', 'AudioManager::m_activeSounds'),
    (r'\bm_ambientGroup\b', 'm_ambient_group', 'AudioManager::m_ambientGroup'),
    (r'\bm_ambientVolume\b', 'm_ambient_volume', 'AudioManager/ConfigManager::m_ambientVolume'),
    (r'\bm_masterVolume\b', 'm_master_volume', 'AudioManager/ConfigManager::m_masterVolume'),
    (r'\bm_soundVolume\b', 'm_sound_volume', 'AudioManager/ConfigManager::m_soundVolume'),
    (r'\bm_musicVolume\b', 'm_music_volume', 'AudioManager/ConfigManager::m_musicVolume'),
    (r'\bm_windowWidth\b', 'm_window_width', 'ConfigManager::m_windowWidth'),
    (r'\bm_windowHeight\b', 'm_window_height', 'ConfigManager::m_windowHeight'),
    (r'\bm_magicShortcut\b', 'm_magic_shortcut', 'ConfigManager::m_magicShortcut'),
    (r'\bm_recentShortcut\b', 'm_recent_shortcut', 'ConfigManager::m_recentShortcut'),
    (r'\bm_sfxGroup\b', 'm_sfx_group', 'AudioManager::m_sfxGroup'),
    (r'\bm_bgmSound\b', 'm_bgm_sound', 'AudioManager::m_bgmSound'),
    (r'\bm_uiVolume\b', 'm_ui_volume', 'AudioManager/ConfigManager::m_uiVolume'),
    (r'\bm_uiGroup\b', 'm_ui_group', 'AudioManager::m_uiGroup'),
    (r'\bm_listenerX\b', 'm_listener_x', 'AudioManager::m_listenerX'),
    (r'\bm_listenerY\b', 'm_listener_y', 'AudioManager::m_listenerY'),

    # =========================================================================
    # STRUCT DATA FIELDS (plain structs, no m_ prefix per standard)
    # Only fields unique to AudioManager structs (safe for global replacement)
    # =========================================================================
    (r'\bsoundInitialized\b', 'sound_initialized', 'ActiveSound::soundInitialized'),
    (r'\bstartOrder\b', 'start_order', 'ActiveSound::startOrder'),
    (r'\bbufferRef\b', 'buffer_ref', 'ActiveSound::bufferRef'),
    (r'\bframeCount\b', 'frame_count', 'DecodedSound::frameCount'),
    (r'\bsampleRate\b', 'sample_rate', 'DecodedSound::sampleRate'),
    (r'\binUse\b', 'in_use', 'ActiveSound::inUse'),

    # =========================================================================
    # LOCAL VARIABLES AND PARAMETERS — camelCase/Hungarian → snake_case
    # Only identifiers that are unique enough for safe global replacement
    # =========================================================================
    (r'\bengineSampleRate\b', 'engine_sample_rate', 'AudioManager local: engineSampleRate'),
    (r'\bdecoderConfig\b', 'decoder_config', 'AudioManager local: decoderConfig'),
    (r'\bengineConfig\b', 'engine_config', 'AudioManager local: engineConfig'),
    (r'\binstanceCount\b', 'instance_count', 'AudioManager local: instanceCount'),
    (r'\btotalFrames\b', 'total_frames', 'AudioManager local: totalFrames'),
    (r'\bbytesPerFrame\b', 'bytes_per_frame', 'AudioManager local: bytesPerFrame'),
    (r'\bframesRead\b', 'frames_read', 'AudioManager local: framesRead'),
    (r'\bpanValue\b', 'pan_value', 'AudioManager local: panValue'),
    (r'\bdataSize\b', 'data_size', 'AudioManager local: dataSize'),
    (r'\bmaxDist\b', 'max_dist', 'AudioManager local: maxDist'),
    (r'\bwasEnabled\b', 'was_enabled', 'AudioManager local: wasEnabled'),
    (r'\bpDecoded\b', 'decoded', 'AudioManager local: pDecoded → decoded'),
    (r'\bpOldest\b', 'oldest', 'AudioManager local: pOldest → oldest'),
    (r'\bpGroup\b', 'group', 'AudioManager local: pGroup → group'),
    (r'\bpSlot\b', 'slot', 'AudioManager local: pSlot → slot'),
    # filePath — used in AudioManager.h/cpp only (3 occurrences), safe
    (r'\bfilePath\b', 'file_path', 'AudioManager param: filePath'),
    # trackName — used in AudioManager + Game.cpp, safe
    (r'\btrackName\b', 'track_name', 'AudioManager/Game param: trackName'),
    # worldX/worldY — used in AudioManager + Camera.h, safe
    (r'\bworldX\b', 'world_x', 'AudioManager/Camera param: worldX'),
    (r'\bworldY\b', 'world_y', 'AudioManager/Camera param: worldY'),

    # ConfigManager locals/params
    (r'\bs_NumValidResolutions\b', 's_num_valid_resolutions', 'ConfigManager static: s_NumValidResolutions'),
    (r'\bs_ValidResolutions\b', 's_valid_resolutions', 'ConfigManager static: s_ValidResolutions'),
    (r'\bbValidResolution\b', 'valid_resolution', 'ConfigManager local: bValidResolution'),
    (r'\bbestIndex\b', 'best_index', 'ConfigManager local: bestIndex'),
    (r'\bbestDiff\b', 'best_diff', 'ConfigManager local: bestDiff'),
    (r'\bbValid\b', 'valid', 'ConfigManager local: bValid'),

    # =========================================================================
    # ConfigManager private method — Clamp (only used in ConfigManager)
    # Put this last since it's a short word
    # =========================================================================
    (r'\bClamp\b', 'clamp', 'ConfigManager::Clamp (private utility)'),
]

files = sorted(
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
)

total_changes = 0
for f in files:
    with open(f, 'r', encoding='utf-8', errors='replace') as fh:
        content = fh.read()
    original = content
    for pattern, replacement, desc in REPLACEMENTS:
        content = re.sub(pattern, replacement, content)
    if content != original:
        with open(f, 'w', encoding='utf-8', newline='') as fh:
            fh.write(content)
        total_changes += 1
        print(f'  Updated: {os.path.basename(f)}')

print(f'\nAgent 7 done. {total_changes} file(s) updated, {len(REPLACEMENTS)} replacements applied.')
