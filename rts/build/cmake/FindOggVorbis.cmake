# Downloaded from: https://github.com/KDE/kdelibs/blob/KDE/4.14/cmake/modules/FindOggVorbis.cmake
# License: see the accompanying COPYING-CMAKE-SCRIPTS file
#
# Modifications:
# 2008.01.16 Tobi Vollebregt -- Moved ${OGG_LIBRARY} to the back of OGGVORBIS_LIBRARIES,
#                               this allows vorbis to link to ogg on MinGW.
#                            -- Moved ${VORBIS_LIBRARY} just before OGGVORBIS_LIBRARIES,
#                               this allows vorbis{file,enc} to link to vorbis on MinGW.

# - Try to find the OggVorbis libraries
# Once done this will define
#
#  OGGVORBIS_FOUND - system has OggVorbis
#  OGGVORBIS_VERSION - set either to 1 or 2
#  OGGVORBIS_INCLUDE_DIR - the OggVorbis include directory
#  OGGVORBIS_LIBRARIES - The libraries needed to use OggVorbis
#  OGG_LIBRARY         - The Ogg library
#  VORBIS_LIBRARY      - The Vorbis library
#  VORBISFILE_LIBRARY  - The VorbisFile library
#  VORBISENC_LIBRARY   - The VorbisEnc library

# Copyright (c) 2006, Richard Laerkaeng, <richard@goteborg.utfors.se>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


include (CheckLibraryExists)

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)
find_path(OGG_INCLUDE_DIR ogg/ogg.h)

find_library(OGG_LIBRARY NAMES libogg ogg ogg-0)
find_library(VORBIS_LIBRARY NAMES libvorbis vorbis vorbis-0)
find_library(VORBISFILE_LIBRARY NAMES libvorbisfile vorbisfile vorbisfile-3)
find_library(VORBISENC_LIBRARY NAMES libvorbisenc vorbisenc vorbisenc-2 libvorbis vorbis)

mark_as_advanced(VORBIS_INCLUDE_DIR OGG_INCLUDE_DIR
                 OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY VORBISENC_LIBRARY)


if (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)
   
   set(OGGVORBIS_LIBRARIES ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBISENC_LIBRARY})
   
   set(_CMAKE_REQUIRED_LIBRARIES_TMP ${CMAKE_REQUIRED_LIBRARIES})
   set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OGGVORBIS_LIBRARIES})
   check_library_exists(vorbis vorbis_bitrate_addblock "" HAVE_LIBVORBISENC2)
   set(CMAKE_REQUIRED_LIBRARIES ${_CMAKE_REQUIRED_LIBRARIES_TMP})
   
   if (HAVE_LIBVORBISENC2)
      set (OGGVORBIS_VERSION 2)
   else (HAVE_LIBVORBISENC2)
      set (OGGVORBIS_VERSION 1)
   endif (HAVE_LIBVORBISENC2)

endif (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OggVorbis REQUIRED_VARS VORBIS_LIBRARY OGG_LIBRARY VORBISFILE_LIBRARY VORBISENC_LIBRARY
                                  VORBIS_INCLUDE_DIR OGG_INCLUDE_DIR)

if (OggVorbis_FOUND)
   if (NOT TARGET Ogg::ogg)
      add_library(Ogg::ogg UNKNOWN IMPORTED)
      
      set_target_properties(Ogg::ogg PROPERTIES
                            INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}"
                            IMPORTED_LOCATION ${OGG_LIBRARY}
      )
   endif()
   
   if (NOT TARGET vorbis::vorbisenc)
      add_library(vorbis::vorbisenc UNKNOWN IMPORTED)
      
      set_target_properties(vorbis::vorbisenc PROPERTIES
                            IMPORTED_LOCATION ${VORBISENC_LIBRARY}
      )
   endif()
   
   if (NOT TARGET vorbis::vorbisfile)
      add_library(vorbis::vorbisfile UNKNOWN IMPORTED)
      
      set_target_properties(vorbis::vorbisfile PROPERTIES
                            IMPORTED_LOCATION ${VORBISFILE_LIBRARY}
      )
   endif()
   
   if (NOT TARGET vorbis::vorbis)
      add_library(vorbis::vorbis UNKNOWN IMPORTED)
      
      set_target_properties(vorbis::vorbis PROPERTIES
                            INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIR}"
                            IMPORTED_LOCATION ${VORBIS_LIBRARY}
      )
   endif()
endif()
