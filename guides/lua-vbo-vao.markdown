---
layout: post
title: Lua VBO and VAO
parent: Guides
permalink: guides/lua-vbo-vao
author: lhog
---

# Lua VBO and VAO

## Crash course

### What is VBO/VAO

Read excellent intro here: [Hello-Triangle](https://learnopengl.com/Getting-started/Hello-Triangle)

{: .note-title }
> TLDR
>
> It's a modern and most performant way of loading geometry attributes like position, color, normal vector, texture coordinates and generally any attribute you need to GPU for rendering.

### What is VBO?

VBO is simply an array of interleaved data.

Interleaved means that data of different types go one after another forming an element.

After the first element comes second element structured in the same way, etc. See:

![image]({{ site.baseurl }}/assets/guides/lua-vbo-vao-1.png)

### What types of VBO exist:

Two main types of VBO are vertex buffer and index buffer.

Vertex buffer holds vertex data, index buffer holds indices used to index into vertex buffer.

One canonical example of the use of index buffer is drawing of rectangle.

Rectangles have 4 vertices, but since GPU draws with triangles, rectangle needs to broken down into triangles. In this case triangles are specified as list of indices referencing vertex buffer to render the rectangle:

![image]({{ site.baseurl }}/assets/guides/lua-vbo-vao-2.png)

{: .note }
Use of index buffer is optional, you can skip it, but you will have to duplicate vertex data instead to produce the same two triangles, which is often not desired, especially if you have big geometry.

One additional more advanced type of VBO is instance buffer.

Unlike vertex VBO, which defines per-vertex data, instance VBO defines per instance (per shape) data.

For example you might want to draw exactly the same complex shape N times in N different places, this is where instancing makes sense. You can read more [here](https://learnopengl.com/Advanced-OpenGL/Instancing)

As said, in Recoil instancing is done by means of instance buffer (alternative implementations are possible by out of scope of this basic tutorial):

![image]({{ site.baseurl }}/assets/guides/lua-vbo-vao-3.png)

Here in the example we use instance buffer to offset instances of rectangle in screen space, other possibilities exist too: rotate, re-color, etc.

### What is VAO

VAO serves as glue to tie together various VBOs above, description of their type, description of what each individual attribute inside each VBO means: name, size, type, etc. VAO allows one to define attributes completely flexible.

Usually you want something like position, color, texture coordinates, but you can absolutely skip each one and supply whatever information you need.

Below the schematic of what VAO is to VBOs:

![image]({{ site.baseurl }}/assets/guides/lua-vbo-vao-4.png)

There can be up to 16 input attributes, this number is shared between vertex attributes (mandatory most of the time) and instance attributes (optional).

## VBO and VAO creation

```lua
local someVAO = gl.GetVAO() --get empty VAO

local vertVBO = gl.GetVBO(GL.ARRAY_BUFFER, true) --empty VBO, "GL.ARRAY_BUFFER" means it's either vertex or instance buffer, "true" means this buffer will be optimized by GL driver for frequent updates. Here by the variable name you can guess it's supposed to be vertex buffer

local instVBO = gl.GetVBO(GL.ARRAY_BUFFER, true) --empty VBO, "GL.ARRAY_BUFFER" means it's either vertex or instance buffer, "true" means this buffer will be optimized by GL driver for frequent updates. Here by the variable name you can guess it's supposed to be instance buffer

local indxVBO = gl.GetVBO(GL.ELEMENT_ARRAY_BUFFER, false) -- empty index buffer, not going to be frequently updated ("false").

vertVBO:Define(1000, { --someVBO is created to hold 1000 "elements", see pics above what element is. If suddenly the number of elements exceeds 1000, the buffer will not accept new data, "someVBO" will need to be remand and rebound to VAO
    {id = 0, name = "pos", size = 4}, -- "pos" attribute will hold 4 floats (float is the default type, if "type" is not specified). "id" in the shader must be 0
    {id = 1, name = "color", type=GL.UNSIGNED_BYTE, normalized = true, size = 3}, -- "color" is represented by 3 unsigned bytes (values from 0 to 255), values are normalized (in this case divided by 255 to get float inside shader). "id" in the shader must be 1. This can be useful to hold RGB data.
})

instVBO:Define(2, { --reserve space for 2 elements of the type described below
     {id = 2, name = "posBias", size = 4}, -- posBias is 4 floats attribute of id = 2, note that ids here and ids of vertVBO cannot duplicate. We will use it to offset instances in space
})

indxVBO:Define(2000, GL.UNSIGNED_INT) --defines index buffer with capacity of 2000 elements of type unsigned integer (32 bit integer), other possibilities for type are GL.UNSIGNED_SHORT and GL.UNSIGNED_BYTE, representing 16 bit and 8 bit unsigned integers respectively. If no type is given GL.UNSIGNED_SHORT is the default) - it makes sense as it allows to index 65534 vertices and occupies only 2 bytes per one index.

--here we attach (glue) VBOs into VAO. Note in theory you can use (and I use sometimes) completely empty VAO (no attached buffers), but most often you will want to attach (and create before) at least vertex buffer.
someVAO:AttachVertexBuffer(vertVBO) --note vertVBO and instVBO were created with the same command (except for definition), the only way to tell apart instance buffer from vertex buffer is to see what command was used to attach the VBO to the VAO.
someVAO:AttachInstanceBuffer(instVBO)
someVAO:AttachIndexBuffer(indxVBO)
-- only one attachment of certain type can be made. E.g. you can't attach two vertex buffers.
```

## Uploading VBO data to VBOs
Now we have VBOs structure defined. Time to upload some useful data, that will later be used to draw stuff.
```lua
local vertexData = {
    -- element #1
    0, 0, 0, 1, --this goes into "pos"
    127, 0, 0, --this goes into "color"
    -- element #2
    1, 1, 1, 1, --this goes into "pos"
    127, 127, 0, --this goes into "color"
    -- element #3
    1, 0, 0, 1, --this goes into "pos"
    127, 127, 127, --this goes into "color"
   -- etc
}
vertVBO:Upload(vertexData)

local instanceData = {
    -- element #1
    100, 100, 0, 1, --this goes into "posBias"
    -- element #2
    100, 200, 0, 1, --this goes into "posBias"
    -- element #3
    200, 100, 0, 1, --this goes into "posBias"
   -- etc
}
instVBO:Upload(instanceData)

local indexData = {
    0, 1, 2, --one triangle
    1, 2, 3, --second triangle
    -- etc
}
indxVBO:Upload(indexData)
```
Here I'm showing very basic upload use. All data is uploaded in one go. This is recommended on initial upload or in case your data is static.
Upload() has tons of options to upload only selected attribute, do partial upload, etc:
Upload(tableData, optionalAttribIdx, optionalElementOffset, optionalLuaStartIndex, optionalLuaFinishIndex).
Ask for extended description when you master basic upload!

### Performance Notes

The Upload function as shown above allows updating contiguous blocks of the VBO, which can allow for batched updating of some or all elements of a VBO.
Generally, uploading a single element takes about 5us for 5 vec4's of data. However, we have found that in general, if you want to update more than about 20% of the contents of a VBO, it is faster to just upload the whole VBO instead of partial updates. 

See the code here for the benchmark: 

https://gist.github.com/Beherith/c965cff2a81253e37aed25f4db0c0fce


### Drawing with VAOs
```lua
---somewhere in widget:Draw...()

-- draw WITHOUT index buffer (index buffer is not needed and won't be used if attached)
gl.UseShader(someShader) --yes, you need a shader. No, without shader you won't see a pixel
someVAO:DrawArrays(GL.TRIANGLES, numberOfElements, firstElementIndex, numberOfInstances, firstInstanceIndex) --GL.TRIANGLES means every 3 element in vertex buffer are used to output a triangle. Besides GL.TRIANGLES you can draw with points, lines, stripes, and tons of other stuff. See https://docs.gl/gl4/glDrawArrays , the rest of options are optional and self descriptive
gl.UseShader(0)


-- draw WITH index buffer (index buffer must be attached to VAO)
gl.UseShader(someShader) --yes, you need a shader. No, without shader you won't see a pixel
someVAO:DrawElements(GL.TRIANGLES, numberOfIndices, indexOfFirstIndex, numberOfInstances, baseVertex) --GL.TRIANGLES means every 3 element in index buffer are used to index into vertex buffer to output a triangle. Besides GL.TRIANGLES you can draw with points, lines, stripes, and tons of other stuff. See https://docs.gl/gl4/glDrawElements , the rest of options are optional and mostly self descriptive
gl.UseShader(0)
```
