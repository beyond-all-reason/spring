FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    curl gcc g++ g++-mingw-w64-x86-64-posix git ninja-build make ccache pipx p7zip-full binutils zstd nasm \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libpci-dev \
    xorg-dev libgl1-mesa-dev libglu1-mesa-dev \
 && rm -rf /var/lib/apt/lists/*

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git
ENV MINGWLIBS=/build/mingwlibs64

# Keep the Windows feature set identical to Linux and avoid GPL/nonfree codecs.
ARG FFMPEG_REVISION=68af2cc3feb8c78aec2722c728fd87f03515fa7c
RUN git clone https://git.ffmpeg.org/ffmpeg.git /tmp/ffmpeg \
 && cd /tmp/ffmpeg \
 && git checkout --detach ${FFMPEG_REVISION} \
 && test "$(git rev-parse HEAD)" = "${FFMPEG_REVISION}" \
 && ./configure --prefix=/build/mingwlibs64 --target-os=mingw32 --arch=x86_64 \
    --cross-prefix=x86_64-w64-mingw32- --enable-cross-compile \
    --enable-static --disable-shared --enable-pic --disable-programs --disable-doc --disable-network \
    --disable-avdevice --disable-avfilter --disable-postproc --disable-everything \
    --enable-avcodec --enable-avformat --enable-avutil --enable-swscale --enable-swresample \
    --enable-decoder=h264,hevc,aac --enable-demuxer=mov --enable-parser=h264,hevc,aac \
 && make -j$(nproc) \
 && make install \
 && cp COPYING.LGPLv2.1 /build/mingwlibs64/FFMPEG-COPYING.LGPLv2.1 \
 && printf '%s\n' "revision=${FFMPEG_REVISION}" > /build/mingwlibs64/FFMPEG-MANIFEST.txt \
 && grep '^FFMPEG_CONFIGURATION=' ffbuild/config.mak >> /build/mingwlibs64/FFMPEG-MANIFEST.txt \
 && rm -rf /tmp/ffmpeg

# Copy toolchain early so SDL3 build can use it
COPY amd64-windows/toolchain.cmake .

# Build SDL3 for MinGW cross-compile (mingwlibs64 only ships SDL2)
ARG SDL3_VERSION="3.2.10"
RUN curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz" -o SDL3.tar.gz \
 && tar xzf SDL3.tar.gz \
 && mkdir -p SDL3-${SDL3_VERSION}/build \
 && cd SDL3-${SDL3_VERSION}/build \
 && cmake .. -DCMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=/build/mingwlibs64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_TEST=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_CMAKE_CONFIG=ON \
    -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_PULSEAUDIO=OFF -DSDL_ALSA=OFF \
    -DSDL_OPENGL=ON -DSDL_OPENGL_ES=ON -DSDL_DIRECTX=ON \
    -G "Unix Makefiles" \
 && cmake --build . -j$(nproc) \
 && cmake --install . \
 && cd / && rm -rf SDL3.tar.gz SDL3-*

# Set up default cmake toolchain
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
