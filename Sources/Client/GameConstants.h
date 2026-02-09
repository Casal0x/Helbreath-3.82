#pragma once
#include <cstdint>

namespace game_limits {
    constexpr int max_sprites          = 25000;
    constexpr int max_tiles            = 1000;
    constexpr int max_effect_sprites   = 300;
    constexpr int max_sound_effects    = 200;
    constexpr int max_chat_msgs        = 500;
    constexpr int max_whisper_msgs     = 5;
    constexpr int max_chat_scroll_msgs = 80;
    constexpr int max_effects          = 300;
    constexpr int max_menu_items       = 140;
    constexpr int max_text_dlg_lines   = 300;
    constexpr int max_magic_types      = 100;
    constexpr int max_skill_types      = 60;
    constexpr int max_weather_objects  = 600;
    constexpr int max_game_msgs        = 300;
    constexpr int max_guild_names      = 100;
    constexpr int max_sell_list        = 12;
    constexpr int socket_block_limit   = 300;
}

namespace ui_layout {
    constexpr int btn_size_x    = 74;
    constexpr int btn_size_y    = 20;
    constexpr int left_btn_x    = 30;
    constexpr int right_btn_x   = 154;
    constexpr int btn_y         = 292;
}

namespace input_config {
    constexpr int double_click_time_ms  = 300;
    constexpr int double_click_tolerance = 4;
}

// CursorStatus enum is defined in CursorTarget.h

enum class ServerType : uint8_t {
    Game = 1,
    Log  = 2
};

// Backward compatibility aliases (old code compiles unchanged)
#define DEF_BTNSZX              ui_layout::btn_size_x
#define DEF_BTNSZY              ui_layout::btn_size_y
#define DEF_LBTNPOSX            ui_layout::left_btn_x
#define DEF_RBTNPOSX            ui_layout::right_btn_x
#define DEF_BTNPOSY             ui_layout::btn_y
#define DEF_SOCKETBLOCKLIMIT    game_limits::socket_block_limit
#define DEF_MAXSPRITES          game_limits::max_sprites
#define DEF_MAXTILES            game_limits::max_tiles
#define DEF_MAXEFFECTSPR        game_limits::max_effect_sprites
#define DEF_MAXSOUNDEFFECTS     game_limits::max_sound_effects
#define DEF_MAXCHATMSGS         game_limits::max_chat_msgs
#define DEF_MAXWHISPERMSG       game_limits::max_whisper_msgs
#define DEF_MAXCHATSCROLLMSGS   game_limits::max_chat_scroll_msgs
#define DEF_MAXEFFECTS          game_limits::max_effects
#define DEF_MAXMENUITEMS        game_limits::max_menu_items
#define DEF_TEXTDLGMAXLINES     game_limits::max_text_dlg_lines
// DEF_MAXMAGICTYPE and DEF_MAXSKILLTYPE are defined in NetConstants.h (shared)
#define DEF_MAXWHETHEROBJECTS   game_limits::max_weather_objects
#define DEF_MAXGAMEMSGS         game_limits::max_game_msgs
#define DEF_MAXGUILDNAMES       game_limits::max_guild_names
#define DEF_MAXSELLLIST         game_limits::max_sell_list
#define DEF_CURSORSTATUS_NULL       0
#define DEF_CURSORSTATUS_PRESSED    1
#define DEF_CURSORSTATUS_SELECTED   2
#define DEF_CURSORSTATUS_DRAGGING   3
#define DEF_DOUBLECLICKTIME         input_config::double_click_time_ms
#define DEF_DOUBLECLICKTOLERANCE    input_config::double_click_tolerance
#define DEF_SERVERTYPE_GAME         static_cast<int>(ServerType::Game)
#define DEF_SERVERTYPE_LOG          static_cast<int>(ServerType::Log)
