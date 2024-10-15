#!/bin/bash

set -e -u -o pipefail

cd /build/out/install

function split_debug {
    set -e -u -o pipefail

    file="$1"
    if ! objdump -h "$file" | grep -q ".debug_" || objdump -h "$file" | grep -q .gnu_debuglink; then
        echo "skipping $file"
        return
    fi
    echo "stripping $file"
    filename="$(basename "$file")"
    debugfile="$(dirname "$file")/${filename%.*}.dbg"
    objcopy --only-keep-debug ${file} ${debugfile}
    strip --strip-debug --strip-unneeded ${file}
    objcopy --add-gnu-debuglink=${debugfile} ${file}
}
export -f split_debug

# We split debug info from all binaries in parallel using xargs -P0
find \( -regex '.*\.\(dll\|so\|exe\)' -o -type f ! -name '*.dbg' -executable \) -print0 \
    | xargs -0 -P0 -n1 bash -c 'split_debug "$0"'
