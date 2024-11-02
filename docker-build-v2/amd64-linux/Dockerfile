# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04
ENV ENGINE_PLATFORM=amd64-linux

RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl gcc-13 g++-13 git ninja-build curl p7zip-full python3.8 python3.8-distutils \
    xz-utils build-essential \
    libgl-dev \
    xorg-dev \
    libxcb1-dev \
    libxcb-util-dev \
    libxcb-render-util0-dev \
    libxcb-xkb-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-xinerama0-dev \
    libxcb-cursor-dev \
    libxcb-composite0-dev \
    libxcb-ewmh-dev \
    libxcb-res0-dev \
    uuid-dev \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/*

# pip for python3.8
RUN curl -L -O https://bootstrap.pypa.io/get-pip.py && python3.8 get-pip.py && rm get-pip.py
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 10

# We need a newer ccache for compression support
ARG CCACHE_VERSION="4.10.2"
RUN curl -L -O https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && tar -xf ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && mv ccache-${CCACHE_VERSION}-linux-x86_64/ccache /usr/bin \
 && rm -rf ccache-${CCACHE_VERSION}-linux-x86_64*

# And need newer zstd :(, fortunatelly it's quick build
RUN git clone --depth 1 --branch v1.5.6 https://github.com/facebook/zstd.git \
 && cd zstd \
 && CC=g++-13 make -j$(nproc) \
 && mv programs/zstd /usr/local/bin \
 && cd .. \
 && rm -rf zstd

RUN pip3 install --upgrade pip \
 && pip3 install scikit-build \
 && pip3 install cmake==3.27.* \
 && pip3 install conan==2.9.1

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
#RUN git clone --depth=1 https://github.com/beyond-all-reason/spring-static-libs.git -b 18.04 spring-static-libs
#ENV PKG_CONFIG_LIBDIR=/build/spring-static-libs/lib/pkgconfig
#ENV PKG_CONFIG="pkg-config --define-prefix --static"
#ENV CMAKE_PREFIX_PATH=/build/spring-static-libs/
ENV PREFER_STATIC_LIBS=TRUE

# Set up default cmake toolchain
COPY toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
ENV CONAN_HOME=/build/.conan2
