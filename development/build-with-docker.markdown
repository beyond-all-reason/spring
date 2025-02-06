---
layout: post
title: Building with Docker
parent: Development
permalink: development/build-with-docker
author: verybadsoldier
---

# Building with Docker

## Introduction

There is a docker image available that is based on Ubuntu being able to compile the engine.

The image can be used as a one-shot command to compile the engine and provide the artifacts by a docker volume but it can also be used as a development environment.

### Docker Image
There are two ways to obtain the docker image.

1. Checkout the repository, change into the directory `docker-build` and build the image yourself using the command: `docker build -t springrts-build .`. Additional arguments can be passed to configure the image creation process (see below).

2. The image is also available on [Docker Hub](https://hub.docker.com/repository/docker/verybadsoldier/springrts-build). It will be auto-downloaded if you use it in regular docker commands like `docker run verybadsoldier/springrts-build:latest build`. So you can replace the image name with `verybadsoldier/springrts-build:latest` in all docker commands described below.
The image on Docker Hub might not be updated at all times but we try to keep it up-to-date as good as possible.

When building the image the following arguments are available:

| Parameter|      Default |  Description |
|:----------:|:-------------:|:------|
| mxe_version |  02852a7b690aa411ce2a2089deea25a7292a33d6 | Version of MXE to use. This can be a commit hash but also a tag or branch name  |
| mxe_gcc |  gcc11 | Defines which gcc plugin to use in MXE |
| cmake_version |  3.16.* | Defines a CMake version string to be used when installing CMake via `pip`   |
| ccache_version |  v4.5.1 | Version of ccache to use. This can be a commit hash but also a tag or branch name   |

## Quickstart

### Building

To build branch `BAR105` from `https://github.com/beyond-all-reason/spring` (default values) run this command:
```bash
docker run -it springrts-build build
```

The output artifacts can be found in a volume linked to the container. Accessible e.g. via the Docker Desktop application.

### Develop
To start a development environment:
```bash
docker run -it springrts-build dev
```

You will get a bash shell with the spring source code checked out ready to make modifications and compile by typing `make` or `cmake --build .`:
```
----------------------------------------------
SpringRTS development environment has been set up successfully
Source code directory: /spring
Build directory: /spring/build
----------------------------------------------
<springdev> root@e0d3fbe4fffd:/spring/build$ _
```

## General
The image is based on Ubuntu 18.04 so created Linux binaries are runnable on that version and later. Windows binaries are build using a MXE cross-compile environment.

The image utilizes `ccache` to speedup consecutive compilation processes. It is recommended to reuse the cache data in different containers by configuring the ccache volume accordingly.

The image can be used with different commands that are described below:
1. `build`
2. `dev`
3. `shell` (just open a bash shell)

The build process consists of multiple steps. Each step is represented by a shell script and they are run consecutively:

1. `00_setup.sh`
2. `01_clone.sh`
3. `02_configure.sh`
4. `04_fill_portable_dir_linux-64.sh`and `04_fill_portable_dir_windows-64.sh`
5. `05_fill_debugsymbol_dir.sh`
6. `06_fill_build_options_file.sh`
7. `07_pack_build_artifacts.sh`
8. `08_copy_to_publish_dir.sh`

## Parameters
Both commands can be configured with these arguments:

| Parameter|      Default |  Description |
|:----------:|:-------------:|:------|
| -b |  BAR105 | The branch to build from the spring project |
| -u |  https://github.com/beyond-all-reason/spring     | URL to a Spring Git repository |
| -a | https://github.com/beyond-all-reason |  Prefix for the URLs used to clone auxiliary repos.  The following URL will e.g. be cloned: https://github.com/beyond-all-reason/BARbarIAn |
| -d | 0 | Dummy mode: to not actually clone or compile but just produce zero-size artifacts
| -e | 1 | Enable ccache
| -c | \<empty\> | Configures gcc's `-march` and `-mtune`flag
| -r | -O3 -g -DNDEBUG | CXX flags for RELWITHDEBINFO config
| -f | \<empty\> | CXXFLAGS and CFLAGS flags for gcc
| -s | 1 | Enable stripping of symbols from artifacts
| -z | | Enable generation ccache debug data in /ccache_dbg
| -p | windows-64 | Which target platform to setup build for (use "linux-64" or "windows-64")

### Building (command `run`)

The container can be started by passing `run` as the first parameter to run the compilation process for one target platform and produce an archive package that will contain the runnable engine with all library dependencies.

The following directories contain output data and can be used as a volume to access the files:
1. `/publish` - This is the directory the build process will copy all produced artifacts into
2. `/ccache` - The image is using `ccache` to speed up the build process. To make use of the cache you have to run different build runs using the same cache data directory. 
2. `/ccache_dbg` - Directory where ccache debug data will be placed

```bash
docker run -v D:\myspringbuild:/publish -it springrts-build
```

### Building from another GitHub Repository

```bash
Build branch `gl4` from repository at `https://github.com/beyond-all-reason/spring`:
docker run -it springrts-build build -u https://github.com/beyond-all-reason/spring -b gl4 -p linux-64
```

## Development

The image can also be used as a development environment. When starting the development mode then only the first three steps are executed:
1. `00_setup.sh`
2. `01_clone.sh`
3. `02_configure.sh`

So the source code will be cloned into the container and the build will be configured using CMake. You will find yourself inside the development shell ready to start the compilation process.
In you started the container with a raw `/bin/bash` shell (didn't use `build` or `dev` command) you can start the dev shell manually by running `dev.sh` with the usual arguments.

When working in the development shell you can always start the step scripts manually if needed. But be aware that running e.g. `01_clone.sh` will wipe the `/spring` directory completely and create a clean checkout. So make sure to not lose your changes!
As some of the steps will modify shell variables all scripts should be sourced (instead of started regularly), e.g. `. /scripts/01.clone.sh`.

### Reconfiguring
In case you want to switch the configuration inside the development shell you you can run `00_setup.sh <arga>`. 

For example:

```bash
. /scripts/00_setup.sh -p linux-64
```
