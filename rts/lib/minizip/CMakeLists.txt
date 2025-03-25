set(miniZipSources
		"unzip.c"
		"zip.c"
		"ioapi.c"
	)

add_definitions(-DNOCRYPT -DNOUNCRYPT)
find_package(ZLIB 1.2.7 REQUIRED)

add_library(minizip STATIC EXCLUDE_FROM_ALL ${miniZipSources})
target_link_libraries(minizip ZLIB::ZLIB)
set_target_properties(minizip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

set_global(SPRING_MINIZIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib")
set_global(SPRING_MINIZIP_LIBRARY minizip)