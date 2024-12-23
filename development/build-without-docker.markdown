---
layout: post
title: Building without Docker
parent: Development
permalink: development/build-without-docker
author: p2004a
---

# Building without Docker

The [https://github.com/beyond-all-reason/spring/tree/BAR105/docker-build/scripts](scripts) folder is the source of truth and best reference to figure out how to invoke and configure things.

## Compilation

It's arguable that compilation of spring for both Linux and Windows is just easier from Linux. So, we will only describe compilation on Linux.
- Windows: There is [WSL](https://docs.microsoft.com/en-us/windows/wsl/) and it works great.
- Linux: To not have to install all the dev dependencies, compilers etc directly in the base system, and for compatibility, you can use [distrobox](https://github.com/89luca89/distrobox) and develop there. It's fine to do it without that, but it's just very convenient.

This instruction was tested on Debian based system and Arch, but it should also work on all other Linux distributions once you figure out the list of packages to install.

### Install system dependencies

#### Debian based systems

Compilers, basic utilities, and helpful for developing:

```bash
sudo apt-get install -y cmake g++ ccache ninja-build clang lld git clangd socat \
  python3-pip g++-mingw-w64-x86-64-posix
sudo pip install compdb
```

Spring engine dependencies

```bash
sudo apt-get install -y doxygen libsdl2-dev libdevil-dev libcurl4-openssl-dev \
  p7zip-full libopenal-dev libogg-dev libvorbis-dev libunwind-dev libfreetype-dev \
  libglew-dev libminizip-dev libfontconfig-dev libjsoncpp-dev
```

#### Arch

Compilers, basic utilities, and helpful for developing (except for mingw cross compiler that is instelled for Debian based distros):

```bash
sudo pacman -S base-devel cmake ccache git openssh ninja lld socat clang python-pip
sudo pip install compdb
```

Recoil engine dependencies

```bash
sudo pacman -S curl sdl2 devil p7zip openal libogg libvorbis libunwind freetype2 glew \
  minizip fontconfig jsoncpp
```

And to make sure that openal has some functioning sound backend:

```bash
sudo pacman -S libpulse
```

### Fetch source

```bash
git clone https://github.com/beyond-all-reason/spring.git
cd spring
git submodule update --init --recursive
# for windows compilation
git clone https://github.com/beyond-all-reason/mingwlibs64.git mingwlibs64
```

### Compilation

This part is the most annoying: configuring build is done using cmake, and the command lines are quite large.

#### Toolchains

First, you should have a few toolchains configured. Toolchains select the compiler and target operating system. You can store them in the `toolchain` directory in the spring repo. Linux toolchains use `lld` linker as it's much faster.

`toolchain/clang_x86_64-pc-linux-gnu.cmake`:
```cmake
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
SET(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
SET(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
```

`toolchain/gcc_x86_64-pc-linux-gnu.cmake`:
```cmake
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "gcc")
SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
SET(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
SET(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
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

With cmake we are building outside of the source, so create directory like `builddir-win`, or `builddir-dbg` and inside them we can run cmake invocations. There are plenty of possible configuration so we will just list a bunch that can be used as a starting point.

In all of them:
- We use Ninja generator, as Ninja is the quickest to actually execute the build process, scan for changes etc.
- Using ccache to make next compilation quicker
- Install dir is simply `install`, so that after configuing build with cmake, you can just run `ninja && ninja install` and get all the files ready for usage in the `install` directory in builddir.

----
Basic release with debug info, shared libraries Linux build with GCC:

```bash
cmake \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/gcc_x86_64-pc-linux-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG -fdiagnostics-color=always" \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG -fdiagnostics-color=always" \
	-DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
	-DAI_TYPES=NATIVE \
	-DINSTALL_PORTABLE=ON \
	-DCMAKE_USE_RELATIVE_PATHS:BOOL=1
	-DBINDIR:PATH=./ \
	-DLIBDIR:PATH=./ \
	-DDATADIR:PATH=./ \
	-DCMAKE_INSTALL_PREFIX=install \
	-G Ninja \
	..
```

Fast unoptimized debug Linux shared libraries build with Clang and generation of [`compile_commands.json`](https://clang.llvm.org/docs/JSONCompilationDatabase.html) file.
```bash
cmake \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/clang_x86_64-pc-linux-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_FLAGS_DEBUG="-O1 -g -fcolor-diagnostics" \
	-DCMAKE_C_FLAGS_DEBUG="-O1 -g -fcolor-diagnostics" \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DDEBUG_MAX_WARNINGS=OFF \
	-DAI_TYPES=NATIVE \
	-DINSTALL_PORTABLE=ON \
	-DCMAKE_USE_RELATIVE_PATHS:BOOL=1 \
	-DBINDIR:PATH=./ \
	-DLIBDIR:PATH=./ \
	-DDATADIR:PATH=./ \
	-DCMAKE_INSTALL_PREFIX=install \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-G Ninja \
	..
```

Windows release with minimal line debug info static cross compilation with mingw64:
```bash
cmake \
	-DCMAKE_TOOLCHAIN_FILE="../toolchain/gcc_x86_64-pc-windows-gnu.cmake" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g1 -DNDEBUG -fdiagnostics-color=always" \
	-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g1 -DNDEBUG -fdiagnostics-color=always" \
	-DAI_TYPES=NATIVE \
	-DINSTALL_PORTABLE=ON \
	-DCMAKE_INSTALL_PREFIX:PATH=install \
	-GNinja ..
```

----

### Source code completion

In the clang debug above, we also enabled creation of `compile_commands.json` file that can be then used by IMHO the best C++ language server [clangd](https://clangd.llvm.org/). The main problem with cmake generated compilation databases is that it doesn't contain entries for header files. That can be fixed with [compdb](https://github.com/Sarcasm/compdb) utility installed in the beggining. Running from top repo directory:

```bash
compdb -p builddir-clang/ list > compile_commands.json
```

Clangd will then just pick it up.

If running in distrobox, and with clangd in distrobox, you might need to override clangd invocation in the LSP supporting editor to something like: `distrobox enter --no-tty {container_name} spring -- socat tcp-listen:{port},reuseaddr exec:clangd`

When developing on Windows, with clangd on Linux, and WSL container mapped to `L:` drive, the invocation might looks like this:  `wsl.exe socat tcp-listen:${port},reuseaddr exec:'clangd --path-mappings=L:/home=/home,L:/usr=/usr'`. Only drawback is that ofc `#ifdef _WIN32` blocks won't have completion in such setup.

Example `spring.sublime-project` for Sublime Text on Linux with [LSP](https://lsp.sublimetext.io/) server pckages:

```json
{
	"folders":
	[
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
					"spring",
					"--",
					"socat",
					"tcp-listen:{port},reuseaddr",
					"exec:clangd"
				],
				"tcp_port": 0,
				"scopes": ["source.c", "source.c++"],
				"syntaxes": [
					"Packages/C++/C.sublime-syntax",
					"Packages/C++/C++.sublime-syntax",
				],
				"languageId": "cpp",
			}
		}
	}
}
```
