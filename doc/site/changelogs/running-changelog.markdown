---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2025.01.6**.

# Caveats
These are the entries which may require special attention when migrating:
* network protocol: scribbling-related (draw, point, erase) messages now send coordinates as `uint32` instead of `int16`.
This may break replay parsing.
* missiles now obey `myGravity` when expired.
* server no longer automatically forcestarts the game if there is nobody connected after 30s.
* fixed the `SPRING_LOG_SECTIONS` environment var, it no longer requires a comma in front.

# Features

### Lua language server support
LDoc has been replaced by [Lua Language Server](https://luals.github.io/) compatible annotations. This allows for language server support when editing Lua code (namely autocompletion and type checking).
Type definitions can be found in the [Lua library repo](https://github.com/beyond-all-reason/recoil-lua-library). This is intended to be included as a submodule in projects that use the engine.
[Lua API docs]({{ site.baseurl }}{% link lua-api/docs/index.md %}) are now generated from LLS definitions instead of LDoc. This has caused a regression in docs quality, with all docs on a single page and some docs missing information. Improvements to the docs are being considered.
For more information see the [Lua Language Server guide]({{ site.baseurl }}{% link guides/lua-language-server.markdown %}).

### Unit groups
* units no longer removed from groups at the start of their death animation.
* added `Spring.SetUnitNoGroup(unitID, bool noGroup) → nil`, whether a unit can be added to groups.
* added `Spring.GetUnitNoGroup(unitID) → bool noGroup`.

### Infra-adjacent
* fixed the `SPRING_LOG_SECTIONS` environment var, it no longer requires a comma in front.
* server no longer automatically forcestarts the game if there is nobody connected after 30s.

### Misc
* add `wupget:ActiveCommandChanged(cmdID?, cmdType?) → nil`.
* the `Spring.GiveOrder` family of functions now accept `nil` as params (same as `{}`) and options (same as `0`).
* add `Spring.SetUnitStorage(unitID, "m"|"e", value) → nil`.
* add `Spring.GetUnitStorage(unitID) → numbers metal, energy`.
* added `Game.buildGridResolution`, number which is currently 2. This means that buildings created via native build orders
are aligned to 2 squares.
* missiles now obey `myGravity` when expired.
* the `allowHoverUnitStrafing` modrule now defaults to `false`. Previously it defaulted to `false` for HAPFS and `true` for QTPFS.
* bumpmapped water (aka `/water 4`) now has a different default texture.

### Fixes
* fixed `Spring.ShareResources(teamID, "units", nil)` breaking due to the explicit nil.
* fix scribblings and labels breaking on maps larger than 64xN.
