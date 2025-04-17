---
layout: post
title: Modrules and (un)related concepts
parent: Articles
permalink: articles/modrules-and-others
author: sprunk
---

### Mod options, mod info, mod rules, rules params
These concepts have similar names (which stems from how Recoil games used to be called "mods", even original/standalone ones)
and all deal with customizability in some way, so often get confused with each other. But they are not directly related.
This article will briefly describe all of them in a way that hopefully prevents any ambiguity.

### Mod info
* metadata about the game's archive. Things like name and version.
* lives in `./modinfo.lua`, hence the name.
* many mature games put `"$VERSION"` as the version. This is a magic value replaced by the Rapid distribution system with the actual version string. Check out Rapid's documentation for specifics.
* this file is both necessary and sufficient for a Recoil game, and its contents can consist of just the `name` entry as well.
* for engine devs: note that in engine internals, modinfo is **not** represented by the `CModInfo` class, that one is actually _mod rules_! Modinfo is a "class 1 meta file" only really referred to in the archive scanner.

### Mod rules
* a hardcoded set of knobs for tweaking engine behaviour.
* some technical (like the choice of pathfinding system or allocation limits) and some gameplay related (like the resource cost scaling for repairing).
* everything is a single constant global value. No per-unit rules, no changing at runtime, no rules outside the limited set exposed by the engine.
* read from `./gamedata/modrules.lua`, hence the name. Internal engine C++ code stores them in the `CModInfo` class.
* can depend on modoptions.

### Mod options
* per-match game setup.
* a set of arbitrary key-value pairs supplied by the host; interpretation is solely up to the game.
* usually live in `./modoptions.lua`, but this is just a convention for lobbies (this is where `unitsync` looks). The engine doesn't read this, nor enforce defaults etc.
* usually used for things like enabling custom modes and game setups (add lava, disable units etc.), but also is the technical means for supplying singleplayer mission data.
* maps can suggest and interpret modoptions too, these are often called mapoptions.

### Rules params
* dynamic per-unit/team or gamewide values.
* arbitrary key-value pairs set and interpreted by the game.
* usually used for storing attributes like "is burning" or "slowdown %" or such.
* can depend on both modoptions and modrules.
