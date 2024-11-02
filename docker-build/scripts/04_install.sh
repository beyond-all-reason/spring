cd "${BUILD_DIR}"

make install

# Manually remove the lib, include and share directories coming
# from the GNUInstallDirs. Unfortunately it's not possible to
# easily disable in CMake all installation targets exported when
# using add_subdirectory so we drop them from installation manually.
# This is done to heavily reduce the resulting install size.
cd "${INSTALL_DIR}"
rm -rf lib include share
