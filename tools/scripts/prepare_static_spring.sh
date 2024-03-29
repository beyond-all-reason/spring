#!/bin/sh
set -e

WORKDIR=${PWD}
TMPDIR=${WORKDIR}/tmp
INCLUDEDIR=${WORKDIR}/include
LIBDIR=${WORKDIR}/lib

cmake .. -DPREFER_STATIC_LIBS:BOOL=1 \
         -DIL_IL_HEADER:PATH=${INCLUDEDIR}/IL/il.h -DIL_INCLUDE_DIR:PATH=${INCLUDEDIR} -DIL_IL_LIBRARY:PATH=${LIBDIR}/libIL.a \
         -DPNG_PNG_INCLUDE_DIR:PATH=${INCLUDEDIR} -DPNG_LIBRARY_RELEASE:PATH=${LIBDIR}/libpng.a \
         -DJPEG_INCLUDE_DIR:PATH=${INCLUDEDIR} -DJPEG_LIBRARY:PATH=${LIBDIR}/libjpeg.a \
         -DTIFF_INCLUDE_DIR:PATH=${INCLUDEDIR} -DTIFF_LIBRARY_RELEASE:PATH=${LIBDIR}/libtiff.a \
         -DZLIB_INCLUDE_DIR:PATH=${INCLUDEDIR} -DZLIB_LIBRARY_RELEASE:PATH=${LIBDIR}/libz.a \
         -DGLEW_INCLUDE_DIR:PATH=${INCLUDEDIR} -DGLEW_LIBRARIES:PATH=${LIBDIR}/libGLEW.a \
         -DLIBUNWIND_INCLUDE_DIRS:PATH=${INCLUDEDIR} -DLIBUNWIND_LIBRARY:PATH=${LIBDIR}/libunwind.a \
         -DXCB_LIBRARIES:PATH=/usr/lib/x86_64-linux-gnu/libxcb.a \
         -G Ninja
