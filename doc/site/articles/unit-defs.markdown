---
layout: post
title: Unit defs
parent: Articles
permalink: articles/unit-defs
author: sprunk
---

# Unit types basics

**Each unit in Spring/Recoil belongs to a single type**.
By default, all units of a type are the same as far as most traits (for example health) go.
Units don't necessarily need to fully conform to their type - **most traits can be changed per-unit**.

This is **similar to other RTS engines**.
For example, a default Starcraft2 marine has 45 health.
Specific marines can actually have 55 health (after an upgrade), or 22 health (with 50% difficulty handicap), or 200 health (if they're the special campaign marine Jim Raynor).
But marines, as a generalized type, have 45 health.
It works **essentially the same way in Recoil**.
Note that the **type itself cannot be modified at runtime**, though you can still easily apply a modified value to every unit of a type.

The **set of unit types is static**.
You **cannot dynamically add new types**, though you **can generate them beforehand**, including via code.

The information about a unit type is usually called a **unit def** (from "definition"), and sometimes the type itself is referred to by "unit def" as well.
This article will talk about and compare the two ways that unit defs are often dealt with that are often confused.

As a general remark, the same notes apply not just to unit types, but also feature types and weapon types and can be applied there directly. 

## Unit def files

At their simplest, games can provide **a set of simple Lua files with unit defs in the `./units` subfolder**, which **return a table** like this:
```lua
-- ./units/light_tank.lua
return {
  light_tank = {
    health = 100,
    maxVelocity = 1,
    requiresSuperSecretResearch = false,
    customParams = { flammable = 1, },
    ...
  },
}
```
The above defines a unit def named "light_tank" with the appropriate stats.
Note that each def **must have a unique name**.

Note also the keys used inside: `health` and `maxVelocity` are standard and interpreted by the engine, you can find their meanings in the documentation.
**The `customParams` table is somewhat special as it is itself standard but its contents are not**: anything inside (in this case `customParams.flammable`) needs to be handled by the game.
Lastly, `requiresSuperSecretResearch` is non-standard.
We will get back to the non-standard ones later.

In the example above, the filename matches the single def inside and is basically just a static set of values.
This **is generally the convention** for various reasons (for example it makes it easier for a mod to replace units one by one, and the files can get fairly long), but it **doesn't have to** be the case.
In particular, you **can put multiple units there and have Lua code inside** to generate values, for example:
```lua
-- ./units/various_tanks.lua
local base_tank_health = Shared.base_tank_health
return {
  light_tank = {
    health = base_tank_health * 1,
    maxVelocity = 1,
    requiresSuperSecretResearch = false,
    customParams = { flammable = 1, },
    ...
  },
  heavy_tank = {
    health = base_tank_health * 2.5,
    maxVelocity = 0.3,
    requiresSuperSecretResearch = true,
    customParams = { flammable = 0, },
    ...
  }
}
```

At some point you **might want to post-process** these values.
For example you'd like to try out a sweeping design change, or maybe the match at hand is a custom scenario with different rules.
For those cases, **default engine content lets you supply a file, `./gamedata/unitdefs_post.lua`**, which has access to a `UnitDefs` table with all the defs.
For example, let's say you want to make all units slightly faster, but the healthiest ones more fragile using some arbitrary formula:
```lua
-- ./gamedata/unitdefs_post.lua
for unitDefName, unitDef in pairs(UnitDefs) do
  UnitDefs[unitDefName] = lowerkeys(unitDef)
end

for unitDefName, unitDef in pairs(UnitDefs) do
  if unitDef.maxvelocity then
    unitDef.maxvelocity = unitDef.maxvelocity * 1.2
  end

  if unitDef.health and unitDef.health > 300 then
    unitDef.health = 300 + math.sqrt(unitDef.health - 300)
  end

  if unitDef.requiressupersecretresearch then
    unitDef.customparams.tech_level_required = 4
  end
end
```

First, the `lowerkeys` function.
Maybe some def files defined "maxVelocity" with an uppercase 'V' and some "maxvelocity" with a lowercase 'v', maybe even some used "MAXVELOCITY".
In Lua, these are all different keys.
The engine doesn't mind either way and accepts any casing, but Lua does, so calling lowerkeys **makes sure that handling those in post-processing does not become a hassle**.
Some games put the lowerkeys call inside the individual def files as a convention; you also **don't have to do this at all** if you pay attention and don't expect this to be a problem.

Note that **there are checks** so that calculation only happens if the value is defined, **even for the standard ones** (i.e. `if unitDef.health and...`).
This is because **while the engine does fill in defaults** (so you don't need to define velocity for buildings, for instance) **it does so at a later point**: so far everything is still **just a regular Lua table**.
In particular, you can make use of the fact that an entry is not defined and/or fill it with some default calculated by you.

Another thing going on here is the handling non-standard entries in the table.
Anything which is not a standard key **is going to be discarded by the engine after this point unless it's inside the `customparams` table**.
This can be useful if you just want to define a helper for post-processing - in particular this can even be something like a function.
`customParams` let you keep non-standard values, but **only allows strings as keys, and strings and numbers as values**.

There is also a **pre-processing file, `./gamedata/unitdefs_pre.lua`**, which can prepare data **before any def files are read**.
It exposes **a global `Shared` table**, which can be populated there and which can be seen used in one of the examples above.
Use it to propagate reference values to multiple defs without having to redefine them.

## The `UnitDefs` table inside wupgets

This is where things get somewhat messy. The engine exposes a table also called `UnitDefs` to wupgets.
However, **this is NOT the same table as the one above**.
The table above gets parsed into internal engine structures and is gone.
The engine exposes a new table with those parsed values. This means that, compared to unit def files:

 * the overall table is **indexed by numerical IDs, not the name**.
So it's (say) `UnitDefs[123]` instead of `UnitDefs["light_tank"]`.
Default engine content provides **a `UnitDefNames` table indexed by the name** though.
The internal name is also **provided in the table under the key `"name"`**.
The majority of **wupget interfaces use the numerical ID**.

 * keys are **not the same**.
For example, metal cost is read as `buildCostMetal` from unit def files, but exposed as `metalCost` in `UnitDefs`.
In particular, they often **have "proper" uppercase even if the post-processing file is all lowercase**.
This may well be **the most common misconception** and mistake when it comes to defs!
Remember, **don't copy-paste keys between unit def files and wupgets** or vice versa.

 * unused values are discarded.
In the example above, `UnitDefs[x].requireSuperSecretTech` (and the lowercase spelling) is `nil`.

 * **values are not the same**.
For example, speed is read as elmos/frame from unit def files, but exposed as elmos/second.
Some values have caps or offsets applied. See the documentation for specifics.

## Advanced technicalities

Here's some looser remarks around these topics.

### Unit def files

In truth, the engine **only directly reads one file for all def file processing, which is `./gamedata/defs.lua`**.
This file is **provided in basecontent** and for all the various def types (unit, weapon, etc) it loads a pre-processing file, individual legacy def files (TDF, FBI and other Total Annihilation formats), the individual Lua def files under `./units/`, and then the post-processing file.
The practical effect is that you can **customize the loading process somewhat** (load things in a different order, or from elsewhere than `./units/`, etc.) if you don't like the default one.

A limited number of **wupget interfaces are available** during def loading.
This includes **getters involving the match** in general (map, player roster etc), of which most importantly **mod-options**.
You can use them to customise a match (for example, maybe aircraft fly higher on maps with lava or something like that).

VFS is also available, **as a game dev you can expose interfaces for modders** or mappers by attempting to load specific files.

### UnitDefs

A unitDef is a proxy table with the `__index` meta-method.
**According to measurements** this makes it somewhat **slower than a plain Lua table**, so it might be worth **caching if a wupget mostly uses a single field** from it.

There is a **defs-editing dev mode where you can edit defs**, toggled via `/editdefs` (requires cheats).
In this mode, changes are done by just **assigning to a unitDef in Lua code**, which **isn't normally possible**.
Keep in mind that there is **no standard widget** yet to allow easy editing, and that **editing the def files will do nothing**
(of course unless you make your editing widget read them, but remember the caveat where the keys and values differ between
unit defs and `UnitDefs`). This mode is **not usable for game logic** and will desync if used in multiplayer.

There's three **minor differences between `WeaponDefs` and `UnitDefs`/`FeatureDefs`**:
 * `WeaponDefs` are 0-indexed while the others are 1-indexed. Beware of `for i = 1, #WeaponDefs do`, this is incorrect!
 * **negative weaponDefIDs are valid and mean things like lava or collisions**. Check the `Game.envDamageTypes` table.
 * it is possible to iterate over all keys of a unitDef via `for key, value in unitDef:pairs() do`, but this is currently not possible for either weapon or feature defs.

