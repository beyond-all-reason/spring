/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstGL.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Rendering/GL/myGL.h"


/******************************************************************************
 * OpenGL Constants
 * @see rts/Lua/LuaConstGL.cpp
******************************************************************************/

bool LuaConstGL::PushEntries(lua_State* L)
{
#define PUSH_GL(cmd) LuaPushNamedNumber(L, #cmd, GL_ ## cmd)

	/***
	 * Drawing Primitives
	 * @section primitives
	 */

	/*** @table GL
	 *
	 * @param POINTS number
	 * @param LINES number
	 * @param LINE_LOOP number
	 * @param LINE_STRIP number
	 * @param TRIANGLES number
	 * @param TRIANGLE_STRIP number
	 * @param TRIANGLE_FAN number
	 * @param QUADS number
	 * @param QUAD_STRIP number
	 * @param POLYGON number
	 * @param PATCHES number
	 */
	PUSH_GL(POINTS);
	PUSH_GL(LINES);
	PUSH_GL(LINE_LOOP);
	PUSH_GL(LINE_STRIP);
	PUSH_GL(TRIANGLES);
	PUSH_GL(TRIANGLE_STRIP);
	PUSH_GL(TRIANGLE_FAN);
	PUSH_GL(QUADS);
	PUSH_GL(QUAD_STRIP);
	PUSH_GL(POLYGON);

	PUSH_GL(LINE_STRIP_ADJACENCY);
	PUSH_GL(LINES_ADJACENCY);
	PUSH_GL(TRIANGLE_STRIP_ADJACENCY);
	PUSH_GL(TRIANGLES_ADJACENCY);
	PUSH_GL(PATCHES);

	/***
	 * BlendingFactorDest
	 * @section blendingfactordest
	 */

	/*** @table GL
	 *
	 * @param ZERO number
	 * @param ONE number
	 * @param SRC_COLOR number
	 * @param ONE_MINUS_SRC_COLOR number
	 * @param SRC_ALPHA number
	 * @param ONE_MINUS_SRC_ALPHA number
	 * @param DST_ALPHA number
	 * @param ONE_MINUS_DST_ALPHA number
	 */
	PUSH_GL(ZERO);
	PUSH_GL(ONE);
	PUSH_GL(SRC_COLOR);
	PUSH_GL(ONE_MINUS_SRC_COLOR);
	PUSH_GL(SRC_ALPHA);
	PUSH_GL(ONE_MINUS_SRC_ALPHA);
	PUSH_GL(DST_ALPHA);
	PUSH_GL(ONE_MINUS_DST_ALPHA);

/***
 * BlendingFactorSrc
 * @section blendingfactorsrc
 */

/***
 * @table GL
 * @param ZERO number
 * @param ONE number
 * @param SRC_COLOR number
 * @param ONE_MINUS_SRC_COLOR number
 * @param SRC_ALPHA number
 * @param ONE_MINUS_SRC_ALPHA number
 * @param DST_ALPHA number
 * @param ONE_MINUS_DST_ALPHA number
 * @param DST_COLOR number
 * @param ONE_MINUS_DST_COLOR number
 * @param SRC_ALPHA_SATURATE number
 * @param FUNC_ADD number
 * @param FUNC_SUBTRACT number
 * @param FUNC_REVERSE_SUBTRACT number
 * @param MIN number
 * @param MAX number
 */
	PUSH_GL(DST_COLOR);
	PUSH_GL(ONE_MINUS_DST_COLOR);
	PUSH_GL(SRC_ALPHA_SATURATE);

	PUSH_GL(FUNC_ADD);
	PUSH_GL(FUNC_SUBTRACT);
	PUSH_GL(FUNC_REVERSE_SUBTRACT);
	PUSH_GL(MIN);
	PUSH_GL(MAX);

/***
 * AlphaFunction and DepthFunction
 * @section alphadepth
 */

/*** @table GL
 *
 * @param NEVER number
 * @param LESS number
 * @param EQUAL number
 * @param LEQUAL number
 * @param GREATER number
 * @param NOTEQUAL number
 * @param GEQUAL number
 * @param ALWAYS number
 */
	PUSH_GL(NEVER);
	PUSH_GL(LESS);
	PUSH_GL(EQUAL);
	PUSH_GL(LEQUAL);
	PUSH_GL(GREATER);
	PUSH_GL(NOTEQUAL);
	PUSH_GL(GEQUAL);
	PUSH_GL(ALWAYS);

/***
 * LogicOp
 * @section logicop
 */

/***
 * @table GL
 * @param CLEAR number
 * @param AND number
 * @param AND_REVERSE number
 * @param COPY number
 * @param AND_INVERTED number
 * @param NOOP number
 * @param XOR number
 * @param OR number
 * @param NOR number
 * @param EQUIV number
 * @param INVERT number
 * @param OR_REVERSE number
 * @param COPY_INVERTED number
 * @param OR_INVERTED number
 * @param NAND number
 * @param SET number
 */
	PUSH_GL(CLEAR);
	PUSH_GL(AND);
	PUSH_GL(AND_REVERSE);
	PUSH_GL(COPY);
	PUSH_GL(AND_INVERTED);
	PUSH_GL(NOOP);
	PUSH_GL(XOR);
	PUSH_GL(OR);
	PUSH_GL(NOR);
	PUSH_GL(EQUIV);
	PUSH_GL(INVERT);
	PUSH_GL(OR_REVERSE);
	PUSH_GL(COPY_INVERTED);
	PUSH_GL(OR_INVERTED);
	PUSH_GL(NAND);
	PUSH_GL(SET);

	/***
	 * Culling
	 * @section culling
	 */

	/***
	 * @table GL
	 * @param BACK number
	 * @param FRONT number
	 * @param FRONT_AND_BACK number
	 */
	PUSH_GL(BACK);
	PUSH_GL(FRONT);
	PUSH_GL(FRONT_AND_BACK);

	/***
	 * PolygonMode
	 * @section polygonmode
	 */

	/***
	 * @table GL
	 * @param POINT number
	 * @param LINE number
	 * @param FILL number
	 */
	PUSH_GL(POINT);
	PUSH_GL(LINE);
	PUSH_GL(FILL);

	/***
	 * Clear Bits
	 * @section clearbits
	 */

	/***
	 * @table GL
	 * @param DEPTH_BUFFER_BIT number
	 * @param ACCUM_BUFFER_BIT number
	 * @param STENCIL_BUFFER_BIT number
	 * @param COLOR_BUFFER_BIT number
	 */
	PUSH_GL(DEPTH_BUFFER_BIT);
	PUSH_GL(ACCUM_BUFFER_BIT);
	PUSH_GL(STENCIL_BUFFER_BIT);
	PUSH_GL(COLOR_BUFFER_BIT);

	/***
	 * ShadeModel
	 * @section shademodel
	 */

	/***
	 * @table GL
	 * @param FLAT number
	 * @param SMOOTH number
	 */
	PUSH_GL(FLAT);
	PUSH_GL(SMOOTH);

	/***
	 * MatrixMode
	 * @section matrixmode
	 */

	/***
	 * @table GL
	 * @param MODELVIEW number
	 * @param PROJECTION number
	 * @param TEXTURE number
	 */
	PUSH_GL(MODELVIEW);
	PUSH_GL(PROJECTION);
	PUSH_GL(TEXTURE);

	/***
	 * Texture Filtering
	 * @section texturefiltering
	 */

	/***
	 * @table GL
	 * @param NEAREST number
	 * @param LINEAR number
	 * @param NEAREST_MIPMAP_NEAREST number
	 * @param LINEAR_MIPMAP_NEAREST number
	 * @param NEAREST_MIPMAP_LINEAR number
	 * @param LINEAR_MIPMAP_LINEAR number
	 */
	PUSH_GL(NEAREST);
	PUSH_GL(LINEAR);
	PUSH_GL(NEAREST_MIPMAP_NEAREST);
	PUSH_GL(LINEAR_MIPMAP_NEAREST);
	PUSH_GL(NEAREST_MIPMAP_LINEAR);
	PUSH_GL(LINEAR_MIPMAP_LINEAR);

	/***
	 * Texture Clamping
	 * @section textureclamping
	 */

	/***
	 * @table GL
	 * @param REPEAT number
	 * @param MIRRORED_REPEAT number
	 * @param CLAMP number
	 * @param CLAMP_TO_EDGE number
	 * @param CLAMP_TO_BORDER number
	 */
	PUSH_GL(REPEAT);
	PUSH_GL(MIRRORED_REPEAT);
	PUSH_GL(CLAMP);
	PUSH_GL(CLAMP_TO_EDGE);
	PUSH_GL(CLAMP_TO_BORDER);

	/***
	 * Texture Environment
	 * @section textureenvironment
	 */

	/***
	 * @table GL
	 * @param TEXTURE_ENV number
	 * @param TEXTURE_ENV_MODE number
	 * @param TEXTURE_ENV_COLOR number
	 * @param MODULATE number
	 * @param DECAL number
	 * @param BLEND number
	 * @param REPLACE number
	 */
	PUSH_GL(TEXTURE_ENV);
	PUSH_GL(TEXTURE_ENV_MODE);
	PUSH_GL(TEXTURE_ENV_COLOR);
	PUSH_GL(MODULATE);
	PUSH_GL(DECAL);
	PUSH_GL(BLEND);
	PUSH_GL(REPLACE);

	/// @field GL_TEXTURE_FILTER_CONTROL
	PUSH_GL(TEXTURE_FILTER_CONTROL);
	/// @field GL_TEXTURE_LOD_BIAS
	PUSH_GL(TEXTURE_LOD_BIAS);

	/***
	 * Texture Generation
	 * @section texturegeneration
	 */

	/***
	 * @table GL
	 * @param TEXTURE_GEN_MODE number
	 * @param EYE_PLANE number
	 * @param OBJECT_PLANE number
	 * @param EYE_LINEAR number
	 * @param OBJECT_LINEAR number
	 * @param SPHERE_MAP number
	 * @param NORMAL_MAP number
	 * @param REFLECTION_MAP number
	 * @param S number
	 * @param T number
	 * @param R number
	 * @param Q number
	 */
	PUSH_GL(TEXTURE_GEN_MODE);
	PUSH_GL(EYE_PLANE);
	PUSH_GL(OBJECT_PLANE);
	PUSH_GL(EYE_LINEAR);
	PUSH_GL(OBJECT_LINEAR);
	PUSH_GL(SPHERE_MAP);
	PUSH_GL(NORMAL_MAP);
	PUSH_GL(REFLECTION_MAP);
	PUSH_GL(S);
	PUSH_GL(T);
	PUSH_GL(R);
	PUSH_GL(Q);

	/***
	 * glPushAttrib() bits
	 * @section glpushattribbits
	 */

	/***
	 * @table GL
	 * @param CURRENT_BIT number
	 * @param POINT_BIT number
	 * @param LINE_BIT number
	 * @param POLYGON_BIT number
	 * @param POLYGON_STIPPLE_BIT number
	 * @param PIXEL_MODE_BIT number
	 * @param LIGHTING_BIT number
	 * @param FOG_BIT number
	 * @param DEPTH_BUFFER_BIT number
	 * @param ACCUM_BUFFER_BIT number
	 * @param STENCIL_BUFFER_BIT number
	 * @param VIEWPORT_BIT number
	 * @param TRANSFORM_BIT number
	 * @param ENABLE_BIT number
	 * @param COLOR_BUFFER_BIT number
	 * @param HINT_BIT number
	 * @param EVAL_BIT number
	 * @param LIST_BIT number
	 * @param TEXTURE_BIT number
	 * @param SCISSOR_BIT number
	 * @param ALL_ATTRIB_BITS number
	 */
	PUSH_GL(CURRENT_BIT);
	PUSH_GL(POINT_BIT);
	PUSH_GL(LINE_BIT);
	PUSH_GL(POLYGON_BIT);
	PUSH_GL(POLYGON_STIPPLE_BIT);
	PUSH_GL(PIXEL_MODE_BIT);
	PUSH_GL(LIGHTING_BIT);
	PUSH_GL(FOG_BIT);
	PUSH_GL(DEPTH_BUFFER_BIT);
	PUSH_GL(ACCUM_BUFFER_BIT);
	PUSH_GL(STENCIL_BUFFER_BIT);
	PUSH_GL(VIEWPORT_BIT);
	PUSH_GL(TRANSFORM_BIT);
	PUSH_GL(ENABLE_BIT);
	PUSH_GL(COLOR_BUFFER_BIT);
	PUSH_GL(HINT_BIT);
	PUSH_GL(EVAL_BIT);
	PUSH_GL(LIST_BIT);
	PUSH_GL(TEXTURE_BIT);
	PUSH_GL(SCISSOR_BIT);
	//PUSH_GL(ALL_ATTRIB_BITS);  // floating point clip
	LuaPushNamedNumber(L, "ALL_ATTRIB_BITS", -1.0f);

	/***
	 * glHint() targets
	 * @section glhinttargets
	 */

	/***
	 * @table GL
	 * @param FOG_HINT number
	 * @param LINE_SMOOTH_HINT number
	 * @param POINT_SMOOTH_HINT number
	 * @param POLYGON_SMOOTH_HINT number
	 * @param PERSPECTIVE_CORRECTION_HINT number
	 */
	PUSH_GL(FOG_HINT);
	PUSH_GL(PERSPECTIVE_CORRECTION_HINT);

	/***
	 * glHint() modes
	 * @section glhintmodes
	 */

	/***
	 * @table GL
	 * @param DONT_CARE number
	 * @param FASTEST number
	 * @param NICEST number
	 */
	PUSH_GL(DONT_CARE);
	PUSH_GL(FASTEST);
	PUSH_GL(NICEST);

	/***
	 * Light Specification
	 * @section lightspecification
	 */

	/***
	 * @table GL
	 * @param AMBIENT number
	 * @param DIFFUSE number
	 * @param SPECULAR number
	 * @param POSITION number
	 * @param SPOT_DIRECTION number
	 * @param SPOT_EXPONENT number
	 * @param SPOT_CUTOFF number
	 * @param CONSTANT_ATTENUATION number
	 * @param LINEAR_ATTENUATION number
	 * @param QUADRATIC_ATTENUATION number
	 */
	PUSH_GL(AMBIENT);
	PUSH_GL(DIFFUSE);
	PUSH_GL(SPECULAR);
	PUSH_GL(POSITION);
	PUSH_GL(SPOT_DIRECTION);
	PUSH_GL(SPOT_EXPONENT);
	PUSH_GL(SPOT_CUTOFF);
	PUSH_GL(CONSTANT_ATTENUATION);
	PUSH_GL(LINEAR_ATTENUATION);
	PUSH_GL(QUADRATIC_ATTENUATION);

	/***
	 * Shader Types
	 * @section shadertypes
	 */

	/***
	 * @table GL
	 * @param VERTEX_SHADER number
	 * @param TESS_CONTROL_SHADER number
	 * @param TESS_EVALUATION_SHADER number
	 * @param GEOMETRY_SHADER number
	 * @param FRAGMENT_SHADER number
	 */
	PUSH_GL(VERTEX_SHADER);
	PUSH_GL(TESS_CONTROL_SHADER);
	PUSH_GL(TESS_EVALUATION_SHADER);
	PUSH_GL(GEOMETRY_SHADER_EXT);
	PUSH_GL(FRAGMENT_SHADER);

	/***
	 * Geometry Shader Parameters
	 * @section geometryshaderparameters
	 */

	/***
	 * @table GL
	 * @param GEOMETRY_INPUT_TYPE number
	 * @param GEOMETRY_OUTPUT_TYPE number
	 * @param GEOMETRY_VERTICES_OUT number
	 */
	PUSH_GL(GEOMETRY_INPUT_TYPE_EXT);
	PUSH_GL(GEOMETRY_OUTPUT_TYPE_EXT);
	PUSH_GL(GEOMETRY_VERTICES_OUT_EXT);

	/***
	 * Tesselation control shader parameters
	 * @section tesselationcontrolshaderparameters
	 */

	/***
	 * @table GL
	 * @param PATCH_VERTICES number
	 * @param PATCH_DEFAULT_OUTER_LEVEL number
	 * @param PATCH_DEFAULT_INNER_LEVEL number
	 */
	PUSH_GL(PATCH_VERTICES);
	PUSH_GL(PATCH_DEFAULT_OUTER_LEVEL);
	PUSH_GL(PATCH_DEFAULT_INNER_LEVEL);

	/***
	 * OpenGL Data Types
	 * @section OpenGL_Data_Types
	 */

	/***
	 * @table GL
	 * @param BYTE number
	 * @param UNSIGNED_BYTE number
	 * @param SHORT number
	 * @param UNSIGNED_SHORT number
	 * @param INT number
	 * @param UNSIGNED_INT number
	 * @param FLOAT number
	 * @param FLOAT_VEC4 number
	 * @param INT_VEC4 number
	 * @param UNSIGNED_INT_VEC4 number
	 * @param FLOAT_MAT4 number
	 */
	PUSH_GL(BYTE);
	PUSH_GL(UNSIGNED_BYTE);
	PUSH_GL(SHORT);
	PUSH_GL(UNSIGNED_SHORT);
	PUSH_GL(INT);
	PUSH_GL(UNSIGNED_INT);
	PUSH_GL(FLOAT);
	PUSH_GL(FLOAT_VEC4);
	PUSH_GL(INT_VEC4);
	PUSH_GL(UNSIGNED_INT_VEC4);
	PUSH_GL(FLOAT_MAT4);

	/***
	 * OpenGL Buffer Types
	 * @section OpenGL_Buffer_Types
	 */

	/***
	 * @table GL
	 * @param ELEMENT_ARRAY_BUFFER number
	 * @param ARRAY_BUFFER number
	 * @param UNIFORM_BUFFER number
	 * @param SHADER_STORAGE_BUFFER number
	 *
	 */
	PUSH_GL(ELEMENT_ARRAY_BUFFER);
	PUSH_GL(ARRAY_BUFFER);
	PUSH_GL(UNIFORM_BUFFER);
	PUSH_GL(SHADER_STORAGE_BUFFER);

	//Texture targets
	PUSH_GL(TEXTURE_1D);
	PUSH_GL(TEXTURE_2D);
	PUSH_GL(TEXTURE_3D);
	PUSH_GL(TEXTURE_CUBE_MAP);
	PUSH_GL(TEXTURE_2D_MULTISAMPLE);

	//Image formats
	PUSH_GL(RGBA32F);
	PUSH_GL(RGBA16F);
	PUSH_GL(RG32F);
	PUSH_GL(RG16F);
	PUSH_GL(R11F_G11F_B10F);
	PUSH_GL(R32F);
	PUSH_GL(R16F);
	PUSH_GL(RGBA32UI);
	PUSH_GL(RGBA16UI);
	PUSH_GL(RGB10_A2UI);
	PUSH_GL(RGBA8UI);
	PUSH_GL(RG32UI);
	PUSH_GL(RG16UI);
	PUSH_GL(RG8UI);
	PUSH_GL(R32UI);
	PUSH_GL(R16UI);
	PUSH_GL(R8UI);
	PUSH_GL(RGBA32I);
	PUSH_GL(RGBA16I);
	PUSH_GL(RGBA8I);
	PUSH_GL(RG32I);
	PUSH_GL(RG16I);
	PUSH_GL(RG8I);
	PUSH_GL(R32I);
	PUSH_GL(R16I);
	PUSH_GL(R8I);
	PUSH_GL(RGBA16);
	PUSH_GL(RGB10_A2);
	PUSH_GL(RGBA8);
	PUSH_GL(RG16);
	PUSH_GL(RG8);
	PUSH_GL(R16);
	PUSH_GL(R8);
	PUSH_GL(RGBA16_SNORM);
	PUSH_GL(RGBA8_SNORM);
	PUSH_GL(RG16_SNORM);
	PUSH_GL(RG8_SNORM);
	PUSH_GL(R16_SNORM);
	PUSH_GL(R8_SNORM);
	PUSH_GL(DEPTH_COMPONENT16);
	PUSH_GL(DEPTH_COMPONENT24);
	PUSH_GL(DEPTH_COMPONENT32);
	PUSH_GL(DEPTH_COMPONENT32F);

	//access specifiers
	PUSH_GL(READ_ONLY);
	PUSH_GL(WRITE_ONLY);
	PUSH_GL(READ_WRITE);

	//memory barrier bits
	PUSH_GL(VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
	PUSH_GL(ELEMENT_ARRAY_BARRIER_BIT);
	PUSH_GL(UNIFORM_BARRIER_BIT);
	PUSH_GL(TEXTURE_FETCH_BARRIER_BIT);
	PUSH_GL(SHADER_IMAGE_ACCESS_BARRIER_BIT);
	PUSH_GL(COMMAND_BARRIER_BIT);
	PUSH_GL(PIXEL_BUFFER_BARRIER_BIT);
	PUSH_GL(TEXTURE_UPDATE_BARRIER_BIT);
	PUSH_GL(BUFFER_UPDATE_BARRIER_BIT);
	PUSH_GL(FRAMEBUFFER_BARRIER_BIT);
	PUSH_GL(TRANSFORM_FEEDBACK_BARRIER_BIT);
	PUSH_GL(ATOMIC_COUNTER_BARRIER_BIT);
	PUSH_GL(SHADER_STORAGE_BARRIER_BIT);
	PUSH_GL(ALL_BARRIER_BITS);

	PUSH_GL(COLOR_ATTACHMENT0);
	PUSH_GL(COLOR_ATTACHMENT1);
	PUSH_GL(COLOR_ATTACHMENT2);
	PUSH_GL(COLOR_ATTACHMENT3);
	PUSH_GL(COLOR_ATTACHMENT4);
	PUSH_GL(COLOR_ATTACHMENT5);
	PUSH_GL(COLOR_ATTACHMENT6);
	PUSH_GL(COLOR_ATTACHMENT7);
	PUSH_GL(COLOR_ATTACHMENT8);
	PUSH_GL(COLOR_ATTACHMENT9);
	PUSH_GL(COLOR_ATTACHMENT10);
	PUSH_GL(COLOR_ATTACHMENT11);
	PUSH_GL(COLOR_ATTACHMENT12);
	PUSH_GL(COLOR_ATTACHMENT13);
	PUSH_GL(COLOR_ATTACHMENT14);
	PUSH_GL(COLOR_ATTACHMENT15);
	PUSH_GL(DEPTH_ATTACHMENT);
	PUSH_GL(STENCIL_ATTACHMENT);



/******************************************************************************
 * FBO Attachments
 * @section fboattachments
******************************************************************************/

	/// @field GL_COLOR_ATTACHMENT0_EXT 0x8CE0
	PUSH_GL(COLOR_ATTACHMENT0_EXT);
	/// @field GL_COLOR_ATTACHMENT1_EXT 0x8CE1
	PUSH_GL(COLOR_ATTACHMENT1_EXT);
	/// @field GL_COLOR_ATTACHMENT2_EXT 0x8CE2
	PUSH_GL(COLOR_ATTACHMENT2_EXT);
	/// @field GL_COLOR_ATTACHMENT3_EXT 0x8CE3
	PUSH_GL(COLOR_ATTACHMENT3_EXT);
	/// @field GL_COLOR_ATTACHMENT4_EXT 0x8CE4
	PUSH_GL(COLOR_ATTACHMENT4_EXT);
	/// @field GL_COLOR_ATTACHMENT5_EXT 0x8CE5
	PUSH_GL(COLOR_ATTACHMENT5_EXT);
	/// @field GL_COLOR_ATTACHMENT6_EXT 0x8CE6
	PUSH_GL(COLOR_ATTACHMENT6_EXT);
	/// @field GL_COLOR_ATTACHMENT7_EXT 0x8CE7
	PUSH_GL(COLOR_ATTACHMENT7_EXT);
	/// @field GL_COLOR_ATTACHMENT8_EXT 0x8CE8
	PUSH_GL(COLOR_ATTACHMENT8_EXT);
	/// @field GL_COLOR_ATTACHMENT9_EXT 0x8CE9
	PUSH_GL(COLOR_ATTACHMENT9_EXT);
	/// @field GL_COLOR_ATTACHMENT10_EXT 0x8CEA
	PUSH_GL(COLOR_ATTACHMENT10_EXT);
	/// @field GL_COLOR_ATTACHMENT11_EXT 0x8CEB
	PUSH_GL(COLOR_ATTACHMENT11_EXT);
	/// @field GL_COLOR_ATTACHMENT12_EXT 0x8CEC
	PUSH_GL(COLOR_ATTACHMENT12_EXT);
	/// @field GL_COLOR_ATTACHMENT13_EXT 0x8CED
	PUSH_GL(COLOR_ATTACHMENT13_EXT);
	/// @field GL_COLOR_ATTACHMENT14_EXT 0x8CEE
	PUSH_GL(COLOR_ATTACHMENT14_EXT);
	/// @field GL_COLOR_ATTACHMENT15_EXT 0x8CEF
	PUSH_GL(COLOR_ATTACHMENT15_EXT);

	/// @field GL_DEPTH_ATTACHMENT_EXT 0x8D00
	PUSH_GL(DEPTH_ATTACHMENT_EXT);
	/// @field GL_STENCIL_ATTACHMENT_EXT 0x8D20
	PUSH_GL(STENCIL_ATTACHMENT_EXT);

	return true;
}


/******************************************************************************
 * Not included, but useful texture Formats
 * @section textureformats
******************************************************************************/

/// @field GL_RGBA16F_ARB 0x881A

/// @field GL_RGBA32F_ARB 0x8814

/// @field GL_DEPTH_COMPONENT 0x1902

/******************************************************************************
 * Not included, but useful RBO Formats
 * @section rboformats
******************************************************************************/

/// @field GL_RGB 0x1907

/// @field GL_RGBA 0x1908

/// @field GL_DEPTH_COMPONENT 0x1902

/// @field GL_STENCIL_INDEX 0x1901

/******************************************************************************
 * Not included, but useful FBO Targets
 * @section fbotargets
******************************************************************************/

/// @field GL_FRAMEBUFFER_EXT 0x8D40

/// @field GL_READ_FRAMEBUFFER_EXT 0x8CA8

/// @field GL_DRAW_FRAMEBUFFER_EXT 0x8CA9

/******************************************************************************
 * Not included, but useful FBO Status
 * @section fbostatus
******************************************************************************/

/// @field GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5

/// @field GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6

/// @field GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7

/// @field GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8

/// @field GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9

/// @field GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA

/// @field GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB

/// @field GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC

/// @field GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD

/// @field GL_FRAMEBUFFER_STATUS_ERROR_EXT 0x8CDE
