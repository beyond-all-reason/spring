# The version of SDL we have is too old
# and doesn't provide a proper config file.
# We need to create imported targets for the config

find_package(SDL2 QUIET CONFIG)

find_library(SDL2_LIBRARY
             NAMES
              SDL2
             PATHS
              ${SDL2_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIRS SDL2_LIBRARIES SDL2_LIBRARY)
mark_as_advanced(SDL2_LIBRARIES SDL2_LIBRARY)

if (SDL2_FOUND AND NOT TARGET SDL2::SDL2)
  add_library(SDL2::SDL2 UNKNOWN IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                        IMPORTED_LOCATION ${SDL2_LIBRARY}
  )
endif()
