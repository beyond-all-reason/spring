---
layout: post
title: Profiling with Tracy
parent: Development
permalink: development/profiling-with-tracy
author: beherith
---

# How to profile the engine using tracy

## Using a prebuilt binary:

1. Download Tracy, unzip it anywhere: https://github.com/wolfpld/tracy/releases/tag/v0.9.1

2. Get the exact engine version we are currently using from https://engine-builds.beyondallreason.dev/index.html

3. Find the exact engine version folder you are currently using (for example, for BAR that would be `BAR/data/engine`)

4. A. Rename the old `spring.exe` to `spring_vanilla.exe`, and extract `spring.exe` from the archive to this engine folder.

5. A. Start the game as you would otherwise via the launcher.

6. Launch `Tracy.exe`, and hit connect. If it throws an instrumentation error, connect again.

![image](https://github.com/beyond-all-reason/spring/assets/109391/830e5c6e-b37f-48ab-9adc-cc297cefff46)

8. Analyze profile.

## Building with Tracy support

The following options are available:

- `TRACY_ENABLE`: Enable tracy profiling
- `TRACY_ON_DEMAND`: On demand profiling is *slightly* more expensive, but it
allows to run the build with tracing like regular build and attach late in game,
where regular trace would just run out of memory because of size.
- `TRACY_PROFILE_MEMORY`: Profile memory allocations. It's pretty expensive and
some places that use raw malloc have to be used with care.
- `RECOIL_DETAILED_TRACY_ZONING`: Enable additional detailed tracy zones (only enable this for testing/debugging)

For example, building with docker and tracy support enabled:

```bash
./build.sh -o -t RELEASE -C -DTRACY_ENABLE=1 -p linux-64
```
