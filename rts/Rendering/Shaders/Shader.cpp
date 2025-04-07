/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/LuaShaderContainer.h"
#include "Rendering/Shaders/GLSLCopyState.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"

#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/SpringHash.h"
#include "System/Log/ILog.h"

#include "System/Config/ConfigHandler.h"

#include <algorithm>
#include <array>
#ifdef DEBUG
	#include <cstring> // strncmp
#endif

#include "System/Misc/TracyDefs.h"



/*****************************************************************/

#define LOG_SECTION_SHADER "Shader"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SHADER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SHADER

/*****************************************************************/

CONFIG(bool, UseShaderCache).defaultValue(true).description("If already compiled shaders should be shared via a cache, reducing compiles of already compiled shaders.");


/*****************************************************************/

static bool glslIsValid(GLuint obj)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool isShader = glIsShader(obj);
	assert(glIsShader(obj) || glIsProgram(obj));

	GLint compiled;
	if (isShader)
		glGetShaderiv(obj, GL_COMPILE_STATUS, &compiled);
	else
		glGetProgramiv(obj, GL_LINK_STATUS, &compiled);

	return compiled;
}


static std::string glslGetLog(GLuint obj)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool isShader = glIsShader(obj);
	assert(glIsShader(obj) || glIsProgram(obj));

	int infologLength = 0;
	int maxLength = 0;

	if (isShader)
		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &maxLength);
	else
		glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &maxLength);

	std::string infoLog;
	infoLog.resize(maxLength);

	if (isShader)
		glGetShaderInfoLog(obj, maxLength, &infologLength, &infoLog[0]);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, &infoLog[0]);

	infoLog.resize(infologLength);
	return infoLog;
}

static bool ExtractGlslVersion(std::string* src, std::string* version)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto pos = src->find("#version ");

	if (pos != std::string::npos) {
		const auto eol = src->find('\n', pos) + 1;
		*version = src->substr(pos, eol - pos);
		src->erase(pos, eol - pos);
		return true;
	}
	return false;
}

/*****************************************************************/


namespace Shader {
	static NullShaderObject nullShaderObject_(0, "");
	static NullProgramObject nullProgramObject_("NullProgram");

	NullShaderObject* nullShaderObject = &nullShaderObject_;
	NullProgramObject* nullProgramObject = &nullProgramObject_;


	/*****************************************************************/

	unsigned int IShaderObject::GetHash() const {
		unsigned int hash = 127;
		hash = spring::LiteHash((const void*)   srcText.data(),    srcText.size(), hash); // srcTextHash is not worth it, only called on reload
		hash = spring::LiteHash((const void*)modDefStrs.data(), modDefStrs.size(), hash);
		hash = spring::LiteHash((const void*)rawDefStrs.data(), rawDefStrs.size(), hash); // rawDefStrsHash is not worth it, only called on reload
		return hash;
	}

	std::string IShaderObject::GetShaderSource(const std::string& fileName)
	{
		// get rid of '\r\n's so the output looks nicer in the infolog
		static const auto ReplaceCRLF = [](std::string& src) {
			std::string::size_type pos = 0;
			while ((pos = src.find("\r\n", pos)) != std::string::npos) {
				src.replace(pos, 2, "\n");
				++pos;
			}
		};

		std::string soSource;

		if (fileName.find("void main()") != std::string::npos) {
			soSource = fileName; // fileName content is actually the source code
			ReplaceCRLF(soSource);
			return soSource;
		}

		std::string soPath = "shaders/" + fileName;

		CFileHandler soFile(soPath);

		if (soFile.FileExists()) {
			soSource.resize(soFile.FileSize());
			soFile.Read(&soSource[0], soFile.FileSize());
			reloadRequested = false;
		}
		else {
			LOG_L(L_ERROR, "[%s] file not found \"%s\"", __FUNCTION__, soPath.c_str());
		}

		ReplaceCRLF(soSource);
		return soSource;
	}

	bool IShaderObject::ReloadFromDisk()
	{
		RECOIL_DETAILED_TRACY_ZONE;
		reloadRequested = true;
		std::string newText = GetShaderSource(srcFile);

		if (newText != srcText) {
			srcText = std::move(newText);
			return true;
		}

		return false;
	}


	/*****************************************************************/

	GLSLShaderObject::GLSLShaderObject(
		unsigned int shType,
		const std::string& shSrcFile,
		const std::string& shSrcDefs
	): IShaderObject(shType, shSrcFile, shSrcDefs)
	{ }

	GLSLShaderObject::CompiledShaderObjectUniquePtr GLSLShaderObject::CompileShaderObject()
	{
		CompiledShaderObjectUniquePtr res(new CompiledShaderObject(), [](CompiledShaderObject* so) {
			glDeleteShader(so->id);
			so->id = 0;
			spring::SafeDelete(so);
		});

		assert(!srcText.empty());

		std::string sourceStr = srcText;
		std::string defFlags  = rawDefStrs + "\n" + modDefStrs;
		std::string versionStr;

		// extract #version pragma and put it on the first line (only allowed there)
		// version pragma in definitions overrides version pragma in source (if any)
		ExtractGlslVersion(&sourceStr, &versionStr);
		ExtractGlslVersion(&defFlags,  &versionStr);

		if (!versionStr.empty()) EnsureEndsWith(&versionStr, "\n");
		if (!defFlags.empty())   EnsureEndsWith(&defFlags,   "\n");

		std::array sources = {
			"// SHADER VERSION\n",
			versionStr.c_str(),
			"// SHADER FLAGS\n",
			defFlags.c_str(),
			"// SHADER SOURCE\n",
			sourceStr.c_str()
		};

		res->id = glCreateShader(type);

		glShaderSource(res->id, sources.size(), &sources[0], nullptr);
		glCompileShader(res->id);

		res->valid = glslIsValid(res->id);
		res->log   = glslGetLog(res->id);

		if (!res->valid && logReporting) {
			const std::string& name = srcFile.find("void main()") != std::string::npos ? "unknown" : srcFile;
			LOG_L(L_WARNING, "[GLSL-SO::%s] shader-object name: %s, compile-log:\n%s\n", __FUNCTION__, name.c_str(), res->log.c_str());
			LOG_L(L_WARNING, "\n%s%s%s%s%s%s", sources[0], sources[1], sources[2], sources[3], sources[4], sources[5]);
		}

		return res;
	}




	/*****************************************************************/

	IProgramObject::IProgramObject(const std::string& poName)
		: name(poName)
		, objID(0)
		, logReporting(true)
		, valid(false)
		, bound(false) {
	}

	void IProgramObject::SetLogReporting(bool b, bool shObjects)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		logReporting = b;
		if (shObjects) {
			for (IShaderObject*& so : shaderObjs) {
				so->SetLogReporting(b);
			}
		}
	}

	void IProgramObject::Release() {
		RECOIL_DETAILED_TRACY_ZONE;
		for (IShaderObject*& so: shaderObjs) {
			so->Release();
			delete so;
		}

		uniformStates.clear();
		shaderObjs.clear();
		luaTextures.clear();
		log.clear();

		valid = false;
	}

	void IProgramObject::AttachShaderObject(IShaderObject* so)
	{
#ifdef _DEBUG
	#ifndef HEADLESS
		const auto it = std::find_if(shaderObjs.cbegin(), shaderObjs.cend(), [type = so->GetType()](Shader::IShaderObject* so) {
			return so->GetType() == type;
		});
		assert(it == shaderObjs.cend());
	#endif
#endif
		shaderObjs.push_back(so);
	}

	bool IProgramObject::RemoveShaderObject(GLenum soType) {
		RECOIL_DETAILED_TRACY_ZONE;
		for (size_t i = 0; i < shaderObjs.size(); ++i) {
			IShaderObject*& so = shaderObjs[i];
			if (so->GetType() == soType) {
				so->Release();
				delete so;

				shaderObjs.erase(shaderObjs.begin() + i);
				return true;
			}
		}

		return false;
	}

	void IProgramObject::Enable()
	{
		RECOIL_DETAILED_TRACY_ZONE;
		assert(!bound);
		shaderHandler->SetCurrentlyBoundProgram(this);
		bound = true;
	}

	void IProgramObject::Disable()
	{
		RECOIL_DETAILED_TRACY_ZONE;
		assert(bound);
		shaderHandler->SetCurrentlyBoundProgram(nullptr);
		bound = false;
	}

	bool IProgramObject::LoadFromLua(const std::string& filename) {
		RECOIL_DETAILED_TRACY_ZONE;
		return Shader::LoadFromLua(this, filename);
	}

	void IProgramObject::RecompileIfNeeded(bool validate)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		if (shaderFlags.HashSet() && !shaderFlags.Updated())
			return;

		Reload(!shaderFlags.HashSet(), validate);
		PrintDebugInfo();
	}

	void IProgramObject::PrintDebugInfo()
	{
	#if DEBUG
		LOG_L(L_DEBUG, "Uniform States for program-object \"%s\":", name.c_str());
		LOG_L(L_DEBUG, "Defs:\n %s", (shaderFlags.GetString()).c_str());
		LOG_L(L_DEBUG, "Uniforms:");

		for (const auto& p : uniformStates) {
			const bool curUsed = GetUniformLocation(p.second.GetName()) >= 0;
			if (!p.second.IsInitialized()) {
				LOG_L(L_DEBUG, "\t%s: uninitialized used=%i", (p.second.GetName()), int(curUsed));
			} else {
				LOG_L(L_DEBUG, "\t%s: x=float:%f;int:%i y=%f z=%f used=%i", (p.second.GetName()), p.second.GetFltValues()[0], p.second.GetIntValues()[0], p.second.GetFltValues()[1], p.second.GetFltValues()[2], int(curUsed));
			}
		}
	#endif
	}

	UniformState* IProgramObject::GetNewUniformState(const char* name)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		const auto hash = hashString(name);
		const auto it = uniformStates.emplace(hash, UniformState{name});

		UniformState* us = &(it.first->second);
		us->SetLocation(GetUniformLoc(name));

		return us;
	}


	void IProgramObject::AddTextureBinding(const int texUnit, const std::string& luaTexName)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		LuaMatTexture luaTex;

		if (!LuaOpenGLUtils::ParseTextureImage(nullptr, luaTex, luaTexName))
			return;

		luaTextures[texUnit] = luaTex;
	}

	void IProgramObject::BindTextures() const
	{
		RECOIL_DETAILED_TRACY_ZONE;
		for (const auto& p: luaTextures) {
			glActiveTexture(GL_TEXTURE0 + p.first);
			(p.second).Bind();
		}
		glActiveTexture(GL_TEXTURE0);
	}




	/*****************************************************************/

	ARBProgramObject::ARBProgramObject(const std::string& poName): IProgramObject(poName) {
		objID = -1; // not used for ARBProgramObject instances
		uniformTarget = -1;
	}

	void ARBProgramObject::SetUniformTarget(int target) {
		RECOIL_DETAILED_TRACY_ZONE;
		uniformTarget = target;
	}
	int ARBProgramObject::GetUnitformTarget() {
		RECOIL_DETAILED_TRACY_ZONE;
		return uniformTarget;
	}

	void ARBProgramObject::Enable() {
		RECOIL_DETAILED_TRACY_ZONE;
		RecompileIfNeeded(true);
		for (const IShaderObject* so: shaderObjs) {
			glEnable(so->GetType());
			glBindProgramARB(so->GetType(), so->GetObjID());
		}
		IProgramObject::Enable();
	}
	void ARBProgramObject::Disable() {
		RECOIL_DETAILED_TRACY_ZONE;
		for (const IShaderObject* so: shaderObjs) {
			glBindProgramARB(so->GetType(), 0);
			glDisable(so->GetType());
		}
		IProgramObject::Disable();
	}

	void ARBProgramObject::Link() {
		RECOIL_DETAILED_TRACY_ZONE;
		RecompileIfNeeded(false);
		valid = true;

		for (const IShaderObject* so: shaderObjs) {
			valid &= so->IsValid();
		}
	}
	void ARBProgramObject::Reload(bool reloadFromDisk, bool validate) {
		RECOIL_DETAILED_TRACY_ZONE;
		for (IShaderObject* so: GetAttachedShaderObjs()) {
			if (reloadFromDisk) so->ReloadFromDisk();
			so->Compile();
		}

		// make HashSet() true
		shaderFlags.UpdateHash();

		if (validate) Validate();
	}


	void ARBProgramObject::SetUniform1i(int idx, int   v0                              ) { glProgramEnvParameter4fARB(uniformTarget, idx, float(v0), float( 0), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { glProgramEnvParameter4fARB(uniformTarget, idx, float(v0), float(v1), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { glProgramEnvParameter4fARB(uniformTarget, idx, float(v0), float(v1), float(v2), float( 0)); }
	void ARBProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { glProgramEnvParameter4fARB(uniformTarget, idx, float(v0), float(v1), float(v2), float(v3)); }
	void ARBProgramObject::SetUniform1f(int idx, float v0                              ) { glProgramEnvParameter4fARB(uniformTarget, idx, v0, 0.0f, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { glProgramEnvParameter4fARB(uniformTarget, idx, v0,   v1, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { glProgramEnvParameter4fARB(uniformTarget, idx, v0,   v1,   v2, 0.0f); }
	void ARBProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { glProgramEnvParameter4fARB(uniformTarget, idx, v0,   v1,   v2,   v3); }

	void ARBProgramObject::SetUniform2iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] =    0; vv[3] =    0; glProgramEnvParameter4fvARB(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform3iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] =    0; glProgramEnvParameter4fvARB(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform4iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glProgramEnvParameter4fvARB(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform2fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = 0.0f; vv[3] = 0.0f; glProgramEnvParameter4fvARB(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform3fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = 0.0f; glProgramEnvParameter4fvARB(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform4fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glProgramEnvParameter4fvARB(uniformTarget, idx,          vv); }




	/*****************************************************************/

	GLSLProgramObject::GLSLProgramObject(const std::string& poName): IProgramObject(poName), curSrcHash(0) {
		RECOIL_DETAILED_TRACY_ZONE;
		objID = glCreateProgram();
	}

	void GLSLProgramObject::BindAttribLocation(const std::string& name, uint32_t index)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		attribLocations[name] = index;
	}

	void GLSLProgramObject::BindOutputLocation(const std::string& name, uint32_t index)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		outputLocations[name] = index;
	}

	void GLSLProgramObject::Enable() {
		RECOIL_DETAILED_TRACY_ZONE;
		RecompileIfNeeded(true);
		EnableRaw();
	}

	void GLSLProgramObject::EnableRaw() {
		RECOIL_DETAILED_TRACY_ZONE;
		glUseProgram(objID);
		IProgramObject::Enable();
	}
	void GLSLProgramObject::DisableRaw() {
		RECOIL_DETAILED_TRACY_ZONE;
		IProgramObject::Disable();
		glUseProgram(0);
	}

	void GLSLProgramObject::Link() {
		RECOIL_DETAILED_TRACY_ZONE;
		RecompileIfNeeded(false);
		assert(glIsProgram(objID));
	}

	bool GLSLProgramObject::Validate() {
		RECOIL_DETAILED_TRACY_ZONE;
		GLint validated = 0;

		glValidateProgram(objID);
		glGetProgramiv(objID, GL_VALIDATE_STATUS, &validated);
		valid = bool(validated);

		// append the validation-log
		log += glslGetLog(objID);

		const auto ReturnHelper = [this](const char* fn) -> bool {
			if (!valid && logReporting)
				LOG_L(L_ERROR, "[GLSL-PO::%s] program-object name: %s is not valid. Log:\n%s", fn, name.c_str(), log.c_str());
			return valid;
		};

	#ifdef _DEBUG
		{
			// check if there are unset uniforms left
			GLsizei numUniforms, maxUniformNameLength;
			glGetProgramiv(objID, GL_ACTIVE_UNIFORMS, &numUniforms);
			glGetProgramiv(objID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);

			if (maxUniformNameLength <= 0)
				return ReturnHelper(__func__);

			std::string bufname(maxUniformNameLength, 0);
			for (uint32_t i = 0; i < numUniforms; ++i) {
				GLsizei nameLength = 0;
				GLint size = 0;
				GLint blockIdx = -1;
				GLenum type = 0;
				glGetActiveUniform(objID, i, maxUniformNameLength, &nameLength, &size, &type, &bufname[0]);
				bufname[nameLength] = 0;

				glGetActiveUniformsiv(objID, 1, &i, GL_UNIFORM_BLOCK_INDEX, &blockIdx);

				if (nameLength == 0)
					continue;

				if (strncmp(&bufname[0], "gl_", 3) == 0)
					continue;

				if (uniformStates.find(hashString(&bufname[0])) != uniformStates.end())
					continue;

				if (blockIdx >= 0) //ignore UBO
					continue;

				if (logReporting)
					LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, unset uniform: %s", __func__, name.c_str(), &bufname[0]);
			}
		}
		{
			GLsizei numAttributes, maxNameLength = 0;
			glGetProgramiv(objID, GL_ACTIVE_ATTRIBUTES, &numAttributes);
			glGetProgramiv(objID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);

			if (maxNameLength <= 0)
				return ReturnHelper(__func__);

			std::string bufname(maxNameLength, 0);
			for (int i = 0; i < numAttributes; ++i) {
				GLsizei nameLength = 0;
				GLint size = 0;
				GLenum type = 0;
				glGetActiveAttrib(objID, i, maxNameLength, &nameLength, &size, &type, &bufname[0]);
				bufname[maxNameLength - 1] = 0;

				if (nameLength == 0)
					continue;

				if (strncmp(&bufname[0], "gl_", 3) == 0)
					continue;

				GLint loc = glGetAttribLocation(objID, &bufname[0]);

				if (loc < 0) {
					if (logReporting)
						LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, atttrib: %s has undefined location", __func__, name.c_str(), &bufname[0]);
					valid = false;
				}
			}
		}
	#endif

		return ReturnHelper(__func__);
	}

	void GLSLProgramObject::Release() {
		RECOIL_DETAILED_TRACY_ZONE;
		IProgramObject::Release();
		glDeleteProgram(objID);
		shaderFlags.Clear();

		objID = 0;
		curSrcHash = 0;
	}

	void GLSLProgramObject::Reload(bool reloadFromDisk, bool validate) {
		RECOIL_DETAILED_TRACY_ZONE;
		const unsigned int oldProgID = objID;
		const unsigned int oldSrcHash = curSrcHash;

		const bool oldValid = IsValid();
		valid = false;

		{
			// NOTE: this does not preserve the #version pragma
			for (IShaderObject*& so: shaderObjs) {
				so->SetDefinitions(shaderFlags.GetString());
			}

			log.clear();
		}

		// reload shader from disk if requested or necessary
		if ((reloadFromDisk || !oldValid || (oldProgID == 0))) {
			for (IShaderObject*& so: shaderObjs) {
				so->ReloadFromDisk();
			}
		}

		{
			// create shader source hash
			curSrcHash = shaderFlags.UpdateHash();

			for (const IShaderObject* so: shaderObjs) {
				curSrcHash ^= so->GetHash();
			}
		}

		// early-exit: empty program (TODO: delete it?)
		if (shaderObjs.empty()) {
			if (logReporting) {
				LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, shader objects list is empty\n", __func__, name.c_str());
			}
			return;
		}

		bool deleteOldShader = true;

		{
			objID = 0;

			// push old program to cache and pop new if available
			if (oldValid && configHandler->GetBool("UseShaderCache")) {
				CShaderHandler::ShaderCache& shadersCache = shaderHandler->GetShaderCache();
				deleteOldShader = !shadersCache.Push(oldSrcHash, oldProgID);
				objID = shadersCache.Find(curSrcHash);
			}
		}

		// recompile if not found in cache (id 0)
		if (objID == 0) {
			objID = glCreateProgram();

			bool shadersValid = true;
			for (IShaderObject*& so: shaderObjs) {
				assert(dynamic_cast<GLSLShaderObject*>(so));

				auto gso = static_cast<GLSLShaderObject*>(so);
				auto obj = gso->CompileShaderObject();

				if (obj->valid) {
					glAttachShader(objID, obj->id);
				} else {
					shadersValid = false;
				}
			}

			if (!shadersValid)
				return;

			for (const auto& [name, index] : attribLocations) {
				glBindAttribLocation(objID, index, name.c_str());
			}

			for (const auto& [name, index] : outputLocations) {
				glBindFragDataLocation(objID, index, name.c_str());
			}

			glLinkProgram(objID);

			valid = glslIsValid(objID);
			log += glslGetLog(objID);

			if (!IsValid() && logReporting) {
				LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, link-log:\n%s\n", __func__, name.c_str(), log.c_str());
			}

			#ifdef _DEBUG
			if (IsValid()) {
				for (const auto& [name, index] : attribLocations) {
					GLint indexOut = glGetAttribLocation(objID, name.c_str());
					if (indexOut == -1) {
						LOG_L(L_WARNING, "[GLSL-PO::%s] Attribute %s for program %u is unused(-1)", __func__, name.c_str(), objID);
					} 
					else if (indexOut != index) {
						LOG_L(L_ERROR, "[GLSL-PO::%s] Setting attribute %s to location %d(requested %d) for program %u", __func__, name.c_str(), indexOut, index, objID);
						assert(false);
					}
				}
			}
			#endif

		} else {
			valid = true;
		}

		{
			GLsizei numUniforms, maxUniformNameLength;
			glGetProgramiv(objID, GL_ACTIVE_UNIFORMS, &numUniforms);
			glGetProgramiv(objID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);

			if (maxUniformNameLength > 0) {
				std::string bufname(maxUniformNameLength, 0);
				for (int i = 0; i < numUniforms; ++i) {
					GLsizei nameLength = 0;
					GLint size = 0;
					GLenum type = 0;
					glGetActiveUniform(objID, i, maxUniformNameLength, &nameLength, &size, &type, &bufname[0]);
					bufname[nameLength] = 0;

					if (nameLength == 0)
						continue;

					if (strncmp(bufname.c_str(), "gl_", 3) == 0)
						continue;

					// clear all uniform locations
					GetUniformState(bufname)->SetLocation(GL_INVALID_INDEX);
				}
			}
		}

		// FIXME: fails on ATI, see https://springrts.com/mantis/view.php?id=4715
		if (validate && (!globalRendering->haveAMD && !globalRendering->haveIntel))
			Validate();

		// copy full program state from old to new program (uniforms etc.)
		if (IsValid())
			GLSLCopyState(objID, oldValid ? oldProgID : 0, &uniformStates);

		// delete old program when not further used
		if (deleteOldShader)
			glDeleteProgram(oldProgID);
	}

	int GLSLProgramObject::GetUniformType(const int idx) {
		GLint size = 0;
		GLenum type = 0;
		// NB: idx can not be a *location* returned by glGetUniformLoc except on Nvidia
		glGetActiveUniform(objID, idx, 0, nullptr, &size, &type, nullptr);
		assert(size == 1); // arrays aren't handled yet
		return type;
	}

	int GLSLProgramObject::GetUniformLoc(const char* name) {
		return glGetUniformLocation(objID, name);
	}

	void GLSLProgramObject::SetUniformLocation(const std::string& name) {
		uniformLocs.push_back(hashString(name.c_str()));
		GetUniformLocation(name);
	}

	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0)                               { assert(IsBound()); if (uState->Set(v0            )) glUniform1i(uState->GetLocation(), v0             ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0)                               { assert(IsBound()); if (uState->Set(v0            )) glUniform1f(uState->GetLocation(), v0             ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1)                     { assert(IsBound()); if (uState->Set(v0, v1        )) glUniform2i(uState->GetLocation(), v0, v1         ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1)                     { assert(IsBound()); if (uState->Set(v0, v1        )) glUniform2f(uState->GetLocation(), v0, v1         ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1, int   v2)           { assert(IsBound()); if (uState->Set(v0, v1, v2    )) glUniform3i(uState->GetLocation(), v0, v1, v2     ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1, float v2)           { assert(IsBound()); if (uState->Set(v0, v1, v2    )) glUniform3f(uState->GetLocation(), v0, v1, v2     ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1, int   v2, int   v3) { assert(IsBound()); if (uState->Set(v0, v1, v2, v3)) glUniform4i(uState->GetLocation(), v0, v1, v2, v3 ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1, float v2, float v3) { assert(IsBound()); if (uState->Set(v0, v1, v2, v3)) glUniform4f(uState->GetLocation(), v0, v1, v2, v3 ); }

	void GLSLProgramObject::SetUniform1v(UniformState* uState, GLsizei count, const int*   v) { assert(IsBound()); if (uState->Set2v(v)) glUniform1iv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform1v(UniformState* uState, GLsizei count, const float* v) { assert(IsBound()); if (uState->Set2v(v)) glUniform1fv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform2v(UniformState* uState, GLsizei count, const int*   v) { assert(IsBound()); if (uState->Set2v(v)) glUniform2iv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform2v(UniformState* uState, GLsizei count, const float* v) { assert(IsBound()); if (uState->Set2v(v)) glUniform2fv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform3v(UniformState* uState, GLsizei count, const int*   v) { assert(IsBound()); if (uState->Set3v(v)) glUniform3iv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform3v(UniformState* uState, GLsizei count, const float* v) { assert(IsBound()); if (uState->Set3v(v)) glUniform3fv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform4v(UniformState* uState, GLsizei count, const int*   v) { assert(IsBound()); if (uState->Set4v(v)) glUniform4iv(uState->GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform4v(UniformState* uState, GLsizei count, const float* v) { assert(IsBound()); if (uState->Set4v(v)) glUniform4fv(uState->GetLocation(), count, v); }

	void GLSLProgramObject::SetUniformMatrix2x2(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set2x2(v, transp)) glUniformMatrix2fv(uState->GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3x3(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set3x3(v, transp)) glUniformMatrix3fv(uState->GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4x4(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set4x4(v, transp)) glUniformMatrix4fv(uState->GetLocation(), 1, transp, v); }

	void GLSLProgramObject::SetUniform1i(int idx, int   v0                              ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0            )) glUniform1i(it->second.GetLocation(), v0            ); }
	void GLSLProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1        )) glUniform2i(it->second.GetLocation(), v0, v1        ); }
	void GLSLProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2    )) glUniform3i(it->second.GetLocation(), v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2, v3)) glUniform4i(it->second.GetLocation(), v0, v1, v2, v3); }
	void GLSLProgramObject::SetUniform1f(int idx, float v0                              ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0            )) glUniform1f(it->second.GetLocation(), v0            ); }
	void GLSLProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1        )) glUniform2f(it->second.GetLocation(), v0, v1        ); }
	void GLSLProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2    )) glUniform3f(it->second.GetLocation(), v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2, v3)) glUniform4f(it->second.GetLocation(), v0, v1, v2, v3); }

	void GLSLProgramObject::SetUniform2iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform2fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2fv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3fv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4fv(it->second.GetLocation(), 1, v); }

	/// variants with count param
	void GLSLProgramObject::SetUniform1iv(int idx, const GLsizei count, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2iv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform2iv(int idx, const GLsizei count, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2iv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform3iv(int idx, const GLsizei count, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3iv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform4iv(int idx, const GLsizei count, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4iv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform1fv(int idx, const GLsizei count, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2fv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform2fv(int idx, const GLsizei count, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2fv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform3fv(int idx, const GLsizei count, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3fv(it->second.GetLocation(), count, v); }
	void GLSLProgramObject::SetUniform4fv(int idx, const GLsizei count, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4fv(it->second.GetLocation(), count, v); }

	void GLSLProgramObject::SetUniformMatrix2fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2x2(v, transp)) glUniformMatrix2fv(it->second.GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3x3(v, transp)) glUniformMatrix3fv(it->second.GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4x4(v, transp)) glUniformMatrix4fv(it->second.GetLocation(), 1, transp, v); }
	ShaderEnabledToken::ShaderEnabledToken(IProgramObject* prog_)
		: prog(prog_)
	{
		if (prog)
			prog->Enable();
	}
	ShaderEnabledToken::~ShaderEnabledToken()
	{
		if (prog)
			prog->Disable();
	}
}
