+++
title = 'Referencing textures'
date = 2025-07-02T21:17:05-00:00
draft = false
author = "mcukstorm"
+++

## Overview
In LUA, Shaders and the RmlUi texture element, textures are primarily referenced using a string in one of the formats described in this article. 

Many related to units or features and in these cases the UnitDefId is used, this id is generated at runtime and can vary if unit defs are added/removed so hard coding these numbers is not advisable and they should be looked up from the unitDef name. See [Unit types basics guide]({{% ref "/docs/guides/getting-started/unit-types-basics/#the-unitdefs-table-inside-wupgets" %}})

## Terminology
- UnitName - a unique string identifier given to a unit, e.g. armcom for Armada Commander in the BAR game content
- UnitDefId - a numeric ID given to this unit by the spring/recoil engine at runtime

## Texture References
Textures in recoil are generally referenced by a string in one of the formats listed below.

`#<UnitDefId#int>` -- Build picture / the image that would be shown in a list of things a constructor could build e.g. `#101`

`^<UnitDefId#int>` -- Radar icon for unit e.g. `^101`

`%<UnitDefId#int>:<texNum#int[0-1]>` -- The unit texture for UnitDefId, each unit has two textures referenced here as 0 & 1 e.g. `%35:1`

The above reference for unit textures can also be used for features (e.g trees). Feature textures are effectively the same as units but always have negative UnitDefId e.g. `%-12:0`

`!<luaTextureId#int>` -- Lua created textures referenced by id provided at time of texture creation

`*<atlasId#int>` -- Atlas/sprite (added by LUA CreateTextureAtlas)

`$<textureName>` -- There are also named textures, with ssmf_splat_normals, extra and info also have subtextures beneath, below is a table of many named textures available.

**FIXME: This table is a work in progress**

| Texture Name           | Description                                                                                              | Notes                                                                                |
|------------------------|----------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------|
| $units                 |                                                                                                          |                                                                                      |
| $units1                |                                                                                                          |                                                                                      |
| $units2                |                                                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| **Cube Maps**          | All textures in this section are TEXTURE_CUBE_MAP_ARB                                                    |                                                                                      |
| $specular              |                                                                                                          |                                                                                      |
| $reflection            |                                                                                                          |                                                                                      |
| $map_reflection        |                                                                                                          |                                                                                      |
| $sky_reflection        |                                                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| **Specials**           |                                                                                                          |                                                                                      |
| $shadow                |                                                                                                          |                                                                                      |
| $shadow_color          |                                                                                                          |                                                                                      |
| $heightmap             |                                                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| **SMF Maps**           |                                                                                                          |                                                                                      |
| $grass                 |                                                                                                          |                                                                                      |
| $detail                |                                                                                                          |                                                                                      |
| $minimap               |                                                                                                          |                                                                                      |
| $shading               |                                                                                                          |                                                                                      |
| $normals               |                                                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| **SSMF Maps**          |                                                                                                          |                                                                                      |
| $ssmf_normals          |                                                                                                          |                                                                                      |
| $ssmf_specular         |                                                                                                          |                                                                                      |
| $ssmf_splat_distr      |                                                                                                          |                                                                                      |
| $ssmf_splat_detail     |                                                                                                          |                                                                                      |
| $ssmf_splat_normals    | Contains a numerically indexed array of splat normals                                                    |                                                                                      |
| $ssmf_splat_normals:X  | Where X is a numeric index                                                                               |                                                                                      |
| $ssmf_sky_refl         |                                                                                                          |                                                                                      |
| $ssmf_emission         |                                                                                                          |                                                                                      |
| $ssmf_parallax         |                                                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
|                        | Items under $info use either a _ or : separator, these are produced by Rendering/Map/InfoTexture classes |                                                                                      |
| $info                  | all map overlays                                                                                         |                                                                                      |
| $info:los              | LOS-map overlay                                                                                          |                                                                                      |
| $info:airlos           |                                                                                                          |                                                                                      |
| $info:height           | height-map overlay                                                                                       |                                                                                      |
| $info:metal            | metal-map overlay                                                                                        |                                                                                      |
| $info:metalextraction  |                                                                                                          |                                                                                      |
| $info:path             | path traversability-map overlay                                                                          |                                                                                      |
| $info:radar            |                                                                                                          |                                                                                      |
| $info:los:X            | LOS-map overlay tracking allyteam X (a numeric allyTeamID), e.g. "$info:los:2"                           | Restricted to the handle's own allyteam unless it has full read access (e.g. fullview spectators) |
| $info:airlos:X         | air LOS-map overlay tracking allyteam X                                                                  | Same restrictions as $info:los:X                                                     |
| $info:radar:X          | radar-map overlay tracking allyteam X                                                                    | Same restrictions as $info:los:X                                                     |
| $info:heat             | Not yet implemented? path heat-map overlay                                                               |                                                                                      |
| $info:flow             | Not yet implemented? path flow-map overlay                                                               |                                                                                      |
| $info:pathcost         | Not yet implemented? path cost-map overlay                                                               |                                                                                      |
| $extra                 | Appears to be an alias of $info?                                                                         |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $map_gbuffer_normtex   | Contains the smoothed normals buffer of the map in view in world space coordinates                       | To get true normal vectors from it, you must multiply the vector by 2 and subtract 1 |
| $map_gbuffer_difftex   | Contains the diffuse texture buffer of the map in view.                                                  |                                                                                      |
| $map_gbuffer_spectex   | Contains the specular textures of the map in view.                                                       |                                                                                      |
| $map_gbuffer_emittex   | For emissive materials (bloom would be the canonical use).                                               |                                                                                      |
| $map_gbuffer_misctex   | For arbitrary shader data.                                                                               |                                                                                      |
| $map_gbuffer_zvaltex   | Contains the depth values (z-buffer) of the map in view.                                                 |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $map_gb_nt             | Alias of $map_gbuffer_normtex                                                                            |                                                                                      |
| $map_gb_dt             | Alias of $map_gbuffer_difftex                                                                            |                                                                                      |
| $map_gb_st             | Alias of $map_gbuffer_spectex                                                                            |                                                                                      |
| $map_gb_et             | Alias of $map_gbuffer_emittex                                                                            |                                                                                      |
| $map_gb_mt             | Alias of $map_gbuffer_misctex                                                                            |                                                                                      |
| $map_gb_zt             | Alias of $map_gbuffer_zvaltex                                                                            |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $model_gbuffer_normtex | Contains the smoothed normals buffer of the models in view in world space coordinates                    | To get true normal vectors from it, you must multiply the vector by 2 and subtract 1 |
| $model_gbuffer_difftex | Contains the diffuse texture buffer of the models in view.                                               |                                                                                      |
| $model_gbuffer_spectex | Contains the specular textures of the models in view.                                                    |                                                                                      |
| $model_gbuffer_emittex | For emissive materials (bloom would be the canonical use).                                               |                                                                                      |
| $model_gbuffer_misctex | For arbitrary shader data.                                                                               |                                                                                      |
| $model_gbuffer_zvaltex | Contains the depth values (z-buffer) of the models in view.                                              |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $mdl_gb_nt             | Alias of $model_gbuffer_normtex                                                                          |                                                                                      |
| $mdl_gb_dt             | Alias of $model_gbuffer_difftex                                                                          |                                                                                      |
| $mdl_gb_st             | Alias of $model_gbuffer_spectex                                                                          |                                                                                      |
| $mdl_gb_et             | Alias of $model_gbuffer_emittex                                                                          |                                                                                      |
| $mdl_gb_mt             | Alias of $model_gbuffer_misctex                                                                          |                                                                                      |
| $mdl_gb_zt             | Alias of $model_gbuffer_zvaltex                                                                          |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $font                  | Font glyph atlas                                                                                         |                                                                                      |
| $smallfont             | Small font glyph atlas                                                                                   |                                                                                      |
| $fontsmall             | Alias of $fontsmall                                                                                      |                                                                                      |
|                        |                                                                                                          |                                                                                      |
| $explosions            | Explosion effects texture atlas                                                                          |                                                                                      |
| $groundfx              | Ground effects texture atlas containing items like ground flashes and seismic circles (game specific)    |                                                                                      |

