---
layout: post
title: Headless and dedicated
parent: Guides
permalink: guides/headless-and-dedi
author: sprunk
---

## Overview

Recoil builds some **extra binaries** that **behave differently** to the primary 'spring.exe' one.
This guide briefly describes what they do and when to use them.

### Headless

The headless build **only does the simulation** and does **not do any rendering**.
**Widgets still work** and are the main way to provide input, all rendering-related call-ins and call-outs just do nothing.

It is **great for automated tasks** such as:
 * gathering **data from replays, general analysis**. Spectate a replay, use a widget to gather data.
 * simulating **AI vs AI** games (either for AI development, or just to produce something analyzable in conjunction with the above).
 * simulate games played on a dedicated binary, to **verify results**. See below.
 * be a game server for a multiplayer game, though only **on limited scale** (perhaps for important games like tournaments). See below.

### Dedicated (aka dedi)

Simulating games can get fairly heavy, such that a game server would **struggle to simulate more than a handful**.
However, players simulate the game anyway, so **a server technically doesn't need to** - this is the idea behind the dedicated binary.
It only collects and **relays network traffic without running the simulation**.

Typically, dedicated servers are used for **automated hosting in large games** since they are quite lightweight.
In theory this leaves games without a single source of truth, but **replays are still produced** which can be ran for verification **later, as needed**.
