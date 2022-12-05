--- Constants related to OpenGL
-- @module ConstGL

--- Drawing Primitives
-- @section primitives

--- @table GL
-- @number POINTS
-- @number LINES
-- @number LINE_LOOP
-- @number LINE_STRIP
-- @number TRIANGLES
-- @number TRIANGLE_STRIP
-- @number TRIANGLE_FAN
-- @number QUADS
-- @number QUAD_STRIP
-- @number POLYGON
-- @number PATCHES

--- BlendingFactorDest
-- @section blendingfactordest

--- @table GL
-- @number ZERO
-- @number ONE
-- @number SRC_COLOR
-- @number ONE_MINUS_SRC_COLOR
-- @number SRC_ALPHA
-- @number ONE_MINUS_SRC_ALPHA
-- @number DST_ALPHA
-- @number ONE_MINUS_DST_ALPHA

--- BlendingFactorSrc
-- @section blendingfactorsrc

--- @table GL
-- @number ZERO
-- @number ONE
-- @number SRC_COLOR
-- @number ONE_MINUS_SRC_COLOR
-- @number SRC_ALPHA
-- @number ONE_MINUS_SRC_ALPHA
-- @number DST_ALPHA
-- @number ONE_MINUS_DST_ALPHA
-- @number DST_COLOR
-- @number ONE_MINUS_DST_COLOR
-- @number SRC_ALPHA_SATURATE

--- AlphaFunction and DepthFunction
-- @section alphadepth

--- @table GL
-- @number NEVER
-- @number LESS
-- @number EQUAL
-- @number LEQUAL
-- @number GREATER
-- @number NOTEQUAL
-- @number GEQUAL
-- @number ALWAYS

--- LogicOp
-- @section logicop

--- @table GL
-- @number CLEAR
-- @number AND
-- @number AND_REVERSE
-- @number COPY
-- @number AND_INVERTED
-- @number NOOP
-- @number XOR
-- @number OR
-- @number NOR
-- @number EQUIV
-- @number INVERT
-- @number OR_REVERSE
-- @number COPY_INVERTED
-- @number OR_INVERTED
-- @number NAND
-- @number SET

--- Culling
-- @section culling

--- @table GL
-- @number BACK
-- @number FRONT
-- @number FRONT_AND_BACK

--- PolygonMode
-- @section polygonmode

--- @table GL
-- @number POINT
-- @number LINE
-- @number FILL

--- Clear Bits
-- @section clearbits

--- @table GL
-- @number DEPTH_BUFFER_BIT
-- @number ACCUM_BUFFER_BIT
-- @number STENCIL_BUFFER_BIT
-- @number COLOR_BUFFER_BIT

--- ShadeModel
-- @section shademodel

--- @table GL
-- @number FLAT
-- @number SMOOTH

--- MatrixMode
-- @section matrixmode

--- @table GL
-- @number MODELVIEW
-- @number PROJECTION
-- @number TEXTURE

--- Texture Filtering
-- @section texturefiltering

--- @table GL
-- @number NEAREST
-- @number LINEAR
-- @number NEAREST_MIPMAP_NEAREST
-- @number LINEAR_MIPMAP_NEAREST
-- @number NEAREST_MIPMAP_LINEAR
-- @number LINEAR_MIPMAP_LINEAR

--- Texture Clamping
-- @section textureclamping

--- @table GL
-- @number REPEAT
-- @number MIRRORED_REPEAT
-- @number CLAMP
-- @number CLAMP_TO_EDGE
-- @number CLAMP_TO_BORDER

--- Texture Environment
-- @section textureenvironment

--- @table GL
-- @number TEXTURE_ENV
-- @number TEXTURE_ENV_MODE
-- @number TEXTURE_ENV_COLOR
-- @number MODULATE
-- @number DECAL
-- @number BLEND
-- @number REPLACE

--- @field GL_TEXTURE_FILTER_CONTROL

--- @field GL_TEXTURE_LOD_BIAS

--- Texture Generation
-- @section texturegeneration

--- @table GL
-- @number TEXTURE_GEN_MODE
-- @number EYE_PLANE
-- @number OBJECT_PLANE
-- @number EYE_LINEAR
-- @number OBJECT_LINEAR
-- @number SPHERE_MAP
-- @number NORMAL_MAP
-- @number REFLECTION_MAP
-- @number S
-- @number T
-- @number R
-- @number Q

--- glPushAttrib() bits
-- @section glpushattribbits

--- @table GL
-- @number CURRENT_BIT
-- @number POINT_BIT
-- @number LINE_BIT
-- @number POLYGON_BIT
-- @number POLYGON_STIPPLE_BIT
-- @number PIXEL_MODE_BIT
-- @number LIGHTING_BIT
-- @number FOG_BIT
-- @number DEPTH_BUFFER_BIT
-- @number ACCUM_BUFFER_BIT
-- @number STENCIL_BUFFER_BIT
-- @number VIEWPORT_BIT
-- @number TRANSFORM_BIT
-- @number ENABLE_BIT
-- @number COLOR_BUFFER_BIT
-- @number HINT_BIT
-- @number EVAL_BIT
-- @number LIST_BIT
-- @number TEXTURE_BIT
-- @number SCISSOR_BIT
-- @number ALL_ATTRIB_BITS

--- glHint() targets
-- @section glhinttargets

--- @table GL
-- @number FOG_HINT
-- @number LINE_SMOOTH_HINT
-- @number POINT_SMOOTH_HINT
-- @number POLYGON_SMOOTH_HINT
-- @number PERSPECTIVE_CORRECTION_HINT

--- glHint() modes
-- @section glhintmodes

--- @table GL
-- @number DONT_CARE
-- @number FASTEST
-- @number NICEST

--- Light Specification
-- @section lightspecification

--- @table GL
-- @number AMBIENT
-- @number DIFFUSE
-- @number SPECULAR
-- @number POSITION
-- @number SPOT_DIRECTION
-- @number SPOT_EXPONENT
-- @number SPOT_CUTOFF
-- @number CONSTANT_ATTENUATION
-- @number LINEAR_ATTENUATION
-- @number QUADRATIC_ATTENUATION

--- Shader Types
-- @section shadertypes

--- @table GL
-- @number VERTEX_SHADER
-- @number TESS_CONTROL_SHADER
-- @number TESS_EVALUATION_SHADER
-- @number GEOMETRY_SHADER
-- @number FRAGMENT_SHADER

--- Geometry Shader Parameters
-- @section geometryshaderparameters

--- @table GL
-- @number GEOMETRY_INPUT_TYPE
-- @number GEOMETRY_OUTPUT_TYPE
-- @number GEOMETRY_VERTICES_OUT

--- Tesselation control shader parameters
-- @section tesselationcontrolshaderparameters

--- @table GL
-- @number PATCH_VERTICES
-- @number PATCH_DEFAULT_OUTER_LEVEL
-- @number PATCH_DEFAULT_INNER_LEVEL

--- Texture Formats
-- @section textureformats

--- @field GL_RGBA16F_ARB 0x881A

--- @field GL_RGBA32F_ARB 0x8814

--- @field GL_DEPTH_COMPONENT 0x1902

--- @field GL_DEPTH_COMPONENT16 0x81A5

--- @field GL_DEPTH_COMPONENT24 0x81A6

--- @field GL_DEPTH_COMPONENT32 0x81A7

--- RBO Formats
-- @section rboformats

--- @field GL_RGB 0x1907

--- @field GL_RGBA 0x1908

--- @field GL_DEPTH_COMPONENT 0x1902

--- @field GL_STENCIL_INDEX 0x1901

--- FBO Targets
-- @section fbotargets

--- @field GL_FRAMEBUFFER_EXT 0x8D40

--- @field GL_READ_FRAMEBUFFER_EXT 0x8CA8

--- @field GL_DRAW_FRAMEBUFFER_EXT 0x8CA9

--- FBO Status
-- @section fbostatus

--- @field GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5

--- @field GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6

--- @field GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7

--- @field GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8

--- @field GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9

--- @field GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA

--- @field GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB

--- @field GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC

--- @field GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD

--- @field GL_FRAMEBUFFER_STATUS_ERROR_EXT 0x8CDE

-- FBO Attachments
--- @section fboattachments

--- @field GL_COLOR_ATTACHMENT0_EXT 0x8CE0

--- @field GL_COLOR_ATTACHMENT15_EXT 0x8CEF

--- @field GL_DEPTH_ATTACHMENT_EXT 0x8D00

--- @field GL_STENCIL_ATTACHMENT_EXT 0x8D20
