cmake_minimum_required(VERSION 3.0)
project(zappy_server C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-format-security -g3")

add_subdirectory(libs)

# Add your source files
set(
    SOURCES
    src/main.c
    src/server.c
    src/netctl.c
    src/options.c
    src/actions.c
    src/game/game.c
    src/game/map_handling.c
    src/game/ai_commandes.c
    src/game/gui_communication.c
    src/timeout.c
)

# Create an executable target
add_executable(zappy_server ${SOURCES})
target_include_directories(zappy_server PRIVATE inc)
target_link_libraries(zappy_server PRIVATE list m)
