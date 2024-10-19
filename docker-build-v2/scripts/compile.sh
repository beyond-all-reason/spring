#!/bin/bash

set -e

cmake --build /build/out
cmake --install /build/out

# Manually remove the lib, include and share directories coming
# from the GNUInstallDirs. Unfortunately it's not possible to
# easily disable in CMake all installation targets exported when
# using add_subdirectory so we drop them from installation manually.
# This is done to heavily reduce the resulting install size.
cd /build/out/install
rm -rf lib include share
