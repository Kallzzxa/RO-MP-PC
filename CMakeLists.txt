cmake_minimum_required(VERSION 3.16)
project(romp-pc VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib via FetchContent (diunduh saat build pertama).
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# raygui: single-header, ambil langsung.
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raygui)

add_executable(romp
    src/main.cpp
    src/app.cpp
    src/app.h
    src/pc_launch.cpp
    src/pc_launch.h
    src/net/pc_client.cpp
    src/net/pc_client.h
    src/net/pc_browser.cpp
    src/net/pc_browser.h
    src/net/romp_protocol.h
    src/ui/ui_browser.cpp
    src/ui/ui_game.cpp
    src/ui/raygui_impl.cpp
)
target_include_directories(romp PRIVATE
    ${raygui_SOURCE_DIR}/src
    src
)
target_link_libraries(romp PRIVATE raylib)

if (WIN32)
    target_link_libraries(romp PRIVATE ws2_32)
endif()

# Headless test (tanpa GUI): build terpisah via tests/CMakeLists.txt
