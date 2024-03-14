---
layout: post
title: Release 105-902
parent: Changelogs
permalink: changelogs/changelog-105-902
author: sprunk
---

The changelog since release 105-861 **until minor release 105-902**, which happened in April 2022.

## Caveats
* Default trees are now provided by basecontent. Chopped engine trees (removed engine tree generators and renderers). Trees have models/textures donated by 0 A.D. so these have outlived their utility.

## Features and fixes

### GLDB queries

{: .warning }
> This feature only existed until release 105-2314 and is now removed.

Add `Spring.MakeGLDBQuery(bool forced) → bool ok` to create a query to the OpenGL drivers database. There's generally only one query at a time allowed, forcing rewrites it.

Add `Spring.GetGLDBQuery(bool blocking)` to receive the results of the query made with the call above.
Blocking means it will wait for the answer so probably only use it in the lobby and not the game.
Possible returns:
* `nil` if there was no query or the query failed completely
* `false` if it's not ready yet and call isn't blocking
* `true, false` if it finished and drivers don't have issues
* `true, true, number glMajor, number glMinor, string URL, string driver`.

### Texturing
* engine can load HDR textures (`.hdr` format/extension) with automatic upcasting to FLOAT internal storage format (takes 4x more memory). No user side changes are needed
* LegacyAtlas now produces mipmaps. Previously it never did.
* s3o textures reload fix for Assimp models

### Defs
* fix unitdef.useFootPrintCollisionVolume. Now the `useFootPrintCollisionVolume` tag takes precedence over the default sphere that appears when `collisionVolumeScales` are zero.
* basecontent `setupdefs.lua` fills `UnitDefs[x].hasShield` correctly
* added modrules to set default flanking min/max damages, though this was already possible (and is better done) via unitdefs_post.

### Miscellaneous
* add boolean springsetting `RotOverheadClampMap`, default true, disabling it allows the rotatable overhead camera to move outside of map boundaries.
* selection keys: add new filter `Guarding` (first command in the queue is guard) and new count `SelectClosestToCursor` (always a single unit, doesn't work with `SelectNum_X` and doesn't pick the second-closest if appending, etc).
* area-reclaim: always ignore wrecks being rezzed (now the CTRL modifier is only for features with `autoreclaimable = false` in their def; use single-target reclaim for things being rezzed)
* fixed `Spring.TraceScreenRay()` failing to hit terrain
