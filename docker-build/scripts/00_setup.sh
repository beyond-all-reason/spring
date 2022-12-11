PLATFORM="windows-64"
DUMMY=
ENABLE_CCACHE=1
DEBUG_CCACHE=
STRIP_SYMBOLS=1

MYARCHTUNE=""
MYCFLAGS=""
MYBUILDTYPE="RELWITHDEBINFO"
RELWITHDEBINFOFLAGS="-O3 -g -DNDEBUG"
DEBUGFLAGS="-Og -g -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS"
RELEASEFLAGS="-O3 -DNDEBUG"
PROFILEFLAGS="-O3 -pg -DNDEBUG -DPROFILE"

SPRING_DIR="/spring"
BUILD_DIR="/spring/build"
ARTIFACTS_DIR="/artifacts"
INSTALL_DIR="${BUILD_DIR}/install"


function print_usage() {
    echo "usage:"
    echo "  -p      platform to build"
    echo "  -d      dummy mode"
    echo "  -e      enable ccache"
    echo "  -c      archtune flags"
    echo "  -t      build type: RELWITHDEBINFO (default), DEBUG, RELEASE, PROFILE"
    echo "  -r      build type flags override"
    echo "  -f      c/c++ flags"
    echo "  -s      strip debug symbols"
    echo "  -z      enable ccache debug"
    echo "  -w      suppress outdated container warning"
    echo "  -o      only build legacy, headless or dedicated"
    echo "  -h      print this help"
    exit 1
}

while getopts :p:dc:ht:r:f:s:z:e:wo: flag
do
    case "${flag}" in
        p) PLATFORM=${OPTARG};;
        d) DUMMY=1;;
        h) print_usage;;
        e) ENABLE_CCACHE=${OPTARG};;
        c) MYARCHTUNE=${OPTARG};;
        t) MYBUILDTYPE=${OPTARG};;
        r) MYBUILDTYPEFLAGS=${OPTARG};;
        f) MYCFLAGS=${OPTARG};;
        s) STRIP_SYMBOLS=${OPTARG};;
        z) DEBUG_CCACHE=${OPTARG};;
        w) SUPPRESS_OUTDATED=true;;
        o) BUILD_CONFIG=${OPTARG};;
        \:) printf "argument missing from -%s option\n" $OPTARG >&2
            exit 2
            ;;
        \?) printf "unknown option: -%s\n" $OPTARG >&2
            exit 2
            ;;
    esac
done

if [ ! ${CI} ]; then
    trap 'exit 1' SIGINT
    if ! cmp -s -- "/Dockerfile" "${SPRING_DIR}/docker-build/Dockerfile"; then
        echo "WARNING: Docker container is outdated. Please run init_container.sh."
        if [ ${SUPPRESS_OUTDATED} ]; then
            echo "Outdated warning suppressed."
        else
            echo "Add -w flag to suppress this warning."
            read -r -p "Continue anyway? [y/N] " response
            case "$response" in
                [yY][eE][sS]|[yY]) ;;
                *) exit 1;;
            esac
        fi
    fi

    BUILD_DIR="${BUILD_DIR}-${PLATFORM}-${MYBUILDTYPE}"
    INSTALL_DIR="${BUILD_DIR}/install"
fi

if [ -z ${MYBUILDTYPEFLAGS+x} ]; then
    var="${MYBUILDTYPE}FLAGS"
    MYBUILDTYPEFLAGS="${!var}"
fi

if [ "${PLATFORM}" != "windows-64" ] && [ "${PLATFORM}" != "linux-64" ]; then
    echo "Unsupported platform: '${PLATFORM}'" >&2
    exit 1
fi

# configuring ccache
if [ "${ENABLE_CCACHE}" != "1" ]; then
    echo "Disabling ccache"
    export CCACHE_DISABLE=1
else
    echo "Using ccache in directory: ${CCACHE_DIR}"

    if [ ${CI} ]; then
        export CCACHE_MAXSIZE=1.4G
    fi

    if [ "${DEBUG_CCACHE}" == "1" ]; then
        echo "ccache debugging enabled"
        export CCACHE_DEBUG=1
        mkdir -p /ccache_dbg
    fi
fi


echo "---------------------------------"
echo "Starting buildchain with following configuration:"
echo "Platform: ${PLATFORM}"
echo "Local artifact dir: ${ARTIFACTS_DIR}"
echo "Local SpringRTS source dir: ${SPRING_DIR}"
echo "Build type: ${MYBUILDTYPE}"
echo "${MYBUILDTYPE} compilation flags: ${MYBUILDTYPEFLAGS}"
echo "Archtune flags: ${MYARCHTUNE}"
echo "Extra compilation flags: ${MYCFLAGS}"
echo "Dummy mode: ${DUMMY}"
echo "Enable ccache: ${ENABLE_CCACHE}"
echo "Debug ccache: ${DEBUG_CCACHE}"
echo "Strip debug symbols: ${STRIP_SYMBOLS}"
echo "---------------------------------"
