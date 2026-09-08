# AGENTS.md - Coding Agent Guidelines for RecoilEngine

This document provides essential information for AI coding agents working on the RecoilEngine codebase.

## Project Overview

RecoilEngine is an open-source real-time strategy (RTS) game engine written in C++23. It is a fork and continuation of the Spring RTS engine (version 105.0).

## Build Commands

### Submodules

The repo uses git submodules for vendored libraries (`rts/lib/*`, `tools/pr-downloader`, AI skirmish bots, etc.). If you cloned without `--recurse-submodules`, initialize them before building:
```bash
git submodule update --init --recursive
```

### Building the Engine

**Using Docker (Recommended):**
```bash
# Full build (default: RELWITHDEBINFO, -O3 -g -DNDEBUG, Ninja)
# Output lands in build-<arch>-<os>/ (e.g. build-amd64-linux/) and the
# ready-to-use install in build-amd64-linux/install/
docker-build-v2/build.sh linux

# Parallelism
docker-build-v2/build.sh -j 8 linux

# Windows cross-build
docker-build-v2/build.sh windows

# Change optimization level — trailing -D… is forwarded to configure.sh and
# overrides the baked-in RELWITHDEBINFO default.
docker-build-v2/build.sh linux -DCMAKE_BUILD_TYPE=DEBUG
docker-build-v2/build.sh linux -DCMAKE_BUILD_TYPE=RELEASE
docker-build-v2/build.sh linux -DCMAKE_BUILD_TYPE=PROFILE

# Combine cmake options (configure phase)
docker-build-v2/build.sh linux -DBUILD_spring-headless=OFF -DTRACY_ENABLE=ON

# List all available cmake options and their current values
docker-build-v2/build.sh --configure linux -LH

# Build a specific target — use --compile so args flow to `cmake --build`,
# not to configure. Without --compile, `-t …` would be rejected by configure.
docker-build-v2/build.sh --compile linux -t engine-headless
docker-build-v2/build.sh --compile linux -t engine-legacy
docker-build-v2/build.sh --compile linux -t tests --verbose

# Split the phases
docker-build-v2/build.sh --configure linux      # configure only
docker-build-v2/build.sh --compile linux        # compile only (reuses existing config)
```

**Without Docker:**
```bash
# Create build directory
mkdir -p build && cd build

# Configure — project requires C++23 (clang ≥ 17 or gcc ≥ 13 on PATH).
# CMAKE_BUILD_TYPE defaults to RELWITHDEBINFO when omitted.
cmake ..

# Optional: Default generator is Unix Makefiles; add `-G Ninja` for faster builds if ninja is installed.
cmake -G Ninja ..

# Optional: pin to gcc-13 + gold linker via the in-repo toolchain file
# (tracked under docker-build-v2/; same compiler the docker build uses).
cmake \
    -DCMAKE_TOOLCHAIN_FILE=../docker-build-v2/images/all-linux/toolchain.cmake ..

# Optional: speed up incremental builds with ccache
cmake \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache ..

# Change optimization level by re-running cmake (no wipe required):
cmake -DCMAKE_BUILD_TYPE=DEBUG ..          # no optimization, full symbols
cmake -DCMAKE_BUILD_TYPE=RELEASE ..        # optimized, no debug info
cmake -DCMAKE_BUILD_TYPE=RELWITHDEBINFO .. # optimized + debug info (default)
cmake -DCMAKE_BUILD_TYPE=PROFILE ..        # optimized + profiling hooks

# Build (generator-agnostic — works under Ninja or Make)
cmake --build .

# Build a specific target
cmake --build . --target engine-headless
cmake --build . --target engine-legacy
cmake --build . --target engine-dedicated
cmake --build . --target tests
```

> The docker flow writes to **`build-<arch>-<os>/`** (e.g. `build-amd64-linux/`
> or `build-amd64-windows/`), which is a different directory than the `build/`
> used by this flow. When running tests, point commands at whichever build
> directory you populated.

### Build Types
- `DEBUG` - Debug build with full symbols and no optimization
- `RELEASE` - Optimized release build
- `RELWITHDEBINFO` - Release with debug info (default)
- `PROFILE` - Profiling build

### Build Targets
- `engine-legacy` — main interactive engine build
- `engine-headless` — headless engine (no graphics)
- `engine-dedicated` — dedicated server
- `unitsync` — unitsync shared library
- `pr-downloader` — content downloader tool
- `tests` — phony; builds every `test_*` executable under `build/test/`
- `check` — phony; depends on `engine-headless` + all `test_*` executables, then runs ctest with `--output-on-failure -V`
- `install` — install into `CMAKE_INSTALL_PREFIX`

## Testing

### Writing Tests
See `test/AGENTS.md` for details on writing tests, available compile flags, patterns, and test helpers.

### Running Tests

**Build and run all tests:**
```bash
# From build/ — ctest / check recipes below assume a non-docker build.
# For a docker build, run `docker-build-v2/build.sh --compile linux -t check`
# (runs ctest inside the container) or invoke the binaries in
# build-amd64-linux/test/ directly.
cmake --build . --target tests    # build all test executables (no run)
ctest                             # run all tests (does not rebuild)
# OR
cmake --build . --target check    # rebuild engine-headless + all tests, then run ctest -V
```

`check` is the safe default when iterating; bare `ctest` is faster when nothing relevant has changed since the last build.

**Run a single test (from repo root):**
```bash
# Tests are built as executable binaries under <build-folder>/test/
# Pattern: <build-folder>/test/test_<TestName>
# where <build-folder> depends on if you built in docker or not (see above).

./build/test/test_Float3
./build/test/test_Matrix44f
./build/test/test_SyncedPrimitive
./build/test/test_UDPListener

# Catch2: show passing assertions too
./build/test/test_Float3 -s

# Run a specific test case by name (positional arg matches TEST_CASE name, supports wildcards)
./build/test/test_Float3 "Float3"
./build/test/test_Float3 "Float34_*"
```

**Run via CTest (from inside build/):**
```bash
# Filter by regex, show output only on failure
ctest -R Float3 --output-on-failure

# Same, but verbose (full stdout regardless of result)
ctest -R Float3 -V

# List all registered tests without running
ctest -N
```

### Test Locations
- Unit tests: `test/engine/`
- Test sources use `#include <catch_amalgamated.hpp>`
- Each test is compiled as a separate executable named `test_<TestName>`

## Code Style Guidelines

### Indentation and Formatting
- **Use tabs for indentation** (see `.editorconfig`)
- Tab width: configure your editor to display tabs as you prefer
- Line endings: platform-appropriate (LF on Linux, CRLF on Windows)

### File Headers
All source files must begin with the GPL license header:
```cpp
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
```

### Include Guards
Use `#pragma once` for new headers, migrate ifdef guards to `#pragma once` whenever you edit a header file.

### Naming Conventions

**Classes and Structs:**
- PascalCase: `Button`, `GameVersion`, `RectangleOverlapHandler`
- Descriptive names: avoid abbreviations unless widely known

**Functions and Methods:**
- PascalCase for public methods: `DrawSelf()`, `HandleEvent()`
- Getters/setters: `GetMajor()`, `Label()`

**Variables:**
- Member variables: camelCase or snake_case: `label`, `clicked`, `hovered`
- Local variables: camelCase preferred
- Boolean variables: often use `is`, `has` prefixes or simple names: `Hovered`, `Clicked`

**Constants:**
- Compile-time constants: UPPER_CASE or PascalCase
- const variables: camelCase

**Namespaces:**
- lowercase: `agui`, `spring`, `springversion`

**Macros:**
- UPPER_CASE with underscores: `LOG_LEVEL_DEBUG`, `CR_DECLARE_STRUCT`

### Braces and Formatting

**Functions:**
```cpp
// Opening brace on new line
void ClassName::MethodName()
{
	// body
}
```

**Control structures:**
```cpp
// Opening brace on same line
if (condition) {
	// body
} else {
	// body
}

for (int i = 0; i < n; ++i) {
	// body
}

switch (value) {
	case CASE_ONE:
		// handle
		break;
	default:
		// handle
}
```

**Classes:**
```cpp
class ClassName : public BaseClass
{
public:
	ClassName();
	virtual ~ClassName();

	void PublicMethod();

private:
	void privateMethod();

	int memberVariable;
};
```

### Imports and Includes

**Order:**
1. Corresponding header (for .cpp files)
2. System/STL headers (angle brackets)
3. Library headers (angle brackets)
4. Project headers (quotes)

**Example:**
```cpp
#include "ClassName.h"              // Corresponding header

#include <cassert>                  // System headers
#include <string>
#include <vector>

#include <SDL2/SDL.h>              // Library headers

#include "System/Log/ILog.h"       // Project headers
#include "Rendering/GL/myGL.h"
```

**Include paths:**
- Use relative paths from `rts/` directory
- Example: `#include "System/float3.h"`

### Error Handling

**Assertions:**
```cpp
#include <cassert>

assert(pointer != nullptr);
assert(index >= 0 && index < size);
```

**Logging:**
Use the built-in logging system with severity levels:
```cpp
#include "System/Log/ILog.h"

LOG_L(L_DEBUG, "Debug message: %s", value);
LOG_L(L_INFO, "Info message");
LOG_L(L_WARNING, "Warning: %d items", count);
LOG_L(L_ERROR, "Error occurred in %s", functionName);
LOG_L(L_FATAL, "Fatal error - cannot continue");
```

Log levels (defined in `System/Log/Level.h`):
- `L_DEBUG` - Fine-grained debug info
- `L_INFO` - General information
- `L_NOTICE` - Always outputted (default level)
- `L_DEPRECATED` - Deprecation warnings
- `L_WARNING` - Potentially harmful situations
- `L_ERROR` - Errors that allow continued execution
- `L_FATAL` - Severe errors causing abort

**Exceptions:**
```cpp
#include <stdexcept>

throw std::runtime_error("Description of error");
```

Custom exceptions may be defined in specific modules.

### Comments

**File-level:**
- GPL license header (required)
- Brief description of file purpose

**Documentation:**
```cpp
/**
 * @brief Brief description
 *
 * Detailed description of the function/class.
 * 
 * @param paramName Description of parameter
 * @return Description of return value
 */
```

**Inline comments:**
```cpp
// Single-line comments for brief notes
// Use // for C++ code, /* */ for C code
```

### C++ Features

**C++ Standard:** C++23

**Modern C++ usage:**
- Use `constexpr` for compile-time constants
- Prefer `auto` for type deduction when type is obvious
- Use range-based for loops
- Use smart pointers where appropriate
- Use `nullptr` instead of `NULL` or `0`

**Synced code:**
The engine has special macros for synchronized multiplayer code:
```cpp
ENTER_SYNCED_CODE();
// synced operations
LEAVE_SYNCED_CODE();
```

### Platform-Specific Code

Use preprocessor directives for platform-specific code:
```cpp
#ifdef WIN32
	// Windows-specific code
#endif

#ifdef HEADLESS
	// Headless build code
#endif

#if defined(__GNUC__)
	// GCC-specific code
#endif
```

## CMake Guidelines

### CMakeLists.txt Style
- Follow `.cmakelintrc` configuration
- Use tabs for indentation in CMake files
- Keep lines reasonably short

## Project Structure

- `rts/` - Main engine source code
  - `rts/System/` - Core system code
  - `rts/Game/` - Game logic
  - `rts/Sim/` - Simulation code
  - `rts/Rendering/` - Graphics rendering
  - `rts/Lua/` - Lua scripting interface
  - `rts/aGui/` - GUI components
  - `rts/lib/` - External libraries
- `test/` - Unit tests
- `tools/` - Utility tools
- `AI/` - AI interface code
- `cont/` - Content files
- `doc/` - Documentation

## AI Usage Policy

**IMPORTANT:** This project has a strict AI usage policy. See `AI_POLICY.md` for full details.

**Key requirements:**
1. **Disclose all AI usage** - State the tool used and extent of assistance
2. **PRs must reference accepted issues** - No drive-by AI-generated PRs
3. **Human verification required** - All AI-generated code must be tested by a human
4. **No AI-generated media** - Only text and code allowed
5. **Human-in-the-loop required** - Review and edit all AI-generated content

**Maintainers are exempt** from these rules; they use AI at their discretion.

## Important Notes

### Synced Code
The engine uses deterministic simulation for multiplayer. Code that affects game state must maintain sync across clients. Look for `SYNCCHECK` and `streflop` references.

### Threading
The engine uses custom thread pools. See `THREADPOOL` define and related code.

### Testing Changes
- For Lua changes: write a test widget
- For other changes: manual testing procedure required
- Automated tests are encouraged but not always required due to complexity

### Before Submitting PRs
1. Test your changes thoroughly
2. Reference an accepted issue
3. Document what testing was performed
4. Follow the workflow in `contributing.md`
5. Disclose any AI assistance used

### Additional docs
Please see @coding-agents/ for additional documentation:
- coding-agents/ENGINE_PERFORMANCE.md — notes on scale targets and engine performance internals. Useful for performance related changes.
- coding-agents/BACKWARDS_COMPATIBILITY.md - notes on when we should strive to be backwards compatible. Reference it for any major reworks or api changes.
## Additional Resources

- Official website: https://recoilengine.org
- Documentation: https://recoilengine.org/docs/
- Build guide: https://recoilengine.org/development/building-without-docker/
- Discord: https://discord.gg/GUpRg6Wz3e
- GitHub issues: https://github.com/beyond-all-reason/RecoilEngine/issues
