#!/bin/bash

set -e -u -o pipefail

cd /build/src

branch=$(git rev-parse --abbrev-ref HEAD)
tag_name="{$branch}$(git describe --abbrev=7)_$ENGINE_PLATFORM"
bin_name=spring_bar_$tag_name-minimal-portable.7z
dbg_name=spring_bar_$tag_name-minimal-symbols.tar.zst

cd /build/out/install

# Compute md5 hashes of all files in archive. We additionally gzip it as gzip adds
# checksum to the list itself. To validate just `zcat files.md5.gz | md5sum -c -`
find . -type f ! -name '*.dbg' ! -name files.md5.gz -exec md5sum {} \; | gzip > files.md5.gz

rm -f /build/artifacts/$bin_name /build/artifacts/$dbg_name

# Trigger compression of main binaries and debug info concurrently
7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on /build/artifacts/$bin_name ./* -xr\!*.dbg &
tar cvf - $(find ./ -name '*.dbg') | zstd -T0 > /build/artifacts/$dbg_name &
wait
