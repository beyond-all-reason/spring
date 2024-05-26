# Spring supplied CMake build file

INCLUDE_DIRECTORIES(BEFORE ../lua/include)

SET(luaPandaSources
	Debugger/debugger_lib/libpdebug.cpp
)


ADD_LIBRARY(luaPanda STATIC EXCLUDE_FROM_ALL ${luaPandaSources})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Debugger/
	DESTINATION ${DATADIR}
	FILES_MATCHING PATTERN "*.lua"
)