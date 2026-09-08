INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts/lib)

ADD_SUBDIRECTORY(fmt)
set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
ADD_SUBDIRECTORY(glad)
ADD_SUBDIRECTORY(smmalloc)

# Mimalloc - only build if USE_MIMALLOC is enabled
if (USE_MIMALLOC)
	# Disable mimalloc's built-in override - we provide our own custom override
	set(MI_OVERRIDE OFF CACHE BOOL "Disable mimalloc built-in override" FORCE)
	# Disable mimalloc tests and shared library to speed up build
	set(MI_BUILD_TESTS OFF CACHE BOOL "Disable mimalloc tests" FORCE)
	set(MI_BUILD_SHARED OFF CACHE BOOL "Disable mimalloc shared library" FORCE)
	set(MI_BUILD_STATIC ON CACHE BOOL "Enable mimalloc static library" FORCE)

	# Integrate ASAN/Valgrind tracking with mimalloc
	# When the engine is built with USE_ASAN, enable MI_TRACK_ASAN for mimalloc
	# When the engine is built with USE_VALGRIND, enable MI_TRACK_VALGRIND for mimalloc
	if (USE_ASAN)
		set(MI_TRACK_ASAN ON CACHE BOOL "Enable mimalloc ASAN support" FORCE)
	endif()
	if (VALGRIND_FOUND)
		set(MI_TRACK_VALGRIND ON CACHE BOOL "Enable mimalloc Valgrind support" FORCE)
	endif()

	if(MINGW)
		# Temporarily remove _WIN32_WINNT to avoid redefinition warning with mimalloc
		# mimalloc defines _WIN32_WINNT=0x600 for MinGW in its CMakeLists.txt
		remove_definitions(-D_WIN32_WINNT=0x0601)

		# Suppress visibility attribute warnings for ALL mimalloc targets.
		add_compile_options(-Wno-attributes)
	endif()

	ADD_SUBDIRECTORY(mimalloc)

	if(MINGW)
		# Restore _WIN32_WINNT definition after mimalloc
		add_definitions(-D_WIN32_WINNT=0x0601)
	endif()
endif()

ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luasocket)
ADD_SUBDIRECTORY(minizip)
ADD_SUBDIRECTORY(nowide)
ADD_SUBDIRECTORY(headlessStubs)
if (ENABLE_STREFLOP)
ADD_SUBDIRECTORY(streflop)
endif ()

SET(GFLAGS_BUILD_SHARED_LIBS          FALSE)
SET(GFLAGS_BUILD_STATIC_LIBS          TRUE)
SET(GFLAGS_BUILD_gflags_LIB           FALSE)
SET(GFLAGS_BUILD_gflags_nothreads_LIB TRUE)
SET(GFLAGS_BUILD_PACKAGING            FALSE)
SET(GFLAGS_BUILD_TESTING              FALSE)
SET(GFLAGS_INSTALL_HEADERS            FALSE)
SET(GFLAGS_INSTALL_SHARED_LIBS        FALSE)
SET(GFLAGS_INSTALL_STATIC_LIBS        FALSE)

#Meh-meh
if (WIN32)
	remove_definitions(-DUNICODE -D_UNICODE)
endif()
ADD_SUBDIRECTORY(gflags)
if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
endif()


# to change assimp for springs need, see fix_assimp.sh
# sadly still manual changes are needed!

set(DISABLED_ASSIMP_FORMATS "")
foreach(var AMF AC ASE ASSBIN ASSXML B3D BVH DXF CSM HMP IRRMESH IRR LWS MD2 MD3 MD5 MDC MDL MMD NFF NDO OFF OGRE OPENGEX PLY MS3D COB IFC XGL FBX Q3D Q3BSP RAW SIB SMD STL TERRAGEN 3D X X3D GLTF 3MF)
	list(APPEND DISABLED_ASSIMP_FORMATS "ASSIMP_BUILD_${var}_IMPORTER")
endforeach()
set(ENABLED_ASSIMP_FORMATS "")
foreach(var 3DS COLLADA LWO OBJ BLEND)
	list(APPEND ENABLED_ASSIMP_FORMATS "ASSIMP_BUILD_${var}_IMPORTER")
endforeach()

foreach(var
		${DISABLED_ASSIMP_FORMATS}
		BUILD_SHARED_LIBS
		ASSIMP_BUILD_ASSIMP_TOOLS
		ASSIMP_BUILD_TESTS
		ASSIMP_NO_EXPORT
		ASSIMP_OPT_BUILD_PACKAGES
		ASSIMP_BUILD_ASSIMP_TOOLS
		ASSIMP_BUILD_NONFREE_C4D_IMPORTER
		ASSIMP_DOUBLE_PRECISION
	)
	set("${var}" OFF CACHE STRING "forced by Recoil build env" FORCE)
endforeach()

foreach(var
		${ENABLED_ASSIMP_FORMATS}
	)
	set("${var}" ON CACHE STRING "forced by Recoil build env" FORCE)
endforeach()

foreach(var ASSIMP_BUILD_STATIC_LIB ASSIMP_NO_EXPORT)
	message(STATUS "Forcing assimp option ${var} to on")
	set("${var}" ON CACHE STRING "forced by Recoil build env" FORCE)
endforeach()

ADD_SUBDIRECTORY(assimp)
target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OWN_ZLIB)

# Suppress warnings in vendored assimp (we're not going to modify the library code)
# Vendored assimp 4.0.1 trips these warnings in its own code.
# Recheck this list if assimp is upgraded; later releases fixed some of these.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(_assimp_warns
		-Wno-pragma-pack
		-Wno-ordered-compare-function-pointers
		-Wno-deprecated-declarations
		-Wno-tautological-bitwise-compare
		-Wno-unused-but-set-variable
	)
	target_compile_options(assimp PRIVATE ${_assimp_warns})
	if (TARGET IrrXML)
		target_compile_options(IrrXML PRIVATE -Wno-pragma-pack)
	endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(_assimp_warns
		-Wno-class-memaccess
		-Wno-deprecated-declarations
		-Wno-tautological-compare
		-Wno-maybe-uninitialized
	)
	target_compile_options(assimp PRIVATE ${_assimp_warns})
endif()

# simdjson, a fastgltf dependency (manually added to submodules to work around the build system limitations)
set(SIMDJSON_AVX512_ALLOWED OFF CACHE BOOL "forced by Recoil build env" FORCE)
set(SIMDJSON_BASH OFF CACHE BOOL "forced by Recoil build env" FORCE)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|armv8")
	# ARM64: enable native NEON implementation (well-tested in simdjson)
	set(SIMDJSON_IMPLEMENTATION "arm64" CACHE STRING "forced by Recoil build env" FORCE)
else()
	# x86: use fallback only (original behavior)
	set(SIMDJSON_IMPLEMENTATION "fallback" CACHE STRING "forced by Recoil build env" FORCE)
endif()

set(SIMDJSON_ENABLE_THREADS     OFF CACHE BOOL "forced by Recoil build env" FORCE)
set(SIMDJSON_EXCEPTIONS     ON CACHE BOOL "forced by Recoil build env" FORCE)
ADD_SUBDIRECTORY(simdjson)

# fastgltf
set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "forced by Recoil build env" FORCE)
set(FASTGLTF_USE_64BIT_FLOAT OFF CACHE BOOL "forced by Recoil build env" FORCE)
set(FASTGLTF_USE_CUSTOM_SMALLVECTOR OFF CACHE BOOL "forced by Recoil build env" FORCE)
ADD_SUBDIRECTORY(fastgltf)

ADD_SUBDIRECTORY(squish)
ADD_SUBDIRECTORY(rg-etc1)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# Benign clang-only warnings in vendored rg_etc1.cpp (no-op labs() on unsigned +
	# bare mark-as-used statements).
	# We're not going to touch the decades old impl, so suppress the warnings.
	target_compile_options(rgetc1 PRIVATE -Wno-absolute-value -Wno-unused-value)
endif()

option(TRACY_ENABLE "Enable tracy profiling" OFF)

# On demand profiling is *slightly* more expensive, but it allows to run the
# build with tracing like regular build and attach late in game, where regular
# trace would just run out of memory because of size.
option(TRACY_ON_DEMAND "Enable tracy profiling" ON)

# Off by default because it's pretty expensive and because some places use
# raw malloc have to be used with care.
option(TRACY_PROFILE_MEMORY "Profile memory allocations" OFF)

add_subdirectory(tracy)

if (TRACY_ENABLE)
	option(RMLUI_TRACY_PROFILING "Enable RmlUi profiling with Tracy." ON)
	# We have our own memory profiling, never enable RmlUi one.
	set(RMLUI_TRACY_MEMORY_PROFILING OFF CACHE BOOL "Enable RmlUi tracy profiling")

	# Disable the additional RmlUI multiconfig configuration as it only applies to RmlUI
	set(RMLUI_TRACY_CONFIGURATION OFF CACHE BOOL "" FORCE)
endif()

# We resolve freetype here to make sure RmlUi find_package picks the same
# correct version as picked by the engine.
# !!! If you change freetype resolution here, change it also in rts/builds/legacy
find_freetype_hack() # hack to find different named freetype.dll
find_package_static(Freetype 2.8.1 REQUIRED)

# lunasvg does not expect number constants to be floats, so we undo this cmake flag for its compilation
string(REPLACE "-fsingle-precision-constant" "" NEW_CMAKE_FLAGS ${CMAKE_CXX_FLAGS})
set(SAVED_CMAKE_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NEW_CMAKE_FLAGS})
add_subdirectory(lunasvg)
set(CMAKE_CXX_FLAGS ${SAVED_CMAKE_FLAGS})

add_subdirectory(RmlUi)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND TARGET rmlui_core)
	# Known GCC-only -Warray-bounds false positive in vendored RmlUi at -O2+.
	# Fixed in GCC 14 -- retest dropping this once CI is off GCC 13.
	target_compile_options(rmlui_core PRIVATE -Wno-array-bounds)
endif()
