SET(headlessStubsSources
		"glewstub.c"
		"glstub.c"
		"sdlstub.c"
	)

find_package(SDL2 MODULE REQUIRED)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
target_link_libraries(headlessStubs PUBLIC SDL2::SDL2)
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
