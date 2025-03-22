target_sources(Rml PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_Backend.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_SystemInterface.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3_Recoil.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_VFSFileInterface.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/ElementLuaTexture.cpp"
)

add_subdirectory(SolLua)
