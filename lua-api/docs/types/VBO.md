---
layout: default
title: VBO
parent: Lua API
permalink: lua-api/types/VBO
---

# class VBO





Vertex Buffer Object

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L35-L41" target="_blank">source</a>]

---

## methods
---

### VBO.BindBufferRange
---
```lua
function VBO.BindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex integer
```
@param `index` - should be in the range between
`5 < index < GL_MAX_UNIFORM_BUFFER_BINDINGS` value (usually 31)

@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise





Bind a range within a buffer object to an indexed buffer target

Generally mimics
https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBufferRange.xhtml
except offset and size are specified in number of elements / element indices.

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1431-L1445" target="_blank">source</a>]


### VBO.Define
---
```lua
function VBO.Define(
  size: number,
  attribs: (number|VBOAttributeDef[])
) ->  nil
```
@param `size` - The maximum number of elements this VBO can have.

@param `attribs` - When number, the maximum number of elements this VBO can have.

Otherwise, an array of arrays specifying the layout.






Specify the kind of VBO you will be using.

```lua


### VBO.Delete
---
```lua
function VBO.Delete() ->  nil
```





[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L109-L113" target="_blank">source</a>]


### VBO.Download
---
```lua
function VBO.Download(
  attributeIndex: integer?,
  elementOffset: integer?,
  elementCount: number?,
  forceGPURead: boolean?
) ->  unknown
```
@param `attributeIndex` - (Default: -1) when supplied with non-default value: only data
from specified attribute will be downloaded - otherwise all attributes are
downloaded

@param `elementOffset` - (Default: 0) download data starting from this element

@param `elementCount` - number of elements to download

@param `forceGPURead` - (Default: false) force downloading the data from GPU buffer as opposed
to using shadow RAM buffer






[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L677-L688" target="_blank">source</a>]


### VBO.DumpDefinition
---
```lua
function VBO.DumpDefinition() ->  nil
```





Logs the definition of the VBO to the console

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1467-L1471" target="_blank">source</a>]


### VBO.GetBufferSize
---
```lua
function VBO.GetBufferSize()
 -> elementsCount number
 -> bufferSizeInBytes number
 -> size number

```





[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L585-L591" target="_blank">source</a>]


### VBO.GetID
---
```lua
function VBO.GetID() -> bufferID number
```





Gets the OpenGL Buffer ID

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1488-L1492" target="_blank">source</a>]


### VBO.InstanceDataFromFeatureDefIDs
---
```lua
function VBO.InstanceDataFromFeatureDefIDs(
  featureDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1244-L1269" target="_blank">source</a>]


### VBO.InstanceDataFromFeatureIDs
---
```lua
function VBO.InstanceDataFromFeatureIDs(
  featureIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1321-L1337" target="_blank">source</a>]


### VBO.InstanceDataFromUnitDefIDs
---
```lua
function VBO.InstanceDataFromUnitDefIDs(
  unitDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout:
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1205-L1230" target="_blank">source</a>]


### VBO.InstanceDataFromUnitIDs
---
```lua
function VBO.InstanceDataFromUnitIDs(
  unitIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout

```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1283-L1309" target="_blank">source</a>]


### VBO.MatrixDataFromProjectileIDs
---
```lua
function VBO.MatrixDataFromProjectileIDs(
  projectileIDs: (integer|integer[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> matDataVec number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer,integer,integer))

```

@return `matDataVec` - 4x4 matrix





[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1349-L1359" target="_blank">source</a>]


### VBO.ModelsVBO
---
```lua
function VBO.ModelsVBO() -> buffer (nil|number)
```

@return `buffer` - size in bytes





Binds engine side vertex or index VBO containing models (units, features) data.

Also fills in VBO definition data as they're set for engine models (no need to do VBO:Define()).

[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1186-L1193" target="_blank">source</a>]


### VBO.UnbindBufferRange
---
```lua
function VBO.UnbindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex number
```
@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise





[<a href="https://github.com/beyond-all-reason/spring/blob/f0c829c6e793112ab93b1ec454b2e6f2a1767d4c/rts/Lua/LuaVBOImpl.cpp#L1452-L1460" target="_blank">source</a>]


### VBO.Upload
---
```lua
function VBO.Upload(
  vboData: number[],
  attributeIndex: integer?,
  elemOffset: integer?,
  luaStartIndex: integer?,
  luaFinishIndex: integer?
)
 -> indexData number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer,integer,integer))

```
@param `vboData` - a lua array of values to upload into the VBO

@param `attributeIndex` - (Default: -1)

If supplied with non-default value then the data from vboData will only be
used to upload the data to this particular attribute.

The whole vboData is expected to contain only attributeIndex data.

Otherwise all attributes get updated sequentially across attributes and elements.

@param `elemOffset` - (Default: 0) Which VBO element to start uploading data from Lua array into.

@param `luaStartIndex` - (Default: 1) Start uploading from that element in supplied Lua array.

@param `luaFinishIndex` - Consider this element the last element in Lua array.






Uploads the data (array of floats) into the VBO

```lua
vbo:Upload(posArray, 0, 1)




