FROM docker.io/ubuntu:24.04

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full binutils \
 && rm -rf /var/lib/apt/lists/*

# pipx --global since 1.5 we don't have https://github.com/pypa/pipx/issues/754
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.25.*

WORKDIR /build
RUN mkdir src out target cache bin && chmod a+rwx out

RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git

COPY toolchain.cmake .
COPY ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
COPY cmake_config.sh bin/cmake_config
