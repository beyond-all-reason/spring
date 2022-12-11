Building with Docker is recommended, as normal compilation may fail or produce broken builds.

1. Once run `init_container.sh` to init the Docker container.
2. Use `build.sh` to compile. `cache` directory will be created which contains static libraries and ccache to speed up subsequent builds.
3. Build will be located at `(source directory)/build-(platform)-(build type)/install`.

List of available building flags can be printed with `build.sh -h`.

By default Windows builds are produced. To make a Linux build use `-p linux-64` flag.

`-o legacy` can be used to speed up compilation by disabling headless and dedicated builds.
