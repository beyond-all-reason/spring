# required for man pages (optional) and base files (mandatory)

### Make base-files (aka sdz's)
add_subdirectory(base)

### Install base-files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${DATADIR}
		PATTERN ".gitignore" EXCLUDE
		PATTERN "base" EXCLUDE
		PATTERN "freedesktop" EXCLUDE
		PATTERN "CMakeLists.txt" EXCLUDE
		PATTERN "CMakeCache" EXCLUDE
		PATTERN "CMakeFiles" EXCLUDE
		PATTERN "Makefile" EXCLUDE
		PATTERN "cmake_install.cmake" EXCLUDE
		PATTERN "springrc-template-insource.txt" EXCLUDE
	)

### Install freedesktop-files, mime-types etc.
if    (UNIX)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/applications/spring.desktop
		DESTINATION
			${APPLICATIONS_DIR}
		)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/mime/spring.xml
		DESTINATION
			${MIME_DIR}/packages
		)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/pixmaps/spring.png
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/pixmaps/application-x-spring-demo.png
		DESTINATION
			${PIXMAPS_DIR}
		)
endif (UNIX)

if (NOT EXISTS ${Spring_BINARY_DIR}/springsettings.cfg)
	if(WIN32)
		set(PLACEHOLDER ";")
	else()
		set(PLACEHOLDER ":")
	endif()
	set("SpringDataDirInSource" "${Spring_BINARY_DIR}${PLACEHOLDER}${Spring_SOURCE_DIR}/cont")
	set("FontFile" "${Spring_SOURCE_DIR}/cont/fonts/FreeSansBold.otf")
	set("SmallFontFile" "${Spring_SOURCE_DIR}/cont/fonts/FreeSansBold.otf")
	configure_file(springrc-template-insource.txt ${Spring_BINARY_DIR}/springsettings.cfg)
else()
	message(STATUS "${Spring_BINARY_DIR}/springsettings.cfg already exists!")
endif()

### Install dependencies

if    (WIN32)
	# Weird tricks to not copy subdirs and other random stuff
	set(FILE_LIST "")
	if    (DEFINED CONAN_BIN_DIRS)
		foreach(DIR IN LISTS CONAN_BIN_DIRS)
			file(GLOB FILES "${DIR}/*.dll*")
			list(APPEND FILE_LIST ${FILES})
		endforeach()
		install (PROGRAMS ${FILE_LIST} DESTINATION ${BINDIR})
	endif (DEFINED CONAN_BIN_DIRS)

	file(GLOB FILES "${MINGWLIBS}/lib/*.dll*")
	install (PROGRAMS ${FILES} DESTINATION ${BINDIR})

	#execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-print-file-name=libstdc++-6.dll" OUTPUT_VARIABLE STDCXX_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)
	#install(FILES ${STDCXX_LIBRARY} DESTINATION ${BINDIR})

	# pthreadGC2 is a dependency of ThreadPool
#	find_library(PTHREADGC2_LIBRARY NAMES pthreadGC2)
#	if(NOT EXISTS ${PTHREADGC2_LIBRARY})
#		message(SEND_ERROR "pthreadGC2 not found!")
#	endif()
#	install(FILES ${PTHREADGC2_LIBRARY} DESTINATION ${BINDIR})
else  (WIN32)
	if    (NOT PREFER_STATIC_LIBS)
		# Weird tricks to not copy subdirs and other random stuff
		if    (DEFINED CONAN_LIB_DIRS)
			set(FILE_LIST "")
			file(MAKE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
			foreach(DIR IN LISTS CONAN_LIB_DIRS)
				file(GLOB FILES "${DIR}/*.so*")
				list(APPEND FILE_LIST ${FILES})
				list(GET FILES 0 L)
				string(REGEX REPLACE ".*/(.*)" "\\1" L ${L})
				string(REGEX REPLACE "\\.so.*" "\.a" L_a ${L})
				file(CREATE_LINK "${L}" "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${L_a}" SYMBOLIC)
			endforeach()
			install (PROGRAMS ${FILE_LIST} TYPE LIB)
		endif (DEFINED CONAN_LIB_DIRS)

		# Copy system libraries
		set(LIBS mng IL libatomic.so.1 libgcc_s.so.1 X11 Xau Xext Xrender Xfixes Xdmcp Xcursor xcb tiff lcms2 jbig deflate libmd.so.0 libbsd.so.0 GL GLU GLdispatch GLX OpenGL Lerc)

		if    (BUNDLED_LIBC)
			list(APPEND LIBS ld-linux-x86-64.so.2 c m libpthread.so.0 libstdc++.so.6 expat resolv librt.so.1 libdl.so.2)
		endif (BUNDLED_LIBC)

		foreach(L IN LISTS LIBS)
			# CMake seems to match static libraries if it haven't found dynamic, so make sure library is valid
			if (NOT ${L} MATCHES "\\.[0-9]+$")
				set (L "${L}.so")
				if (NOT ${L} MATCHES "^lib")
					set (L "lib${L}")
				endif()
			endif()

			find_library(L_PATH ${L} NO_CACHE)
			string(REGEX REPLACE "\\.so.*" "\.so" L_PATHS ${L_PATH})
			file(GLOB L_PATHS "${L_PATHS}*")
			install (PROGRAMS ${L_PATHS} TYPE LIB)
			unset(L_PATH)
		endforeach()
	endif (NOT PREFER_STATIC_LIBS)
endif (WIN32)

