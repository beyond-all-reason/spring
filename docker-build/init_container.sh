#!/bin/bash

DOCKER_BUILD_DIR="$(dirname -- "$( readlink -f -- "$0"; )";)"

DOCKER_BUILD_ARGS=(
    -f "${DOCKER_BUILD_DIR}/Dockerfile"
    -t "bar-spring:latest"
)

DOCKER_BUILDKIT=1 docker build "${DOCKER_BUILD_ARGS[@]}" "${DOCKER_BUILD_DIR}"
