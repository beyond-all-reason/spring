
add_subdirectory(unitsync)
add_subdirectory(DemoTool)

if    (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")
	message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/ is missing, please run\n git submodule init && git submodule update")
endif ()

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	add_subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)

# TODO: Clean this mess up together with other CMake refactoring.
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS "")
set(CMAKE_MODULE_LINKER_FLAGS "")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
# This block is needed only because we need to clean global compile flags above
# for pr-downloader as we don't want to inherit everything that engine uses.
if(PREFER_STATIC_LIBS OR MINGW)
	check_cxx_accepts_flag("-static-libgcc" HAVE_STATIC_LIBGCC_FLAG)
	if(HAVE_STATIC_LIBGCC_FLAG)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
	endif (HAVE_STATIC_LIBGCC_FLAG)
	check_cxx_accepts_flag("-static-libstdc++" HAVE_STATIC_LIBSTDCXX_FLAG)
	if(HAVE_STATIC_LIBSTDCXX_FLAG)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
	endif (HAVE_STATIC_LIBSTDCXX_FLAG)
endif()
add_subdirectory(pr-downloader)

# add install-pr-downloader target
set(myInstallDeps pr-downloader)
set(myInstallDirs "tools/pr-downloader")
create_install_target("pr-downloader" myInstallDeps myInstallDirs)
