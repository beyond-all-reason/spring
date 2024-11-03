# Try to find libunwind
#
# based on: http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries#Using_external_libraries_that_CMake_doesn.27t_yet_have_modules_for
# written by Maj.Boredom
#
# Once done this will define
#   LIBUNWIND_FOUND - System has libunwind
# and optionally,
#   LIBUNWIND_INCLUDE_DIRS - The libunwind include directories
#   LIBUNWIND_LIBRARIES - The libraries needed to use libunwind
#   LIBUNWIND_DEFINITIONS - Compiler switches needed to use libunwind
#

find_package(PkgConfig)

set(LIB_STD_ARGS
      PATH_SUFFIXES
          lib
          lib64
      PATHS
          ${PROJECT_BINARY_DIR}
          ${PROJECT_SOURCE_DIR}
          $ENV{LD_LIBRARY_PATH}
          $ENV{LIBRARY_PATH}
          /usr/lib
          /usr/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib/system
)

find_path(LIBUNWIND_INCLUDE_DIR libunwind.h)
find_path(LIBUNWIND_PKGCONFIG_DIR libunwind.pc
          PATHS ${libunwind_DIR}
          PATH_SUFFIXES lib/pkgconfig
)

if (APPLE AND LIBUNWIND_INCLUDE_DIR)
  # FIXME: OS X 10.10 doesn't have static libunwind.a only dynamic libunwind.dylib;
  #        link with "-framework Cocoa"
  set(LIBUNWIND_LIBRARY "-framework Cocoa")
else ()
  find_library(LIBUNWIND_LIBRARY NAMES unwind ${LIB_STD_ARGS})
endif ()


if (LIBUNWIND_INCLUDE_DIR AND LIBUNWIND_LIBRARY)
  set(LIBUNWIND_DEFINITIONS "LIBUNWIND")
  set(LIBUNWIND_INCLUDE_DIRS ${LIBUNWIND_INCLUDE_DIR})
  set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARY})
  
  if (NOT TARGET libunwind::libunwind)
    add_library(libunwind::libunwind UNKNOWN IMPORTED)
    set_target_properties(libunwind::libunwind PROPERTIES
                          INTERFACE_COMPILE_DEFINITIONS ${LIBUNWIND_DEFINITIONS}
                          INTERFACE_INCLUDE_DIRECTORIES ${LIBUNWIND_INCLUDE_DIR}
                          IMPORTED_LOCATION ${LIBUNWIND_LIBRARY}
    )
  endif()
endif()


if (LIBUNWIND_PKGCONFIG_DIR AND EXISTS "${LIBUNWIND_PKGCONFIG_DIR}/libunwind.pc")
  file(STRINGS "${LIBUNWIND_PKGCONFIG_DIR}/libunwind.pc" unwind_version_str REGEX "^Version:[ \t]+.+")
  
  string(REGEX REPLACE "^Version:[ \t]+(.+)" "\\1" LIBUNWIND_VERSION_STRING "${unwind_version_str}")
  unset(unwind_version_str)
endif ()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libunwind
                                  REQUIRED_VARS LIBUNWIND_LIBRARY LIBUNWIND_INCLUDE_DIR
                                  VERSION_VAR LIBUNWIND_VERSION_STRING)
mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARY)
