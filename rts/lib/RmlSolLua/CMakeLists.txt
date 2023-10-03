# CMakeList.txt : CMake project for RmlSolLua, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.13)

project ("RmlSolLua")

set (CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_library (RmlSolLua STATIC)

find_library (RmlCore NAMES RmlUi RmlCore)

target_sources (RmlSolLua
	PRIVATE
		"src/RmlSolLua.h"
		"src/RmlSolLua.cpp"
		"src/bind/bind.cpp"
		"src/bind/bind.h"
		"src/bind/Colour.cpp"
		"src/bind/Context.cpp"
		"src/bind/Convert.cpp"
		"src/bind/DataModel.cpp"
		"src/bind/Document.cpp"
		"src/bind/Element.cpp"
		"src/bind/ElementDerived.cpp"
		"src/bind/ElementForm.cpp"
		"src/bind/Event.cpp"
		"src/bind/Global.cpp"
		"src/bind/Log.cpp"
		"src/bind/Vector.cpp"
		"src/plugin/SolLuaDataModel.cpp"
		"src/plugin/SolLuaDataModel.h"
		"src/plugin/SolLuaDocument.cpp"
		"src/plugin/SolLuaDocument.h"
		"src/plugin/SolLuaEventListener.cpp"
		"src/plugin/SolLuaEventListener.h"
		"src/plugin/SolLuaInstancer.cpp"
		"src/plugin/SolLuaInstancer.h"
		"src/plugin/SolLuaPlugin.cpp"
		"src/plugin/SolLuaPlugin.h"
	PUBLIC
		"src/RmlSolLua.h"
)

target_include_directories (RmlSolLua
	PRIVATE
		${PROJECT_SOURCE_DIR}/src/
		${PROJECT_SOURCE_DIR}/include/
		../RmlUi/Include/
		../
		../lua/include

	PUBLIC
		${PROJECT_SOURCE_DIR}/include/
)


# TODO: Add tests and install targets if needed.
