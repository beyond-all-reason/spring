SET(headlessStubsSources
		"gladstub.cpp"
		"sdlstub.c"
	)

find_package(SDL3 REQUIRED CONFIG)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
target_link_libraries(headlessStubs PUBLIC $<COMPILE_ONLY:SDL3::SDL3>)
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
