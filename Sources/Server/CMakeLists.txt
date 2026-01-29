cmake_minimum_required(VERSION 3.16)
project(HBServer VERSION 3.82 LANGUAGES CXX C)

# Force failure on Windows - use Visual Studio solution instead
if(WIN32)
    message(FATAL_ERROR "This CMakeLists.txt is for Unix/Linux/macOS only. Use Helbreath.sln for Windows builds.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Server source files (excluding Windows-specific files)
set(SERVER_SOURCES
    AccountSqliteStore.cpp
    Client.cpp
    CommonTypes.cpp
    EntityManager.cpp
    Game.cpp
    GameConfigSqliteStore.cpp
    LoginServer.cpp
    Map.cpp
    MapInfoSqliteStore.cpp
    NetworkMsg.cpp
    Npc.cpp
    PartyManager.cpp
    Quest.cpp
    Tile.cpp
    
    # Unix-specific implementations
    Platform_Unix.cpp
    XSocket_Unix.cpp
    main_unix.cpp
)

# Server header files
set(SERVER_HEADERS
    AccountSqliteStore.h
    ActionID_Server.h
    BuildItem.h
    Client.h
    CommonTypes.h
    CrusadeCore.h
    DelayEvent.h
    DynamicObject.h
    EntityManager.h
    Fish.h
    Game.h
    GameConfigSqliteStore.h
    GlobalDef.h
    GuildsMan.h
    IrcBot.h
    Item.h
    LoginServer.h
    Magic.h
    Map.h
    MapInfoSqliteStore.h
    MessageIndex.h
    Mineral.h
    Misc.h
    NetMessages.h
    NetworkMsg.h
    Npc.h
    OccupyFlag.h
    PartyManager.h
    Platform.h
    Portion.h
    Quest.h
    ServerMessages.h
    Skill.h
    StrategicPoint.h
    TeleportLoc.h
    TempNpcItem.h
    Tile.h
    UserMessages.h
    Version.h
    XSocket.h
)

# SQLite source (compile directly)
set(SQLITE_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Server/Includes/SQLite/sqlite3.c
)

# Create executable
add_executable(HBServer
    ${SERVER_SOURCES}
    ${SERVER_HEADERS}
    ${SQLITE_SOURCE}
)

# Include directories
target_include_directories(HBServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Server/Includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Server/Includes/SQLite
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Shared
)

# Platform-specific libraries
if(APPLE)
    # macOS specific
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    target_link_libraries(HBServer PRIVATE 
        ${CORE_FOUNDATION}
        pthread
        dl
    )
elseif(UNIX AND NOT APPLE)
    # Linux specific
    target_link_libraries(HBServer PRIVATE 
        pthread
        dl
        rt
    )
endif()

# Compiler definitions
target_compile_definitions(HBServer PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(HBServer PRIVATE
        -Wall
        -Wno-unknown-pragmas
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-sign-compare
    )
endif()

# Output directory
set_target_properties(HBServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Print configuration
message(STATUS "===========================================")
message(STATUS "Helbreath Server v${PROJECT_VERSION}")
message(STATUS "===========================================")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "===========================================")
