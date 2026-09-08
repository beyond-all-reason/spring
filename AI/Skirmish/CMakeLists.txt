### Skirmish AIs
#
# Functions and macros defined in this file:
# * skirmish_ai_message
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC            "${CMAKE_CURRENT_SOURCE_DIR}")

# Print a message, but only if we are allowed to speak.
macro    (skirmish_ai_message type msg)
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "${msg}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)
endmacro (skirmish_ai_message typemsg)


# Add all Skirmish AI submodules
get_list_of_submodules(SKIRMISH_AI_DIRS)
set(DEPS_AI_SKIRMISH "")
set(_skirmish_ai_clang_warns
	-Wno-implicit-const-int-float-conversion
	-Wno-parentheses-equality
	-Wno-deprecated-declarations
	-Wno-inconsistent-missing-override
	-Wno-unused-lambda-capture
	-Wno-pessimizing-move
	-Wno-unneeded-internal-declaration
	-Wno-unused-local-typedef
	-Wno-unused-private-field
	-Wno-unused-but-set-variable
	-Wno-mismatched-tags
	-Wno-vla-cxx-extension
	-Wno-unused-variable
)
set(_skirmish_ai_gcc_warns
	-Wno-maybe-uninitialized
	-Wno-deprecated-declarations
)
foreach    (skirmishAIDir ${SKIRMISH_AI_DIRS})
	add_subdirectory(${skirmishAIDir})
	if (TARGET ${skirmishAIDir})
		list(APPEND DEPS_AI_SKIRMISH ${skirmishAIDir})
		# Vendored Skirmish AI submodules (e.g. BARb, CircuitAI) don't compile
		# cleanly under modern compilers; suppress the warnings here rather
		# than patching the submodule CMakeLists.
		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(${skirmishAIDir} PRIVATE ${_skirmish_ai_clang_warns})
		elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			target_compile_options(${skirmishAIDir} PRIVATE ${_skirmish_ai_gcc_warns})
		endif()
	endif()
endforeach (skirmishAIDir)
make_global(DEPS_AI_SKIRMISH)
