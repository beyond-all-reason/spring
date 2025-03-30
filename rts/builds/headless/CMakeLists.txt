# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

add_definitions(-DHEADLESS)
add_definitions(-DNO_SOUND)
remove_definitions(-DAVI_CAPTURING)

set(OpenGL_GL_PREFERENCE LEGACY)

find_package(OpenGL 3.0 REQUIRED)

# NOTE(create_headless_target): We don't need to copy COMPILE_DEFINTIONS till the add_definitions are used.
# We don't need to copy COMPILE_DEFINTIONS because the headless defines are defined at the folder level
# and we don't want to inherit original ones
function(create_headless_target_from targetName)
	get_target_property(targetIncludes ${targetName} INCLUDE_DIRECTORIES)
	get_target_property(targetSources ${targetName} SOURCES)

	set(newTargetName "${targetName}Headless")
	add_library(${newTargetName} STATIC)
	if (targetIncludes)
		target_include_directories(${newTargetName} PRIVATE ${targetIncludes})
	endif()
	target_sources(${newTargetName} PRIVATE ${targetSources})
endfunction()


# headlessstubs are our stubs that replace libGL, libGLU, libGLEW, libSDL (yes really!)
list(APPEND engineHeadlessLibraries headlessStubs)
list(APPEND engineHeadlessLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineHeadlessLibraries engineSystemNet)
list(APPEND engineHeadlessLibraries ${engineCommonLibraries})
#list(APPEND engineHeadlessLibraries engineaGui)
list(APPEND engineHeadlessLibraries no-sound)
list(APPEND engineHeadlessLibraries engineSim)
list(APPEND engineHeadlessLibraries squish)
list(APPEND engineHeadlessLibraries pr-downloader)
list(APPEND engineHeadlessLibraries RmlUi::Core)
list(APPEND engineHeadlessLibraries RmlUi::Debugger)
list(APPEND engineHeadlessLibraries lunasvg)

include_directories(${ENGINE_SRC_ROOT_DIR}/lib/assimp/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/asio/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/cereal/include)

create_headless_target_from(Game)
target_link_libraries(GameHeadless PRIVATE
	Tracy::TracyClient
	headlessStubs
	RmlUi::Core
	prd::jsoncpp
	streflop
)
### Build the executable
add_executable(engine-headless ${engineSources} ${ENGINE_ICON})
target_link_libraries(engine-headless no-sound ${engineHeadlessLibraries} GameHeadless no-sound)

if    (MINGW)
	# To enable console output/force a console window to open
	set_target_properties(engine-headless PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
endif (MINGW)


### Install the executable
install(TARGETS engine-headless DESTINATION ${BINDIR})

# Only build & install spring-headless executable & dependencies
# use cases:
# * make spring-headless
# * make install-spring-headless
create_engine_build_and_install_target(headless)
