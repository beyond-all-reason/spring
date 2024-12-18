#!/bin/bash

export CMAKE_TOOLCHAIN_FILE=

conan install \
  -u \
  -r conancenter \
  -pr:h conan_profile \
  -pr:b conan_build_profile \
  --build=missing \
  /build/src
