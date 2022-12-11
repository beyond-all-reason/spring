# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Test whether the C++ compiler supports certain flags.
# Once done, this will define the following vars.
# They will be empty if the flag is not supported,
# or contain the flag if it is supported.
#
# VISIBILITY_HIDDEN            -fvisibility=hidden
# VISIBILITY_INLINES_HIDDEN    -fvisibility-inlines-hidden
# SSE_FLAGS                    -msse -mfpmath=sse
# IEEE_FP_FLAG                 -fvisibility-inlines-hidden
# LTO_FLAGS                    -flto -fwhopr
#
# Note: gcc for windows supports these flags, but gives lots of errors when
#       compiling, so use them only for linux builds.

include(TestCXXAcceptsFlag)

# Helper
macro    (CHECK_AND_ADD_FLAGS dest)
	foreach    (flag ${ARGN})
		check_cxx_accepts_flag("${flag}" has_${flag})
		if    (has_${flag})
			set(${dest} "${${dest}} ${flag}")
		else (has_${flag})
			message("compiler doesn't support: ${flag}")
		endif (has_${flag})
	endforeach (flag ${ARGN})
endmacro (CHECK_AND_ADD_FLAGS)



if    (NOT DEFINED VISIBILITY_HIDDEN)
	set(VISIBILITY_HIDDEN "")
	if    (NOT WIN32 AND NOT APPLE)
		check_and_add_flags(VISIBILITY_HIDDEN -fvisibility=hidden)
	endif (NOT WIN32 AND NOT APPLE)
endif (NOT DEFINED VISIBILITY_HIDDEN)


if    (NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	set(VISIBILITY_INLINES_HIDDEN "")
	if    (NOT WIN32)
		check_and_add_flags(VISIBILITY_INLINES_HIDDEN -fvisibility-inlines-hidden)
	endif (NOT WIN32)
endif (NOT DEFINED VISIBILITY_INLINES_HIDDEN)

If    (NOT DEFINED IEEE_FP_FLAG)
	If   (MSVC)
		Set(IEEE_FP_FLAG "/fp:strict")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		Message(WARNING "Clang detected, disabled IEEE-FP")
	Else (MSVC)
		CHECK_CXX_ACCEPTS_FLAG("-mieee-fp" HAS_IEEE_FP_FLAG)
		If    (HAS_IEEE_FP_FLAG)
			Set(IEEE_FP_FLAG "-mieee-fp")
		Else  (HAS_IEEE_FP_FLAG)
			Message(WARNING "IEEE-FP support is missing, online play is highly discouraged with this build")
			Set(IEEE_FP_FLAG "")
		EndIf (HAS_IEEE_FP_FLAG)
	Endif(MSVC)
EndIf (NOT DEFINED IEEE_FP_FLAG)


If    (NOT DEFINED CXX17_FLAGS)
	CHECK_AND_ADD_FLAGS(CXX17_FLAGS "-std=c++17")
EndIf (NOT DEFINED CXX17_FLAGS)


If    (NOT MSVC AND NOT DEFINED LTO_FLAGS)
	Set(LTO_FLAGS "")
	CHECK_AND_ADD_FLAGS(LTO_FLAGS -flto)
EndIf (NOT MSVC AND NOT DEFINED LTO_FLAGS)



IF    (NOT MSVC AND NOT DEFINED MARCH)
	Set(MARCH "")

	# 32bit
	CHECK_CXX_ACCEPTS_FLAG("-march=i686" HAS_I686_FLAG_)
	IF    (HAS_I686_FLAG_)
		Set(MARCH "i686")
	EndIf (HAS_I686_FLAG_)

	# 64bit
	if    ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
		# always syncs with 32bit
		check_cxx_accepts_flag("-march=x86_64" HAS_X86_64_FLAG_)
		if    (HAS_X86_64_FLAG_)
			set(MARCH "x86_64")
		endif (HAS_X86_64_FLAG_)
	endif ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
endif (NOT MSVC AND NOT DEFINED MARCH)

if   (NOT MSVC)
	if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		CHECK_CXX_ACCEPTS_FLAG("-mno-tls-direct-seg-refs" HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
		if (HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
			set(NO_TLS_DIRECT_SEG_REFS -mno-tls-direct-seg-refs)
		endif()
	endif()
endif()


if   (CMAKE_COMPILER_IS_GNUCXX)
	set(MPX_FLAGS "")
	check_and_add_flags(MPX_FLAGS -fcheck-pointer-bounds -mmpx -Wchkp)
endif()
