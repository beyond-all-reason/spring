set(SOURCES
    smmalloc.cpp
    smmalloc_generic.cpp
    smmalloc_tls.cpp
    )

set(HEADERS
    smmalloc.h
    )

ADD_LIBRARY(smmalloc STATIC EXCLUDE_FROM_ALL ${SOURCES} ${HEADERS})
SET_TARGET_PROPERTIES(smmalloc PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
target_include_directories(smmalloc SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})