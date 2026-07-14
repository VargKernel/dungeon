cmake_minimum_required(VERSION 3.14)
project(dungeon)

set(CMAKE_CXX_STANDARD 17)

add_executable(dungeon
    main.cpp
    core/level.cpp
    core/renderer.cpp
    core/display.cpp
    core/console.cpp
    core/app.cpp
    core/tiles.cpp
    game/game_mode.cpp
    game/game_controls.cpp
    game/game_panel.cpp
    game/interactions.cpp
    editor/editor_mode.cpp
    editor/editor_controls.cpp
    editor/editor_panel.cpp
    settings/settings_mode.cpp
    settings/settings_panel.cpp
)

target_include_directories(dungeon PRIVATE core game editor settings)

if(WIN32)

    set(SDL_ROOT ${CMAKE_SOURCE_DIR}/win_libs/x86_64)

    target_include_directories(dungeon PRIVATE
        ${SDL_ROOT}/include
    )

    target_link_directories(dungeon PRIVATE
        ${SDL_ROOT}/lib
    )

    target_compile_definitions(dungeon PRIVATE SDL_MAIN_HANDLED)

    target_link_libraries(dungeon
        SDL2
        SDL2_image
        SDL2_ttf
    )

else()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_image SDL2_ttf)

    target_include_directories(dungeon PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(dungeon ${SDL2_LIBRARIES})

endif()

add_custom_command(TARGET dungeon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E rm -f
        "${CMAKE_CURRENT_BINARY_DIR}/world"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/world"
        "${CMAKE_CURRENT_BINARY_DIR}/world"
    COMMENT "Linking assets/ and world/ into build directory")
