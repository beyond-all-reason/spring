target_sources(NetServer PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/AutohostInterface.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/GameServer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/GameParticipant.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Protocol/BaseNetProtocol.cpp"
	)
target_sources(NetClient PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Protocol/NetProtocol.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/NetCommands.cpp"
	)


