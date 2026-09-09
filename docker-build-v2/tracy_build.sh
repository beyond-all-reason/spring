#!/bin/sh

start_time=$(date +%s)

./build.sh windows -DTRACY_ENABLE=ON

cp ../build-amd64-windows/install/spring.exe "/mnt/c/Users/Peti/Documents/My Games/Spring/engine/recoil_2026.06.04/spring.exe"

end_time=$(date +%s)
elapsed_time=$((end_time - start_time))

echo "Copied spring.exe, elapsed time: $elapsed_time seconds"

powershell.exe '[console]::beep(261.6,700)'
