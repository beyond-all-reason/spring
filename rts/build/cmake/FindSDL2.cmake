# SDL3 provides proper CMake config files.
# This module is kept for backward compatibility only.
find_package(SDL3 QUIET CONFIG)

if(SDL3_FOUND AND NOT TARGET SDL2::SDL2)
  # Provide compatibility alias
  add_library(SDL2::SDL2 INTERFACE IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    INTERFACE_LINK_LIBRARIES "SDL3::SDL3"
  )
endif()
