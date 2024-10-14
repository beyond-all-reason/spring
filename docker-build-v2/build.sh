#!/bin/bash

set -e -u -o pipefail

if [[ "$#" -lt 1 || ! "$1" =~ ^(windows|linux)$ ]]; then
    echo "USAGE: $0 {windows|linux} [cmake_flag...]"
    exit 1
fi
os="$1"

cd "$(dirname "$(readlink -f "$0")")/.."
mkdir -p build-$os .cache/ccache-$os

# Use localy build image if available, and pull from upstream if not
image=recoil-build-amd64-$os:latest
if [[ -z "$(docker images -q $image 2> /dev/null)" ]]; then
  image=ghcr.io/beyond-all-reason/recoil-build-amd64-$os:latest
  docker pull $image > /dev/null
fi

docker run -it --rm \
    -v /etc/passwd:/etc/passwd:ro \
    -v /etc/group:/etc/group:ro \
    --user=$(id -u):$(id -g) \
    -v $(pwd):/build/src:ro \
    -v $(pwd)/.cache/ccache-$os:/build/cache:rw \
    -v $(pwd)/build-$os:/build/out:rw \
    $image \
    bash /build/src/docker-build-v2/scripts/build.sh "${@:2}"
