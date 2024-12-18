#!/bin/bash

conan graph info \
  -u \
  -r conancenter \
  -pr:h conan_profile \
  -pr:b conan_build_profile \
  --format=html \
  /build/src > /build/out/graph.html
