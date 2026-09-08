include_directories(${Spring_SOURCE_DIR}/rts/lib/asio/include)
include_directories(${Spring_SOURCE_DIR}/rts)
add_library(engineSystemNet STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/LocalConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/LoopbackConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/PackPacket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/ProtocolDef.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/RawPacket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Socket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UDPConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UDPListener.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UnpackPacket.cpp"
	)

if(RECOIL_ENABLE_CXX_MODULES)
	set(engineSystemNetModuleFiles
		"${CMAKE_CURRENT_SOURCE_DIR}/Exception.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/ProtocolDef.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/RawPacket.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/PackPacket.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/UnpackPacket.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/Connection.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/LoopbackConnection.cppm"
		"${CMAKE_CURRENT_SOURCE_DIR}/LocalConnection.cppm"
	)

	set_source_files_properties(${engineSystemNetModuleFiles}
		PROPERTIES LANGUAGE CXX
	)

	set_target_properties(engineSystemNet
		PROPERTIES CXX_SCAN_FOR_MODULES ON
	)

	target_include_directories(engineSystemNet
	PUBLIC
		"${Spring_SOURCE_DIR}/rts"
	PRIVATE
		"${Spring_SOURCE_DIR}/rts/lib/asio/include"
)
endif()