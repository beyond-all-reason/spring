cd "${INSTALL_DIR}"

EXECUTABLES=$(find -name '*.dll')" "$(find -name '*.exe')" "$(find -name '*.so')" "$(find -name 'spring*' -executable)y

set +e
for tostripfile in ${EXECUTABLES}; do
    if [ -f ${tostripfile} ]; then
        objdump -h ${tostripfile} | grep -q .gnu_debuglink
        dl=$?
        objdump -h ${tostripfile} | grep -q ".debug_"
        gd=$?
        if [[ $dl -ne 0 && $gd -eq 0 ]]; then
            debugfile=$(dirname $tostripfile)/$(echo $(basename $tostripfile) | cut -f 1 -d '.').dbg
            echo "stripping ${tostripfile}, producing ${debugfile}"
            objcopy --only-keep-debug ${tostripfile} ${debugfile} && \
            strip --strip-debug --strip-unneeded ${tostripfile} && \
            objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
        else
            echo "not stripping ${tostripfile}"
        fi
    fi
done
wait
