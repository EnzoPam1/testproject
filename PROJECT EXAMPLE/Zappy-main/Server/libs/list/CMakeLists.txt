cmake_minimum_required(VERSION 3.15)

project(list C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g3")

set(
SOURCES
    src/list_add_elem_at_back.c
    src/list_add_elem_at_front.c
    src/list_add_elem_at_position.c
    src/list_clear.c
    src/list_del_elem_at_back.c
    src/list_del_elem_at_front.c
    src/list_del_elem_at_position.c
    src/list_dump.c
    src/list_get_elem_at_back.c
    src/list_get_elem_at_front.c
    src/list_get_elem_at_position.c
    src/list_get_size.c
    src/list_is_empty.c
    src/list_sort.c
)

add_library(list STATIC ${SOURCES})
target_include_directories(list PUBLIC .)
