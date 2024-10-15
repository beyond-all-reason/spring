#!/bin/bash

set -e

cmake --fresh -S /build/src -B /build/out \
    -DCMAKE_INSTALL_PREFIX:PATH=/build/out/install \
    -DAI_EXCLUDE_REGEX="^CppTestAI$" \
    -DUSERDOCS_PLAIN=ON \
    -DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -G Ninja \
    "$@"

cmake --build /build/out
cmake --install /build/out

# Manually remove the lib, include and share directories coming
# from the GNUInstallDirs. Unfortunately it's not possible to
# easily disable in CMake all installation targets exported when
# using add_subdirectory so we drop them from installation manually.
# This is done to heavily reduce the resulting install size.
cd /build/out/install
rm -rf lib include share
