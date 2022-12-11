FROM ubuntu:22.10

# suppress questions from apt
ENV DEBIAN_FRONTEND=noninteractive

ENV CONAN_USER_HOME="/conan"
ENV CONAN_V2_MODE=1

RUN apt-get update -y \
    && apt-get install --fix-missing -y \
        g++ \
        g++-mingw-w64-x86-64-posix \
        coreutils \
        git \
        ninja-build \
        p7zip-full \
        pigz \
        ccache \
        libatomic1 \
        python3-pip \
        pkg-config \
        clang-format \
        cmake \
        mold \
        libglu-dev \
        libopengl-dev \
        libegl-dev \
        libx11-dev \
        libx11-xcb-dev \
        libfontenc-dev \
        libice-dev \
        libsm-dev \
        libxau-dev \
        libxaw7-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxdamage-dev \
        libxdmcp-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbfile-dev \
        libxmu-dev \
        libxmuu-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev \
        libxres-dev \
        libxss-dev \
        libxt-dev \
        libxtst-dev \
        libxv-dev \
        libxvmc-dev \
        libxxf86vm-dev \
        xtrans-dev \
        libxcb-render0-dev \
        libxcb-render-util0-dev \
        libxcb-xkb-dev \
        libxcb-icccm4-dev \
        libxcb-image0-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-shape0-dev \
        libxcb-sync-dev \
        libxcb-xfixes0-dev \
        libxcb-xinerama0-dev \
        xkb-data \
        libxcb-dri3-dev \
        uuid-dev \
        libxcb-util-dev \
        libdevil-dev \
        libmng-dev \
        libtiff-dev \
    && pip install conan

ENV CCACHE_DIR=/ccache
ENV CCACHE_DEBUGDIR=/ccache_dbg

ENTRYPOINT ["/bin/bash", "/scripts/entrypoint.sh"]

# used for container version checking for local builds
COPY Dockerfile /Dockerfile
