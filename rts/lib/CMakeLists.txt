
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts/lib)

ADD_SUBDIRECTORY(smmalloc)
ADD_SUBDIRECTORY(libcpuid)
ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luasocket)
ADD_SUBDIRECTORY(minizip)
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

ADD_SUBDIRECTORY(gflags)


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
	set("${var}" OFF CACHE STRING "forced off by spring build env" FORCE)
endforeach()

foreach(var
		${ENABLED_ASSIMP_FORMATS}
	)
	set("${var}" ON CACHE STRING "forced on by spring build env" FORCE)
endforeach()

foreach(var ASSIMP_BUILD_STATIC_LIB ASSIMP_NO_EXPORT)
	message(STATUS "Forcing assimp option ${var} to on")
	set("${var}" ON CACHE STRING "forced on by spring build env" FORCE)
endforeach()

ADD_SUBDIRECTORY(assimp)
target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OWN_ZLIB)

if    (NOT HEADLESS_SYSTEM)

	if    (USE_LIBSQUISH)
		ADD_SUBDIRECTORY(rg-etc1)
		ADD_SUBDIRECTORY(squish)
	endif (USE_LIBSQUISH)
endif (NOT HEADLESS_SYSTEM)

option(TRACY_ENABLE "Enable tracy profiling" OFF)

# On demand profiling is *slightly* more expensive, but it allows to run the
# build with tracing like regular build and attach late in game, where regular
# trace would just run out of memory because of size.
option(TRACY_ON_DEMAND "Enable tracy profiling" ON)

# Off by default because it's pretty expensive and because some places use
# raw malloc have to be used with care.
option(TRACY_PROFILE_MEMORY "Profile memory allocations" OFF)

add_subdirectory(tracy)

# TODO(p2004): Enabling tracy for RmlUi is at the momemnt broken and waits for
#              resolution upstream https://github.com/mikke89/RmlUi/issues/516.
# if (TRACY_ENABLE)
# 	set(ENABLE_TRACY_PROFILING ON CACHE BOOL "Enable RmlUi tracy profiling")
# 	# Place the include directories for tracy in TRACY_INCLUDE_DIR as that's
# 	# what RmlUi expects to be set in their FindTracy.cmake
# 	get_target_property(TRACY_INCLUDE_DIR TracyClient INCLUDE_DIRECTORIES)
# endif()

# We resolve freetype here to make sure RmlUi find_package picks the same
# correct version as picked by the engine.
# !!! If you change freetype resolution here, change it also in rts/builds/legacy
find_freetype_hack() # hack to find different named freetype.dll
find_package_static(Freetype REQUIRED)

# option(BUILD_LUA_BINDINGS "RmlUI lua " TRUE)
add_subdirectory(RmlUi)
add_subdirectory(RmlSolLua)
