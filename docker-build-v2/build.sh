#!/bin/bash

set -e -u -o pipefail

if [[ $(id -u) -eq 0 && -z "${SKIP_ROOT_CHECK:-}" ]]; then
  echo "You are trying to run build.sh as root, that won't work!"
  echo ""
  echo "If you get permission errors when running docker, check if you've finished the"
  echo "post installation steps and are member of \`docker\` system group."
  echo "See official docs: https://docs.docker.com/engine/install/linux-postinstall/"
  exit 2
fi

USAGE="Usage: $0 [-h|--help] [--configure|--compile] [-j|--jobs {number_of_jobs}] [--arch {arm64|amd64}] {windows|linux} [cmake_flag...]"
export CONFIGURE=true
export COMPILE=true
export CMAKE_BUILD_PARALLEL_LEVEL=

case $(uname -m) in
  x86_64) ARCH=amd64 ;;
  aarch64) ARCH=arm64 ;;
  *) ARCH=unknown ;;
esac

OS=
while (( $# > 0 )); do
  case $1 in
    --configure)
      CONFIGURE=true
      COMPILE=false
      shift
      ;;
    --compile)
      CONFIGURE=false
      COMPILE=true
      shift
      ;;
    -h|--help)
      echo "$USAGE"
      echo "Options:"
      echo "  -h, --help   print this help message"
      echo "  --configure  only configure, don't compile"
      echo "  --compile    only compile, don't configure"
      echo "  -j, --jobs   number of concurrent processes to use when building"
      echo "  --arch       arm64 or amd64, defaults to host"
      echo ""
      echo "Some behaviors can be changed by setting environment variables. Consult the script source for those more advanced use cases."
      exit 0
      ;;
    --arch)
      shift
      ARCH="$1"
      shift
      ;;
    -j|--jobs)
      shift
      # Match numeric, starting with non-zero digit
      if ! [[ "${1-}" =~ ^[1-9]+[0-9]*$ ]]; then
        echo "--jobs requires a number"
        echo ""
        echo "$USAGE"
        exit 1
      fi
      CMAKE_BUILD_PARALLEL_LEVEL="$1"
      shift
      ;;
    windows|linux)
      OS="$1"
      shift
      break
      ;;
    *)
      break
  esac
done
if [[ -z $OS ]]; then
  echo "$USAGE"
  exit 1
fi

PLATFORM="$ARCH-$OS"
if ! [[ "$PLATFORM" =~ ^(amd64-windows|amd64-linux|arm64-linux)$ ]]; then
  echo "Target platform $PLATFORM is not supported, supported platforms are:"
  echo " - amd64-windows"
  echo " - amd64-linux"
  echo " - arm64-linux"
  echo ""
  echo "$USAGE"
  exit 1
fi

cd "$(dirname "$(readlink -f "$0")")/.."

# The engine uses git submodules quite extensively and it's a common noob trap
# that people forget to update and initialize them. Let's block the build when
# we detect that it's the case and allow to continue after creation of escape
# hatch file.
UNSYNCED_SUBMODULES="$(git submodule status --recursive | { grep -E '^(\+|-)' || test $? = 1; } | awk '{ print " - " $2 }')"
if [[ -n "$UNSYNCED_SUBMODULES" ]]; then
  echo "WARNING: You have unsynced git submodules!"
  echo ""
  echo "$UNSYNCED_SUBMODULES"
  echo ""
  echo "Running following command should be sufficient to synchronize them:"
  echo ""
  echo "  git submodule update --init --recursive"
  echo ""
  if [[ -f ".i-understand-git-submodules.txt" ]]; then
    echo "Continuing the build because .i-understand-git-submodules.txt file exists."
  else
    echo 'If that is intended and you know what you are doing create `.i-understand-git-submodules.txt` file to skip this warning.'
    echo ""
    echo "Exiting the build."
    exit 1
  fi
fi

mkdir -p build-$PLATFORM .cache/ccache-$PLATFORM

# Build container image selection, allow overriding.
if [[ -n "${CONTAINER_IMAGE:-}" ]]; then
  IMAGE="$CONTAINER_IMAGE"
else
  source docker-build-v2/images_versions.sh
  IMAGE=ghcr.io/beyond-all-reason/recoil-build-$PLATFORM@${image_version[$PLATFORM]}
fi

source docker-build-v2/_resolve_container_runtime.sh

# With the most common rootful docker as runtime, the users inside of the
# container maps directly to users on the host and because user in container
# is root, all files created in mounted volumes are owned by root outside of
# container. To avoid this, we mount /etc/passwd and /etc/group and use --user
# flag to run the container as current host user.
#
# This is not the case when using rootless podman or docker, because the root
# inside of container is mapped via user namespaces to the calling user on
# the host. Another option we handle is Docker Desktop, which runs containers
# in a separate VM and does special remapping for mounted volumes, except when
# we run from WSL because then it's WSL that's the VM.
#
# Because this heuristic might not work in some esoteric setups we haven't
# foreseen, we allow specifying behavior with FORCE_UID_FLAGS and
# FORCE_NO_UID_FLAGS environment variables. We also try to detect the
# misconfiguration inside the container and error out with instructions to set
# the variables and report the issue.
UID_FLAGS=""
if [[ -n "${FORCE_UID_FLAGS:-}" ]] || (
       [[ -z "${FORCE_NO_UID_FLAGS:-}" && "$RUNTIME" == "docker" ]] &&
       [[ "$(docker info -f '{{.OperatingSystem}}')" != "Docker Desktop" || -n "${WSL_DISTRO_NAME:-}" ]] &&
       ! docker info -f '{{.SecurityOptions}}' | grep -q rootless
   ); then
    UID_FLAGS="-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro --user=$(id -u):$(id -g)"
fi

# Allow passing extra arguments to runtime for example to mount additional volumes
EXTRA_ARGS=()
if [[ -n "${CONTAINER_RUNTIME_EXTRA_ARGS:-}" ]]; then
  eval "EXTRA_ARGS=($CONTAINER_RUNTIME_EXTRA_ARGS)"
fi

# Support running directly from Windows without WSL layer: we need to pass real
# native Windows path to docker.
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
  CWD="$(cygpath -w -a .)"
  P="\\"
else
  CWD="$(pwd)"
  P="/"
fi

# Handle git worktrees: the container needs access to the shared .git directory
# for version generation. In a worktree, --absolute-git-dir returns the
# worktree-specific dir while --git-common-dir returns the shared .git root.
WORKTREE_MOUNTS=""
GIT_DIR=$(git rev-parse --absolute-git-dir)
GIT_COMMON_DIR=$(git rev-parse --path-format=absolute --git-common-dir)
if [[ "$GIT_DIR" != "$GIT_COMMON_DIR" ]]; then
  WORKTREE_MOUNTS="-v $GIT_COMMON_DIR:$GIT_COMMON_DIR:ro"
fi

# Docker's -t requires stdin AND stdout to be TTYs; in CI, pipes, or agent
# contexts one or both are missing and docker errors out with "the input
# device is not a TTY". Only add -t when it's safe; -i is harmless either
# way (non-interactive stdin just sees EOF).
TTY_FLAG=
if [[ -t 0 && -t 1 ]]; then
  TTY_FLAG=-t
fi

$RUNTIME run --platform=linux/$ARCH -i $TTY_FLAG --rm \
    -v "$CWD${P}":/build/src:z,ro \
    -v "$CWD${P}.cache${P}ccache-$PLATFORM":/build/cache:z,rw \
    -v "$CWD${P}build-$PLATFORM":/build/out:z,rw \
    $UID_FLAGS \
    $WORKTREE_MOUNTS \
    -e CONFIGURE \
    -e COMPILE \
    -e CMAKE_BUILD_PARALLEL_LEVEL \
    "${EXTRA_ARGS[@]}" \
    $IMAGE \
    bash -c '
set -e

if [[ "$(id -u)" != "$(stat -c %u /build/src)" ]]; then
  echo "Error: Inside the container, the user ($(id -u)) does not match"
  echo "the owner of the source code files ($(stat -c %u /build/src))."
  echo ""
  echo "This likely means that the script failed to apply heuristics and"
  echo "set flags for the runtime correctly. Please report this issue on"
  echo "GitHub and include information about your environment and output"
  echo "of \`docker info\`."
  echo ""
  echo "As a workaround, try setting the environment variable"
  echo "FORCE_UID_FLAGS=1 or FORCE_NO_UID_FLAGS=1."
  exit 1
fi

cd /build/src/docker-build-v2/scripts
$CONFIGURE && ./configure.sh "$@"
if $COMPILE; then
  if $CONFIGURE; then
    ./compile.sh
  else
    ./compile.sh "$@"
  fi
  # When compiling for windows, we must strip debug info because windows does
  # not handle the output binary size...
  if [[ $ENGINE_PLATFORM =~ .*windows ]]; then
    ./split-debug-info.sh
  fi
fi
' -- "$@"
