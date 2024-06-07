cmake_minimum_required(VERSION 3.10)

project(MaxEmbed)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fmax-errors=2 -Wall -O3 -g")

add_subdirectory(client)
add_subdirectory(partition)