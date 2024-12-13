SET(headlessStubsSources
		"glewstub.c"
		"glstub.c"
		"sdlstub.c"
	)

find_package(SDL2 MODULE REQUIRED)
find_package(OpenGL REQUIRED)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
target_link_libraries(headlessStubs PUBLIC $<COMPILE_ONLY:SDL2::SDL2> $<COMPILE_ONLY:OpenGL::GL> $<COMPILE_ONLY:OpenGL::GLU>)
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
