---
layout: default
title: Game
parent: Lua API
permalink: lua-api/globals/Game
---

{% raw %}

# global Game






## fields


### Game.maxUnits

```lua
Game.maxUnits : number
```




### Game.maxTeams

```lua
Game.maxTeams : number
```




### Game.maxPlayers

```lua
Game.maxPlayers : number
```




### Game.squareSize

```lua
Game.squareSize : number
```



Divide Game.mapSizeX or Game.mapSizeZ by this to get engine's "mapDims" coordinates. The resolution of height, yard and type maps.


### Game.metalMapSquareSize

```lua
Game.metalMapSquareSize : number
```



The resolution of metalmap (for use in API such as Spring.GetMetalAmount etc.)


### Game.gameSpeed

```lua
Game.gameSpeed : number
```



Number of simulation gameframes per second


### Game.startPosType

```lua
Game.startPosType : number
```




### Game.ghostedBuildings

```lua
Game.ghostedBuildings : boolean
```




### Game.mapChecksum

```lua
Game.mapChecksum : string
```




### Game.modChecksum

```lua
Game.modChecksum : string
```




### Game.mapDamage

```lua
Game.mapDamage : boolean
```




### Game.mapName

```lua
Game.mapName : string
```




### Game.mapDescription

```lua
Game.mapDescription : string
```



= string Game.mapHumanName


### Game.mapHardness

```lua
Game.mapHardness : number
```




### Game.mapX

```lua
Game.mapX : number
```




### Game.mapY

```lua
Game.mapY : number
```




### Game.mapSizeX

```lua
Game.mapSizeX : number
```



in worldspace/opengl coords. Divide by Game.squareSize to get engine's "mapDims" coordinates


### Game.mapSizeZ

```lua
Game.mapSizeZ : number
```



in worldspace/opengl coords. Divide by Game.squareSize to get engine's "mapDims" coordinates


### Game.gravity

```lua
Game.gravity : number
```




### Game.tidal

```lua
Game.tidal : number
```




### Game.windMin

```lua
Game.windMin : number
```




### Game.windMax

```lua
Game.windMax : number
```




### Game.extractorRadius

```lua
Game.extractorRadius : number
```




### Game.waterDamage

```lua
Game.waterDamage : number
```




### Game.envDamageTypes

```lua
Game.envDamageTypes : table
```



Containing {def}IDs of environmental-damage sources


### Game.gameName

```lua
Game.gameName : string
```




### Game.gameShortName

```lua
Game.gameShortName : string
```




### Game.gameVersion

```lua
Game.gameVersion : string
```




### Game.gameMutator

```lua
Game.gameMutator : string
```




### Game.gameDesc

```lua
Game.gameDesc : string
```




### Game.requireSonarUnderWater

```lua
Game.requireSonarUnderWater : boolean
```




### Game.transportAir

```lua
Game.transportAir : number
```




### Game.transportShip

```lua
Game.transportShip : number
```




### Game.transportHover

```lua
Game.transportHover : number
```




### Game.transportGround

```lua
Game.transportGround : number
```




### Game.fireAtKilled

```lua
Game.fireAtKilled : number
```




### Game.fireAtCrashing

```lua
Game.fireAtCrashing : number
```




### Game.constructionDecay

```lua
Game.constructionDecay : boolean
```




### Game.reclaimAllowEnemies

```lua
Game.reclaimAllowEnemies : boolean
```




### Game.reclaimAllowAllies

```lua
Game.reclaimAllowAllies : boolean
```




### Game.constructionDecayTime

```lua
Game.constructionDecayTime : number
```




### Game.constructionDecaySpeed

```lua
Game.constructionDecaySpeed : number
```




### Game.multiReclaim

```lua
Game.multiReclaim : number
```




### Game.reclaimMethod

```lua
Game.reclaimMethod : number
```




### Game.reclaimUnitMethod

```lua
Game.reclaimUnitMethod : number
```




### Game.reclaimUnitEnergyCostFactor

```lua
Game.reclaimUnitEnergyCostFactor : number
```




### Game.reclaimUnitEfficiency

```lua
Game.reclaimUnitEfficiency : number
```




### Game.reclaimFeatureEnergyCostFactor

```lua
Game.reclaimFeatureEnergyCostFactor : number
```




### Game.repairEnergyCostFactor

```lua
Game.repairEnergyCostFactor : number
```




### Game.resurrectEnergyCostFactor

```lua
Game.resurrectEnergyCostFactor : number
```




### Game.captureEnergyCostFactor

```lua
Game.captureEnergyCostFactor : number
```




### Game.springCategories

```lua
Game.springCategories : table<string,integer>
```



```lua
    example: {
      ["vtol"]         = 0,  ["special"]      = 1,  ["noweapon"]     = 2,
      ["notair"]       = 3,  ["notsub"]       = 4,  ["all"]          = 5,
      ["weapon"]       = 6,  ["notship"]      = 7,  ["notland"]      = 8,
      ["mobile"]       = 9,  ["kbot"]         = 10, ["antigator"]    = 11,
      ["tank"]         = 12, ["plant"]        = 13, ["ship"]         = 14,
      ["antiemg"]      = 15, ["antilaser"]    = 16, ["antiflame"]    = 17,
      ["underwater"]   = 18, ["hover"]        = 19, ["phib"]         = 20,
      ["constr"]       = 21, ["strategic"]    = 22, ["commander"]    = 23,
      ["paral"]        = 24, ["jam"]          = 25, ["mine"]         = 26,
      ["kamikaze"]     = 27, ["minelayer"]    = 28, ["notstructure"] = 29,
      ["air"]          = 30
    }
```


### Game.armorTypes

```lua
Game.armorTypes : table<(string|integer),(integer|string)>
```



(bidirectional)
```lua
    example: {
      [1]  = amphibious,   [2] = anniddm,     [3] = antibomber,
      [4]  = antifighter,  [5] = antiraider,  [6] = atl,
      [7]  = blackhydra,   [8] = bombers,     [9] = commanders,
      [10] = crawlingbombs, ...

      ["amphibious"]   = 1, ["anniddm"]    = 2, ["antibomber"] = 3
      ["antifighter"]  = 4, ["antiraider"] = 5, ["atl"]        = 6
      ["blackhydra"]   = 7, ["bombers"]    = 8, ["commanders"] = 9
      ["crawlingbombs"]= 10, ...
    }
```


### Game.textColorCodes

```lua
Game.textColorCodes : TextColorCode {
    Color: string,
    ColorAndOutline: string,
    Reset: string,
}
```



Table containing keys that represent the color code operations during font rendering




{% endraw %}