# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

set(KTX_VERSION 4.99.99)
project(KTX-Software
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

# OPTIONS

set( KTX_FEATURE_STATIC_LIBRARY "Create static libraries (shared otherwise)" ON)

set(BASISU_SUPPORT_SSE ON)
set(BASISU_SUPPORT_OPENCL OFF)

set(KTX_EMBED_BITCODE OFF)

option( KTX_FEATURE_KTX1 "Enable KTX 1 support" ON )
option( KTX_FEATURE_KTX2 "Enable KTX 2 support" ON )
option( KTX_FEATURE_VULKAN "Enable Vulkan texture upload" OFF )
option( KTX_FEATURE_GL_UPLOAD "Enable OpenGL texture upload" OFF )

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
    set(LINUX TRUE)
endif()

set(bitness 64)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR FORCE32)
        set(bitness 32)
endif()

if(KTX_FEATURE_STATIC_LIBRARY)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

# Global compile & link options including optimization flags
if(MSVC)
    # With /W4 VS2015 (V19.0) issues many warnings that VS2017 & 2019 don't
    # so only increase warning level for recent versions.
    add_compile_options($<$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.16>:/W4>)
    add_compile_options( $<IF:$<CONFIG:Debug>,/Gz,/O2> )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
       OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options(-Wall -Wextra)
    add_compile_options( $<IF:$<CONFIG:Debug>,-O0,-O3> )
    if(EMSCRIPTEN)
        add_link_options( $<IF:$<CONFIG:Debug>,-gsource-map,-O3> )
    else()
        add_link_options( $<IF:$<CONFIG:Debug>,-g,-O3> )
    endif()
else()
    message(ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

set(KTX_MAIN_SRC
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu/transcoder/basisu_containers.h
    lib/basisu/transcoder/basisu_containers_impl.h
    lib/basisu/transcoder/basisu_file_headers.h
    lib/basisu/transcoder/basisu_transcoder_internal.h
    lib/basisu/transcoder/basisu_transcoder_uastc.h
    lib/basisu/transcoder/basisu_transcoder.cpp
    lib/basisu/transcoder/basisu_transcoder.h
    lib/basisu/transcoder/basisu.h
    lib/basisu/zstd/zstd.c
    lib/checkheader.c
    lib/dfdutils/createdfd.c
    lib/dfdutils/colourspaces.c
    lib/dfdutils/dfd.h
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c
    lib/dfdutils/vk2dfd.inl
    lib/dfdutils/vulkan/vk_platform.h
    lib/dfdutils/vulkan/vulkan_core.h
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
    lib/gl_format.h
    lib/hashlist.c
    lib/info.c
    lib/ktxint.h
    lib/memstream.c
    lib/memstream.h
    lib/strings.c
    lib/swap.c
    lib/texture.c
    lib/texture.h
    lib/texture2.c
    lib/texture2.h
    lib/uthash.h
    lib/vk_format.h
    lib/vkformat_check.c
    lib/vkformat_enum.h
    lib/vkformat_str.c
    )


# Main library
add_library( ktx ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

# Read-only library
add_library( ktx_read ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

macro(commom_lib_settings lib write)

    if(TARGET mkvk)
        # Creating vulkan headers is only required when Vulkan SDK updates.
        add_dependencies(${lib} mkvk)
    endif()

    set_target_properties(${lib} PROPERTIES
        PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktx.h
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )
    if(IOS)
        set_target_properties(${lib} PROPERTIES
            FRAMEWORK TRUE
        )
    endif()

    target_compile_definitions(
        ${lib}
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
    PRIVATE
        LIBKTX
    )

    # C/C++ Standard
    target_compile_features(${lib} PUBLIC c_std_99 cxx_std_11)

    # Compiler Warning Flags
    if(EMSCRIPTEN)
        target_compile_options(${lib} PRIVATE
            -Wno-nested-anon-types
            -Wno-gnu-anonymous-struct
        )
    else()
        target_compile_options(${lib} PRIVATE
            # clang options
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:
                -Wno-nested-anon-types
                -Wno-gnu-anonymous-struct
            >
            $<$<CXX_COMPILER_ID:GNU>:
                -Wno-cast-function-type
            >
            # not clang options
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
                -Wno-pedantic
            >
        )
    endif()

    target_include_directories(
        ${lib}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder>
        $<INSTALL_INTERFACE:lib/basisu/transcoder>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/zstd>
        $<INSTALL_INTERFACE:lib/basisu/zstd>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/other_include>
        $<INSTALL_INTERFACE:other_include>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<INSTALL_INTERFACE:utils>
    )

    if( LIB_TYPE STREQUAL STATIC )
        target_compile_definitions(${lib} PUBLIC KHRONOS_STATIC)
    endif()

    # To reduce size, don't support transcoding to ancient formats.
    target_compile_definitions(${lib} PRIVATE BASISD_SUPPORT_FXT1=0)

    # TODO: make options for all formats and good per-platform defaults
    # - BASISD_SUPPORT_UASTC
    # - BASISD_SUPPORT_DXT1 (BC1)
    # - BASISD_SUPPORT_DXT5A (BC3/4/5)
    # - BASISD_SUPPORT_BC7
    # - BASISD_SUPPORT_BC7_MODE5
    # - BASISD_SUPPORT_PVRTC1
    # - BASISD_SUPPORT_ETC2_EAC_A8
    # - BASISD_SUPPORT_ASTC
    # - BASISD_SUPPORT_ATC
    # - BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY
    # - BASISD_SUPPORT_ETC2_EAC_RG11
    # - BASISD_SUPPORT_FXT1
    # - BASISD_SUPPORT_PVRTC2

    if(WIN32)
        target_compile_definitions(
            ${lib}
        PRIVATE
            # Only set dllexport when building a shared library.
            $<$<STREQUAL:${LIB_TYPE},SHARED>:KTX_API=__declspec\(dllexport\)>
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
        # The generator automatically sets the needed VCLinker
        # option when a .def file is seen in sources.
        # The def files that we add have a different syntax depending on the ABI
        if(MINGW)
            target_sources(
                ${lib}
            PRIVATE
                lib/internalexport_mingw.def
                $<${write}:lib/internalexport_write_mingw.def>
            )
            # Need these flags if mingw happens to target the ucrt (new) rather 
            # than the legacy msvcrt. Otherwise tests will fail to run because
            # the necessary dlls will be missing. If we statically link 
            # them instead it's fine. This does not cause any abberations if 
            # the mingw toolchain targets msvcrt instead.
            target_link_options(${lib} PUBLIC -static-libgcc -static-libstdc++)
        else()
            target_sources(
                ${lib}
            PRIVATE
                lib/internalexport.def
                $<${write}:lib/internalexport_write.def>
            )
        endif()
    elseif(EMSCRIPTEN)
        target_compile_definitions(${lib} PRIVATE
            # To reduce size, don't support transcoding to formats not
            # supported # by WebGL.
            BASISD_SUPPORT_ATC=0
            BASISD_SUPPORT_PVRTC2=0
            # Don't support higher quality mode to avoid 64k table.
            BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0
            KTX_OMIT_VULKAN=1
        )
    endif()

    if(KTX_FEATURE_KTX1)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX1)
        target_sources(
            ${lib}
        PRIVATE
            lib/texture1.c
            lib/texture1.h
        )
    endif()

    if(KTX_FEATURE_KTX2)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX2)
    endif()

    if(WIN32)
        # By wrapping in generator expression we force multi configuration
        # generators (like Visual Studio) to take the exact path and not
        # change it.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
        
        if(MINGW)
            # Check if the Threads package is provided; if using Mingw it MIGHT be
            find_package(Threads)
            if(Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
                target_compile_definitions(ktx PRIVATE WIN32_HAS_PTHREADS)
                target_link_libraries(ktx PRIVATE Threads::Threads)
            endif()
        endif()

    elseif(APPLE)
        if(KTX_EMBED_BITCODE)
            target_compile_options(${lib} PRIVATE "-fembed-bitcode")
        endif()
        if(NOT IOS)
            # Set a common RUNTIME_OUTPUT_DIR for all targets, so that
            # INSTALL RPATH is functional in build directory as well.
            # BUILD_WITH_INSTALL_RPATH is necessary for working code signing.
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KTX_BUILD_DIR}/$<CONFIG>)
        endif()

    elseif(LINUX)
        find_package(Threads REQUIRED)
        target_link_libraries(
            ${lib}
        PUBLIC
            dl
            Threads::Threads
        )
    endif()

	target_compile_definitions( ${lib} PRIVATE KTX_OMIT_VULKAN=1)
endmacro(commom_lib_settings)

set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

commom_lib_settings(ktx 1)
commom_lib_settings(ktx_read 0)

target_compile_definitions(
    ktx_read
PRIVATE
    # We're reading the files ourselves so don't need Basis KTX v2 support.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=0
)

# Adding write capability to target ktx

set(BASISU_ENCODER_CXX_SRC
    lib/basisu/encoder/basisu_backend.cpp
    lib/basisu/encoder/basisu_backend.h
    lib/basisu/encoder/basisu_basis_file.cpp
    lib/basisu/encoder/basisu_basis_file.h
    lib/basisu/encoder/basisu_bc7enc.cpp
    lib/basisu/encoder/basisu_bc7enc.h
    lib/basisu/encoder/basisu_comp.cpp
    lib/basisu/encoder/basisu_comp.h
    lib/basisu/encoder/basisu_enc.cpp
    lib/basisu/encoder/basisu_enc.h
    lib/basisu/encoder/basisu_etc.cpp
    lib/basisu/encoder/basisu_etc.h
    lib/basisu/encoder/basisu_frontend.cpp
    lib/basisu/encoder/basisu_frontend.h
    lib/basisu/encoder/basisu_gpu_texture.cpp
    lib/basisu/encoder/basisu_gpu_texture.h
    lib/basisu/encoder/basisu_kernels_declares.h
    lib/basisu/encoder/basisu_kernels_imp.h
    lib/basisu/encoder/basisu_kernels_sse.cpp
    lib/basisu/encoder/basisu_miniz.h
    lib/basisu/encoder/basisu_opencl.cpp
    lib/basisu/encoder/basisu_opencl.h
    lib/basisu/encoder/basisu_pvrtc1_4.cpp
    lib/basisu/encoder/basisu_pvrtc1_4.h
    lib/basisu/encoder/basisu_resample_filters.cpp
    lib/basisu/encoder/basisu_resampler_filters.h
    lib/basisu/encoder/basisu_resampler.cpp
    lib/basisu/encoder/basisu_resampler.h
    lib/basisu/encoder/basisu_ssim.cpp
    lib/basisu/encoder/basisu_ssim.h
    lib/basisu/encoder/basisu_uastc_enc.cpp
    lib/basisu/encoder/basisu_uastc_enc.h
    lib/basisu/encoder/cppspmd_flow.h
    lib/basisu/encoder/cppspmd_math.h
    lib/basisu/encoder/cppspmd_math_declares.h
    lib/basisu/encoder/cppspmd_sse.h
    lib/basisu/encoder/cppspmd_type_aliases.h
)

target_sources(
    ktx
PRIVATE
    lib/basis_encode.cpp
    lib/astc_encode.cpp
    ${BASISU_ENCODER_C_SRC}
    ${BASISU_ENCODER_CXX_SRC}
    lib/writer1.c
    lib/writer2.c
)

# Turn off these warnings until Rich fixes the occurences.
# It it not clear to me if generator expressions can be used here
# hence the long-winded way.
#message(STATUS
#        "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID} "
#        "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}"
#)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
# Currently no need to disable any warnings in basisu code. Rich fixed them.
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(
        # It's too much work to discriminate which files need which warnings
        # disabled.
        ${BASISU_ENCODER_CXX_SRC}
        PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-variable;-Wno-class-memaccess;-Wno-misleading-indentation;-Wno-extra;-Wno-deprecated-copy;-Wno-parentheses;-Wno-strict-aliasing"
    )
    set_source_files_properties(
        lib/basisu/transcoder/basisu_transcoder.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-function;-Wno-unused-variable;-Wno-class-memaccess;-Wno-maybe-uninitialized"
    )
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "11.0")
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-but-set-variable"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        # Versions equivalency from https://en.wikipedia.org/wiki/Xcode#Xcode_11.x_-_14.x_(since_SwiftUI_framework)_2
        if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "14")
            set( clang_version ${CMAKE_CXX_COMPILER_VERSION})
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.1.0")
            set( clang_version "13.0.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.0.0")
            set( clang_version "12.0.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0.5")
            set( clang_version "11.1.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_EQUAL "12.0.0")
            set( clang_version "10.0.0")
        else()
            message(FATAL "Unsupported AppleClang version")
        endif()
    else()
        set( clang_version ${CMAKE_CXX_COMPILER_VERSION} )
    endif()
    if (${clang_version} VERSION_GREATER_EQUAL "12.0.0")
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-deprecated-copy;-Wno-uninitialized-const-reference"
        )
        # Primarily an issue on windows using clang/clangcl
        # Lowest tested clang version was 12.0.0 and highest was 14.0.4
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-function"
        )
    else()
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter"
        )
    endif()
    if (${clang_version} VERSION_GREATER_EQUAL "13.0.0")
        get_source_file_property(zstd_options lib/basisu/zstd/zstd.c COMPILE_OPTIONS)
        set_source_files_properties(
            # We haven't fixed zstd.c because the fix would have to be applied
            # every time the upstream script is used to create an updated
            # single file decoder.
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "${zstd_options};-Wno-unused-but-set-variable"
        )
    endif()
    if (${clang_version} VERSION_GREATER_EQUAL "15.0")
        # These are for Emscripten which is ahead of xcode in its clang
        # version. Also future proofing for when xcode catches up.
        set_source_files_properties(
            ${BASISU_ENCODER_CXX_SRC}
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-variable;-Wno-unused-parameter"
        )
        set_source_files_properties(
            lib/basisu/transcoder/basisu_transcoder.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-function;-Wno-unused-variable"
        )
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-but-set-variable;-Wno-bitwise-instead-of-logical"
        )
    endif()
else()
    message(ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

target_include_directories(
    ktx
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu>
    $<INSTALL_INTERFACE:lib/basisu>
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_INCLUDE_DIRS}>
)
target_compile_definitions(
    ktx
PUBLIC
    KTX_FEATURE_WRITE
PRIVATE
    # BASISD_SUPPORT_KTX2 has to be 1 to compile the encoder. We
    # don't use it. Hopefully it doesn't add too much code. We're using
    # the zstd encoder in basisu by explicitly including the file in our
    # source list. We don't need the related code in the encoder.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=1
    $<$<BOOL:${BASISU_SUPPORT_SSE}>:BASISU_SUPPORT_SSE=1>
    $<$<NOT:$<BOOL:${BASISU_SUPPORT_SSE}>>:BASISU_SUPPORT_SSE=0>
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:BASISU_SUPPORT_OPENCL=1>
    $<$<NOT:$<BOOL:${BASISU_SUPPORT_OPENCL}>>:BASISU_SUPPORT_OPENCL=0>
)
target_compile_options(
    ktx
PRIVATE
    $<$<AND:$<BOOL:${BASISU_SUPPORT_SSE}>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:
        -msse4.1
    >
)
target_link_libraries(
    ktx
PRIVATE
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_LIBRARIES}>
)

add_library(objUtil STATIC
    utils/argparser.cpp
    utils/argparser.h
    utils/ktxapp.h
    utils/sbufstream.h
    utils/scapp.h
    utils/stdafx.h
    utils/unused.h
    )
target_include_directories(
    objUtil
PUBLIC
    utils
)

# In C++ apps that use statically linked Libraries all compilatiom units must
# be compiled with matching symbol visibility settings to avoid warnings from
# clang. Many 3rd party libraries, including libassimp which is used by the
# load test apps that statically link also to several internal libraries, use
# "hidden" to avoid conflicts with other libraries.
#
# TODO: set "hidden" as a global option. I do not want to take the time right
# now to deal with the fallout from hiding globals in libktx. Apart from
# having to mark all the public symbols of libktx for clang and gcc with
# __attribute__((visibility("default"))) there will be ramifications to
# texturetests and unittests. Marking the public symbols is easy for those
# already tagged with KTX_API. But all the symbols exported via
# internalexport.def and internalexport_write.def have to be tagged with
# KTX_API which may also require additional inclusion of ktx.h to get the
# definition.
set (STATIC_APP_LIB_SYMBOL_VISIBILITY hidden)
set_target_properties(objUtil PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)
if(KTX_FEATURE_STATIC_LIBRARY AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set_source_files_properties(
        lib/astc_encode.cpp
        PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden"
    )
endif()

add_subdirectory(interface/basisu_c_binding)

# Only one architecture is supported at once, if neither of
# ISA_SSE41 and ISA_SSE2  are defined ISA_AVX2 is chosen.
# If ISA_AVX2 fails to compile user must chose other x86 options.
# On arm based systems ISA_NEON is default

list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" ASTC_BUILD_UNIVERSAL)
list(LENGTH CMAKE_OSX_ARCHITECTURES ARCHITECTURE_COUNT)

if(${ASTC_BUILD_UNIVERSAL} EQUAL -1 AND ARCHITECTURE_COUNT LESS_EQUAL 1)
    if (${ISA_NONE})
        set(ASTC_LIB_TARGET astcenc-none-static)
    else()
        if(CPU_ARCHITECTURE STREQUAL x86_64 OR CPU_ARCHITECTURE STREQUAL x86)
            if (${ISA_SSE41})
                set(ASTC_LIB_TARGET astcenc-sse4.1-static)
            elseif (${ISA_SSE2})
                set(ASTC_LIB_TARGET astcenc-sse2-static)
            else()
                set(ISA_AVX2 ON)
                set(ASTC_LIB_TARGET astcenc-avx2-static)
            endif()
            if(CPU_ARCHITECTURE STREQUAL x86)
                set(ISA_NONE ON)
                set(ISA_AVX2 OFF)
                set(ASTCENC_POPCNT 0)
                set(ASTC_LIB_TARGET astcenc-none-static)
            endif()
        elseif(CPU_ARCHITECTURE STREQUAL armv8 OR CPU_ARCHITECTURE STREQUAL arm64)
            set(ASTC_LIB_TARGET astcenc-neon-static)
            set(ISA_NEON ON)
        else()
            message(STATUS "Unsupported ISA for ${CPU_ARCHITECTURE} arch, using ISA_NONE.")
            set(ASTC_LIB_TARGET astcenc-none-static)
            set(ISA_NONE ON)
        endif()
    endif()
else()
    set(ASTC_LIB_TARGET astcenc-static)
endif()

# astcenc
set(CLI OFF) # Only build as library not the CLI astcencoder
add_subdirectory(lib/astc-encoder)
set_property(TARGET ${ASTC_LIB_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(ktx PRIVATE ${ASTC_LIB_TARGET})

set(KTX_INSTALL_TARGETS ktx)
if(KTX_FEATURE_STATIC_LIBRARY AND NOT APPLE)
    list(APPEND KTX_INSTALL_TARGETS ${ASTC_LIB_TARGET})
endif()

# Install

if(APPLE OR LINUX)
    # Have library's name links as separate component
    set(KTX_NAMELINKS ON)
    install(TARGETS ${KTX_INSTALL_TARGETS}
        EXPORT KTXTargets
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        FRAMEWORK
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_SKIP
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
    install(TARGETS ${KTX_INSTALL_TARGETS}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_ONLY
    )
else()
    # No name links on Windows
    set(KTX_NAMELINKS OFF)
    install(TARGETS ${KTX_INSTALL_TARGETS}
        EXPORT KTXTargets
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT dev
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT library
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
endif()

install(EXPORT KTXTargets
    FILE KtxTargets.cmake
    NAMESPACE KTX::
    DESTINATION lib/cmake/ktx
    COMPONENT library
)

install( FILES
    "cmake/KtxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KtxConfigVersion.cmake"
    DESTINATION lib/cmake/ktx
    COMPONENT library
)