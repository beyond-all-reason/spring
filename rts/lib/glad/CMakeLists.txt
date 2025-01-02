cmake_minimum_required(VERSION 3.0)
project(Glad)

if    (UNIX AND NOT MINGW)
	add_library(glad glad.c glad_glx.c)
else  (UNIX AND NOT MINGW)
	add_library(glad glad.c)
endif (UNIX AND NOT MINGW)

target_include_directories(glad PUBLIC /)