/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstGL.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Rendering/GL/myGL.h"

/***
 * OpenGL Constants
 * @enum GL
 */

bool LuaConstGL::PushEntries(lua_State* L)
{
/*** @field GL.cmd integer */
#define PUSH_GL(cmd) LuaPushNamedNumber(L, #cmd, GL_##cmd)

	/***
	 * Drawing Primitives
	 * @section primitives
	 */

	/*** @field GL.POINTS integer */
	PUSH_GL(POINTS);
	/*** @field GL.LINES integer */
	PUSH_GL(LINES);
	/*** @field GL.LINE_LOOP integer */
	PUSH_GL(LINE_LOOP);
	/*** @field GL.LINE_STRIP integer */
	PUSH_GL(LINE_STRIP);
	/*** @field GL.TRIANGLES integer */
	PUSH_GL(TRIANGLES);
	/*** @field GL.TRIANGLE_STRIP integer */
	PUSH_GL(TRIANGLE_STRIP);
	/*** @field GL.TRIANGLE_FAN integer */
	PUSH_GL(TRIANGLE_FAN);
	/*** @field GL.QUADS integer */
	PUSH_GL(QUADS);
	/*** @field GL.QUAD_STRIP integer */
	PUSH_GL(QUAD_STRIP);
	/*** @field GL.POLYGON integer */
	PUSH_GL(POLYGON);

	/*** @field GL.LINE_STRIP_ADJACENCY integer */
	PUSH_GL(LINE_STRIP_ADJACENCY);
	/*** @field GL.LINES_ADJACENCY integer */
	PUSH_GL(LINES_ADJACENCY);
	/*** @field GL.TRIANGLE_STRIP_ADJACENCY integer */
	PUSH_GL(TRIANGLE_STRIP_ADJACENCY);
	/*** @field GL.TRIANGLES_ADJACENCY integer */
	PUSH_GL(TRIANGLES_ADJACENCY);
	/*** @field GL.PATCHES integer */
	PUSH_GL(PATCHES);

	/***
	 * BlendingFactorDest
	 * @section blendingfactordest
	 */

	/*** @field GL.ZERO integer */
	PUSH_GL(ZERO);
	/*** @field GL.ONE integer */
	PUSH_GL(ONE);
	/*** @field GL.SRC_COLOR integer */
	PUSH_GL(SRC_COLOR);
	/*** @field GL.ONE_MINUS_SRC_COLOR integer */
	PUSH_GL(ONE_MINUS_SRC_COLOR);
	/*** @field GL.SRC_ALPHA integer */
	PUSH_GL(SRC_ALPHA);
	/*** @field GL.ONE_MINUS_SRC_ALPHA integer */
	PUSH_GL(ONE_MINUS_SRC_ALPHA);
	/*** @field GL.DST_ALPHA integer */
	PUSH_GL(DST_ALPHA);
	/*** @field GL.ONE_MINUS_DST_ALPHA integer */
	PUSH_GL(ONE_MINUS_DST_ALPHA);

	/***
	 * BlendingFactorSrc
	 * @section blendingfactorsrc
	 */

	/*** @field GL.DST_COLOR integer */
	PUSH_GL(DST_COLOR);
	/*** @field GL.ONE_MINUS_DST_COLOR integer */
	PUSH_GL(ONE_MINUS_DST_COLOR);
	/*** @field GL.SRC_ALPHA_SATURATE integer */
	PUSH_GL(SRC_ALPHA_SATURATE);

	/*** @field GL.FUNC_ADD integer */
	PUSH_GL(FUNC_ADD);
	/*** @field GL.FUNC_SUBTRACT integer */
	PUSH_GL(FUNC_SUBTRACT);
	/*** @field GL.FUNC_REVERSE_SUBTRACT integer */
	PUSH_GL(FUNC_REVERSE_SUBTRACT);
	/*** @field GL.MIN integer */
	PUSH_GL(MIN);
	/*** @field GL.MAX integer */
	PUSH_GL(MAX);

	/***
	 * AlphaFunction and DepthFunction
	 * @section alphadepth
	 */

	/*** @field GL.NEVER integer */
	PUSH_GL(NEVER);
	/*** @field GL.LESS integer */
	PUSH_GL(LESS);
	/*** @field GL.EQUAL integer */
	PUSH_GL(EQUAL);
	/*** @field GL.LEQUAL integer */
	PUSH_GL(LEQUAL);
	/*** @field GL.GREATER integer */
	PUSH_GL(GREATER);
	/*** @field GL.NOTEQUAL integer */
	PUSH_GL(NOTEQUAL);
	/*** @field GL.GEQUAL integer */
	PUSH_GL(GEQUAL);
	/*** @field GL.ALWAYS integer */
	PUSH_GL(ALWAYS);

	/***
	 * LogicOp
	 * @section logicop
	 */

	/*** @field GL.CLEAR integer */
	PUSH_GL(CLEAR);
	/*** @field GL.AND integer */
	PUSH_GL(AND);
	/*** @field GL.AND_REVERSE integer */
	PUSH_GL(AND_REVERSE);
	/*** @field GL.COPY integer */
	PUSH_GL(COPY);
	/*** @field GL.AND_INVERTED integer */
	PUSH_GL(AND_INVERTED);
	/*** @field GL.NOOP integer */
	PUSH_GL(NOOP);
	/*** @field GL.XOR integer */
	PUSH_GL(XOR);
	/*** @field GL.OR integer */
	PUSH_GL(OR);
	/*** @field GL.NOR integer */
	PUSH_GL(NOR);
	/*** @field GL.EQUIV integer */
	PUSH_GL(EQUIV);
	/*** @field GL.INVERT integer */
	PUSH_GL(INVERT);
	/*** @field GL.OR_REVERSE integer */
	PUSH_GL(OR_REVERSE);
	/*** @field GL.COPY_INVERTED integer */
	PUSH_GL(COPY_INVERTED);
	/*** @field GL.OR_INVERTED integer */
	PUSH_GL(OR_INVERTED);
	/*** @field GL.NAND integer */
	PUSH_GL(NAND);
	/*** @field GL.SET integer */
	PUSH_GL(SET);

	/***
	 * Culling
	 * @section culling
	 */

	/*** @field GL.BACK integer */
	PUSH_GL(BACK);
	/*** @field GL.FRONT integer */
	PUSH_GL(FRONT);
	/*** @field GL.FRONT_AND_BACK integer */
	PUSH_GL(FRONT_AND_BACK);

	/***
	 * PolygonMode
	 * @section polygonmode
	 */

	/*** @field GL.POINT integer */
	PUSH_GL(POINT);
	/*** @field GL.LINE integer */
	PUSH_GL(LINE);
	/*** @field GL.FILL integer */
	PUSH_GL(FILL);

	/***
	 * ShadeModel
	 * @section shademodel
	 */

	/*** @field GL.FLAT integer */
	PUSH_GL(FLAT);
	/*** @field GL.SMOOTH integer */
	PUSH_GL(SMOOTH);

	/***
	 * MatrixMode
	 * @section matrixmode
	 */

	/*** @field GL.MODELVIEW integer */
	PUSH_GL(MODELVIEW);
	/*** @field GL.PROJECTION integer */
	PUSH_GL(PROJECTION);
	/*** @field GL.TEXTURE integer */
	PUSH_GL(TEXTURE);

	/***
	 * Texture Filtering
	 * @section texturefiltering
	 */

	/*** @field GL.NEAREST integer */
	PUSH_GL(NEAREST);
	/*** @field GL.LINEAR integer */
	PUSH_GL(LINEAR);
	/*** @field GL.NEAREST_MIPMAP_NEAREST integer */
	PUSH_GL(NEAREST_MIPMAP_NEAREST);
	/*** @field GL.LINEAR_MIPMAP_NEAREST integer */
	PUSH_GL(LINEAR_MIPMAP_NEAREST);
	/*** @field GL.NEAREST_MIPMAP_LINEAR integer */
	PUSH_GL(NEAREST_MIPMAP_LINEAR);
	/*** @field GL.LINEAR_MIPMAP_LINEAR integer */
	PUSH_GL(LINEAR_MIPMAP_LINEAR);

	/***
	 * Texture Clamping
	 * @section textureclamping
	 */

	/*** @field GL.REPEAT integer */
	PUSH_GL(REPEAT);
	/*** @field GL.MIRRORED_REPEAT integer */
	PUSH_GL(MIRRORED_REPEAT);
	/*** @field GL.CLAMP integer */
	PUSH_GL(CLAMP);
	/*** @field GL.CLAMP_TO_EDGE integer */
	PUSH_GL(CLAMP_TO_EDGE);
	/*** @field GL.CLAMP_TO_BORDER integer */
	PUSH_GL(CLAMP_TO_BORDER);

	/***
	 * Texture Environment
	 * @section textureenvironment
	 */

	/*** @field GL.TEXTURE_ENV integer */
	PUSH_GL(TEXTURE_ENV);
	/*** @field GL.TEXTURE_ENV_MODE integer */
	PUSH_GL(TEXTURE_ENV_MODE);
	/*** @field GL.TEXTURE_ENV_COLOR integer */
	PUSH_GL(TEXTURE_ENV_COLOR);
	/*** @field GL.MODULATE integer */
	PUSH_GL(MODULATE);
	/*** @field GL.DECAL integer */
	PUSH_GL(DECAL);
	/*** @field GL.BLEND integer */
	PUSH_GL(BLEND);
	/*** @field GL.REPLACE integer */
	PUSH_GL(REPLACE);

	/*** @field GL.TEXTURE_FILTER_CONTROL integer */
	PUSH_GL(TEXTURE_FILTER_CONTROL);
	/*** @field GL.TEXTURE_LOD_BIAS integer */
	PUSH_GL(TEXTURE_LOD_BIAS);

	/***
	 * Texture Generation
	 * @section texturegeneration
	 */

	/*** @field GL.TEXTURE_GEN_MODE integer */
	PUSH_GL(TEXTURE_GEN_MODE);
	/*** @field GL.EYE_PLANE integer */
	PUSH_GL(EYE_PLANE);
	/*** @field GL.OBJECT_PLANE integer */
	PUSH_GL(OBJECT_PLANE);
	/*** @field GL.EYE_LINEAR integer */
	PUSH_GL(EYE_LINEAR);
	/*** @field GL.OBJECT_LINEAR integer */
	PUSH_GL(OBJECT_LINEAR);
	/*** @field GL.SPHERE_MAP integer */
	PUSH_GL(SPHERE_MAP);
	/*** @field GL.NORMAL_MAP integer */
	PUSH_GL(NORMAL_MAP);
	/*** @field GL.REFLECTION_MAP integer */
	PUSH_GL(REFLECTION_MAP);
	/*** @field GL.S integer */
	PUSH_GL(S);
	/*** @field GL.T integer */
	PUSH_GL(T);
	/*** @field GL.R integer */
	PUSH_GL(R);
	/*** @field GL.Q integer */
	PUSH_GL(Q);

	/***
	 * glPushAttrib() bits
	 * @section glpushattribbits
	 */

	/*** @field GL.CURRENT_BIT integer */
	PUSH_GL(CURRENT_BIT);
	/*** @field GL.POINT_BIT integer */
	PUSH_GL(POINT_BIT);
	/*** @field GL.LINE_BIT integer */
	PUSH_GL(LINE_BIT);
	/*** @field GL.POLYGON_BIT integer */
	PUSH_GL(POLYGON_BIT);
	/*** @field GL.POLYGON_STIPPLE_BIT integer */
	PUSH_GL(POLYGON_STIPPLE_BIT);
	/*** @field GL.PIXEL_MODE_BIT integer */
	PUSH_GL(PIXEL_MODE_BIT);
	/*** @field GL.LIGHTING_BIT integer */
	PUSH_GL(LIGHTING_BIT);
	/*** @field GL.FOG_BIT integer */
	PUSH_GL(FOG_BIT);
	/*** @field GL.DEPTH_BUFFER_BIT integer */
	PUSH_GL(DEPTH_BUFFER_BIT);
	/*** @field GL.ACCUM_BUFFER_BIT integer */
	PUSH_GL(ACCUM_BUFFER_BIT);
	/*** @field GL.STENCIL_BUFFER_BIT integer */
	PUSH_GL(STENCIL_BUFFER_BIT);
	/*** @field GL.VIEWPORT_BIT integer */
	PUSH_GL(VIEWPORT_BIT);
	/*** @field GL.TRANSFORM_BIT integer */
	PUSH_GL(TRANSFORM_BIT);
	/*** @field GL.ENABLE_BIT integer */
	PUSH_GL(ENABLE_BIT);
	/*** @field GL.COLOR_BUFFER_BIT integer */
	PUSH_GL(COLOR_BUFFER_BIT);
	/*** @field GL.HINT_BIT integer */
	PUSH_GL(HINT_BIT);
	/*** @field GL.EVAL_BIT integer */
	PUSH_GL(EVAL_BIT);
	/*** @field GL.LIST_BIT integer */
	PUSH_GL(LIST_BIT);
	/*** @field GL.TEXTURE_BIT integer */
	PUSH_GL(TEXTURE_BIT);
	/*** @field GL.SCISSOR_BIT integer */
	PUSH_GL(SCISSOR_BIT);
	/*** @field GL.ALL_ATTRIB_BITS integer */
	// PUSH_GL(ALL_ATTRIB_BITS);  // floating point clip
	LuaPushNamedNumber(L, "ALL_ATTRIB_BITS", -1.0f);

	/***
	 * glHint() targets
	 * @section glhinttargets
	 */

	/*** @field GL.FOG_HINT integer */
	PUSH_GL(FOG_HINT);
	/*** @field GL.PERSPECTIVE_CORRECTION_HINT integer */
	PUSH_GL(PERSPECTIVE_CORRECTION_HINT);

	/***
	 * glHint() modes
	 * @section glhintmodes
	 */

	/*** @field GL.DONT_CARE integer */
	PUSH_GL(DONT_CARE);
	/*** @field GL.FASTEST integer */
	PUSH_GL(FASTEST);
	/*** @field GL.NICEST integer */
	PUSH_GL(NICEST);

	/***
	 * Light Specification
	 * @section lightspecification
	 */

	/*** @field GL.AMBIENT integer */
	PUSH_GL(AMBIENT);
	/*** @field GL.DIFFUSE integer */
	PUSH_GL(DIFFUSE);
	/*** @field GL.SPECULAR integer */
	PUSH_GL(SPECULAR);
	/*** @field GL.POSITION integer */
	PUSH_GL(POSITION);
	/*** @field GL.SPOT_DIRECTION integer */
	PUSH_GL(SPOT_DIRECTION);
	/*** @field GL.SPOT_EXPONENT integer */
	PUSH_GL(SPOT_EXPONENT);
	/*** @field GL.SPOT_CUTOFF integer */
	PUSH_GL(SPOT_CUTOFF);
	/*** @field GL.CONSTANT_ATTENUATION integer */
	PUSH_GL(CONSTANT_ATTENUATION);
	/*** @field GL.LINEAR_ATTENUATION integer */
	PUSH_GL(LINEAR_ATTENUATION);
	/*** @field GL.QUADRATIC_ATTENUATION integer */
	PUSH_GL(QUADRATIC_ATTENUATION);

	/***
	 * Shader Types
	 * @section shadertypes
	 */

	/*** @field GL.VERTEX_SHADER integer */
	PUSH_GL(VERTEX_SHADER);
	/*** @field GL.TESS_CONTROL_SHADER integer */
	PUSH_GL(TESS_CONTROL_SHADER);
	/*** @field GL.TESS_EVALUATION_SHADER integer */
	PUSH_GL(TESS_EVALUATION_SHADER);
	/*** @field GL.GEOMETRY_SHADER_EXT integer */
	PUSH_GL(GEOMETRY_SHADER_EXT);
	/*** @field GL.FRAGMENT_SHADER integer */
	PUSH_GL(FRAGMENT_SHADER);

	/***
	 * Geometry Shader Parameters
	 * @section geometryshaderparameters
	 */

	/*** @field GL.GEOMETRY_INPUT_TYPE_EXT integer */
	PUSH_GL(GEOMETRY_INPUT_TYPE_EXT);
	/*** @field GL.GEOMETRY_OUTPUT_TYPE_EXT integer */
	PUSH_GL(GEOMETRY_OUTPUT_TYPE_EXT);
	/*** @field GL.GEOMETRY_VERTICES_OUT_EXT integer */
	PUSH_GL(GEOMETRY_VERTICES_OUT_EXT);

	/***
	 * Tesselation control shader parameters
	 * @section tesselationcontrolshaderparameters
	 */

	/*** @field GL.PATCH_VERTICES integer */
	PUSH_GL(PATCH_VERTICES);
	/*** @field GL.PATCH_DEFAULT_OUTER_LEVEL integer */
	PUSH_GL(PATCH_DEFAULT_OUTER_LEVEL);
	/*** @field GL.PATCH_DEFAULT_INNER_LEVEL integer */
	PUSH_GL(PATCH_DEFAULT_INNER_LEVEL);

	/***
	 * OpenGL Data Types
	 * @section OpenGL_Data_Types
	 */

	/*** @field GL.BYTE integer */
	PUSH_GL(BYTE);
	/*** @field GL.UNSIGNED_BYTE integer */
	PUSH_GL(UNSIGNED_BYTE);
	/*** @field GL.SHORT integer */
	PUSH_GL(SHORT);
	/*** @field GL.UNSIGNED_SHORT integer */
	PUSH_GL(UNSIGNED_SHORT);
	/*** @field GL.INT integer */
	PUSH_GL(INT);
	/*** @field GL.UNSIGNED_INT integer */
	PUSH_GL(UNSIGNED_INT);
	/*** @field GL.FLOAT integer */
	PUSH_GL(FLOAT);
	/*** @field GL.FLOAT_VEC4 integer */
	PUSH_GL(FLOAT_VEC4);
	/*** @field GL.INT_VEC4 integer */
	PUSH_GL(INT_VEC4);
	/*** @field GL.UNSIGNED_INT_VEC4 integer */
	PUSH_GL(UNSIGNED_INT_VEC4);
	/*** @field GL.FLOAT_MAT4 integer */
	PUSH_GL(FLOAT_MAT4);

	/***
	 * OpenGL Buffer Types
	 * @section OpenGL_Buffer_Types
	 */

	/*** @field GL.ELEMENT_ARRAY_BUFFER integer */
	PUSH_GL(ELEMENT_ARRAY_BUFFER);
	/*** @field GL.ARRAY_BUFFER integer */
	PUSH_GL(ARRAY_BUFFER);
	/*** @field GL.UNIFORM_BUFFER integer */
	PUSH_GL(UNIFORM_BUFFER);
	/*** @field GL.SHADER_STORAGE_BUFFER integer */
	PUSH_GL(SHADER_STORAGE_BUFFER);

	// Texture targets
	/*** @field GL.TEXTURE_1D integer */
	PUSH_GL(TEXTURE_1D);
	/*** @field GL.TEXTURE_2D integer */
	PUSH_GL(TEXTURE_2D);
	/*** @field GL.TEXTURE_3D integer */
	PUSH_GL(TEXTURE_3D);
	/*** @field GL.TEXTURE_CUBE_MAP integer */
	PUSH_GL(TEXTURE_CUBE_MAP);
	/*** @field GL.TEXTURE_2D_MULTISAMPLE integer */
	PUSH_GL(TEXTURE_2D_MULTISAMPLE);

	// Image formats
	/*** @field GL.RGBA32F integer */
	PUSH_GL(RGBA32F);
	/*** @field GL.RGBA16F integer */
	PUSH_GL(RGBA16F);
	/*** @field GL.RG32F integer */
	PUSH_GL(RG32F);
	/*** @field GL.RG16F integer */
	PUSH_GL(RG16F);
	/*** @field GL.R11F_G11F_B10F integer */
	PUSH_GL(R11F_G11F_B10F);
	/*** @field GL.R32F integer */
	PUSH_GL(R32F);
	/*** @field GL.R16F integer */
	PUSH_GL(R16F);
	/*** @field GL.RGBA32UI integer */
	PUSH_GL(RGBA32UI);
	/*** @field GL.RGBA16UI integer */
	PUSH_GL(RGBA16UI);
	/*** @field GL.RGB10_A2UI integer */
	PUSH_GL(RGB10_A2UI);
	/*** @field GL.RGBA8UI integer */
	PUSH_GL(RGBA8UI);
	/*** @field GL.RG32UI integer */
	PUSH_GL(RG32UI);
	/*** @field GL.RG16UI integer */
	PUSH_GL(RG16UI);
	/*** @field GL.RG8UI integer */
	PUSH_GL(RG8UI);
	/*** @field GL.R32UI integer */
	PUSH_GL(R32UI);
	/*** @field GL.R16UI integer */
	PUSH_GL(R16UI);
	/*** @field GL.R8UI integer */
	PUSH_GL(R8UI);
	/*** @field GL.RGBA32I integer */
	PUSH_GL(RGBA32I);
	/*** @field GL.RGBA16I integer */
	PUSH_GL(RGBA16I);
	/*** @field GL.RGBA8I integer */
	PUSH_GL(RGBA8I);
	/*** @field GL.RG32I integer */
	PUSH_GL(RG32I);
	/*** @field GL.RG16I integer */
	PUSH_GL(RG16I);
	/*** @field GL.RG8I integer */
	PUSH_GL(RG8I);
	/*** @field GL.R32I integer */
	PUSH_GL(R32I);
	/*** @field GL.R16I integer */
	PUSH_GL(R16I);
	/*** @field GL.R8I integer */
	PUSH_GL(R8I);
	/*** @field GL.RGBA16 integer */
	PUSH_GL(RGBA16);
	/*** @field GL.RGB10_A2 integer */
	PUSH_GL(RGB10_A2);
	/*** @field GL.RGBA8 integer */
	PUSH_GL(RGBA8);
	/*** @field GL.RG16 integer */
	PUSH_GL(RG16);
	/*** @field GL.RG8 integer */
	PUSH_GL(RG8);
	/*** @field GL.R16 integer */
	PUSH_GL(R16);
	/*** @field GL.R8 integer */
	PUSH_GL(R8);
	/*** @field GL.RGBA16_SNORM integer */
	PUSH_GL(RGBA16_SNORM);
	/*** @field GL.RGBA8_SNORM integer */
	PUSH_GL(RGBA8_SNORM);
	/*** @field GL.RG16_SNORM integer */
	PUSH_GL(RG16_SNORM);
	/*** @field GL.RG8_SNORM integer */
	PUSH_GL(RG8_SNORM);
	/*** @field GL.R16_SNORM integer */
	PUSH_GL(R16_SNORM);
	/*** @field GL.R8_SNORM integer */
	PUSH_GL(R8_SNORM);
	/*** @field GL.DEPTH_COMPONENT16 integer */
	PUSH_GL(DEPTH_COMPONENT16);
	/*** @field GL.DEPTH_COMPONENT24 integer */
	PUSH_GL(DEPTH_COMPONENT24);
	/*** @field GL.DEPTH_COMPONENT32 integer */
	PUSH_GL(DEPTH_COMPONENT32);
	/*** @field GL.DEPTH_COMPONENT32F integer */
	PUSH_GL(DEPTH_COMPONENT32F);

	// access specifiers

	/*** @field GL.READ_ONLY integer */
	PUSH_GL(READ_ONLY);
	/*** @field GL.WRITE_ONLY integer */
	PUSH_GL(WRITE_ONLY);
	/*** @field GL.READ_WRITE integer */
	PUSH_GL(READ_WRITE);

	// memory barrier bits

	/*** @field GL.VERTEX_ATTRIB_ARRAY_BARRIER_BIT integer */
	PUSH_GL(VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
	/*** @field GL.ELEMENT_ARRAY_BARRIER_BIT integer */
	PUSH_GL(ELEMENT_ARRAY_BARRIER_BIT);
	/*** @field GL.UNIFORM_BARRIER_BIT integer */
	PUSH_GL(UNIFORM_BARRIER_BIT);
	/*** @field GL.TEXTURE_FETCH_BARRIER_BIT integer */
	PUSH_GL(TEXTURE_FETCH_BARRIER_BIT);
	/*** @field GL.SHADER_IMAGE_ACCESS_BARRIER_BIT integer */
	PUSH_GL(SHADER_IMAGE_ACCESS_BARRIER_BIT);
	/*** @field GL.COMMAND_BARRIER_BIT integer */
	PUSH_GL(COMMAND_BARRIER_BIT);
	/*** @field GL.PIXEL_BUFFER_BARRIER_BIT integer */
	PUSH_GL(PIXEL_BUFFER_BARRIER_BIT);
	/*** @field GL.TEXTURE_UPDATE_BARRIER_BIT integer */
	PUSH_GL(TEXTURE_UPDATE_BARRIER_BIT);
	/*** @field GL.BUFFER_UPDATE_BARRIER_BIT integer */
	PUSH_GL(BUFFER_UPDATE_BARRIER_BIT);
	/*** @field GL.FRAMEBUFFER_BARRIER_BIT integer */
	PUSH_GL(FRAMEBUFFER_BARRIER_BIT);
	/*** @field GL.TRANSFORM_FEEDBACK_BARRIER_BIT integer */
	PUSH_GL(TRANSFORM_FEEDBACK_BARRIER_BIT);
	/*** @field GL.ATOMIC_COUNTER_BARRIER_BIT integer */
	PUSH_GL(ATOMIC_COUNTER_BARRIER_BIT);
	/*** @field GL.SHADER_STORAGE_BARRIER_BIT integer */
	PUSH_GL(SHADER_STORAGE_BARRIER_BIT);
	/*** @field GL.ALL_BARRIER_BITS integer */
	PUSH_GL(ALL_BARRIER_BITS);

	/*** @field GL.COLOR_ATTACHMENT0 integer */
	PUSH_GL(COLOR_ATTACHMENT0);
	/*** @field GL.COLOR_ATTACHMENT1 integer */
	PUSH_GL(COLOR_ATTACHMENT1);
	/*** @field GL.COLOR_ATTACHMENT2 integer */
	PUSH_GL(COLOR_ATTACHMENT2);
	/*** @field GL.COLOR_ATTACHMENT3 integer */
	PUSH_GL(COLOR_ATTACHMENT3);
	/*** @field GL.COLOR_ATTACHMENT4 integer */
	PUSH_GL(COLOR_ATTACHMENT4);
	/*** @field GL.COLOR_ATTACHMENT5 integer */
	PUSH_GL(COLOR_ATTACHMENT5);
	/*** @field GL.COLOR_ATTACHMENT6 integer */
	PUSH_GL(COLOR_ATTACHMENT6);
	/*** @field GL.COLOR_ATTACHMENT7 integer */
	PUSH_GL(COLOR_ATTACHMENT7);
	/*** @field GL.COLOR_ATTACHMENT8 integer */
	PUSH_GL(COLOR_ATTACHMENT8);
	/*** @field GL.COLOR_ATTACHMENT9 integer */
	PUSH_GL(COLOR_ATTACHMENT9);
	/*** @field GL.COLOR_ATTACHMENT10 integer */
	PUSH_GL(COLOR_ATTACHMENT10);
	/*** @field GL.COLOR_ATTACHMENT11 integer */
	PUSH_GL(COLOR_ATTACHMENT11);
	/*** @field GL.COLOR_ATTACHMENT12 integer */
	PUSH_GL(COLOR_ATTACHMENT12);
	/*** @field GL.COLOR_ATTACHMENT13 integer */
	PUSH_GL(COLOR_ATTACHMENT13);
	/*** @field GL.COLOR_ATTACHMENT14 integer */
	PUSH_GL(COLOR_ATTACHMENT14);
	/*** @field GL.COLOR_ATTACHMENT15 integer */
	PUSH_GL(COLOR_ATTACHMENT15);
	/*** @field GL.DEPTH_ATTACHMENT integer */
	PUSH_GL(DEPTH_ATTACHMENT);
	/*** @field GL.STENCIL_ATTACHMENT integer */
	PUSH_GL(STENCIL_ATTACHMENT);


	/******************************************************************************
	 * FBO Attachments
	 * @section fboattachments
	 ******************************************************************************/

	/*** @field GL.COLOR_ATTACHMENT0_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT0_EXT);
	/*** @field GL.COLOR_ATTACHMENT1_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT1_EXT);
	/*** @field GL.COLOR_ATTACHMENT2_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT2_EXT);
	/*** @field GL.COLOR_ATTACHMENT3_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT3_EXT);
	/*** @field GL.COLOR_ATTACHMENT4_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT4_EXT);
	/*** @field GL.COLOR_ATTACHMENT5_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT5_EXT);
	/*** @field GL.COLOR_ATTACHMENT6_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT6_EXT);
	/*** @field GL.COLOR_ATTACHMENT7_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT7_EXT);
	/*** @field GL.COLOR_ATTACHMENT8_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT8_EXT);
	/*** @field GL.COLOR_ATTACHMENT9_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT9_EXT);
	/*** @field GL.COLOR_ATTACHMENT10_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT10_EXT);
	/*** @field GL.COLOR_ATTACHMENT11_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT11_EXT);
	/*** @field GL.COLOR_ATTACHMENT12_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT12_EXT);
	/*** @field GL.COLOR_ATTACHMENT13_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT13_EXT);
	/*** @field GL.COLOR_ATTACHMENT14_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT14_EXT);
	/*** @field GL.COLOR_ATTACHMENT15_EXT integer */
	PUSH_GL(COLOR_ATTACHMENT15_EXT);

	/*** @field GL.DEPTH_ATTACHMENT_EXT integer */
	PUSH_GL(DEPTH_ATTACHMENT_EXT);
	/*** @field GL.STENCIL_ATTACHMENT_EXT integer */
	PUSH_GL(STENCIL_ATTACHMENT_EXT);

	return true;

	/******************************************************************************
	 * OpenGL Object Types
	 * @section objecttypes
	 ******************************************************************************/

	/*** @field GL.BUFFER integer */
	PUSH_GL(BUFFER);
	/*** @field GL.SHADER integer */
	PUSH_GL(SHADER);
	/*** @field GL.PROGRAM integer */
	PUSH_GL(PROGRAM);
	/*** @field GL.VERTEX_ARRAY integer */
	PUSH_GL(VERTEX_ARRAY);
	/*** @field GL.QUERY integer */
	PUSH_GL(QUERY);
	/*** @field GL.PROGRAM_PIPELINE integer */
	PUSH_GL(PROGRAM_PIPELINE);
	/*** @field GL.TRANSFORM_FEEDBACK integer */
	PUSH_GL(TRANSFORM_FEEDBACK);
	/*** @field GL.RENDERBUFFER integer */
	PUSH_GL(RENDERBUFFER);
	/*** @field GL.FRAMEBUFFER integer */
	PUSH_GL(FRAMEBUFFER);

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
