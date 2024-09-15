#!/bin/sh
export PKG_CONFIG_LIBDIR="/build/spring-static-libs/lib/pkgconfig"
export PKG_CONFIG="pkg-config --define-prefix --static"
cmake \
    -S /build/src \
    -B /build/out \
    -DCMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_SYSTEM_PREFIX_PATH=/build/spring-static-libs \
    -DCMAKE_INSTALL_PREFIX:PATH=/build/target \
    -DPREFER_STATIC_LIBS:BOOL=1 \
    -DCMAKE_USE_RELATIVE_PATHS:BOOL=1 \
    -DBINDIR:PATH=./ \
    -DLIBDIR:PATH=./ \
    -DDATADIR:PATH=./ \
    -DMANDIR:PATH=share/man \
    -DDOCDIR:PATH=doc \
    -DAI_TYPES=NATIVE \
    -DAI_EXCLUDE_REGEX="^CppTestAI$" \
    -DUSERDOCS_PLAIN=ON \
    -DINSTALL_PORTABLE=ON \
    -DWITH_MAPCOMPILER=OFF \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    "$@"
