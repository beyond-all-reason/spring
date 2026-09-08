cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULES_SPRING "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_SPRING}")

# enable flag overrides for compilers
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_MODULES_SPRING}/OverrideFlags.cmake")

project(Spring)

# Force Unicode WinAPI
if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

find_package(Threads REQUIRED)

option(INSTALL_PORTABLE "Make a portable installation (instead of regular system installation)" TRUE)

# Allow to set PREFER_STATIC_LIBS from env, defaults to false
option(PREFER_STATIC_LIBS "Prefer static libraries when linking deps" $ENV{PREFER_STATIC_LIBS})

# dummy so that qtcreator shows all files (even those of windows)
if(CMAKE_VERSION VERSION_GREATER 3.1.2)
	add_library(platform_sources EXCLUDE_FROM_ALL "")
endif()

# includes rts/build/cmake/Util.cmake & rts/build/cmake/PreferStaticLibs.cmake
include(Util)
include(PreferStaticLibs)

# check for minimal gcc version
set(MIN_GCC_VER "7.0")
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${MIN_GCC_VER}")
		message(FATAL_ERROR "gcc >=${MIN_GCC_VER} required")
	else()
		message(STATUS "gcc ${CMAKE_CXX_COMPILER_VERSION} detected")
	endif()
endif()

# set some internal vars

if (NOT "${CMAKE_C_FLAGS}" STREQUAL "")
	message(WARNING "Using custom C_FLAGS: ${CMAKE_C_FLAGS} this build will very likely not sync in online mode!")
endif()

if (NOT "${CMAKE_CXX_FLAGS}" STREQUAL "")
	message(WARNING "Using custom CXX_FLAGS: ${CMAKE_CXX_FLAGS} this build will very likely not sync in online mode!")
endif()

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if    (CUSTOM_COMPILER_DEFINITIONS)
	add_definitions(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)


### set the default build type
if    (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RELWITHDEBINFO CACHE STRING
		"Choose the type of build, options are: RELWITHDEBINFO DEBUG RELEASE PROFILE"
		FORCE)
endif (NOT CMAKE_BUILD_TYPE)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
if(${CMAKE_BUILD_TYPE_UPPER} MATCHES "^DEBUG")
	set(DEBUG_BUILD TRUE)
else()
	set(DEBUG_BUILD FALSE)
endif()


# By default, the libraries that don't explicitly specify SHARED/STATIC are build statically.
# See https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html
option(BUILD_SHARED_LIBS "Build shared (dynamic) libraries" OFF)

# Tell windows to pass arguments around in @response files
# (32k arg limit workaround)
if    (WIN32)
	set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
	set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
endif (WIN32)


# XCode
if    (APPLE)
	option(MACOSX_BUNDLE "Compile spring to work as a Bundle.app" TRUE)
	add_definitions(${PIC_FLAG})
	if    (MACOSX_BUNDLE)
		add_definitions(-DMACOSX_BUNDLE)
	endif (MACOSX_BUNDLE)
else  (APPLE)
	set(MACOSX_BUNDLE FALSE)
endif (APPLE)

### Compiler flags and defines based on build type
include(TestCXXFlags)

## 64-bit architecture check
set(MARCH_FLAG ${MARCH} CACHE STRING "CPU optimization (use `generic` for generic optimization)")
if    (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "RecoilEngine requires a 64-bit build environment.")
endif (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
set(MARCH_BITS 64 CACHE INTERNAL "" FORCE)

# Detect architecture
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|armv8|ARM64|AARCH64")
	set(IS_ARM64_BUILD TRUE)
	message(STATUS "Detected ARM64 architecture: ${CMAKE_SYSTEM_PROCESSOR}")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
	set(IS_ARM64_BUILD FALSE)
	message(STATUS "Detected x86-64 architecture: ${CMAKE_SYSTEM_PROCESSOR}")
else()
	message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}. Only ARM64 and x86-64 are supported.")
endif()

message(STATUS "Building Spring on a 64-bit environment")
set(BUILD_BITS 64 CACHE INTERNAL "Target architecture bitness" FORCE)


### Install paths (relative to CMAKE_INSTALL_PREFIX)
if    (UNIX AND NOT MINGW)
	if (INSTALL_PORTABLE)
		set(BINDIR  "."   CACHE STRING "Where to install binaries")
		set(LIBDIR  "."   CACHE STRING "Where to install libraries")
		set(DATADIR "."   CACHE STRING "Where to install game content")
		set(DOCDIR "doc" CACHE STRING "Where to install basic project documentation (README, LICENSE, etc.)")
	else()
		set(BINDIR  "bin"                CACHE STRING "Where to install binaries")
		set(LIBDIR  "lib"                CACHE STRING "Where to install libraries")
		set(DATADIR "share/games/spring" CACHE STRING "Where to install game content")
		set(DOCDIR  "share/doc/spring-VERSION" CACHE STRING "Where to install basic project documentation (README, LICENSE, etc.)") # TODO set correct version here, and re-enable userdocs in doc/CMakeLists.txt
	endif()

	set(APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set(PIXMAPS_DIR      "share/pixmaps"      CACHE STRING "Where to install icons")
	set(MIME_DIR         "share/mime"         CACHE STRING "Where MIME definitions are located")
else  (UNIX AND NOT MINGW)
	set(BINDIR  "." CACHE STRING "Where to install binaries")
	set(LIBDIR  "." CACHE STRING "Where to install libraries")
	set(DOCDIR  "doc" CACHE STRING "Where to install basic project documentation (README, LICENSE, etc.)")
	set(DATADIR "." CACHE STRING "Where to install game content")

	if    (NOT MINGWLIBS)
		if (DEFINED ENV{MINGWLIBS})
			set(MINGWLIBS $ENV{MINGWLIBS} CACHE PATH "Location of the mingwlibs package")
		elseif(MSVC)
			set(MINGWLIBS ${CMAKE_SOURCE_DIR}/vclibs64 CACHE PATH "Location of the vclibs package")
		else()
			set(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs64 CACHE PATH "Location of the mingwlibs package")
		endif()
	endif ()
	if    (EXISTS "${MINGWLIBS}" AND IS_DIRECTORY "${MINGWLIBS}")
		if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17.0")
			#rectifying dll search issue present in CMake >= 3.17.0, see https://discourse.cmake.org/t/findzlib-fails-starting-with-cmake-3-17/6046/4
			set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll" ${CMAKE_FIND_LIBRARY_SUFFIXES})
		endif()

		# MinGW needs an additional library, mwindows
		# It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -lmwindows
		# (Actually on second look, I think it only needs one of the m* libraries.)
		if(NOT MSVC)
			set(MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
		endif(NOT MSVC)
		set(SDL2_INCLUDE_DIR "${MINGWLIBS}/include/SDL2")
		set(CMAKE_PREFIX_PATH ${MINGWLIBS})
		include_directories(${MINGWLIBS}/include)
		set(CMAKE_LIBRARY_PATH ${MINGWLIBS}/dll ${MINGWLIBS}/lib)

		# some cross-compile fixes
		set(CMAKE_FIND_ROOT_PATH ${MINGWLIBS} ${CMAKE_FIND_ROOT_PATH})
		set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
		set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	else()
		message(WARNING "Compiling for windows. MINGWLIBS '${MINGWLIBS}' is not a valid directory, trying system libs")
	endif ()


	add_definitions(-D_WIN32_WINNT=0x0601) ## Win7 is minimum due to processor group affinity
	add_definitions(-D__USE_MINGW_ANSI_STDIO=1) ## use __mingw_sprintf instead of __builtin_sprintf (it's used by lua in a synced way, and builtin doesn't sync)

	if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		add_definitions(-DWIN32)
	endif()

	if(MSVC)
		set (CMAKE_VS_JUST_MY_CODE_DEBUGGING 1)
		add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
		add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
		add_compile_options(/bigobj)
		add_compile_options(/utf-8) ## new FMT refuses to compile w/o it
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>) ## define __cplusplus value properly / https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170
		add_definitions(-D_ITERATOR_DEBUG_LEVEL=0) ## attempt to make debug MSVC builds faster
		#set(EXE_FLAGS WIN32)
	endif()
endif (UNIX AND NOT MINGW)

## Artificial Intelligence
# In theory, it should be possible to set this to LIBDIR instead
# (next two are relative to CMAKE_INSTALL_PREFIX)
set(AI_LIBS_DIR "${DATADIR}"     CACHE STRING "Where to install Skirmish AI libraries")
set(AI_DATA_DIR "${AI_LIBS_DIR}" CACHE STRING "Where to install Skirmish AI additional files (eg. configuration)")
set(AI_TYPES    "NATIVE"            CACHE STRING "Which AI Interfaces (and Skirmish AIs using them) to build [ALL|NATIVE|NONE]")

## DataDirs
set(BUILTIN_DATADIRS "")
if    (NOT WIN32 AND NOT MACOSX_BUNDLE)
	set(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	set(SPRING_DATADIR_INTERNAL "")
	if    (SPRING_DATADIR)
		set(SPRING_DATADIR_INTERNAL "${SPRING_DATADIR}${PATH_DELIM_T}")
	endif (SPRING_DATADIR)
	if    (CMAKE_INSTALL_PREFIX)
		set(SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${DATADIR}")
	endif (CMAKE_INSTALL_PREFIX)
	set(BUILTIN_DATADIRS "${SPRING_DATADIR_INTERNAL}")
endif (NOT WIN32 AND NOT MACOSX_BUNDLE)
if    (NOT "${AI_LIBS_DIR}" STREQUAL "${DATADIR}")
	# If the AI library dir is not the same as the normal data dir,
	# add it to the internal list of data dirs
	set(BUILTIN_DATADIRS "${BUILTIN_DATADIRS}${PATH_DELIM_T}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${AI_LIBS_DIR}")
endif (NOT "${AI_LIBS_DIR}" STREQUAL "${DATADIR}")
if    (NOT "${AI_DATA_DIR}" STREQUAL "${DATADIR}" AND NOT "${AI_DATA_DIR}" STREQUAL "${AI_LIBS_DIR}")
	# If the AI data dir is not the same as the normal data dir
	# nor the AI library dir, add it to the internal list of data dirs
	set(BUILTIN_DATADIRS "${BUILTIN_DATADIRS}${PATH_DELIM_T}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${AI_DATA_DIR}")
endif (NOT "${AI_DATA_DIR}" STREQUAL "${DATADIR}" AND NOT "${AI_DATA_DIR}" STREQUAL "${AI_LIBS_DIR}")
add_definitions(-DSPRING_DATADIR="${BUILTIN_DATADIRS}")



# Spring Related USE-Flags
set(LOG_LEVEL "DEFAULT" CACHE STRING
		"The minimum log level.
		Log messages below this level
		will not be compiled into the binary.
		Available levels:
		* DEFAULT (DEBUG on debug builds, INFO on release builds)
		* DEBUG (log everything)
		* INFO
		* WARNING (only use this if you know what you are doing!)")
if    (NOT "${LOG_LEVEL}" STREQUAL "DEFAULT")
	add_definitions(-D_LOG_LEVEL_MIN=LOG_LEVEL_${LOG_LEVEL})
endif (NOT "${LOG_LEVEL}" STREQUAL "DEFAULT")

option(SYNCCHECK "Check sync during gameplay (Required for online play!)" TRUE)
if    (SYNCCHECK)
	add_definitions(-DSYNCCHECK)
endif (SYNCCHECK)

option(SYNC_HISTORY "Save sync history (for semidebug builds)" FALSE)
set(SYNC_HISTORY ON CACHE BOOL "Force enable save sync history" FORCE)
if    (SYNC_HISTORY)
	add_definitions(-DSYNC_HISTORY)
endif (SYNC_HISTORY)

option(NO_CREG "Disable creg support" FALSE)
if    (NO_CREG)
	add_definitions(-DNOT_USING_CREG)
endif (NO_CREG)

option(NO_SOUND "No sound output support" FALSE)

if    (WIN32)
	## internal AVI recorder (w/o sound atm)
	option(NO_AVI "Disable in-game video recording" FALSE)
	if    (NOT NO_AVI)
		add_definitions(-DAVI_CAPTURING)
	endif (NOT NO_AVI)

	if   (NOT MSVC)
		## stdout console
		option(WINDOWS_CONSOLE "Show a visible cmd.exe with stdout when spring runs" FALSE)
		if    (NOT WINDOWS_CONSOLE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
		endif (NOT WINDOWS_CONSOLE)
	endif(NOT MSVC)
endif (WIN32)

option(USE_ASAN "Build/link with address sanitizer enabled" FALSE)
if    (USE_ASAN)
	if (UNIX AND NOT MINGW)
		# -fsanitize=address: Enable AddressSanitizer for detecting memory errors
		# -fPIC: Position Independent Code required for shared libraries with ASAN
		# -fno-omit-frame-pointer: Keep frame pointers for better stack traces in error reports
		# -fno-optimize-sibling-calls: Prevent tail call optimization to preserve full call chains
		# -O2: Use moderate optimization for better ASAN error detection and reasonable performance
		add_compile_options("-fsanitize=address" "-fPIC" "-fno-omit-frame-pointer" "-fno-optimize-sibling-calls" "-O2")
		add_link_options("-fsanitize=address")
	endif (UNIX AND NOT MINGW)
	if (MSVC)
		add_compile_options("/fsanitize=address")
	endif (MSVC)
endif (USE_ASAN)

### Tracy related stuff
option(RECOIL_DETAILED_TRACY_ZONING "Enable additional detailed tracy zones (only enable this for testing/debugging)" FALSE)
if    (RECOIL_DETAILED_TRACY_ZONING)
	add_definitions(-DRECOIL_DETAILED_TRACY_ZONING)
else  (RECOIL_DETAILED_TRACY_ZONING)
	remove_definitions(-DRECOIL_DETAILED_TRACY_ZONING)
endif (RECOIL_DETAILED_TRACY_ZONING)

# Note the missing REQUIRED, as headless & dedi may not depend on those.
#  So req. checks are done in the build target's CMakeLists.txt.
find_package(SDL2 MODULE)

find_package_static(DevIL 1.8.0 REQUIRED)

if    (WIN32)
	find_package(Windres REQUIRED)
	set(CMAKE_RC_COMPILER ${WINDRES_BIN})
	find_package(Win32Libs REQUIRED)
endif (WIN32)

if     (WIN32)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_MODULE_PREFIX "")
elseif (UNIX)
	# unix specifics
else   (UNIX)
	# mac
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	include_directories("${COREFOUNDATION_LIBRARY}/Headers")
endif  (WIN32)

option(USE_VALGRIND "Try to support Valgrind" FALSE)
if    (USE_VALGRIND)
	find_package(Valgrind)
	if    (VALGRIND_FOUND)
		add_definitions(-DUSE_VALGRIND)
	else  (VALGRIND_FOUND)
		message(SEND_ERROR "USE_VALGRIND is requested, but Valgrind is not found")
	endif (VALGRIND_FOUND)
endif (USE_VALGRIND)

include (CheckSymbolExists)
check_symbol_exists(strnlen string.h HAVE_STRNLEN)
if (NOT HAVE_STRNLEN)
	message(FATAL_ERROR "this compiler/environment isn't supported, the function strnlen is missing!")
endif()

### Add Link-Time-Optimization flags, if supported (GCC >= 4.5) and enabled
option(LTO "Link Time Optimizations (LTO)" FALSE)
if    (LTO)
	if    (NOT LTO_FLAGS)
		message(WARNING "Tried to enable LTO, but compiler doesn't support it!")
	endif (NOT LTO_FLAGS)
	set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS}           ${LTO_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${LTO_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LTO_FLAGS}")
endif (LTO)


### Memory Access Protection (BoundsChecking)
option(DEBUG_BOUNDSCHECK "Array/Pointer BoundsChecking a.k.a. buffer overflow protection (debug)" FALSE)
if    (DEBUG_BOUNDSCHECK)
	if    (NOT MPX_FLAGS)
		message(WARNING "Tried to enable BoundsCheck, but compiler doesn't support it!")
	endif (NOT MPX_FLAGS)
	set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS}           ${MPX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${MPX_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${MPX_FLAGS}")
endif (DEBUG_BOUNDSCHECK)


### ld.gold
option(COMPRESS_DEBUG_SYMBOLS "Compresses the DebugSymbols (needs ld.gold, beware: slow)" FALSE)
if    (LDGOLD_FOUND)
	message(STATUS "Found ld.gold")
	if    (COMPRESS_DEBUG_SYMBOLS)
		message(STATUS "Compressing debug symbols")
		set(LDGOLD_LINKER_FLAGS " -Wl,--compress-debug-sections=zlib ${LDGOLD_LINKER_FLAGS}")
	endif (COMPRESS_DEBUG_SYMBOLS)

	set(CMAKE_LINKER LD_GOLD)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LDGOLD_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LDGOLD_LINKER_FLAGS}")
else  (LDGOLD_FOUND)
	message(WARNING "You are using ld.bfd, it's much slower than ld.gold.")
endif (LDGOLD_FOUND)

add_definitions(-D_GLIBCXX_USE_NANOSLEEP) # issue in std::chrono with gentoo and few other distros


### our custom OpenMP replacement
option(THREADPOOL "Enable ThreadPools" TRUE)
if (THREADPOOL)
	add_definitions(-DTHREADPOOL)
endif(THREADPOOL)


### warnings
if(NOT MSVC)
	### SigNan
	option(DEBUG_SIGNAL_NANS "Enable NaN-Signaling" ${DEBUG_BUILD})
	if (DEBUG_SIGNAL_NANS AND CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsignaling-nans")
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		## warning: space between quotes and suffix is deprecated in C++23
		## [[nodiscard]] constexpr uint32_t operator"" _hs(const char* str, std::size_t) noexcept
		## it's been quite annoying since we switched to C++23
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-literal-operator")
	endif()

	### CompileTime Warnings
	set(COMMON_WARNINGS "${COMMON_WARNINGS} -Wformat -Wformat-security")

	option(DEBUG_MAX_WARNINGS "Enable verbose compile" ${DEBUG_BUILD})
	if    (DEBUG_MAX_WARNINGS)
		# This would produce the maximum on warnings
		set(DEBUG_WARNINGS "-Wall -Wconversion ${COMMON_WARNINGS}")
	else  (DEBUG_MAX_WARNINGS)
		# This should produce a sane amount of warnings
		set(DEBUG_WARNINGS "-Wall -Wno-sign-compare ${COMMON_WARNINGS}")
	endif (DEBUG_MAX_WARNINGS)
endif(NOT MSVC)


option(REPORT_LUANAN "report nans/infinities in lua" TRUE)
if(REPORT_LUANAN)
        add_definitions(-DREPORT_LUANAN)
endif()



### streflop
option(ENABLE_STREFLOP "build Spring with streflop support (REQUIRED FOR MULTIPLAYER)" TRUE)

if (ENABLE_STREFLOP)
	set(STREFLOP_AUTO ON CACHE BOOL "Enable STREFLOP autodetection" FORCE)
else ()
	add_definitions(-DNOT_USING_STREFLOP)
endif ()

### Asio
add_definitions(-DASIO_STANDALONE)

### sse2neon
if (IS_ARM64_BUILD)
	add_definitions(-DSSE2NEON)
	add_definitions(-DSSE2NEON_SUPPRESS_WARNINGS)

#   These are mentioned by sse2neon docs, do *not* use them. They will cause desyncs!
#	add_definitions(-DSSE2NEON_PRECISE_MINMAX)
#	add_definitions(-DSSE2NEON_PRECISE_DIV)
#	add_definitions(-DSSE2NEON_PRECISE_SQRT)
#	add_definitions(-DSSE2NEON_PRECISE_DP)
endif()


### compiler specific flags
set(CXX_FLAGS_DEBUG_ADDITIONAL "")
if (CMAKE_COMPILER_IS_GNUCXX)
	if    (UNIX)
		check_and_add_flags(CMAKE_CXX_FLAGS "-fdiagnostics-color=auto")
	endif (UNIX)

	if (NOT MARCH_FLAG)
		set(MARCH_FLAG "generic")
	endif (NOT MARCH_FLAG)

	if (MARCH_FLAG STREQUAL "")
		set(MARCH_FLAG "generic")
	endif (MARCH_FLAG STREQUAL "")

	if (NOT MARCH_FLAG STREQUAL "generic")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${MARCH_FLAG} -mtune=${MARCH_FLAG}") #march/mtune define SSE flags
	elseif (IS_ARM64_BUILD)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+simd ${FP_CONTRACT_FLAG}")
	else (NOT MARCH_FLAG STREQUAL "generic")
		set(FALLBACK_SSE_FLAGS "-msse")
		#set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mmmx")

		# worth to test if sync
		#set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mpopcnt -mlzcnt -mabm")

		# disable rest
		#set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mno-3dnow") tests showed it might sync
		set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-sse4a")
		set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mno-avx -mno-fma -mno-fma4 -mno-xop -mno-lwp")
		set(FALLBACK_SSE_FLAGS "${FALLBACK_SSE_FLAGS} -mno-avx2")

		set(FALLBACK_MARCH "x86-64")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${FALLBACK_MARCH} -mtune=generic ${FALLBACK_SSE_FLAGS}")
	endif (NOT MARCH_FLAG STREQUAL "generic")

	if (NOT IS_ARM64_BUILD)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SSE_FLAGS} -mfpmath=sse") #SSE_FLAGS are now user input only
	endif ()
elseif (MSVC)
	set(MSVC_CXX_FLAGS "/arch:SSE2") #default, but still set explicitly
	# silence warnings
	add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	# Clang on ARM64: no SSE flags needed, use armv8-a with NEON SIMD
	if (IS_ARM64_BUILD)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FP_CONTRACT_FLAG}")
	else()
		# Clang on x86_64: use SSE like GCC
		if (NOT MARCH_FLAG OR MARCH_FLAG STREQUAL "" OR MARCH_FLAG STREQUAL "generic")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -mfpmath=sse")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${MARCH_FLAG} -mtune=${MARCH_FLAG}")
		endif()
	endif()
else()
	message(WARNING "unknown compiler")
endif (CMAKE_COMPILER_IS_GNUCXX)

set(CXX_DEBUG_DEFINES          "-DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CXX_RELWITHDEBINFO_DEFINES "-DNDEBUG")
set(CXX_RELEASE_DEFINES        "-DNDEBUG")
set(CXX_PROFILE_DEFINES        "-DNDEBUG -DPROFILE")

#### build types
if    (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	set(MSVC_CXX_FLAGS "${MSVC_CXX_FLAGS} /wd4146 /wd4711")

	set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG}                       /MP ${MSVC_CXX_FLAGS} ${CXX_DEBUG_DEFINES}")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Ob2 /GL /MP ${MSVC_CXX_FLAGS} ${CXX_RELWITHDEBINFO_DEFINES}")
	set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE}        /O2 /Ob2 /GL /MP ${MSVC_CXX_FLAGS} ${CXX_RELEASE_DEFINES}")
	set(CMAKE_CXX_FLAGS_PROFILE         "${CMAKE_CXX_FLAGS_PROFILE}        /O2 /Ob2 /GL /MP ${MSVC_CXX_FLAGS} ${CXX_PROFILE_DEFINES}")

	set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	set(CMAKE_CXX_FLAGS_PROFILE               "${CMAKE_CXX_FLAGS_PROFILE} /LTCG")
else  (MSVC)
	if (CMAKE_COMPILER_IS_GNUCXX)
		# -fsingle-precision-constant: treat FP constants as float, not double (GCC-only)
		# -frounding-math: honor runtime rounding mode (GCC-only)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
		# Clang does not support -fsingle-precision-constant.
		# -frounding-math is supported on Clang 14+; use it if available.
		check_and_add_flags(CMAKE_CXX_FLAGS "-frounding-math")
	endif()
	# Experiment with frame pointers to see whether it improves the quality of the stack unwinds.
	# We're having trouble getting meaningful information form crash reports. Trying this before
	# moving onto more performance-impacting methods.
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
	# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IEEE_FP_FLAG} -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
	if (UNIX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	endif (UNIX)
	if    (MINGW)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mthreads")
	endif (MINGW)

	#set(CMAKE_CXX_FLAGS_DEBUG           "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0     ${DEBUG_WARNINGS}  ${CXX_DEBUG_DEFINES}")
	#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2     ${COMMON_WARNINGS} ${CXX_RELWITHDEBINFO_DEFINES}")
	#set(CMAKE_CXX_FLAGS_RELEASE         "                              -O2     ${COMMON_WARNINGS} ${CXX_RELEASE_DEFINES}")
	#set(CMAKE_CXX_FLAGS_PROFILE         "                              -O2 -pg ${DEBUG_WARNINGS}  ${CXX_PROFILE_DEFINES}")

	#rely on cmake defaults
	set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_WARNINGS}  ${CXX_DEBUG_DEFINES}")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_WARNINGS} ${CXX_RELWITHDEBINFO_DEFINES}")
	set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_WARNINGS} ${CXX_RELEASE_DEFINES}")
	set(CMAKE_CXX_FLAGS_PROFILE         "${CMAKE_CXX_FLAGS_PROFILE} ${DEBUG_WARNINGS}  ${CXX_PROFILE_DEFINES}")


	set(CMAKE_MODULE_LINKER_FLAGS_PROFILE "-pg")
	set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg")

	#set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3") #cmake has -g it by default
	#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb3") #cmake has -g it by default


	if     (MINGW)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -Wl,--enable-auto-import")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -Wl,--enable-auto-import")
	endif  (MINGW)
endif (MSVC)

message(STATUS "Using CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}")

if (CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")
	message (STATUS "Using CMAKE_CXX_FLAGS_RELWITHDEBINFO:${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
elseif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
	message (STATUS "Using CMAKE_CXX_FLAGS_DEBUG:${CMAKE_CXX_FLAGS_DEBUG}")
elseif (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	message (STATUS "Using CMAKE_CXX_FLAGS_RELEASE:${CMAKE_CXX_FLAGS_RELEASE}")
elseif (CMAKE_BUILD_TYPE STREQUAL "PROFILE")
	message (STATUS "Using CMAKE_CXX_FLAGS_PROFILE:${CMAKE_CXX_FLAGS_PROFILE}")
endif (CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")


#######################################
# Source Files
#

# Sub-parts of the build system may add dependencies to this target
add_custom_target(generateSources)

# Make sure git is found if git is used
if   (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	find_package(Git REQUIRED)
endif(EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Generate the version files
set(VERSION_GENERATE_DIR "${CMAKE_BINARY_DIR}")

set(VERSION_GENERATE_SOURCES
		"${VERSION_GENERATE_DIR}/src-generated/engine/System/VersionGenerated.h"
		"${VERSION_GENERATE_DIR}/VERSION"
	)

set_source_files_properties(
	${VERSION_GENERATE_SOURCES}
	PROPERTIES GENERATED TRUE
)

# Find git metadata files to depend on, so version regenerates when commits change
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	if(EXISTS "${CMAKE_SOURCE_DIR}/.git/index")
		list(APPEND VERSION_GIT_DEPS "${CMAKE_SOURCE_DIR}/.git/index")
	endif()
	if(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")
		list(APPEND VERSION_GIT_DEPS "${CMAKE_SOURCE_DIR}/.git/HEAD")
	endif()
endif()

# Declare template files as inputs (for proper dependency tracking)
set(VERSION_TEMPLATE_FILES
	"${CMAKE_SOURCE_DIR}/rts/System/VersionGenerated.h.template"
	"${CMAKE_SOURCE_DIR}/VERSION.template"
)

# Use add_custom_command(OUTPUT ...) to properly declare file outputs.
# This creates real file-level dependencies (not just order-only deps),
# fixing race conditions where compilation starts before version generation.
# Remove any existing generated files first to ensure fresh generation.
add_custom_command(
	OUTPUT ${VERSION_GENERATE_SOURCES}
	COMMAND "${CMAKE_COMMAND}"
		"-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
		"-DCMAKE_MODULES_SPRING=${CMAKE_MODULES_SPRING}"
		"-DGENERATE_DIR=${VERSION_GENERATE_DIR}"
		"-DVERSION_ADDITIONAL=${VERSION_ADDITIONAL}"
		-P "${CMAKE_MODULES_SPRING}/ConfigureVersion.cmake"
		"${VERSION_GENERATE_DIR}"
	DEPENDS ${VERSION_TEMPLATE_FILES} ${VERSION_GIT_DEPS}
	COMMENT "  Configuring Version files ..."
	VERBATIM
)

# Target for dependency purposes
add_custom_target(generateVersionFiles DEPENDS ${VERSION_GENERATE_SOURCES})

add_dependencies(generateSources generateVersionFiles)


### Where the other CMakeLists.txt are

# As the libs are required by all the other sub-projects, we add them first
# Options that affect lib selection must be declared here, before rts/lib is processed
option(USE_MIMALLOC "Use mimalloc memory allocator" TRUE)
add_subdirectory(rts/lib)

# Fix for MinGW assembler limitations with large object files
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

if    (NOT "${AI_TYPES}" STREQUAL "NONE")
	add_subdirectory(AI)
endif (NOT "${AI_TYPES}" STREQUAL "NONE")

# As tools/unitsync/ depends on sources from here, we add this one first
add_subdirectory(rts/System)

# additional tools
add_subdirectory(tools)

# user-docs
add_subdirectory(doc)

### Handle game content
### - Pack base-files
### - Install base-files
### - Install freedesktop stuff
### - Install mingw DLLs
add_subdirectory(cont)

# As rts/builds/* depends on most other stuff, we add this one as last
add_subdirectory(rts)

# Unit tests
# this has to be in root CMakeLists.txt
option(BUILD_TESTING "Build engine tests" ON)
if (BUILD_TESTING)
	enable_testing()
	add_subdirectory(test)
endif ()

if (INSTALL_PORTABLE)
	install(DIRECTORY DESTINATION games)
	install(DIRECTORY DESTINATION maps)
	file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/springsettings_portable.cfg)  # empty settings file for portable install
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/springsettings_portable.cfg DESTINATION . RENAME springsettings.cfg)
endif (INSTALL_PORTABLE)
