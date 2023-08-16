cd "${BUILD_DIR}"

tag_name_short="{${BRANCH_NAME}\}"$(git describe --abbrev=7)
tag_name="{${BRANCH_NAME}}""$(git describe --abbrev=7)_${PLATFORM}"
bin_name=spring_bar_$tag_name-minimal-portable.7z
dbg_name=spring_bar_$tag_name-minimal-symbols.tgz
ccache_dbg_name=ccache_dbg.tgz

mkdir -p /output-data

cd "${INSTALL_DIR}"

# Compute md5 hashes of all files in archive. We additionally gzip it as gzip adds
# checksum to the list itself. To validate just `zcat files.md5.gz | md5sum -c -`
find . -type f ! -name '*.dbg' ! -name files.md5.gz -exec md5sum {} \; | gzip > files.md5.gz

rm -f ../$bin_name
7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ../$bin_name ./* -xr\!*.dbg
# export github output variables
echo ${bin_name} > /output-data/bin_name


cd "${BUILD_DIR}"
# touch empty.dbg - was is it good for??
DEBUGFILES=$(find ./ -name '*.dbg')
tar cvfz $dbg_name ${DEBUGFILES}
echo ${dbg_name} > /output-data/dbg_name

if [ -d /ccache_dbg ]; then
    echo "Packing ccache debug data..."

    echo ${ccache_dbg_name} > /output-data/ccache_dbg_name
    tar cvfz "${PUBLISH_DIR}/${ccache_dbg_name}" -C /ccache_dbg /ccache_dbg > /dev/null 2>&1
else
    touch /output-data/ccache_dbg
    echo "No ccache debug data, so skipping packing it..."
fi
