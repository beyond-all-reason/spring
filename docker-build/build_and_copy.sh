#!/bin/sh

start_time=$(date +%s)

./build.sh -o -t RELEASE -C -DTRACY_ENABLE=1 -w

cp ../build-windows-64-RELEASE/spring.exe "/mnt/c/Users/Peti/Documents/My Games/Spring/engine/105.1.1-2724-g34671c0 bar/spring.exe"

end_time=$(date +%s)
elapsed_time=$((end_time - start_time))

echo "Copied spring.exe, elapsed time: $elapsed_time seconds"

powershell.exe '[console]::beep(261.6,700)'