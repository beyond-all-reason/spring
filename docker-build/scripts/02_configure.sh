cd "${SPRING_DIR}"

if [ ! ${LOCAL_BUILD} ]; then
    rm -rf "${BUILD_DIR}"
fi
mkdir -p "${BUILD_DIR}/bin-dir"

EXTRA_CMAKE_ARGS=()
if [ "${PLATFORM}" == "linux-64" ]; then
    EXTRA_CMAKE_ARGS+=(
        -DCMAKE_SYSTEM_PREFIX_PATH="${LIBS_DIR}"
        -DPREFER_STATIC_LIBS:BOOL=1
        -DCMAKE_USE_RELATIVE_PATHS:BOOL=1
        -DBINDIR:PATH=./
        -DLIBDIR:PATH=./
        -DDATADIR:PATH=./
        -DDOCDIR:PATH=doc
        -DCMAKE_EXE_LINKER_FLAGS_INIT="-fuse-ld=gold"
        -DCMAKE_MODULE_LINKER_FLAGS_INIT="-fuse-ld=gold"
        -DCMAKE_SHARED_LINKER_FLAGS_INIT="-fuse-ld=gold"
    )
    export PKG_CONFIG_LIBDIR=${LIBS_DIR}/lib/pkgconfig
    export PKG_CONFIG="pkg-config --define-prefix --static"
elif [ "${PLATFORM}" == "windows-64" ]; then
    EXTRA_CMAKE_ARGS+=(
        -DMINGWLIBS=${LIBS_DIR}
    )
fi

if [ ${ONLY_LEGACY} ]; then
    EXTRA_CMAKE_ARGS+=(
        -DBUILD_spring-headless=FALSE
        -DBUILD_spring-dedicated=FALSE
    )
fi

cd "${BUILD_DIR}"
cmake \
    -DCMAKE_TOOLCHAIN_FILE="/scripts/${PLATFORM}.cmake" \
    -DMARCH_FLAG="${MYARCHTUNE}" \
    -DCMAKE_CXX_FLAGS="${MYCFLAGS}" \
    -DCMAKE_C_FLAGS="${MYCFLAGS}" \
    -DCMAKE_CXX_FLAGS_${MYBUILDTYPE}="${MYBUILDTYPEFLAGS}" \
    -DCMAKE_C_FLAGS_${MYBUILDTYPE}="${MYBUILDTYPEFLAGS}" \
    -DCMAKE_BUILD_TYPE="${MYBUILDTYPE}" \
    -DAI_TYPES=NATIVE \
    -DCMAKE_INSTALL_PREFIX:PATH="${INSTALL_DIR}" \
    -DUSERDOCS_PLAIN=ON \
    -DINSTALL_PORTABLE=ON \
    -DAI_EXCLUDE_REGEX="^CppTestAI$" \
    "${MYCMAKEFLAGS}" \
    "${EXTRA_CMAKE_ARGS[@]}" \
    "${SPRING_DIR}"
