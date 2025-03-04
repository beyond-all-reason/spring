---
layout: default
title: VBOAttributeDef
parent: Lua API
permalink: lua-api/types/VBOAttributeDef
---

# class VBOAttributeDef





[<a href="https://github.com/beyond-all-reason/spring/blob/f30aea20ae92a148009e573d844c20fa0bb31671/rts/Lua/LuaVBOImpl.cpp#L469-L496" target="_blank">source</a>]

---



## fields
---

### VBOAttributeDef.id
---
```lua
VBOAttributeDef.id : integer
```



The location in the vertex shader layout e.g.: layout (location = 0) in vec2
aPos. optional attrib, specifies location in the vertex shader. If not
specified the implementation will increment the counter starting from 0.
There can be maximum 16 attributes (so id of 15 is max).



### VBOAttributeDef.name
---
```lua
VBOAttributeDef.name : string
```



The name for this VBO, only used for debugging.



### VBOAttributeDef.normalized
---
```lua
VBOAttributeDef.normalized : boolean?
```



(Defaults: `false`)

It's possible to submit say normal without normalizing them first, normalized
will make sure data is normalized.


### VBOAttributeDef.size
---
```lua
VBOAttributeDef.size : integer?
```



Defaults to to 4 for VBO. The number of floats that constitute 1 element in
this buffer. e.g. for the previous layout (location = 0) in vec2 aPos, it
would be size = 2.



### VBOAttributeDef.type
---
```lua
VBOAttributeDef.type : VBODataType
```



(Default: `GL.FLOAT`)

The datatype of this element.



