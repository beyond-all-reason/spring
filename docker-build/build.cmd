@ECHO OFF

for %%d in (%~dp0..) do set ENGINE_GIT_DIR=%%~fd

set LINUX_LIBS_DIR=%ENGINE_GIT_DIR%/cache/spring-static-libs
set WINDOWS_LIBS_DIR=%ENGINE_GIT_DIR%/cache/mingwlibs64
set CCACHE_DIR=%ENGINE_GIT_DIR%/cache/ccache
set DOCKER_SCRIPTS_DIR=%ENGINE_GIT_DIR%/docker-build/scripts

docker run -it --rm ^
            -v %ENGINE_GIT_DIR%:/spring ^
            -v %LINUX_LIBS_DIR%:/spring-static-libs ^
            -v %WINDOWS_LIBS_DIR%:/mingwlibs64 ^
            -v %CCACHE_DIR%:/ccache ^
            -v %DOCKER_SCRIPTS_DIR%:/scripts ^
            bar-spring build -l %*

