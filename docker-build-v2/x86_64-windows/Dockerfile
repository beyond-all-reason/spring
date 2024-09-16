FROM docker.io/ubuntu:24.04

RUN mkdir -p /build/src /build/out /build/target /build/artifacts /build/cache /build/bin
WORKDIR /build

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full \
 && rm -rf /var/lib/apt/lists/*

# pipx --global since 1.5 we don't have https://github.com/pypa/pipx/issues/754
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.25.*

RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git

COPY toolchain.cmake .
COPY ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
COPY cmake_config.sh /build/bin/cmake_config
