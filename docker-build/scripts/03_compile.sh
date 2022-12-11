cd "${BUILD_DIR}"

mold -run ninja -j$(nproc) all
