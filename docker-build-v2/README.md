# Docker Engine Build v2

This directory contains work in progress next version of the Docker engine
build scripts. **If you're on Windows check the [specified section](README.md#windows-specific-guide---step-1-on-windows) first**

## Local usage - Step 2 on Windows

To execute build locally use `build.sh` script

```console
$ docker-build-v2/build.sh --help
Usage: docker-build-v2/build.sh [--help] [--configure|--compile] {windows|linux} [cmake_flag...]
Options:
  --help       print this help message
  --configure  only configure, don't compile
  --compile    only compile, don't configure
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

To list all cmake options and their values run:

```shell
docker-build-v2/build.sh --configure linux -LH
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

## Windows Specific Guide - Step 1 on Windows
`build.sh` depends on the Docker engine, not the Docker desktop. So, to get started, there are a few steps first.

### Using WSL - Ubuntu
1. Head to Microsoft Store and install Ubuntu
2. Follow the [Docker Engine guide](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository) to install it
3. Now you can follow Local Usage to build as normal

#### VS Code Extension
If you're using VSCode as your IDE you can use [Remote Development Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack)

#### Smaller QoL Tips - Windows
- Symbolic Links can replace Remote Development, use them to make edits on your regular Windows OS with whatever IDE you like and sync up with WSL's folder
- They are also useful for quick debugging, link up your `WSL/Ubuntu/{Spring-Repo-Folder}/build-windows/install` with your `{BAR-Game-Folder}/data/engine/{Local-Engine-Folder-Name}` this way you can quickly run `bash build windows` on WSL and it will appear and ready to use in your BAR Folder
- To use Symbolic Links, you can do so with `mklink` command in Terminal. However, it is highly recommended to use [Link Shell Extension (LSE)](https://schinagl.priv.at/nt/hardlinkshellext/linkshellextension.html) just make sure to make a [Restore Point](https://support.microsoft.com/en-us/windows/system-restore-a5ae3ed9-07c4-fd56-45ee-096777ecd14e#:~:text=Apply%20a%20restore%20point%20from,%E2%80%8B%E2%80%8B%E2%80%8B%E2%80%8B%E2%80%8B) just in case


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


For details on private testing, check the wiki [here](https://github.com/beyond-all-reason/spring/wiki/Pre-release-testing-Checklist,-and-release-engine-checklist#private-testing)
