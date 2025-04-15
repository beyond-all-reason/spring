INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts/lib)

ADD_SUBDIRECTORY(glad)
ADD_SUBDIRECTORY(smmalloc)
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

# fastgltf
set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "forced on by Recoil build env" FORCE)
set(FASTGLTF_USE_64BIT_FLOAT OFF CACHE BOOL "forced on by Recoil build env" FORCE)
ADD_SUBDIRECTORY(fastgltf)

ADD_SUBDIRECTORY(squish)
ADD_SUBDIRECTORY(rg-etc1)

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
