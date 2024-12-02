# Spring supplied CMake build file

INCLUDE_DIRECTORIES(BEFORE ../lua/include)

SET(luaSocketSources
	src/auxiliar.cpp
	src/buffer.cpp
	src/except.cpp
	src/inet.cpp
	src/io.cpp
	src/luasocket.cpp
	src/mime.cpp
	src/options.cpp
	src/select.cpp
	src/tcp.cpp
	src/timeout.cpp
	src/udp.cpp
	src/restrictions.cpp
)

IF(WIN32)
LIST(APPEND luaSocketSources
	src/wsocket.cpp
)
ELSE()
LIST(APPEND luaSocketSources
	src/usocket.cpp
	src/unix.cpp
)
ENDIF ()

ADD_LIBRARY(luasocket STATIC EXCLUDE_FROM_ALL ${luaSocketSources})
