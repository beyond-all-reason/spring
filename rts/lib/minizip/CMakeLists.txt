# Spring supplied CMake build file

set(MiniZip_FIND_QUIETLY TRUE)
FIND_PACKAGE_STATIC(MiniZip)

if     (MiniZip_FOUND)
	# Use the system supplied MiniZip library
	message(STATUS "Using minizip library: ${MiniZip_LIBRARY}")

	set_global(SPRING_MINIZIP_INCLUDE_DIR "${MiniZip_INCLUDE_DIR}")
	set_global(SPRING_MINIZIP_LIBRARY "${MiniZip_LIBRARY}")
else   (MiniZip_FOUND)
	# Build our own minizip library
	
	set(miniZipSources
			"unzip"
			"zip"
			"ioapi"
		)

	add_definitions(-DNOCRYPT -DNOUNCRYPT)
	find_package(ZLIB REQUIRED)

	add_library(minizip STATIC EXCLUDE_FROM_ALL ${miniZipSources})
	target_link_libraries(minizip ${ZLIB_LIBRARY})
	set_target_properties(minizip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

	set_global(SPRING_MINIZIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib")
	set_global(SPRING_MINIZIP_LIBRARY minizip)
endif  (MiniZip_FOUND)
