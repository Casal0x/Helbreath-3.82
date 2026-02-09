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

// DEF_MAXMAGICTYPE and DEF_MAXSKILLTYPE are defined in NetConstants.h (shared)
