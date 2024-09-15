#!/bin/sh
cmake \
    -S /build/src \
    -B /build/out \
    -DCMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DMINGWLIBS:PATH=/build/mingwlibs64 \
    -DCMAKE_INSTALL_PREFIX:PATH=/build/target \
    -DAI_TYPES=NATIVE \
    -DAI_EXCLUDE_REGEX="^CppTestAI$" \
    -DUSERDOCS_PLAIN=ON \
    -DINSTALL_PORTABLE=ON \
    -DWITH_MAPCOMPILER=OFF \
    -GNinja \
    -DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    "$@"
