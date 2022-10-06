COMMAND=$1
shift

case "${COMMAND}" in

  build)
    . /scripts/build.sh "$@"
    ;;

  dev)
    . /scripts/dev.sh
    ;;

  shell)
    /bin/bash
    ;;

  *)
    echo "Invalid operation mode: $COMMAND" >&2
    exit 1
    ;;
esac