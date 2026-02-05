cmake_minimum_required(VERSION 3.20)
project(game90 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GAME90_BUILD_MAP_EDITOR "Build the map editor" ON)
option(GAME90_ENABLE_IMGUI "Enable Dear ImGui overlay (via C bridge)" ON)

set(GAME90_WARNINGS -Wall -Wextra -Wpedantic -Werror)

find_package(SDL2 QUIET)
if (SDL2_FOUND)
    if (TARGET SDL2::SDL2)
        set(SDL2_TARGET SDL2::SDL2)
    else()
        add_library(SDL2::SDL2 INTERFACE IMPORTED)
        target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
        set(SDL2_TARGET SDL2::SDL2)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
    set(SDL2_TARGET SDL2::SDL2)
endif()

add_library(imgui_c_bridge STATIC src/ui/imgui_c.cpp)
target_include_directories(imgui_c_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
target_compile_options(imgui_c_bridge PRIVATE ${GAME90_WARNINGS})

if (GAME90_ENABLE_IMGUI)
    target_compile_definitions(imgui_c_bridge PRIVATE GAME90_ENABLE_IMGUI=1)

    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
    if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
        message(FATAL_ERROR "ImGui requested but not found at ${IMGUI_DIR}. Please add ImGui to third_party/imgui.")
    endif()

    add_library(imgui_cpp STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    )

    target_include_directories(imgui_cpp PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )

    target_compile_options(imgui_cpp PRIVATE ${GAME90_WARNINGS})
    target_link_libraries(imgui_cpp PRIVATE ${SDL2_TARGET})
    target_link_libraries(imgui_c_bridge PRIVATE imgui_cpp ${SDL2_TARGET})
else()
    target_compile_definitions(imgui_c_bridge PRIVATE GAME90_ENABLE_IMGUI=0)
endif()

add_executable(game90
    src/game/main.c
    src/game/map.c
    src/game/render.c
)
target_include_directories(game90 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
target_compile_options(game90 PRIVATE ${GAME90_WARNINGS})
target_link_libraries(game90 PRIVATE ${SDL2_TARGET} imgui_c_bridge)

if (GAME90_BUILD_MAP_EDITOR)
    add_executable(map_editor src/editor/map_editor.c)
    target_compile_options(map_editor PRIVATE ${GAME90_WARNINGS})
    target_link_libraries(map_editor PRIVATE ${SDL2_TARGET})
endif()

if (UNIX)
    target_link_libraries(game90 PRIVATE m)
    if (TARGET map_editor)
        target_link_libraries(map_editor PRIVATE m)
    endif()
endif()
