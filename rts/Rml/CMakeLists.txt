set(sources_engine_Rml
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_Backend.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_SystemInterface.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3_Recoil.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/RmlUi_VFSFileInterface.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/ElementLuaTexture.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SVG/SVGPlugin.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SVG/ElementSVG.cpp"
		PARENT_SCOPE
)

add_subdirectory(SolLua)

set(sources_engine_RmlSolLua
		"${sources_RmlSolLua}"
		PARENT_SCOPE
)
