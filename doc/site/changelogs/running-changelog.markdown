---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the changelog **since version 2590**.

# Caveats
These are the entries which may require special attention when migrating:
* explicitly specified weapon def `scarTTL` value is now the actual TTL.
Previously it was multiplied by the ground decals level springsetting,
which defaults to 3, so if you had tweaked scars they may turn out to have
significantly different TTL now.
* weapons with `groundBounce = true` will now bounce even if `numBounces`
is left undefined. This is because `numBounces = -1`, which is the default
value, will now result in infinite bounces as intended instead of 0.
* a bunch of changes in damage and death Lua events, see below.

# Features

### Death events
* `wupget:UnitDestroyed` will pass the builder as the killer if a unit gets reclaimed. Note that
reclaim still does not generate `UnitDamaged` events.
* `wupget:UnitDestroyed` now receives a 7th argument, weaponDefID, with the cause of death.
Widgets receive this info regardless of LoS on the attacker (no new hax, `UnitDamaged` already did this).
* the weaponDefID above is never `nil`, all causes of death are attributable including things like
"was being built in a factory, got cancelled" or "died automatically due to `isFeature` def tag".
Added a bunch of `Game.envDamageTypes` constants for this purpose. See the table at the bottom of the page.
* the 1000000 damage that applies to units being transported in a non-`releaseHeld` transport when it
dies for non-selfD reasons will now be attributed to the new `TransportKilled` damage type in `UnitDamaged`,
previously was `Killed`.

### Misc Lua
* add `math.normalize(x1, x2, ...) → numbers xn1, xn2, ...`. Normalizes a vector. Can have any dimensions (pass and receive each as a separate value).
Returns a zero vector if passed a zero vector.
* `wupget:DrawWorldPreParticles` now has four boolean parameters depending on which phase is being drawn: above water, below water, reflection, refraction.
They aren't mutually exclusive.
* add `Spring.AllocateTable(arraySlots, hashSlots) → {}`. Returns an empty table with more space allocated.
Use as a microoptimisation when you have a big table which you are going to populate with a known number of elements, for example `#UnitDefs`.
* add `Spring.ForceUnitCollisionUpdate(unitID) → nil`. Forces a unit to have correct collisions. Normally, collisions are updated according
to the `unitQuadPositionUpdateRate` modrule, which may leave them unable to be hit by some weapons when moving. Call this for targets of important
weapons (e.g. in `script.FireWeapon` if it's hitscan) if the modrule has a value greater than 1 to ensure reliable hit detection.
* `pairs()` now looks at the `__pairs` metamethod in tables, same as in Lua 5.2.

### Defs
* add `windup` weapon def tag. Delay in seconds before the first projectile of a salvo appears. Has the same mechanics as burst.

## Fixes
* fix draw position for asymmetric models, they no longer disappear when not appropriate.
* fix streaming very small sound files.
* fix `Spring.SetCameraTarget` reverting to the old position.
* the `quadFieldQuadSizeInElmos` modrule now only accepts powers of two instead of breaking at the edges of the map.

### Death damage types listing

| Game.envDamageTypes.??? | Description                                                                                                                                                       |
|-------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AircraftCrashed         | Aircraft hitting the ground                                                                                                                                       |
| Kamikaze                | Unit exploding due to its kamikaze ability                                                                                                                        |
| SelfD                   | Unit exploding after a self-D command and countdown                                                                                                               |
| ConstructionDecay       | Abandoned nanoframe disappearing (the process itself is HP removal)                                                                                               |
| Reclaimed               | Killed via reclaim (the process itself is HP removal)                                                                                                             |
| TurnedIntoFeature       | Unit dying on completion, without explosion, due to `isFeature` def tag                                                                                           |
| TransportKilled         | Unit was in transport which had no `releaseHeld`. If the transport was not self-destructed, the unit also receives 1000000 damage of this type before dying.      |
| FactoryKilled           | Unit was being built in a factory which died. No direct way to discern how exactly the factory died currently, you'll have to wait for the factory's death event. |
| FactoryCancel           | Unit was being built in a factory but the order was cancelled.                                                                                                    |
| UnitScript              | COB unit script ordered the unit's death. Note that LUS has access to normal kill/damage interfaces instead.                                                      |
| SetNegativeHealth       | A unit had less than 0 health for non-damage reasons (e.g. Lua set it so).                                                                                        |
| OutOfBounds             | A unit was thrown way out of map bounds.                                                                                                                          |
| KilledByCheat           | The `/remove` or `/destroy` commands were used.                                                                                                                   |
| KilledByLua             | Default cause of death when using `Spring.DestroyUnit`.                                                                                                           |

`KilledByLua` is guaranteed to be the "last" value, so you can define your own custom damage types via e.g.
```
Game.envDamageTypes.CullingStrike      = Game.KilledByLua - 1
Game.envDamageTypes.SummonTimerExpired = Game.KilledByLua - 2
```