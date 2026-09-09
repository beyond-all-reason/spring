# Locate the minimal FFmpeg libraries used by Recoil video playback.
#
# Defines:
#   FFmpeg_FOUND
#   FFmpeg_VERSION
#   FFmpeg::avcodec, FFmpeg::avformat, FFmpeg::avutil,
#   FFmpeg::swscale, FFmpeg::swresample

find_package(PkgConfig QUIET)

set(_FFmpeg_components avformat avcodec avutil swscale swresample)
foreach(_component IN LISTS _FFmpeg_components)
	if(PkgConfig_FOUND)
		pkg_check_modules(PC_FFmpeg_${_component} QUIET lib${_component})
	endif()

	find_path(FFmpeg_${_component}_INCLUDE_DIR
		NAMES lib${_component}/${_component}.h
		HINTS ${PC_FFmpeg_${_component}_INCLUDE_DIRS}
		PATH_SUFFIXES include
	)
	find_library(FFmpeg_${_component}_LIBRARY
		NAMES ${_component} lib${_component}
		HINTS ${PC_FFmpeg_${_component}_LIBRARY_DIRS}
		PATH_SUFFIXES lib
	)
	mark_as_advanced(FFmpeg_${_component}_INCLUDE_DIR FFmpeg_${_component}_LIBRARY)
endforeach()

include(FindPackageHandleStandardArgs)
if(PC_FFmpeg_avcodec_VERSION)
	set(FFmpeg_VERSION "${PC_FFmpeg_avcodec_VERSION}")
elseif(FFmpeg_avcodec_INCLUDE_DIR)
	file(STRINGS "${FFmpeg_avcodec_INCLUDE_DIR}/libavcodec/version_major.h" _FFmpeg_version_line
		REGEX "^#define LIBAVCODEC_VERSION_MAJOR[ \t]+[0-9]+")
	string(REGEX MATCH "[0-9]+" FFmpeg_VERSION "${_FFmpeg_version_line}")
endif()
find_package_handle_standard_args(FFmpeg
	REQUIRED_VARS
		FFmpeg_avformat_INCLUDE_DIR FFmpeg_avformat_LIBRARY
		FFmpeg_avcodec_INCLUDE_DIR FFmpeg_avcodec_LIBRARY
		FFmpeg_avutil_INCLUDE_DIR FFmpeg_avutil_LIBRARY
		FFmpeg_swscale_INCLUDE_DIR FFmpeg_swscale_LIBRARY
		FFmpeg_swresample_INCLUDE_DIR FFmpeg_swresample_LIBRARY
	VERSION_VAR FFmpeg_VERSION
)

if(FFmpeg_FOUND)
	find_file(FFmpeg_LICENSE_FILE FFMPEG-COPYING.LGPLv2.1
		HINTS "${FFmpeg_avcodec_INCLUDE_DIR}/.." "${FFmpeg_avcodec_INCLUDE_DIR}/../..")
	find_file(FFmpeg_MANIFEST_FILE FFMPEG-MANIFEST.txt
		HINTS "${FFmpeg_avcodec_INCLUDE_DIR}/.." "${FFmpeg_avcodec_INCLUDE_DIR}/../..")
	mark_as_advanced(FFmpeg_LICENSE_FILE FFmpeg_MANIFEST_FILE)
	foreach(_component IN LISTS _FFmpeg_components)
		if(NOT TARGET FFmpeg::${_component})
			add_library(FFmpeg::${_component} UNKNOWN IMPORTED)
			set_target_properties(FFmpeg::${_component} PROPERTIES
				IMPORTED_LOCATION "${FFmpeg_${_component}_LIBRARY}"
				INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${_component}_INCLUDE_DIR}"
			)
			if(PC_FFmpeg_${_component}_LINK_LIBRARIES)
				set_property(TARGET FFmpeg::${_component} PROPERTY
					INTERFACE_LINK_LIBRARIES "${PC_FFmpeg_${_component}_LINK_LIBRARIES}")
			endif()
			if(PC_FFmpeg_${_component}_LDFLAGS_OTHER)
				set_property(TARGET FFmpeg::${_component} PROPERTY
					INTERFACE_LINK_OPTIONS "${PC_FFmpeg_${_component}_LDFLAGS_OTHER}")
			endif()
		endif()
	endforeach()
endif()
