---
layout: default
title: Profiling with Tracy
parent: Guides
permalink: guides/profiling-with-tracy
author: beherith
---

# How to profile the engine using tracy

## Preparation steps:

1. Download Tracy, unzip it anywhere: https://github.com/wolfpld/tracy/releases/tag/v0.9.1

2. Get the exact engine version we are currently using from https://engine-builds.beyondallreason.dev/index.html

3. Find the exact engine version folder you are currently using (for example, for BAR that would be `BAR/data/engine`)

4. A. Rename the old `spring.exe` to `spring_vanilla.exe`, and extract `spring.exe` from the archive to this engine folder.

5. A. Start the game as you would otherwise via the launcher.

6. Launch `Tracy.exe`, and hit connect. If it throws an instrumentation error, connect again.

![image](https://github.com/beyond-all-reason/spring/assets/109391/830e5c6e-b37f-48ab-9adc-cc297cefff46)

8. Analyze profile.
