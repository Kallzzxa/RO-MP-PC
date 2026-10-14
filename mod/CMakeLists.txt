cmake_minimum_required(VERSION 3.16)
# RO-MP playable mod — ASI untuk GTA:SA PC 1.0 US.
# HARUS build 32-bit (Win32), MSVC. plugin-sdk tidak support MinGW.
project(romp-mod LANGUAGES C CXX)

if(NOT MSVC)
    message(FATAL_ERROR "Mod ini butuh MSVC (plugin-sdk). Pakai Visual Studio 2022.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "GTA:SA itu 32-bit — configure dengan -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)

# --- plugin-sdk (submodule di vendor/plugin-sdk) ---
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/plugin_sa/plugin_sa.h")
    message(FATAL_ERROR "plugin-sdk tidak ada. Jalankan: git submodule update --init --recursive")
endif()
file(GLOB_RECURSE PLUGIN_SDK_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/plugin_sa/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/shared/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/hooking/*.cpp"
)
file(GLOB_RECURSE PLUGIN_SDK_HDR
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/plugin_sa/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk/shared/*.h"
)

# --- imgui (submodule) ---
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.h")
    message(FATAL_ERROR "imgui tidak ada. Jalankan: git submodule update --init --recursive")
endif()
set(IMGUI_SRC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_dx9.cpp
    vendor/imgui/backends/imgui_impl_win32.cpp
)

# --- minhook (submodule) ---
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/minhook/include/MinHook.h")
    message(FATAL_ERROR "minhook tidak ada. Jalankan: git submodule update --init --recursive")
endif()
file(GLOB MINHOOK_SRC "vendor/minhook/src/*.c" "vendor/minhook/src/hde/*.c")

# --- net core RO-MP (reuse dari ../src/net, sudah ter-test) ---
set(NET_SRC
    ../src/net/pc_client.cpp
    ../src/net/pc_browser.cpp
)

add_library(romp SHARED
    src/dllmain.cpp
    src/romp_mod.cpp
    src/romp_mod.h
    src/net_worker.cpp
    src/net_worker.h
    src/d3d9_hook.cpp
    src/d3d9_hook.h
    src/overlay.cpp
    src/overlay.h
    src/remote_peds.cpp
    src/remote_peds.h
    ${NET_SRC}
    ${IMGUI_SRC}
    ${MINHOOK_SRC}
    ${PLUGIN_SDK_SRC}
)

target_include_directories(romp PRIVATE
    src
    ../src
    ../src/net
    vendor/plugin-sdk/plugin_sa
    vendor/plugin-sdk/shared
    vendor/plugin-sdk/shared/game
    vendor/imgui
    vendor/imgui/backends
    vendor/minhook/include
)

target_compile_definitions(romp PRIVATE
    GTASA
    _CRT_SECURE_NO_WARNINGS
)

# plugin-sdk butuh ini
target_compile_options(romp PRIVATE /W3 /EHsc /MP)

set_target_properties(romp PROPERTIES
    OUTPUT_NAME "romp"
    SUFFIX ".asi"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
)

# D3D9 + Winsock
target_link_libraries(romp PRIVATE d3d9 ws2_32)
