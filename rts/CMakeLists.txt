# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

set(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")


option(TRACE_SYNC "Enable sync tracker" FALSE)
if (TRACE_SYNC)
	add_definitions(-DTRACE_SYNC)
endif (TRACE_SYNC)

option(SYNCDEBUG "Enable sync debugger (needs SYNCCHECK=true)" FALSE)
if (SYNCDEBUG)
	add_definitions(-DSYNCDEBUG)
	if     (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
		message(WARNING "It is recommended to set CMAKE_BUILD_TYPE to DEBUG for a SYNCDEBUG build")
	endif  ()
	if     (NOT SYNCCHECK)
		message(FATAL_ERROR "You need SYNCCHECK=TRUE for a SYNCDEBUG build")
	endif  ()
	if     (NOT TRACE_SYNC)
		message(WARNING "It is recommended to use TRACE_SYNC=TRUE for a SYNCDEBUG build")
	endif  ()
endif (SYNCDEBUG)

option(DEBUG_GLSTATE "enable GL_STATE_CHECKER" FALSE)
if(DEBUG_GLSTATE)
	add_definitions(-DDEBUG_GLSTATE)
endif()

### give error when not found
find_package_static(DevIL REQUIRED)

### Assemble common include dirs
include_directories(BEFORE lib)
include_directories(BEFORE lib/lua/include)
include_directories(${CMAKE_SOURCE_DIR}/include/AL)
include_directories(${SPRING_MINIZIP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/cereal/include)


### base64 from pr-downloader
list(APPEND engineCommonLibraries "prd::base64")

### smmalloc
list(APPEND engineCommonLibraries "smmalloc")

### Assemble common libraries
add_subdirectory(System/Sound)
if    (NO_SOUND)
	add_definitions(-DNO_SOUND)
endif (NO_SOUND)

### Find include directories and add platform specific libraries
if    (UNIX AND NOT MINGW)
	if    (PREFER_STATIC_LIBS)
		prefer_static_libs()
		find_library(C_LIBRARY c)
		find_library(MATH_LIBRARY m)
		#FIND_LIBRARY(PTHREAD_LIBRARY pthread)
		#FIND_LIBRARY(OMP_LIBRARY gomp) FIXME it's hidden in some subfolders
		unprefer_static_libs()

		list(APPEND engineCommonLibraries ${C_LIBRARY} ${MATH_LIBRARY})
		#list(APPEND engineCommonLibraries ${PTHREAD_LIBRARY} ${OMP_LIBRARY})
	endif (PREFER_STATIC_LIBS)

	# Needed for dynamically loading shared libraries (on some OS)
	list(APPEND engineCommonLibraries ${CMAKE_DL_LIBS})

	# Needed for backtrace* on some systems
	if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
		list(APPEND engineCommonLibraries execinfo)
	endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
endif (UNIX AND NOT MINGW)

find_package_static(ZLIB 1.2.7 REQUIRED)
list(APPEND engineCommonLibraries DevIL::IL)
list(APPEND engineCommonLibraries 7zip prd::jsoncpp ${SPRING_MINIZIP_LIBRARY} ZLIB::ZLIB Tracy::TracyClient)
list(APPEND engineCommonLibraries lua luasocket archives assimp fastgltf
	gflags_nothreads_static)
if(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
	list(APPEND engineCommonLibraries execinfo)
endif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
if (ENABLE_STREFLOP)
	list(APPEND engineCommonLibraries streflop)
endif ()

if (WIN32)
	list(APPEND engineCommonLibraries ${WIN32_LIBRARIES})
endif (WIN32)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	#tcmalloc on freebsd is broken, default disable it (#4754)
	set(USE_TCMALLOC_DEFAULT FALSE)
else()
	set(USE_TCMALLOC_DEFAULT TRUE)
endif()

find_package_static(TCMalloc)
option(USE_TCMALLOC "use tcmalloc (part of google's perftools)" ${USE_TCMALLOC_DEFAULT})
if    (USE_TCMALLOC AND TCMALLOC_LIBRARY)
	message(STATUS "Using tcmalloc")
	list(APPEND engineCommonLibraries ${TCMALLOC_LIBRARY})
endif (USE_TCMALLOC AND TCMALLOC_LIBRARY)


if(UNIX AND NOT (CMAKE_SYSTEM_NAME MATCHES "OpenBSD"))
	find_package_static(Libunwind 1.4.0 REQUIRED)
	prefer_static_libs()
	find_library(LZMA_LIBRARY lzma)
	unprefer_static_libs()
	list(APPEND engineCommonLibraries libunwind::libunwind ${LZMA_LIBRARY})
endif()



### Assemble engine sources
add_subdirectory(Game)
add_subdirectory(Lua)
add_subdirectory(ExternalAI)
add_subdirectory(Rendering)
add_subdirectory(aGui)
add_subdirectory(Rml)
add_subdirectory(Menu)
add_subdirectory(Map)
add_subdirectory(Net)
add_subdirectory(Sim)
#add_subdirectory(System) # this is already added in ../

make_global_var(engineSources
		${sources_engine_Net}
		${sources_engine_Lua}
		${sources_engine_Map}
		${sources_engine_Rendering}
		${sources_engine_aGui}
		${sources_engine_Rml}
		${sources_engine_RmlSolLua}
		${sources_engine_Menu}
		${sources_engine_System}
		${sources_engine_ExternalAI}
	)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/assimp/include) #FIXME: hack for rts/Rendering/Models/IModelParser.cpp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/fastgltf/include)

### Add headers for generated project files (e.g. Code::Blocks)
file(GLOB_RECURSE engineHeaders "*.h" "*.hpp" "*.inl")


add_subdirectory(builds)
