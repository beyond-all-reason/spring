---
layout: post
title: Choose Recoil
parent: Guides
permalink: guides/choose-recoil
author: sprunk
---

## Choose Recoil!

Picking the engine for your game is a very important decision to make.
Here's how Recoil compares to other choices.

### Why shouldn't I pick a general engine (Unity, Godot, etc)?
 * Recoil has **built-in multiplayer RTS facilities**.
This means systems such as pathfinding, targeting, the concept of unit commands, projectile physics, resource system, networking and synchronisation, replays, and many others.
With general engines you'd need to bother integrating these yourself.
 * Recoil **embraces open-source**.
The engine is both free and welcoming to modification.
The GPL license means that Recoil games can always share code with each other, though you can still make a successful commercial game: content/assets such as models and maps, or access to your servers, can remain proprietary.
 * Recoil **is a platform**.
There's a community of veteran RTS enthusiasts around the project that will gladly help you start out.
Host your budding game on existing servers, reuse existing content, don't worry about issues such as distribution and marketing until your game is mature enough to take them on.

### My game is heavily inspired by some existing game XYZ, why shouldn't I just mod it?
 * Recoil **is an open-source platform**, see above.
 * Recoil is **built with modding in mind**.
Most commercial games require you to put in some effort to replace content, and often it is not possible to replace some deeper mechanics without major hackery.
Meanwhile Recoil exposes interfaces for you to do everything you want.

### I'm making a little Starcraft style mod. Why shouldn't I just use SC2 Arcade? It's also a platform for mods.
 * well, perhaps you should! Recoil is oriented towards Total Annihilation style games, and SC2 is still a great platform for modding. **However:**
 * Recoil is **constantly being developed**.
Improvements and features are being added over time.
If your game finds a feature missing you can talk to engine developers who will be glad to try and accommodate your needs.
 * Recoil doesn't interfere with your **ownership of content**.
Code has to be open-sourced, but full ownership of content is still yours, not Blizzard's.

### Recoil is just a fork of Spring, why shouldn't I use the original?
 * **active and easy to reach maintainers** who are happy to accommodate your game's needs.
 * the API (incl. documentation), the feature set, and engine performance have **all improved and keep improving**, check out the changelog.
 * most **major Spring games made the move** and are happy with it.
 * if you still want to pick Spring, that's fine because **Recoil is very back-compatible so you can easily switch later**.
