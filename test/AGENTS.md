# AGENTS.md

This file provides guidance to coding agents when working with code in this repository's test folder.

## Build & Run Tests

```bash
# From build/: build all test executables
cmake --build . --target tests

# From build/: run tests. ctest/check recipes here assume a non-docker
# build. For a docker build, run `docker-build-v2/build.sh --compile linux -t check`
# (runs ctest inside the container) or invoke binaries in
# build-amd64-linux/test/ directly (see below).
ctest                                   # run already-built tests; does not rebuild
cmake --build . --target check          # rebuild engine-headless + all tests first, then ctest -V

# From build/: run ctest with filters
ctest --output-on-failure               # concise; only shows failing output
ctest -R Float3 --output-on-failure     # filter by name regex
ctest -R Float3 -V                      # same, verbose
ctest -N                                # list all registered tests without running

# From repo root: run a single test binary directly (fastest iteration).
# Use build-amd64-linux/ instead of build/ if you built via docker.
./build/test/test_Float3
./build/test/test_Float3 -s             # Catch2: show passing assertions too
./build/test/test_Float3 "Float3"       # filter by TEST_CASE name (supports wildcards)
```

`cmake --build . --target check` is the full-fat target: it depends on `engine-headless` and
every `test_*` executable, so it relinks anything stale before running ctest with
`--output-on-failure -V`. Use bare `ctest` when you want to skip the rebuild.

## Framework

**Catch2** (amalgamated single-header version) in `lib/catch2/`. Custom main in `lib/catch2/catch_main.cpp` with leak detection enabled via `CATCH_AMALGAMATED_CUSTOM_MAIN`.

## Test Organization

```
engine/System/          # Core system tests (math, threading, I/O, serialization)
engine/Sim/Misc/        # Simulation tests (QuadField, Ellipsoid)
lib/luasocket/          # Lua socket restriction tests
other/                  # Mutex benchmarks, memory pool tests
unitsync/               # UnitSync API tests
validation/             # Integration tests (shell scripts that run full game simulation)
tools/CompileFailTest/  # Negative test framework (tests that must NOT compile)
headercheck/            # Header isolation tests (cmake -DHEADERCHECK=ON)
```

## Adding a New Test

1. Create test source in the appropriate subdirectory under `engine/`, `other/`, etc.
2. In `test/CMakeLists.txt`, add a block using the `add_spring_test` macro:
```cmake
set(test_name MyTest)
set(test_src
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testMyTest.cpp"
    ${test_Common_sources}
)
set(test_libs "")
set(test_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")
add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
```
3. The macro creates executable `test_<name>` and registers it with ctest as `test<name>`.

## Common Compile Flags

| Flag | Purpose |
|------|---------|
| `-DUNIT_TEST` | Always set for all tests (global) |
| `-DSYNCCHECK` | Always set for all tests (global) |
| `-DNOT_USING_CREG` | Stubs out `CR_*` macros. Default unless the test exercises save/load serialization. |
| `-DNOT_USING_STREFLOP` | Falls back to `<cmath>`. Default unless the test verifies synced floating-point determinism. |
| `-DBUILDING_AI` | Makes engine headers skip engine-only paths. Pair with `NOT_USING_CREG` and `NOT_USING_STREFLOP`. |
| `-DTHREADPOOL` | Selects the real thread pool over the stub. Set only when the test needs real parallelism. |
| `-DUNITSYNC` | Marks the file as part of unitsync. Only needed for tests that link the unitsync library. |

## Patterns

### Basic test file
```cpp
#include <catch_amalgamated.hpp>
#include "System/Log/ILog.h"

TEST_CASE("MyFeature") {
    CHECK(1 + 1 == 2);
    SECTION("sub-case") {
        CHECK(true);
    }
}
```

### Tests that need timing
```cpp
#include "System/Misc/SpringTime.h"
TEST_CASE("TimingTest") {
    InitSpringTime ist;  // RAII - must be instantiated before using spring_time
    // ...
}
```

### Thread-safe assertions
Catch2 is NOT thread-safe. Multi-threaded tests must guard assertions:
```cpp
static spring::mutex m;
#define SAFE_CHECK(expr) { std::lock_guard lk(m); CHECK(expr); }
```

### Compile-fail tests
Tests that verify code correctly fails to compile. Source uses `#ifdef FAIL` guards:
```cpp
#ifdef FAIL
#ifdef TEST1
    int x = someStronglyTypedEnum;  // must not compile
#endif
#endif
```
Registered in CMakeLists.txt via:
```cmake
spring_test_compile_fail(testName_fail1 ${test_src} "-DTEST1")
```

## Test Helpers (mock/stub files)

- `engine/System/NullGlobalConfig.cpp` — provides default `globalConfig` without full engine init
- `engine/System/Nullerrorhandler.cpp` — stubs `ErrorMessageBox()` to prevent GUI popups
