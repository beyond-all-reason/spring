echo "Moving artifacts to publish dir: '${ARTIFACTS_DIR}'..."

mv "${BUILD_DIR}/${bin_name}" "${BUILD_DIR}/${dbg_name}" "${BUILD_DIR}/${FILENAME_BUILDOPTIONS}" "${ARTIFACTS_DIR}"
