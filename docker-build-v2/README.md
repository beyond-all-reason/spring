# Engine Docker Build v2

This directory contains work in progress next version of the Docker engine
build scripts.

## Local usage

To execute build locally use `build.sh` script

```console
$ docker-build-v2/build.sh
USAGE: docker-build-v2/build.sh {windows|linux} [cmake_flag...]
```

For example

```shell
docker-build-v2/build.sh windows
```

will:

1. Automatically fetch/update Docker image with engine build environment from
   [GitHub packages](https://github.com/beyond-all-reason?tab=packages&repo_name=spring)
2. Configure the release configuration of engine build
3. Compile and install engine using following paths in the repository root:
   - `.cache`: compilation cache
   - `build-windows`: compilation output
   - `build-windows/install`: ready to use installation

### Custom build config

Because script takes cmake arguments compilation can be easily adjusted this
way. For example to compile Linux release with Tracy support and skip building
headless:

```shell
docker-build-v2/build.sh linux -DBUILD_spring-headless=OFF -DTRACY_ENABLE=ON
```

### Custom Docker image

Before downloading official Docker build environment image, `build.sh` will
first lookup if there exists locally a Docker image with tag
`recoil-build-amd64-windows` (and `-linux` for Linux). If you want to adjust
the Docker build image, test local changes, build it with that tag:

```shell
docker build -t recoil-build-amd64-windows docker-build-v2/amd64-windows
```

and `build.sh` will use it.

## Overview

There are two separate build images, one for Windows, one for Linux. The Docker
images are built as part of GitHub actions workflow and stored in the GitHub
Package repository. The images are relatively small (~300-400MiB compressed)
and building them takes 2-3 minutes.

Each of the images contains a complete required build environment with all
dependencies installed (including
[mingwlibs](https://github.com/beyond-all-reason/mingwlibs64) etc.), configured
for proper resolution from engine CMake configuration, and caching with
[ccache](https://ccache.dev/).

The build step is then a platform agnostic invocation of CMake with generic
release build configuration.

To sum up, there is a separation:

- Build environment: dependencies, toolchain, etc. are part of the Docker
  image.
- Build options: e.g. optimization level, are in the platform agnostic build
  script `scripts/build.sh` stored in repository.
- Post build scripts: platform agnostic scripts:
  - `script/split-debug-info.sh`: Splits debug information from binaries
  - `script/package.sh`: Creates 2 archives, one with engine and one with
     debug symbols.
