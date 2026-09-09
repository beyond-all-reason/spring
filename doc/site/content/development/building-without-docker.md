+++
title = 'Building without Docker'
author = 'p2004a'
+++

This article describes how to compile the engine directly on the host without using Docker. [The Docker method](/development/building-with-docker/) is recommended if you are just starting out to get up and running quickly.

The [docker-build-v2](https://github.com/beyond-all-reason/RecoilEngine/tree/master/docker-build-v2) folder is the source of truth and best reference to figure out how to invoke and configure things.

## Compilation

It's arguable that compilation of Recoil for both Linux and Windows is just easier from Linux. In this article, we will only describe compilation on Linux (via WSL on Windows). Alternatively, see the ["Building on MSVC"](https://github.com/beyond-all-reason/RecoilEngine/wiki/Building-on-MSVC) wiki page for how to set up a Visual Studio environment which is excellent for C++ development on Windows.

- Windows: There is [WSL](https://docs.microsoft.com/en-us/windows/wsl/) and it works great.
- Linux: To not have to install all the dev dependencies, compilers, etc., directly in the base system, and for compatibility, you can use [distrobox](https://github.com/89luca89/distrobox) and develop there. It's fine to do it without that, but it's just very convenient.

This instruction was tested on a Debian-based system and Arch, but it should also work on all other Linux distributions once you figure out the list of packages to install.

### Install system dependencies

#### Debian-based systems

Compilers, basic utilities, and tools helpful for development:

```bash
sudo apt-get install -y cmake g++ ccache ninja-build clang mold git clangd socat \
  pipx g++-mingw-w64-x86-64-posix
pipx install compdb
```

Recoil engine dependencies

```bash
sudo apt-get install -y libsdl3-dev libdevil-dev libcurl4-openssl-dev \
  p7zip-full libopenal-dev libogg-dev libvorbis-dev libunwind-dev libfreetype-dev \
  libglew-dev libminizip-dev libfontconfig-dev
```

#### Arch

Compilers, basic utilities, and tools helpful for development (except for the mingw cross compiler that is installed for Debian-based distros):

```bash
sudo pacman -S base-devel cmake ccache git openssh ninja mold socat clang python-pip
sudo pip install compdb
```

Recoil engine dependencies

```bash
sudo pacman -S curl sdl3 devil p7zip openal libogg libvorbis libunwind freetype2 glew \
  minizip fontconfig jsoncpp
```

And to make sure that openal has some functioning sound backend:

```bash
sudo pacman -S libpulse
```

### Fetch source

```bash
git clone https://github.com/beyond-all-reason/RecoilEngine.git --recursive
cd RecoilEngine
# for windows compilation
git clone https://github.com/beyond-all-reason/mingwlibs64.git mingwlibs64
```

### Compilation

This part is the most annoying: configuring the build is done using cmake, and the command lines are quite large.

#### Toolchains

First, you should have a few toolchains configured. Toolchains select the compiler and target operating system. You can store them in the `toolchain` directory in the Recoil repo. Linux toolchains use the `mold` linker as it's much faster.

`toolchain/clang_x86_64-pc-linux-gnu.cmake`:

```cmake
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=mold")
SET(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=mold")
SET(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=mold")
```

`toolchain/gcc_x86_64-pc-linux-gnu.cmake`:

```cmake
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "gcc")
SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=mold")
SET(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=mold")
SET(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=mold")
```

`toolchain/gcc_x86_64-pc-windows-gnu.cmake`:

```cmake
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_C_COMPILER "x86_64-w64-mingw32-gcc-posix")
SET(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++-posix")
SET(CMAKE_RC_COMPILER "x86_64-w64-mingw32-windres")
SET(WINDRES_BIN "x86_64-w64-mingw32-windres")
SET(CMAKE_DLLTOOL "x86_64-w64-mingw32-dlltool")
SET(DLLTOOL "x86_64-w64-mingw32-dlltool")
```

#### CMake command lines

With cmake we are building outside of the source, so create a directory like `builddir-win`, or `builddir-dbg` and inside them we can run cmake invocations. There are plenty of possible configurations so we will just list a bunch that can be used as a starting point.

In all of them:

- We use the Ninja generator, as Ninja is the quickest to actually execute the build process, scan for changes etc.
- Using ccache to make the next compilation quicker
- Install dir is simply `install`, so that after configuring the build with cmake, you can just run `ninja && ninja install` and get all the files ready for usage in the `install` directory in the builddir.

---

Basic release with debug info, shared libraries Linux build with GCC:

```bash
cmake --fresh \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/gcc_x86_64-pc-linux-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
	-DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
	-DCMAKE_COLOR_DIAGNOSTICS=ON \
	-DAI_TYPES=NATIVE \
	-DCMAKE_INSTALL_PREFIX="$(dirname $(realpath "$0"))/install" \
	-G Ninja \
	..
```

Fast unoptimized debug Linux shared libraries build with Clang and generation of a [`compile_commands.json`](https://clang.llvm.org/docs/JSONCompilationDatabase.html) file.

```bash
cmake --fresh \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/clang_x86_64-pc-linux-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_FLAGS_DEBUG="-O1 -g" \
	-DCMAKE_C_FLAGS_DEBUG="-O1 -g" \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DCMAKE_COLOR_DIAGNOSTICS=ON \
	-DDEBUG_MAX_WARNINGS=OFF \
	-DAI_TYPES=NATIVE \
	-DCMAKE_INSTALL_PREFIX="$(dirname $(realpath "$0"))/install" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-G Ninja \
	..
```

Windows release with minimal line debug info static cross compilation with mingw64:

```bash
cmake --fresh \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/gcc_x86_64-pc-windows-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g1 -DNDEBUG" \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g1 -DNDEBUG" \
	-DCMAKE_COLOR_DIAGNOSTICS=ON \
	-DAI_TYPES=NATIVE \
	-DCMAKE_INSTALL_PREFIX="$(dirname $(realpath "$0"))/install" \
	-GNinja ..
```

---

### Source code completion

In the clang debug above, we also enabled the creation of a `compile_commands.json` file that can then be used by IMHO the best C++ language server [clangd](https://clangd.llvm.org/). The main problem with cmake-generated compilation databases is that they don't contain entries for header files. That can be fixed with the [compdb](https://github.com/Sarcasm/compdb) utility that we installed earlier. Running from the top repo directory:

```bash
compdb -p builddir-clang/ list > compile_commands.json
```

Clangd will then just pick it up.

#### Visual Studio Code

We recommend the [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) extension. If you are developing inside of distrobox, there are multiple options documented in https://distrobox.it/posts/integrate_vscode_distrobox/.

#### Other IDEs with Distrobox

If running in distrobox, with clangd in distrobox and an IDE directly on the host, you might need to override the clangd invocation in the LSP supporting editor to something like: `distrobox enter --no-tty {container_name} -- socat tcp-listen:{port},reuseaddr exec:clangd`

When developing on Windows, with clangd on Linux, and a WSL container mapped to the `L:` drive, the invocation might look like this: `wsl.exe socat tcp-listen:${port},reuseaddr exec:'clangd --path-mappings=L:/home=/home,L:/usr=/usr'`. The only drawback is that, of course, `#ifdef _WIN32` blocks won't have completion in such a setup.

Example `recoil.sublime-project` for Sublime Text on Linux with the [LSP](https://lsp.sublimetext.io/) server package:

```json
{
	"folders": [
		{
			"path": "."
		}
	],
	"settings": {
		"tab_size": 4,
		"LSP": {
			"clangd": {
				"enabled": true,
				"command": [
					"distrobox",
					"enter",
					"--no-tty",
					"recoil",
					"--",
					"socat",
					"tcp-listen:{port},reuseaddr",
					"exec:clangd"
				],
				"tcp_port": 0,
				"scopes": ["source.c", "source.c++"],
				"syntaxes": [
					"Packages/C++/C.sublime-syntax",
					"Packages/C++/C++.sublime-syntax"
				],
				"languageId": "cpp"
			}
		}
	}
}
```
