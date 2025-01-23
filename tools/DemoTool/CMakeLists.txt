# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/my/sub/dir/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

set(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")

find_package(ZLIB REQUIRED)

include_directories(${ENGINE_SRC_ROOT_DIR})
include_directories(${CMAKE_BINARY_DIR}/src-generated/engine)
include_directories(${gflags_BINARY_DIR}/include)

add_definitions(-DTOOLS)

set(PLATFORM_SRCS "")
set(PLATFORM_LIBS "")

if     (WIN32)
	list(APPEND PLATFORM_SRCS "${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/Hardware.cpp")
	list(APPEND PLATFORM_SRCS "${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/WinVersion.cpp")

	list(APPEND PLATFORM_LIBS "iphlpapi")
elseif (UNIX)
	list(APPEND PLATFORM_SRCS "${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/Hardware.cpp")
	list(APPEND PLATFORM_LIBS "${CMAKE_DL_LIBS}")
endif  (WIN32)

set(demoToolSpringSources
	${ENGINE_SRC_ROOT_DIR}/Game/GameVersion.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/Players/PlayerStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileSystem.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileSystemAbstraction.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/GZFileHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Misc.cpp
	${ENGINE_SRC_ROOT_DIR}/System/CRC.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Sync/SHA512.cpp
	${ENGINE_SRC_ROOT_DIR}/System/StringUtil.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Net/RawPacket.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/Backend.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFilter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFormatter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/FramePrefixer.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogSinkHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogUtil.c
	${ENGINE_SRC_ROOT_DIR}/System/Log/ConsoleSink.cpp
	${ENGINE_SRC_ROOT_DIR}/System/SafeCStrings.c	
	${ENGINE_SRC_ROOT_DIR}/System/creg/Serializer.cpp
	${ENGINE_SRC_ROOT_DIR}/System/creg/VarTypes.cpp
	${ENGINE_SRC_ROOT_DIR}/System/creg/creg.cpp
)

list(APPEND demoToolSpringSources ${PLATFORM_SRCS})

add_executable(demotool EXCLUDE_FROM_ALL DemoTool.cpp ${demoToolSpringSources})
if (MINGW)
	# To enable console output/force a console window to open
	set_target_properties(demotool PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
endif (MINGW)

target_link_libraries(demotool
		${SPRING_MINIZIP_LIBRARY}
		gflags_nothreads_static
		7zip
		${ZLIB_LIBRARY}
		${PLATFORM_LIBS}
		Tracy::TracyClient
	)
add_dependencies(demotool generateVersionFiles)
