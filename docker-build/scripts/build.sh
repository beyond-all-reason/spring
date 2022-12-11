set -e

. /scripts/00_setup.sh

if [ "${DUMMY}" == "1" ]; then
    echo "*** Enabled dummy mode ***"
    echo "This produces zero size artifacts instantly"

    bin_name="spring.tgz"
    dbg_name="spring.dbg"

    touch "${ARTIFACTS_DIR}/${bin_name}"
    touch "${ARTIFACTS_DIR}/${dbg_name}"
    echo "::set-output name=bin_name::${bin_name}"
    echo "::set-output name=dbg_name::${dbg_name}"

    echo "*** Finished"

    exit 0
fi

. /scripts/01_clone.sh
. /scripts/02_configure.sh
. /scripts/03_compile.sh
. /scripts/04_install.sh

if [ ${CI} ]; then
    if [ "${STRIP_SYMBOLS}" == "1" ]; then
        . /scripts/05_fill_debugsymbol_dir.sh
    fi
    . /scripts/06_fill_build_options_file.sh
    . /scripts/07_pack_build_artifacts.sh
    . /scripts/08_copy_to_artifacts_dir.sh
fi
