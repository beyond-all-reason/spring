#!/bin/bash

set -e -u -o pipefail

USAGE="Usage: $0 [--help] [--deps|--configure|--compile] {windows|linux} [cmake_flag...]"
export DEPS=true
export CONFIGURE=true
export COMPILE=true
export OS=
for arg in "$@"; do
  case $arg in
    --deps)
      DEPS=true
      CONFIGURE=false
      COMPILE=false
      shift
      ;;
    --configure)
      DEPS=false
      CONFIGURE=true
      COMPILE=false
      shift
      ;;
    --compile)
      DEPS=false
      CONFIGURE=false
      COMPILE=true
      shift
      ;;
    --help)
      echo $USAGE
      echo "Options:"
      echo "  --help       print this help message"
      echo "  --deps       only install conan dependencies, don't configure or compile"
      echo "  --configure  only configure, don't compile"
      echo "  --compile    only compile, don't configure"
      exit 0
      ;;
    windows|linux)
      OS="$arg"
      shift
      ;;
    *)
      break
  esac
done
if [[ -z $OS ]]; then
  echo $USAGE
  exit 1
fi

cd "$(dirname "$(readlink -f "$0")")/.."
mkdir -p build-$OS .cache/ccache-$OS .conan2-$OS/profiles
cp docker-build-v2/conan_profile .conan2-$OS/profiles
cp docker-build-v2/amd64-$OS/conan_build_profile .conan2-$OS/profiles

# Use localy build image if available, and pull from upstream if not
image=recoil-build-amd64-$OS:latest
if [[ -z "$(docker images -q $image 2> /dev/null)" ]]; then
  image=ghcr.io/beyond-all-reason/recoil-build-amd64-$OS:latest
  docker pull $image
fi

docker run -it --rm \
    -v $(pwd):/build/src:ro \
    -v $(pwd)/.cache/ccache-$OS:/build/cache:rw \
    -v $(pwd)/build-$OS:/build/out:rw \
    -v $(pwd)/.conan2-$OS:/build/.conan2:rw \
    -e CONFIGURE \
    -e COMPILE \
    -e OS \
    $image \
    bash -c '
set -e
echo "$@"
cd /build/src/docker-build-v2/scripts
$DEPS && ./graph-deps.sh "$@"
$DEPS && ./deps.sh "$@"
$CONFIGURE && ./configure.sh "$@"
export OS
if $COMPILE; then
  ./compile.sh
  # When compiling for windows, we must strip debug info because windows does
  # not handle the output binary size...
  if [[ $ENGINE_PLATFORM =~ .*windows ]]; then
    ./split-debug-info.sh
  fi
fi
' -- "$@"
