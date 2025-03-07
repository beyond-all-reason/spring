cmake_minimum_required(VERSION 3.0)
project(Catch2)

add_library(Catch2 catch_amalgamated.cpp)

target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})