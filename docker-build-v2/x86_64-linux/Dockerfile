FROM docker.io/ubuntu:18.04

RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl gcc-13 g++-13 git ninja-build curl p7zip-full python3-pip python3-setuptools \
    libsdl2-dev libopenal-dev libfreetype6-dev libfontconfig1-dev \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/*

# We need a newer ccache for compression support
ARG CCACHE_VERSION="4.10.2"
RUN curl -L -O https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && tar -xf ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && mv ccache-${CCACHE_VERSION}-linux-x86_64/ccache /usr/bin \
 && rm -rf ccache-${CCACHE_VERSION}-linux-x86_64*

RUN pip3 install --upgrade pip \
 && pip3 install scikit-build \
 && pip3 install cmake==3.25.*

WORKDIR /build
RUN mkdir src out target cache bin && chmod a+rwx out

RUN git clone --depth=1 https://github.com/beyond-all-reason/spring-static-libs.git -b 18.04 spring-static-libs

COPY toolchain.cmake .
COPY ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
COPY cmake_config.sh bin/cmake_config
