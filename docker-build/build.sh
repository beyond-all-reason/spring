#!/bin/bash

DOCKER_BUILD_DIR="$(dirname -- "$( readlink -f -- "$0"; )";)"
ENGINE_GIT_DIR="$(dirname "${DOCKER_BUILD_DIR}")"
CACHE_DIR="${DOCKER_BUILD_DIR}/cache"
CONAN_USER_HOME="${CACHE_DIR}"
CCACHE_DIR="${CACHE_DIR}/ccache"
DOCKER_SCRIPTS_DIR="${DOCKER_BUILD_DIR}/scripts"

# Create directories on the host, otherwise docker would create them as root
mkdir -p "${CCACHE_DIR}"
mkdir -p "${CONAN_USER_HOME}/.conan"

DOCKER_RUN_ARGS=(
    --rm
    -e CI
    -v /etc/passwd:/etc/passwd:ro
    -v /etc/group:/etc/group:ro
    --user="$(id -u):$(id -g)"
    -v "${ENGINE_GIT_DIR}":"/spring"
    -v "${CCACHE_DIR}":"/ccache"
    -v "${CONAN_USER_HOME}":"/conan"
    -v "${DOCKER_SCRIPTS_DIR}":"/scripts"
)
if [ ${CI} ]; then
    ARTIFACTS_DIR="${DOCKER_SCRIPTS_DIR}/artifacts"
    CCACHE_DBG_DIR="${DOCKER_SCRIPTS_DIR}/ccache_dbg"
    mkdir -p "${ARTIFACTS_DIR}"
    mkdir -p "${CCACHE_DBG_DIR}"
    DOCKER_RUN_ARGS+=(
        -v "${CCACHE_DBG_DIR}":"/ccache_dbg"
        -v "${ARTIFACTS_DIR}":"/artifacts"
    )
else
    DOCKER_RUN_ARGS+=(-it)
fi

docker run "${DOCKER_RUN_ARGS[@]}" "bar-spring:latest" build "$@"
