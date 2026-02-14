cmake_minimum_required(VERSION 3.16)
project(HelbreathClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# SFMLEngine static library (9 .cpp files)
# ---------------------------------------------------------------------------
set(SFMLENGINE_SOURCES
    ../SFMLEngine/RendererFactory.cpp
    ../SFMLEngine/SFMLBitmapFont.cpp
    ../SFMLEngine/SFMLInput.cpp
    ../SFMLEngine/SFMLRenderer.cpp
    ../SFMLEngine/SFMLSprite.cpp
    ../SFMLEngine/SFMLSpriteFactory.cpp
    ../SFMLEngine/SFMLTextRenderer.cpp
    ../SFMLEngine/SFMLTexture.cpp
    ../SFMLEngine/SFMLWindow.cpp
)

add_library(SFMLEngine STATIC ${SFMLENGINE_SOURCES})

# ---------------------------------------------------------------------------
# Shared sources (8 .cpp files)
# ---------------------------------------------------------------------------
set(SHARED_SOURCES
    ../Dependencies/Shared/Log/LogBackend.cpp
    ../Dependencies/Shared/Net/ASIOSocket.cpp
    ../Dependencies/Shared/Net/IOServicePool.cpp
    ../Dependencies/Shared/Render/Application.cpp
    ../Dependencies/Shared/Render/Event.cpp
    ../Dependencies/Shared/Render/ISpriteFactory.cpp
    ../Dependencies/Shared/Render/ResolutionConfig.cpp
    ../Dependencies/Shared/Render/TextLib.cpp
)

# ---------------------------------------------------------------------------
# Client sources (all .cpp in Sources/Client/)
# ---------------------------------------------------------------------------
file(GLOB CLIENT_SOURCES "*.cpp")

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(HelbreathClient
    ${CLIENT_SOURCES}
    ${SHARED_SOURCES}
)

# ---------------------------------------------------------------------------
# Include directories (mirrors vcxproj IncludePath)
# ---------------------------------------------------------------------------
set(SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Shared)

set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../SFMLEngine
    ${SHARED_DIR}
    ${SHARED_DIR}/Entity
    ${SHARED_DIR}/Game
    ${SHARED_DIR}/Net
    ${SHARED_DIR}/Render
    ${SHARED_DIR}/Util
    ${SHARED_DIR}/Log
    ${SHARED_DIR}/ASIO
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/Client/Includes
)

target_include_directories(SFMLEngine PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(HelbreathClient PRIVATE ${COMMON_INCLUDE_DIRS})

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
target_compile_definitions(SFMLEngine PRIVATE SFML_STATIC SFML_ENGINE)
target_compile_definitions(HelbreathClient PRIVATE SFML_STATIC SFML_ENGINE)

# ---------------------------------------------------------------------------
# Find SFML 3 — try system install first, fall back to FetchContent
# ---------------------------------------------------------------------------
find_package(SFML 3 COMPONENTS Graphics Window System QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML 3 not found on system — fetching from GitHub (3.0.2)...")
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.2
        GIT_SHALLOW    TRUE
    )
    # Only build the modules we need
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SFML)
endif()

# SFMLEngine needs SFML headers
target_link_libraries(SFMLEngine PRIVATE SFML::Graphics SFML::Window SFML::System)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

target_link_libraries(HelbreathClient PRIVATE
    SFMLEngine
    SFML::Graphics
    SFML::Window
    SFML::System
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
