set(SOURCE_ROOT ../../..)

set(archives_sources
	BufferedArchive.cpp
	DirArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	VirtualArchive.cpp
	ZipArchive.cpp
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkConsole}
	${SOURCE_ROOT}/System/TimeUtil.cpp
)

# Can't remove definitions per target
remove_definitions(-DTHREADPOOL)

# This double library below is needed because engine build is cursed:
# - It wants to within a single CMake invocation build 4 different variants
#   of the same code: dedicated, unitsync, headless, legacy. All libraries
#   that need different properties need to then be create in multiple variants
# - There is extremely poor separation of different engine pieces into well
#   defined libraries (because of the previous problem) and main targets are
#   just created by copying massive lists of source files which allows for
#   a spaghetti of inter dependencies with everything linking against
#   everything.
#
# And that implies:
# - We need to define 2 separate targets with different compilation options
#   because it will be linked against targets with different requirements.
# - Both of those static libraries are *not* self contained, but require the
#   targets it will be linked against to export compatible symbols.

add_library(archives STATIC ${archives_sources})
if (THREADPOOL)
	target_compile_definitions(archives PRIVATE -DTHREADPOOL)
endif()
target_include_directories(archives
	PRIVATE ${SOURCE_ROOT} ${SPRING_MINIZIP_INCLUDE_DIR})
target_link_libraries(archives
	PRIVATE 7zip ${SPRING_MINIZIP_LIBRARY} Tracy::TracyClient)

add_library(archives_nothreadpool STATIC ${archives_sources})
target_compile_options(archives_nothreadpool PRIVATE ${PIC_FLAG})
target_include_directories(archives_nothreadpool
	PRIVATE ${SOURCE_ROOT} ${SPRING_MINIZIP_INCLUDE_DIR})
target_link_libraries(archives_nothreadpool
	PRIVATE 7zip ${SPRING_MINIZIP_LIBRARY})
