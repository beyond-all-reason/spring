./build.sh -p windows-64 -t RELWITHDEBINFO -C -DTRACY_ENABLE=1 -k -o
cp -f ../build-windows-64-RELWITHDEBINFO/spring.exe /media/vm/spring_RELWITHDEBINFO.exe
cp -f ../build-windows-64-RELWITHDEBINFO/publish/spring_bar_{BAR105}105.1.1-2403-g38c273d_windows-64-minimal-symbols.tgz /media/vm/spring.symbols.tgz
