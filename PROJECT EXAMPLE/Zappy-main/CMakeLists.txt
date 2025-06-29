cmake_minimum_required(VERSION 3.0)
project(zappy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

add_subdirectory(Server)
add_subdirectory(GUI)
