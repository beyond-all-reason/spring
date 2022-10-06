# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble libraries
find_package(SDL2 REQUIRED)
set(engineIncludes ${SDL2_INCLUDE_DIR})
set(engineLibraries ${SDL2_LIBRARY})
if("${SDL2_VERSION_STRING}" VERSION_LESS "2")
	message(FATAL_ERROR "Found SDL v${SDL2_VERSION_STRING} while 2 is required!")
endif()

add_definitions(-DHAS_VULKAN)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package_static(OpenGL REQUIRED)
find_package_static(GLEW REQUIRED)
if    (NOT PREFER_STATIC_LIBS)
	list(APPEND engineLibraries ${OPENGL_gl_LIBRARY})
endif (NOT PREFER_STATIC_LIBS)
list(APPEND engineLibraries ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})
list(APPEND engineIncludes ${GLEW_INCLUDE_DIR})

find_package_static(FontConfig)
if    (FONTCONFIG_FOUND)
	list(APPEND engineIncludes ${FONTCONFIG_INCLUDE_DIR})
	list(APPEND engineLibraries ${FONTCONFIG_LIBRARIES})
	add_definitions(-DUSE_FONTCONFIG)
	if    (PREFER_STATIC_LIBS AND UNIX)
		prefer_static_libs()
		find_library(EXPAT_LIBRARY expat)
		find_library(LIBUUID_LIBRARY uuid)
		unprefer_static_libs()
		list(APPEND engineLibraries ${EXPAT_LIBRARY} ${LIBUUID_LIBRARY})
	endif (PREFER_STATIC_LIBS AND UNIX)
endif (FONTCONFIG_FOUND)

find_freetype_hack() # hack to find different named freetype.dll
find_package_static(Freetype REQUIRED)
foreach(f ${FREETYPE_INCLUDE_DIRS})
	list(APPEND engineIncludes ${f})
endforeach(f)
list(APPEND engineLibraries ${FREETYPE_LIBRARY})

if    (UNIX AND NOT PREFER_STATIC_LIBS)
	find_package(X11 REQUIRED)
	list(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB})
endif (UNIX AND NOT PREFER_STATIC_LIBS)

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
endif (APPLE)

if    (USE_LIBSQUISH)
	list(APPEND engineLibraries squish rgetc1)
endif (USE_LIBSQUISH)

list(APPEND engineLibraries ${sound-impl})
list(APPEND engineLibraries engineSystemNet)
list(APPEND engineLibraries ${engineCommonLibraries})
#list(APPEND engineLibraries engineaGui)
list(APPEND engineLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineLibraries engineSim)
list(APPEND engineLibraries pr-downloader_static)

### Assemble external incude dirs
list(APPEND engineIncludes ${OPENAL_INCLUDE_DIR})
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/asio/include)
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include)

include_directories(${engineIncludes})

### Build the executable
add_executable(engine-legacy ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})

set(LINKER_COMMANDS "")

if    (UNIX AND NOT PREFER_STATIC_LIBS)
	list(APPEND LINKER_COMMANDS "-Wl,--disable-new-dtags -Wl,-rpath,'$$ORIGIN/lib'")
endif (UNIX AND NOT PREFER_STATIC_LIBS)

if    (PREFER_STATIC_LIBS)
	list(APPEND LINKER_COMMANDS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lzstd -lwayland-client -lwayland-cursor -lwayland-egl -l:libmng.a -lffi -lxkbcommon -liconv -ljbig -lbrotlidec-static -lbrotlicommon-static -laudio -lLercLib -l:libXau.a -lz -lwebp -ldeflate -l:libXcursor.a -l:libXfixes.a -l:libXrender.a -l:libX11.a -l:libxcb.a -l:libXdmcp.a -Wl,-Bdynamic -lGL -llcms2")
endif (PREFER_STATIC_LIBS)

target_link_libraries(engine-legacy ${engineLibraries} ${LINKER_COMMANDS})


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

create_engine_build_and_install_target(legacy)
