
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
foreach(var AMF AC ASE ASSBIN ASSXML B3D BVH DXF CSM HMP IRRMESH IRR LWS MD2 MD3 MD5 MDC MDL MMD NFF NDO OFF OGRE OPENGEX PLY MS3D COB IFC XGL FBX Q3D Q3BSP RAW SIB SMD STL TERRAGEN 3D X X3D 3MF)
	list(APPEND DISABLED_ASSIMP_FORMATS "ASSIMP_BUILD_${var}_IMPORTER")
endforeach()

set(ENABLED_ASSIMP_FORMATS "")
foreach(var 3DS COLLADA LWO OBJ BLEND GLTF USD)
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
		ASSIMP_INSTALL
		ASSIMP_INJECT_DEBUG_POSTFIX # handle https://github.com/beyond-all-reason/spring/commit/479b42e61a9ee8d5bc8b5a3d772708276c5900e5
	)
	set("${var}" OFF CACHE STRING "forced off by Recoil build env" FORCE)
endforeach()

foreach(var
		${ENABLED_ASSIMP_FORMATS}
	)
	set("${var}" ON CACHE STRING "forced on by Recoil build env" FORCE)
endforeach()

foreach(var ASSIMP_BUILD_STATIC_LIB ASSIMP_NO_EXPORT)
	message(STATUS "Forcing assimp option ${var} to on")
	set("${var}" ON CACHE STRING "forced on by Recoil build env" FORCE)
endforeach()

FIND_PACKAGE(ZLIB)
IF (NOT ZLIB_FOUND)
	set("ASSIMP_BUILD_ZLIB" ON CACHE BOOL "forced on by Recoil build env" FORCE)
endif()


set(CMAKE_CXX_FLAGS_ORIG ${CMAKE_CXX_FLAGS})
if (CMAKE_COMPILER_IS_GNUCXX)
	# Assimp cannot be compiled with "-fsingle-precision-constant"
	string(REPLACE "-fsingle-precision-constant" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif(CMAKE_COMPILER_IS_GNUCXX)

ADD_SUBDIRECTORY(assimp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_ORIG})

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

ADD_SUBDIRECTORY(tracy)
