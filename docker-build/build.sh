#!/bin/sh

ENGINE_GIT_DIR="$(dirname "$(dirname -- "$( readlink -f -- "$0"; )";)")"
CACHE_DIR="$(dirname -- "$( readlink -f -- "$0"; )";)/cache"
LINUX_LIBS_DIR="${CACHE_DIR}/spring-static-libs"
WINDOWS_LIBS_DIR="${CACHE_DIR}/mingwlibs64"
CCACHE_DIR="${CACHE_DIR}/ccache"

# Create directories on the host, otherwise docker would create them as root
mkdir -p "${LINUX_LIBS_DIR}"
mkdir -p "${WINDOWS_LIBS_DIR}"
mkdir -p "${CCACHE_DIR}"

docker run -it --rm                                      \
            -v /etc/passwd:/etc/passwd:ro                \
            -v /etc/group:/etc/group:ro                  \
            --user="$(id -u):$(id -g)"                   \
            -v "${ENGINE_GIT_DIR}":"/spring"             \
            -v "${LINUX_LIBS_DIR}":"/spring-static-libs" \
            -v "${WINDOWS_LIBS_DIR}":"/mingwlibs64"      \
            -v "${CCACHE_DIR}":"/ccache"                 \
            bar-spring build -l "$@"
