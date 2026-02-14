# Changelog

### Bug Fixes
- Server broadcast messages (type 10) were not appearing in the client's global chat area — the event list gate only allowed whispers (type 20) to bypass the floating text slot check, now broadcasts get the same treatment
- ShowChat server command terminal was immediately closing — removed cmd.exe wrapper and launch PowerShell directly to avoid pipe interpretation issues

### Code Cleanup
- Changed three diagnostic chat log messages from `log` to `debug` level so they no longer pollute the chat log with packet validation noise
- ShowChat terminal now filters out `[DEBUG]` lines so only actual chat content is displayed

### Bug Fix — Shield/Weapon Glare Color Pipeline
- Fixed glare rendering producing invisible/non-oscillating glow instead of always-visible color-shifting glow
- Root cause: original DDraw `PutTransSpriteRGB` ADDED color offsets to pixels before additive blending; SFML port used MULTIPLICATIVE tint via `sprite.setColor()` — at `m_draw_flag=0` this zeroed all channels making the glow invisible
- Changed glare color formula from `additive_colored(flag, 0, 0)` to `additive_colored(255, 255-flag, 255-flag)` — keeps primary channel at full brightness, reduces non-primary channels to shift hue toward glare color
- Changed oscillation range from 0-255 to 0-200 to match original DDraw proportion (0-25 on 5-bit channels ≈ 81%)
- Replaced `m_time` with direct `GameClock::get_time_ms()` call in oscillation code

### Bug Fix — Shield/Weapon Glare White Flash
- Fixed `DrawParams::additive_colored(0,0,0)` flashing white instead of being invisible
- Root cause: renderer checked `tintR != 0 || tintG != 0 || tintB != 0` to detect tint, treating (0,0,0) as "no tint" and leaving sprite at white under additive blending
- Added `bool m_has_tint` field to `DrawParams`; factory methods that set tint values also set `m_has_tint = true`; renderer checks `m_has_tint` instead of the != 0 check
- Removed `SFMLBitmapFont.cpp` workaround that clamped (0,0,0) to (1,1,1) for color-replace text

### Code Standards — DrawParams/BitmapTextParams Rename
- Renamed all `DrawParams` fields to `m_` prefix + snake_case (10 fields: `tintR`→`m_tint_r`, `blendMode`→`m_blend_mode`, `useColorKey`→`m_use_color_key`, `alpha`→`m_alpha`, `shadow`→`m_shadow`, `reverse`→`m_reverse`, `fade`→`m_fade`, `additive`→`m_additive`)
- Renamed all `BitmapTextParams` fields to `m_` prefix + snake_case (7 fields: `tintR`→`m_tint_r`, `alpha`→`m_alpha`, `shadow`→`m_shadow`, `color_replace`→`m_color_replace`, `useAdditive`→`m_use_additive`)
- Renamed 13 `DrawParams` factory methods PascalCase→snake_case (`Opaque`→`opaque`, `Alpha`→`alpha_blend`, `Tint`→`tint`, `Shadow`→`shadow`, `Fade`→`fade`, `Additive`→`additive`, etc.)
- Renamed 7 `BitmapTextParams` factory methods (`Default`→`make_default`, `ColorReplace`→`color_replace`, etc.)
- Renamed 2 `IBitmapFont` methods (`GetCharWidth`→`get_char_width`, `DrawTextCentered`→`draw_text_centered`)
- Script: `Scripts/phase_shield_glare.py` — 29 files, ~735 changes

### Code Standards — Struct Member `m_` Prefix
- Added `m_` prefix to all client struct data members per coding standards (11 structs, ~121 members, ~2394 replacements across 78 files)
- Structs: `animation_state`, `EntityMotion`, `FocusedObject`, `TargetObjectInfo`, `DialogBoxInfo`, `EquipmentIndices`, `AnimDef`, `spell_aoe_tile`, `spell_aoe_params`, `AnimParams`, `BenchmarkSlot`
- Three-phase script: Phase C (header declarations), Phase A (accessor-qualified `.member` patterns), Phase B (self-file bare/`this->` access)
- Replaced `this->` workarounds in AnimationState.cpp and EntityMotion.cpp with proper `m_` naming
- Fixup pass for 9 additional accessor patterns missed by main script (info., dropInfo., giveInfo., mfg., focused., def.)
- Scripts: `Scripts/phase_member_prefix.py`, `Scripts/phase_member_prefix_fixup.py`

### Code Standards — Snake Case Cleanup (Phases 1-10)
- Phase 10: Resolved 122 collision groups (300 Hungarian names) where multiple prefixes mapped to the same snake_case target (iCount/sCount/dwCount→count, cData/pData→data, etc.). Script `phase_snake_collisions.py` performed file-level co-occurrence analysis to detect same-scope conflicts; ~5 groups required manual disambiguation (data→msg_data/resp_data, type→snd_type/enchant_type, ret→bool renamed away, temp→temp_buf/temp_int, key→key_val, v4→dw_v4, result→msg_result, exp→exp_gained, v1-v4→dv1-dv4). Fixed parameter/member shadowing in NetworkPacket and QueuedMsg constructors. 268 files modified by script + 14 files hand-fixed.
- Phase 9: Renamed ~936 Hungarian-prefixed parameters and local variables to snake_case (iClientH→client_h, bFlag→flag, dwTime→time, pData→data, cDir→dir, etc.) — 340 files, ~17,771 changes across all modules. Auto-detected prefixes (i,b,c,s,p,dw,w,f,d,sz,st,ms,is,po), collision-filtered 122 groups (300 names deferred), excluded external API fields (KEY_EVENT_RECORD.bKeyDown, sf::Texture::isSmooth, sf::RenderWindow::isOpen), override table for abbreviations (iHP→hp, iSTR→str_stat, iINT→int_stat, pError→error_acc). Wire protocol structs (#pragma pack) excluded.

### Code Standards — Snake Case Cleanup (Phases 1-8) [Historical]
- Phase 1: Removed `__fastcall` keyword (16 occurrences) and renamed shared static members (s_pFactory→s_factory, s_pRenderer→s_renderer, s_pWindow→s_window, s_bInitialized→s_initialized)
- Phase 2: Renamed server methods (ServerConsole::Init→init, WriteLine→write_line, etc.; CrusadeCore::Test→test; Game::Quit→quit; CMsg::Get→get)
- Phase 3: Renamed ~150 server struct members to snake_case (AccountSqliteStore, Game.h structs, Map.h structs, Client.h, Misc.h, PasswordHash)
- Phase 4: Renamed ~36 client struct members (DialogBoxInfo, weather_particle, EventEntry)
- Phase 5: Renamed 20 SFMLEngine member variables to snake_case (SFMLBitmapFont, SFMLInput, SFMLRenderer, SFMLSprite, SFMLSpriteFactory, SFMLTextRenderer, SFMLWindow)
- Phase 6: Renamed ~290 client member variables to snake_case (Game.h ~160, Player.h ~72, Camera.h, PlayerController.h, Tile.h, ServerConsole.h)
- Phase 7: Renamed ~112 shared interface methods to snake_case (IRenderer, ISpriteFactory, ResolutionConfig, TextLib/TextStyle, ConcurrentMsgQueue, IOServicePool) — 199 files, ~3425 changes across Shared+SFMLEngine+Client+Server
- Phase 8: Renamed ~206 remaining client PascalCase methods to snake_case (CursorTarget, ChatManager, FrameTiming, Benchmark, 22 DialogBox_* classes, Screen_*, Overlay_*, ConfigManager, LocalCacheManager, Version, EntityRenderState, EffectManager) plus shared entity helpers (OwnerType, ObjectIDRange, Appearance, PlayerStatus, ItemEffectType::AddEffect, NetMessages enums) — 143+ files, ~1621+ changes

### Code Standards — Earlier Bulk Renames
- Converted client PascalCase method names to snake_case (335 rename entries, 5,510 replacements across 202 files)
- Classes covered: CGame (~130 methods incl. Hotkey_*, DrawObject_On*, config loading), IDialogBox, DialogBoxManager, IGameScreen, GameModeManager, ChatCommandManager, CMapData, CCamera, RenderHelpers, CPlayerRenderer, CNpcRenderer, HotkeyManager, CPlayerController, EntityMotion, CEntityRenderState, EffectManager, FloatingTextManager, WeatherManager, and others
- Stripped type prefixes (b, i, _b, _i) from method names
- Skipped names that collide with Shared enums (GrandMagicResult, MeteorStrikeComing, CannotConstruct, Stop) or SFMLEngine interfaces (Draw, Init, Clear, Get, BeginFrame, EndFrame)
- CursorTarget and FrameTiming deferred to separate manual rename (qualified pattern limitations)
- Converted client types, variables, and member data from PascalCase/Hungarian to snake_case (518 rename entries, 15,588 replacements across 160 files)
- Classes covered: CGame, CTile, CMapData, CPlayerController, CCamera, CEntityRenderState, EntityMotion, AnimationState, FocusedObject, IDialogBox, DialogBoxManager, IGameScreen, GameModeManager, CraftingManager, FishingManager, MagicCastingSystem, CombatSystem, SpellAoE, WeatherManager, ShopManager, InventoryManager, TeleportManager, TextInputManager, EventListManager, GuildManager, BuildItemManager, AudioManager, ConfigManager, FloatingTextManager, EffectManager, HotkeyManager, ChatManager, QuestManager, CrusadeManager, LocalCacheManager, NetworkMessageManager, ChatCommandManager, ItemNameFormatter, Player, RenderHelpers, FrameTiming, Benchmark
- Converted all server and shared Hungarian notation member variables to snake_case (779 rename entries, 38,938 replacements across 91 files)
- Classes covered: CGame, CClient, CNpc, CMap, CTile, CItem, CMagic, CSkill, CQuest, CMsg, CEntityManager, ASIOSocket, GameChatCommandManager, plus all manager classes and small structs
- Stripped type prefixes (m_i, m_b, m_dw, m_c, m_s, m_p, m_w, m_f, m_rc, m_st) and expanded cryptic abbreviations (AR→attack_ratio, DR→defense_ratio, MR→magic_resistance, CD→combo_damage, SSN→skill_progress, etc.)
- Struct data members follow coding standard: no m_ prefix
- Wire protocol structs (#pragma pack) excluded from renaming
- Converted all server and shared PascalCase/camelCase method names to snake_case (577 rename entries, 5,398 replacements across 99 files)
- Classes covered: CGame, CClient, CEntityManager, CMsg, GameChatCommand, PartyManager, ItemManager, CombatManager, MagicManager, SkillManager, WarManager, GuildManager, QuestManager, LootManager, StatusEffectManager, FishingManager, MiningManager, CraftingManager, DelayEventManager, DynamicObjectManager, CMap, CItem, ASIOSocket, ItemEnums, ItemAttributes, IOServicePool, LoginServer, SqliteStores, ServerConsole
- Stripped type prefixes (b, i, c) from method names and expanded abbreviations
- Fixed original typos during rename: WhetherProcessor→weather_processor, TimeStaminarPointsUp→time_stamina_points_up, DissmissGuild→dismiss_guild

### Infrastructure
- Lowercased all 566 files and 8 directories in Binaries/Game/ for case-sensitive filesystem compatibility (sprites, sounds, maps, cache, music, fonts, save, contents)
- Updated 291 source code string literals to reference lowercase file paths (sprite pak names, sound files, cache paths, font/music/config paths)
- Added runtime lowercasing of server-sent map names before building map data file paths
