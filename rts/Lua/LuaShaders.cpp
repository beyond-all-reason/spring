/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaShaders.h"

#include "lib/sol2/sol.hpp"

#include "LuaInclude.h"
#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"
#include "LuaOpenGLUtils.h"
#include "LuaUtils.h"

#include "Game/Camera.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/TypeToStr.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Models/ModelsMemStorageDefs.h"
#include "Rendering/UniformConstants.h"

#include <string>
#include <vector>
#include <algorithm>

int   intUniformArrayBuf[1024] = {0   };
float fltUniformArrayBuf[1024] = {0.0f};


/******************************************************************************
 * Shader creation and management
 *
 * These functions are ONLY available if the graphics adapter supports GLSL.
 * Please test in your scripts if one of them exists before you use them. In headless mode, the gl. callouts are nil.
 *
 * @see rts/Lua/LuaShaders.cpp
******************************************************************************/

bool LuaShaders::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(CreateShader);
	REGISTER_LUA_CFUNC(DeleteShader);
	REGISTER_LUA_CFUNC(UseShader);
	REGISTER_LUA_CFUNC(ActiveShader);

	REGISTER_LUA_CFUNC(GetActiveUniforms);
	REGISTER_LUA_CFUNC(GetUniformLocation);
	REGISTER_LUA_CFUNC(GetSubroutineIndex);
	REGISTER_LUA_CFUNC(Uniform);
	REGISTER_NAMED_LUA_CFUNC("UniformFloat", Uniform);
	REGISTER_LUA_CFUNC(UniformInt);
	REGISTER_LUA_CFUNC(UniformArray);
	REGISTER_LUA_CFUNC(UniformMatrix);
	REGISTER_LUA_CFUNC(UniformSubroutine);

	REGISTER_LUA_CFUNC(GetEngineUniformBufferDef);
	REGISTER_LUA_CFUNC(GetEngineModelUniformDataDef);

	REGISTER_LUA_CFUNC(SetUnitBufferUniforms);
	REGISTER_LUA_CFUNC(SetFeatureBufferUniforms);

	REGISTER_LUA_CFUNC(SetGeometryShaderParameter);
	REGISTER_LUA_CFUNC(SetTesselationShaderParameter);

	REGISTER_LUA_CFUNC(GetShaderLog);

	return true;
}


/******************************************************************************/
/******************************************************************************/

LuaShaders::LuaShaders()
{
	programs.emplace_back(0);
}


LuaShaders::~LuaShaders()
{
	for (auto& program: programs) {
		DeleteProgram(program);
	}

	programs.clear();
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (LuaOpenGL::IsDrawingEnabled(L))
		return;

	luaL_error(L, "%s(): OpenGL calls can only be used in Draw() " "call-ins, or while creating display lists", caller);
}


/******************************************************************************/
/******************************************************************************/

GLuint LuaShaders::GetProgramName(uint32_t progIdx) const
{
	if (progIdx < programs.size())
		return programs[progIdx].id;

	return 0;
}

const LuaShaders::Program* LuaShaders::GetProgram(uint32_t progIdx) const
{
	if (progIdx < programs.size() && progIdx > 0)
		return &programs[progIdx];

	return nullptr;
}

LuaShaders::Program* LuaShaders::GetProgram(uint32_t progIdx)
{
	if (progIdx < programs.size() && progIdx > 0)
		return &programs[progIdx];

	return nullptr;
}


GLuint LuaShaders::GetProgramName(lua_State* L, int index) const
{
	if (luaL_checkint(L, index) <= 0)
		return 0;

	return (GetProgramName(luaL_checkint(L, index)));
}

const LuaShaders::Program* LuaShaders::GetProgram(lua_State* L, int index) const
{
	return (GetProgram(luaL_checkint(L, index)));
}
LuaShaders::Program* LuaShaders::GetProgram(lua_State* L, int index)
{
	return (GetProgram(luaL_checkint(L, index)));
}


uint32_t LuaShaders::AddProgram(const Program& p)
{
	if (!unused.empty()) {
		const uint32_t index = unused.back();
		programs[index] = p;
		unused.pop_back();
		return index;
	}

	programs.push_back(p);
	return (programs.size() - 1);
}


bool LuaShaders::RemoveProgram(uint32_t progIdx)
{
	if (progIdx >= programs.size())
		return false;
	if (!DeleteProgram(programs[progIdx]))
		return false;

	unused.push_back(progIdx);
	return true;
}


bool LuaShaders::DeleteProgram(Program& p)
{
	if (p.id == 0)
		return false;

	for (uint32_t o = 0; o < p.objects.size(); o++) {
		Object& obj = p.objects[o];
		glDetachShader(p.id, obj.id);
		glDeleteShader(obj.id);
	}

	glDeleteProgram(p.id);

	p.objects.clear();
	p.id = 0;
	return true;
}


/******************************************************************************/
/******************************************************************************/

/*** Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.
 *
 * @function gl.GetShaderLog
 * @return string infoLog
 */
int LuaShaders::GetShaderLog(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	lua_pushsstring(L, shaders.errorLog);
	return 1;
}


/******************************************************************************/
/******************************************************************************/


namespace {
	enum {
		UNIFORM_TYPE_MIXED = 0, // includes arrays; float or int
		UNIFORM_TYPE_INT = 1, // includes arrays
		UNIFORM_TYPE_FLOAT = 2, // includes arrays
		UNIFORM_TYPE_FLOAT_MATRIX = 3,
	};

	static void ParseUniformType(lua_State* L, int loc, int type)
	{
		switch (type) {
		case UNIFORM_TYPE_FLOAT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1f(loc, lua_tofloat(L, -1));
				return;
			}
			if (lua_istable(L, -1)) {
				const float* array = fltUniformArrayBuf;
				const int count = LuaUtils::ParseFloatArray(L, -1, fltUniformArrayBuf, sizeof(fltUniformArrayBuf) / sizeof(float));

				switch (count) {
				case 1: { glUniform1f(loc, array[0]); break; }
				case 2: { glUniform2f(loc, array[0], array[1]); break; }
				case 3: { glUniform3f(loc, array[0], array[1], array[2]); break; }
				case 4: { glUniform4f(loc, array[0], array[1], array[2], array[3]); break; }
				default: { glUniform1fv(loc, count, &array[0]); } break;
				}

				return;
			}
		} break;

		case UNIFORM_TYPE_INT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1i(loc, lua_toint(L, -1));
				return;
			}
			if (lua_istable(L, -1)) {
				const int* array = intUniformArrayBuf;
				const int count = LuaUtils::ParseIntArray(L, -1, intUniformArrayBuf, sizeof(intUniformArrayBuf) / sizeof(int));

				switch (count) {
				case 1: { glUniform1i(loc, array[0]); break; }
				case 2: { glUniform2i(loc, array[0], array[1]); break; }
				case 3: { glUniform3i(loc, array[0], array[1], array[2]); break; }
				case 4: { glUniform4i(loc, array[0], array[1], array[2], array[3]); break; }
				default: { glUniform1iv(loc, count, &array[0]); } break;
				}

				return;
			}
		} break;

		case UNIFORM_TYPE_FLOAT_MATRIX: {
			if (lua_istable(L, -1)) {
				float array[16] = { 0.0f };
				const int count = LuaUtils::ParseFloatArray(L, -1, array, 16);

				switch (count) {
				case (2 * 2): { glUniformMatrix2fv(loc, 1, GL_FALSE, array); break; }
				case (3 * 3): { glUniformMatrix3fv(loc, 1, GL_FALSE, array); break; }
				case (4 * 4): { glUniformMatrix4fv(loc, 1, GL_FALSE, array); break; }
				}

				return;
			}
		} break;
		}
	}

	static bool ParseUniformsTable(
		lua_State* L,
		int index,
		int type,
		const LuaShaders::Program& p
	) {
		constexpr const char* fieldNames[] = { "uniform", "uniformInt", "uniformFloat", "uniformMatrix" };
		const char* fieldName = fieldNames[type];

		lua_getfield(L, index, fieldName);

		if (lua_istable(L, -1)) {
			const int tableIdx = lua_gettop(L);

			for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
				if (!lua_israwstring(L, -2))
					continue;

				const char* uniformName = lua_tostring(L, -2);
				const auto iter = p.activeUniforms.find(uniformName);

				if (iter == p.activeUniforms.end()) {
					if (globalRendering->glDebug || globalRendering->glDebugErrors)
						LOG_L(L_WARNING, "[%s] uniform \"%s\" from table \"%s\" not active in shader", __func__, uniformName, fieldName);
					continue;
				}

				// should only need to auto-correct if type == UNIFORM_TYPE_MIXED, but GL debug-errors say otherwise
				switch (iter->second.type) {
				case GL_SAMPLER_1D: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_3D: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_1D_SHADOW: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D_SHADOW: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_CUBE: { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D_MULTISAMPLE: { type = UNIFORM_TYPE_INT; } break;

				case GL_INT: { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC2: { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC3: { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC4: { type = UNIFORM_TYPE_INT; } break;

				case GL_UNSIGNED_INT: { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC2: { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC3: { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC4: { type = UNIFORM_TYPE_INT; } break;

				case GL_FLOAT: { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC2: { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC3: { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC4: { type = UNIFORM_TYPE_FLOAT; } break;

				case GL_FLOAT_MAT2: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;
				case GL_FLOAT_MAT3: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;
				case GL_FLOAT_MAT4: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;

				default: {
					LOG_L(L_WARNING, "[%s] value for uniform \"%s\" from table \"%s\" (GL-type 0x%x) set as int", __func__, uniformName, fieldName, iter->second.type);
					type = UNIFORM_TYPE_INT;
				} break;
				}

				ParseUniformType(L, p.activeUniformLocations.find(uniformName)->second.location, type);
			}
		}

		lua_pop(L, 1);
		return true;
	}

	static GLint FillActiveUniforms(LuaShaders::Program& prog)
	{
		GLint currentProgram = 0;
		GLint numUniforms = 0;
		GLsizei uniformLen = 0;

		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
		glUseProgram(prog.id);
		glGetProgramiv(prog.id, GL_ACTIVE_UNIFORMS, &numUniforms);

		prog.activeUniforms.reserve(numUniforms);

		std::array<char, 512> nameBuffer = { 0 };
		for (int i = 0; i < numUniforms; ++i) {
			LuaShaders::ActiveUniform u;
			LuaShaders::ActiveUniformLocation ul;

			glGetActiveUniform(prog.id, i, nameBuffer.size() - 1, &uniformLen, &u.size, &u.type, nameBuffer.data());

			if (strncmp(nameBuffer.data(), "gl_", 3) == 0)
				continue;

			nameBuffer[uniformLen + 1] = '\0';
			std::string name(nameBuffer.data());

			if (name[uniformLen - 1] == ']') {
				// strip "[0]" postfixes from array-uniform names
				name = name.substr(0, uniformLen - 3);
			}

			ul.location = glGetUniformLocation(prog.id, name.data());
			prog.activeUniforms[name] = u;
			prog.activeUniformLocations[name] = ul;
		}

		return currentProgram;
	}

	static bool ParseUniformSetupTables(lua_State* L, int index, const LuaShaders::Program& p)
	{
		bool ret = true;

		ret = ret && ParseUniformsTable(L, index, UNIFORM_TYPE_MIXED       , p);
		ret = ret && ParseUniformsTable(L, index, UNIFORM_TYPE_INT         , p);
		ret = ret && ParseUniformsTable(L, index, UNIFORM_TYPE_FLOAT       , p);
		ret = ret && ParseUniformsTable(L, index, UNIFORM_TYPE_FLOAT_MATRIX, p);

		return ret;
	}

	/******************************************************************************/
	/******************************************************************************/

	static GLuint CompileObject(
		lua_State* L,
		const std::vector<std::string>& defs,
		const std::vector<std::string>& sources,
		const GLenum type,
		bool& success
	) {
		if (sources.empty()) {
			success = true;
			return 0;
		}

		GLuint obj = glCreateShader(type);
		if (obj == 0) {
			LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
			shaders.errorLog = "Could not create shader object";
			return 0;
		}

		std::vector<const GLchar*> text(defs.size() + sources.size());

		for (uint32_t i = 0; i < defs.size(); i++)
			text[i] = defs[i].c_str();
		for (uint32_t i = 0; i < sources.size(); i++)
			text[defs.size() + i] = sources[i].c_str();

		glShaderSource(obj, text.size(), &text[0], nullptr);
		glCompileShader(obj);

		GLint result;
		glGetShaderiv(obj, GL_COMPILE_STATUS, &result);
		GLchar log[4096];
		GLsizei logSize = sizeof(log);
		glGetShaderInfoLog(obj, logSize, &logSize, log);

		LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
		shaders.errorLog = log;

		if (result != GL_TRUE) {
			if (shaders.errorLog.empty())
				shaders.errorLog = "Empty error message:  code = " + IntToString(result) + " (0x" + IntToString(result, "%04X") + ")";

			glDeleteShader(obj);

			success = false;
			return 0;
		}

		success = true;
		return obj;
	}


	static bool ParseShaderTable(
		lua_State* L,
		const int table,
		const char* key,
		std::vector<std::string>& data
	) {
		lua_getfield(L, table, key);

		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			return true;
		}

		if (lua_israwstring(L, -1)) {
			const std::string& txt = lua_tostring(L, -1);

			if (!txt.empty())
				data.push_back(txt);

			lua_pop(L, 1);
			return true;
		}

		if (lua_istable(L, -1)) {
			const int subtable = lua_gettop(L);

			for (lua_pushnil(L); lua_next(L, subtable) != 0; lua_pop(L, 1)) {
				if (!lua_israwnumber(L, -2)) // key (idx)
					continue;
				if (!lua_israwstring(L, -1)) // val
					continue;

				const std::string& txt = lua_tostring(L, -1);

				if (!txt.empty())
					data.push_back(txt);
			}

			lua_pop(L, 1);
			return true;
		}

		LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
		shaders.errorLog = "\"" + std::string(key) + "\" must be a string or a table value!";

		lua_pop(L, 1);
		return false;
	}


	static void ApplyGeometryParameters(lua_State* L, int table, GLuint prog)
	{
		if (!IS_GL_FUNCTION_AVAILABLE(glProgramParameteriEXT))
			return;

		constexpr struct { const char* name; GLenum param; } parameters[] = {
			{ "geoInputType",   GL_GEOMETRY_INPUT_TYPE_EXT },
			{ "geoOutputType",  GL_GEOMETRY_OUTPUT_TYPE_EXT },
			{ "geoOutputVerts", GL_GEOMETRY_VERTICES_OUT_EXT }
		};

		constexpr size_t count = sizeof(parameters) / sizeof(parameters[0]);

		for (size_t i = 0; i < count; i++) {
			lua_getfield(L, table, parameters[i].name);

			if (lua_israwnumber(L, -1))
				glProgramParameteriEXT(prog, parameters[i].param, /*type*/ lua_toint(L, -1));

			lua_pop(L, 1);
		}
	}
} //anonymous NS

GLint LuaShaders::GetUniformLocation(LuaShaders::Program* p, const char* name)
{
	if (!p)
		return -1;

	const auto iter = p->activeUniformLocations.find(name);
	if (iter == p->activeUniformLocations.cend()) {
		ActiveUniformLocation ul;
		ul.location = glGetUniformLocation(p->id, name);
		p->activeUniformLocations[name] = ul;

		return ul.location;
	}

	return iter->second.location;
}

/***
 * A table of uniform name to value.
 * 
 * The Uniforms are the values you send along with the shader-program. To use
 * them in the shader-program declare them like this: `uniform float frame;`
 * 
 * Specify a Lua array to initialize GLSL arrays.
 * 
 * The engine will automatically fill in an appropriately named uniform for team
 * colour if it is declared;
 *
 * ```glsl
 * uniform vec4 teamColor;
 * ```
 * 
 * @class UniformParam<T> : { [string]: T|T[] }
 */

/***
 * @class ShaderParams
 * 
 * @field vertex string?
 * 
 * The "Vertex" or vertex-shader is your GLSL-Code as string, its written in a
 * C-Dialect.  This shader is busy deforming the geometry of a unit but it can
 * not create new polygons. Use it for waves, wobbling surfaces etc.
 * 
 * @field tcs string?
 * 
 * The "TCS" or Tesselation Control Shader controls how much tessellation a
 * particular patch gets; it also defines the size of a patch, thus allowing it
 * to augment data. It can also filter vertex data taken from the vertex shader.
 * The main purpose of the TCS is to feed the tessellation levels to the
 * Tessellation primitive generator stage, as well as to feed patch data (as its
 * output values) to the Tessellation Evaluation Shader stage.
 * 
 * @field tes string?
 * 
 * The "TES" or Tesselation Evaluation Shader takes the abstract patch generated
 * by the tessellation primitive generation stage, as well as the actual vertex
 * data for the entire patch, and generates a particular vertex from it. Each
 * TES invocation generates a single vertex. It can also take per-patch data
 * provided by the Tessellation Control Shader.
 * 
 * @field geometry string?
 * 
 * The "Geometry" or Geometry-shader can create new vertices and vertice-stripes
 * from points.
 * 
 * @field fragment string?
 * 
 * The "Fragment" or Fragment-shader (sometimes called pixel-Shader) is post
 * processing the already rendered picture (for example drawing stars on the
 * sky).
 * 
 * Remember textures are not always 2 dimensional pictures. They can contain
 * information about the depth, or the third value marks areas and the strength
 * at which these are processed.
 * 
 * @field uniform UniformParam<number>?
 * @field uniformInt UniformParam<integer>?
 * @field uniformFloat UniformParam<number>?
 * @field uniformMatrix UniformParam<number>?
 * @field geoInputType integer? inType
 * @field geoOutputType integer? outType
 * @field geoOutputVerts integer? maxVerts
 * @field definitions string? string of shader #defines"
 */
 
/***
 * Create a shader.
 *
 * @function gl.CreateShader
 * @param shaderParams ShaderParams
 * @return integer shaderID
 */
int LuaShaders::CreateShader(lua_State* L)
{
	const int args = lua_gettop(L);

	if ((args != 1) || !lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to gl.CreateShader()");

	std::vector<std::string> shdrDefs;
	std::vector<std::string> vertSrcs;
	std::vector<std::string> tcsSrcs;
	std::vector<std::string> tesSrcs;
	std::vector<std::string> geomSrcs;
	std::vector<std::string> fragSrcs;
	std::vector<std::string> compSrcs;

	ParseShaderTable(L, 1, "defines", shdrDefs);
	ParseShaderTable(L, 1, "definitions", shdrDefs);

	if (!ParseShaderTable(L, 1,   "vertex", vertSrcs))
		return 0;
	if (!ParseShaderTable(L, 1,      "tcs",  tcsSrcs))
		return 0;
	if (!ParseShaderTable(L, 1,      "tes",  tesSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "geometry", geomSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "fragment", fragSrcs))
		return 0;

	if (!ParseShaderTable(L, 1, "compute", compSrcs))
		return 0;

	const bool graphicSrcEmpty = vertSrcs.empty() && fragSrcs.empty() && geomSrcs.empty() && tcsSrcs.empty() && tesSrcs.empty();
	const bool computeSrcEmpty = compSrcs.empty();

	// tables might have contained empty strings
	if (graphicSrcEmpty && computeSrcEmpty)
		return 0;

	// tables might have contained both types of shader programs
	if (!graphicSrcEmpty && !computeSrcEmpty)
		return 0;

	bool success;
	const GLuint vertObj = CompileObject(L, shdrDefs, vertSrcs, GL_VERTEX_SHADER, success);

	if (!success)
		return 0;

	const GLuint tcsObj = CompileObject(L, shdrDefs,  tcsSrcs, GL_TESS_CONTROL_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		return 0;
	}

	const GLuint tesObj = CompileObject(L, shdrDefs,  tesSrcs,  GL_TESS_EVALUATION_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		return 0;
	}
	const GLuint geomObj = CompileObject(L, shdrDefs, geomSrcs, GL_GEOMETRY_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		glDeleteShader(tesObj);
		return 0;
	}

	const GLuint fragObj = CompileObject(L, shdrDefs, fragSrcs, GL_FRAGMENT_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		glDeleteShader(tesObj);
		glDeleteShader(geomObj);
		return 0;
	}

	const GLuint compObj = CompileObject(L, shdrDefs, compSrcs, GL_COMPUTE_SHADER, success);

	if (!success)
		return 0;

	const GLuint prog = glCreateProgram();

	Program p(prog);

	if (vertObj != 0) {
		glAttachShader(prog, vertObj);
		p.objects.emplace_back(vertObj, GL_VERTEX_SHADER);
	}

	if (tcsObj != 0) {
		glAttachShader(prog, tcsObj);
		p.objects.emplace_back(tcsObj, GL_TESS_CONTROL_SHADER);
	}
	if (tesObj != 0) {
		glAttachShader(prog, tesObj);
		p.objects.emplace_back(tesObj, GL_TESS_EVALUATION_SHADER);
	}

	if (geomObj != 0) {
		glAttachShader(prog, geomObj);
		p.objects.emplace_back(geomObj, GL_GEOMETRY_SHADER);
		ApplyGeometryParameters(L, 1, prog); // done before linking
	}

	if (fragObj != 0) {
		glAttachShader(prog, fragObj);
		p.objects.emplace_back(fragObj, GL_FRAGMENT_SHADER);
	}

	if (compObj != 0) {
		glAttachShader(prog, compObj);
		p.objects.emplace_back(compObj, GL_COMPUTE_SHADER);
	}

	GLint linkStatus;
	GLint validStatus;

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);

	// Parse active uniforms and locations
	GLint currentProgram = FillActiveUniforms(p);

	// Allows setting up uniforms when drawing is disabled
	// (much more convenient for sampler uniforms, and static
	//  configuration values)
	// needs to be called before validation
	ParseUniformSetupTables(L, 1, p);

	glUseProgram(currentProgram);

	glValidateProgram(prog);
	glGetProgramiv(prog, GL_VALIDATE_STATUS, &validStatus);

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	if (linkStatus != GL_TRUE || validStatus != GL_TRUE) {
		GLchar log[4096];
		GLsizei logSize = sizeof(log);
		glGetProgramInfoLog(prog, logSize, &logSize, log);
		shaders.errorLog = log;

		DeleteProgram(p);
		return 0;
	}

	// note: index, not raw ID
	lua_pushnumber(L, shaders.AddProgram(p));
	// also push the program ID
	lua_pushnumber(L, prog);
	return 2;
}


/*** Deletes a shader identified by shaderID
 *
 * @function gl.DeleteShader
 * @param shaderID integer
 */
int LuaShaders::DeleteShader(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	lua_pushboolean(L, shaders.RemoveProgram(luaL_checkint(L, 1)));
	return 1;
}


/*** Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.
 *
 * @function gl.UseShader
 * @param shaderID integer
 * @return boolean linked
 */
int LuaShaders::UseShader(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int progIdx = luaL_checkint(L, 1);
	if (progIdx == 0) {
		glUseProgram(0);
		activeProgram = nullptr;
		lua_pushboolean(L, true);
		return 1;
	}

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	auto* prog = shaders.GetProgram(progIdx);

	if (prog == nullptr) {
		activeProgram = nullptr;
		lua_pushboolean(L, false);
	} else {
		activeProgram = prog;
		glUseProgram(prog->id);
		lua_pushboolean(L, true);
	}
	return 1;
}


/***
 * Binds a shader program identified by shaderID, and calls the Lua func with
 * the specified arguments.
 *
 * Can be used in NON-drawing events (to update uniforms etc.)!
 *
 * @function gl.ActiveShader
 * @param shaderID integer
 * @param func function
 * @param ... any Arguments
 */
int LuaShaders::ActiveShader(lua_State* L)
{
	const int progIdx = luaL_checkint(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	Program* prog = nullptr;

	if (progIdx != 0) {
		LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

		if ((prog = shaders.GetProgram(progIdx)) == nullptr) {
			return 0;
		}
	}

	GLint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

	glUseProgram(prog->id);
	activeProgram = prog;
	activeShaderDepth++;
	const int error = lua_pcall(L, lua_gettop(L) - 2, 0, 0);
	activeShaderDepth--;
	activeProgram = nullptr;
	glUseProgram(currentProgram);

	if (error != 0) {
		LOG_L(L_ERROR, "gl.ActiveShader: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

static const char* UniformTypeString(GLenum type)
{
#define UNIFORM_STRING_CASE(x)                        \
  case (GL_ ## x): {                                  \
    static const std::string str = StringToLower(#x); \
    return str.c_str();                               \
  }

	switch (type) {
		UNIFORM_STRING_CASE(FLOAT)
		UNIFORM_STRING_CASE(FLOAT_VEC2)
		UNIFORM_STRING_CASE(FLOAT_VEC3)
		UNIFORM_STRING_CASE(FLOAT_VEC4)
		UNIFORM_STRING_CASE(FLOAT_MAT2)
		UNIFORM_STRING_CASE(FLOAT_MAT3)
		UNIFORM_STRING_CASE(FLOAT_MAT4)
		UNIFORM_STRING_CASE(SAMPLER_1D)
		UNIFORM_STRING_CASE(SAMPLER_2D)
		UNIFORM_STRING_CASE(SAMPLER_3D)
		UNIFORM_STRING_CASE(SAMPLER_CUBE)
		UNIFORM_STRING_CASE(SAMPLER_1D_SHADOW)
		UNIFORM_STRING_CASE(SAMPLER_2D_SHADOW)
		UNIFORM_STRING_CASE(INT)
		UNIFORM_STRING_CASE(INT_VEC2)
		UNIFORM_STRING_CASE(INT_VEC3)
		UNIFORM_STRING_CASE(INT_VEC4)
		UNIFORM_STRING_CASE(BOOL)
		UNIFORM_STRING_CASE(BOOL_VEC2)
		UNIFORM_STRING_CASE(BOOL_VEC3)
		UNIFORM_STRING_CASE(BOOL_VEC4)
		default: { return "unknown_type"; }
	}
}

/***
 * @class ActiveUniform
 * @field name string
 * @field type string String name of `GL_*` constant.
 * @field length integer The character length of `name`.
 * @field size integer
 * @field location GL
 */

/***
 * Query the active (actually used) uniforms of a shader and identify their
 * names, types (float, int, uint) and sizes (float, vec4, ...).
 *
 * @function gl.GetActiveUniforms
 * @param shaderID integer
 * @return ActiveUniform[] activeUniforms
 */
int LuaShaders::GetActiveUniforms(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const auto* prog = shaders.GetProgram(L, 1);

	if (prog == nullptr)
		return 0;

	lua_createtable(L, prog->activeUniforms.size(), 0);

	GLint i = 0;
	for (const auto& [name, au] : prog->activeUniforms) {
		lua_createtable(L, 0, 5); {
			HSTR_PUSH_STRING(L, "name"    , name);
			HSTR_PUSH_STRING(L, "type"    , UniformTypeString(au.type));
			HSTR_PUSH_NUMBER(L, "length"  , name.size());
			HSTR_PUSH_NUMBER(L, "size"    , au.size);
			HSTR_PUSH_NUMBER(L, "location", prog->activeUniformLocations.at(name).location);
		}
		lua_rawseti(L, -2, i + 1);
		++i;
	}

	return 1;
}


/***
 * Returns the locationID of a shaders uniform. Needed for changing uniform
 * values with function `gl.Uniform`.
 *
 * @function gl.GetUniformLocation
 * @param shaderID integer
 * @param name string
 * @return GL locationID
 */
int LuaShaders::GetUniformLocation(lua_State* L)
{
	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	Program* prog = shaders.GetProgram(L, 1);

	if (prog == nullptr)
		return 0;

	const char* name = luaL_checkstring(L, 2);

	lua_pushnumber(L, GetUniformLocation(prog, name));
	return 1;
}

int LuaShaders::GetSubroutineIndex(lua_State* L)
{
	if (!IS_GL_FUNCTION_AVAILABLE(glGetSubroutineIndex))
		return 0;

	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);

	if (progName == 0)
		return 0;

	const GLenum shaderType = (GLenum)luaL_checkint(L, 2);

	const char* name = luaL_checkstring(L, 3);
	const GLint location = glGetSubroutineIndex(progName, shaderType, name);

	lua_pushnumber(L, location);
	return 1;
}

namespace {
	template<typename T> int SetObjectBufferUniforms(lua_State* L, const char* func)
	{
		const int id = luaL_checkint(L, 1);
		const T* o = LuaUtils::IdToObject<T>(id, func);
		if (o == nullptr)
			luaL_error(L, "gl.%s() Invalid %s id (%d)", func, &spring::TypeToCStr<T>()[1], id);

		ModelUniformData& uni = modelUniformsStorage.GetObjUniformsArray(o);

		std::array<float, ModelUniformData::MAX_MODEL_UD_UNIFORMS> floatArray = {0};
		int size = LuaUtils::ParseFloatArray(L, 2, floatArray.data(), ModelUniformData::MAX_MODEL_UD_UNIFORMS);

		const int offset = std::max(luaL_optint(L, 3, 0), 0);
		size = std::min(size, ModelUniformData::MAX_MODEL_UD_UNIFORMS - offset);

		if (size < 1) {
			lua_pushnumber(L, 0);
			return 1;
		}

		std::copy(floatArray.cbegin(), floatArray.cbegin() + size, std::begin(uni.userDefined) + offset);

		lua_pushnumber(L, size);
		return 1;
	}
}

int LuaShaders::SetUnitBufferUniforms(lua_State* L) { return SetObjectBufferUniforms<CUnit>(L, __func__); }
int LuaShaders::SetFeatureBufferUniforms(lua_State* L) { return SetObjectBufferUniforms<CFeature>(L, __func__); }



/******************************************************************************/
/******************************************************************************/

/***
 * Sets the uniform float value at the locationID for the currently active
 * shader. Shader must be activated before setting uniforms.
 *
 * @function gl.Uniform
 * @param locationID GL|string uniformName
 * @param f1 number
 * @param f2 number?
 * @param f3 number?
 * @param f4 number?
 */
int LuaShaders::Uniform(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = (lua_type(L, 1) == LUA_TSTRING) ? GetUniformLocation(activeProgram, luaL_checkstring(L, 1)) : luaL_checkint(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			glUniform1f(location, luaL_checkfloat(L, 2));
		} break;
		case 2: {
			glUniform2f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
		} break;
		case 3: {
			glUniform3f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
		} break;
		case 4: {
			glUniform4f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4), luaL_checkfloat(L, 5));
		} break;
		default: {
			luaL_error(L, "Incorrect arguments to gl.Uniform()");
		} break;
	}

	return 0;
}


/***
 * Sets the uniform int value at the locationID for the currently active shader.
 * Shader must be activated before setting uniforms.
 *
 * @function gl.UniformInt
 * @param locationID integer|string uniformName
 * @param int1 integer
 * @param int2 integer?
 * @param int3 integer?
 * @param int4 integer?
 */
int LuaShaders::UniformInt(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = (lua_type(L, 1) == LUA_TSTRING) ? GetUniformLocation(activeProgram, luaL_checkstring(L, 1)) : luaL_checkint(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			glUniform1i(location, luaL_checkint(L, 2));
		} break;
		case 2: {
			glUniform2i(location, luaL_checkint(L, 2), luaL_checkint(L, 3));
		} break;
		case 3: {
			glUniform3i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
		} break;
		case 4: {
			glUniform4i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5));
		} break;
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformInt()");
		}
	}

	return 0;
}


#if 0
template<typename type, typename glUniformFunc, typename ParseArrayFunc>
static bool GLUniformArray(lua_State* L, UniformFunc uf, ParseArrayFunc pf)
{
	const GLuint loc = luaL_checkint(L, 1);

	switch (std::bit_ceil <uint32_t> (luaL_optint(L, 4, 32))) {
		case (1 <<  3): { type array[1 <<  3] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  3), &array[0]); return true; } break;
		case (1 <<  4): { type array[1 <<  4] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  4), &array[0]); return true; } break;
		case (1 <<  5): { type array[1 <<  5] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  5), &array[0]); return true; } break;
		case (1 <<  6): { type array[1 <<  6] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  6), &array[0]); return true; } break;
		case (1 <<  7): { type array[1 <<  7] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  7), &array[0]); return true; } break;
		case (1 <<  8): { type array[1 <<  8] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  8), &array[0]); return true; } break;
		case (1 <<  9): { type array[1 <<  9] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  9), &array[0]); return true; } break;
		case (1 << 10): { type array[1 << 10] = {type(0)}; uf(loc, pf(L, 3, array, 1 << 10), &array[0]); return true; } break;
		default: {} break;
	}

	return false;
}
#endif

/***
 * @alias UniformArrayType
 * | 1 # int
 * | 2 # float
 * | 3 # float matrix
 */

/***
 * Sets the an array of uniform values at the locationID for the currently
 * active shader.
 * 
 * Shader must be activated before setting uniforms.
 *
 * @function gl.UniformArray
 * @param locationID integer|string uniformName
 * @param type UniformArrayType
 * @param uniforms number[] Array up to 1024 elements
 */
int LuaShaders::UniformArray(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	if (!lua_istable(L, 3))
		return 0;

	switch (luaL_checkint(L, 2)) {
		case UNIFORM_TYPE_INT: {
			#if 0
			GLUniformArray<int>(L, glUniform1iv, LuaUtils::ParseIntArray);
			#else
			const GLuint location = (lua_type(L, 1) == LUA_TSTRING) ? GetUniformLocation(activeProgram, luaL_checkstring(L, 1)) : luaL_checkint(L, 1);
			const int cnt = LuaUtils::ParseIntArray(L, 3, intUniformArrayBuf, sizeof(intUniformArrayBuf) / sizeof(int));

			glUniform1iv(location, cnt, &intUniformArrayBuf[0]);
			#endif
		} break;

		case UNIFORM_TYPE_FLOAT: {
			#if 0
			GLUniformArray<float>(L, glUniform1fv, LuaUtils::ParseFloatArray);
			#else
			const GLuint location = (lua_type(L, 1) == LUA_TSTRING) ? GetUniformLocation(activeProgram, luaL_checkstring(L, 1)) : luaL_checkint(L, 1);
			const int cnt = LuaUtils::ParseFloatArray(L, 3, fltUniformArrayBuf, sizeof(fltUniformArrayBuf) / sizeof(float));

			glUniform1fv(location, cnt, &fltUniformArrayBuf[0]);
			#endif
		} break;

		default: {
		} break;
	}

	return 0;
}

/***
 * Sets the a uniform mat4 locationID for the currently active shader.
 * 
 * Shader must be activated before setting uniforms.
 * 
 * Can set one one common matrix like shadow, or by passing 16 additional
 * numbers for the matrix.
 *
 * @function gl.UniformMatrix
 * @param locationID integer|string uniformName
 * @param matrix "shadows"|"camera"|"caminv"|"camprj" Name of common matrix.
 */

/***
 * Sets the a uniform mat4 locationID for the currently active shader.
 * 
 * Shader must be activated before setting uniforms.
 * 
 * Can set one one common matrix like shadow, or by passing 16 additional
 * numbers for the matrix.
 *
 * @function gl.UniformMatrix
 * @param locationID integer|string uniformName
 * @param matrix number[] A 2x2, 3x3 or 4x4 matrix.
 */
int LuaShaders::UniformMatrix(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = (lua_type(L, 1) == LUA_TSTRING) ? GetUniformLocation(activeProgram, luaL_checkstring(L, 1)) : luaL_checkint(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
	case 1: {
			if (!lua_isstring(L, 2))
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");

			const char* matName = lua_tostring(L, 2);
			const CMatrix44f* mat = LuaOpenGLUtils::GetNamedMatrix(matName);

			if (mat) {
				glUniformMatrix4fv(location, 1, GL_FALSE, *mat);
			} else {
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
			}
			break;
		}
		case (2 * 2): {
			float array[2 * 2];

			for (int i = 0; i < (2 * 2); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix2fv(location, 1, GL_FALSE, array);
			break;
		}
		case (3 * 3): {
			float array[3 * 3];

			for (int i = 0; i < (3 * 3); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix3fv(location, 1, GL_FALSE, array);
			break;
		}
		case (4 * 4): {
			float array[4 * 4];

			for (int i = 0; i < (4 * 4); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix4fv(location, 1, GL_FALSE, array);
			break;
		}
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
		}
	}

	return 0;
}

int LuaShaders::UniformSubroutine(lua_State* L)
{
	if (!IS_GL_FUNCTION_AVAILABLE(glUniformSubroutinesuiv))
		return 0;
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLenum shaderType = (GLenum)luaL_checkint(L, 1);
	const GLuint index = (GLuint)luaL_checknumber(L, 2);

	// this supports array and even array of arrays, but let's keep it simple
	glUniformSubroutinesuiv(shaderType, 1, &index);
	return 0;
}

/***
 *
 * @function gl.GetEngineUniformBufferDef
 *
 * Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.
 *
 * @param index number
 * @return string glslDefinition
 */
int LuaShaders::GetEngineUniformBufferDef(lua_State* L)
{
	if (!globalRendering->haveGL4)
		return 0;

	const int idx = luaL_checkint(L, 1);
	if (idx < 0 || idx > 1)
		luaL_error(L, "%s(): Invalid UniformConstants buffer index (%d) requested", __func__, idx);

	lua_pushstring(L, UniformConstants::GetInstance().GetGLSLDefinition(idx).c_str());
	return 1;
}

/***
 *
 * @function gl.GetEngineModelUniformDataDef
 *
 * Return the GLSL compliant definition of ModelUniformData structure (per Unit/Feature buffer available on GPU)
 *
 * @param index number
 * @return string glslDefinition
 */
int LuaShaders::GetEngineModelUniformDataDef(lua_State* L)
{
	if (!globalRendering->haveGL4)
		return 0;

	lua_pushstring(L, ModelUniformData::GetGLSLDefinition().c_str());
	return 1;
}

/*** Sets the Geometry shader parameters for shaderID. Needed by geometry shader programs (check the opengl GL_ARB_geometry_shader4 extension for glProgramParameteri)
 *
 * @function gl.SetGeometryShaderParameter
 * @param shaderID integer
 * @param param number
 * @param number number
 * @return nil
 */
int LuaShaders::SetGeometryShaderParameter(lua_State* L)
{
	if (!IS_GL_FUNCTION_AVAILABLE(glProgramParameteriEXT))
		return 0;

	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0)
		return 0;

	const GLenum param = (GLenum)luaL_checkint(L, 2);
	const GLint  value =  (GLint)luaL_checkint(L, 3);

	glProgramParameteriEXT(progName, param, value);
	return 0;
}

/***
 * Sets the tesselation shader parameters for `shaderID`.
 *
 * Needed by tesselation shader programs. (Check the opengl
 * `GL_ARB_tessellation_shader` extension for `glProgramParameteri`).
 * 
 * @function gl.SetTesselationShaderParameter
 * @param param integer
 * @param value integer
 * @return nil
 */
int LuaShaders::SetTesselationShaderParameter(lua_State* L)
{
	if (!IS_GL_FUNCTION_AVAILABLE(glPatchParameteri))
		return 0;

	const GLenum param = (GLenum)luaL_checkint(L, 1);

	if (lua_israwnumber(L, 2)) {
		const GLint value =  (GLint)luaL_checkint(L, 2);
		glPatchParameteri(param, value);
		return 0;
	}
	if (lua_istable(L, 2)) {
		float tessArrayBuf[4] = {0.0f};
		LuaUtils::ParseFloatArray(L, 2, tessArrayBuf, 4);
		glPatchParameterfv(param, &tessArrayBuf[0]);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
