/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaTextures.h"
#include "Rendering/Textures/TextureFormat.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/TexBind.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "Lua/LuaFBOs.h"

#include "fmt/format.h"


namespace Impl {
	static inline bool IsValidLuaTextureTarget(GLenum target) {
		switch(target) {
			case GL_TEXTURE_1D:
			case GL_TEXTURE_2D:
			case GL_TEXTURE_3D:
			//case GL_TEXTURE_1D_ARRAY:
			case GL_TEXTURE_2D_ARRAY:
			//case GL_TEXTURE_RECTANGLE:
			case GL_TEXTURE_CUBE_MAP:
			//case GL_TEXTURE_BUFFER:
			case GL_TEXTURE_2D_MULTISAMPLE:
			//case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
				return true;
			default: break;
		}
		return false;
	}
}

/******************************************************************************/
/******************************************************************************/

std::string LuaTextures::Create(const Texture& tex)
{
	GLenum query = 0;
	if (Impl::IsValidLuaTextureTarget(tex.target)) {
		query = GL::GetBindingQueryFromTarget(tex.target);
	}
	if (!query) {
		LOG_L(L_ERROR, "[LuaTextures::%s] texture-target %d is not supported", __func__, tex.target);
		return "";
	}

	GLint currentBinding;
	glGetIntegerv(query, &currentBinding);

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(tex.target, texID);

	glClearErrors("LuaTex", __func__, globalRendering->glDebugErrors);

	GLenum dataFormat = GL::GetInternalFormatDataFormat(tex.format);
	GLenum dataType   = GL::GetInternalFormatDataType(tex.format);

	switch (tex.target) {
		case GL_TEXTURE_1D: {
			glTexImage1D(tex.target, 0, tex.format, tex.xsize                      , tex.border, dataFormat, dataType, nullptr);
		} break;
		case GL_TEXTURE_2D: {
			glTexImage2D(tex.target, 0, tex.format, tex.xsize, tex.ysize           , tex.border, dataFormat, dataType, nullptr);
		} break;
		case GL_TEXTURE_2D_ARRAY: [[fallthrough]];
		case GL_TEXTURE_3D: {
			glTexImage3D(tex.target, 0, tex.format, tex.xsize, tex.ysize, tex.zsize, tex.border, dataFormat, dataType, nullptr);
		} break;

		case GL_TEXTURE_2D_MULTISAMPLE: {
			assert(tex.samples > 1);

			// 2DMS target only makes sense for FBO's
			if (!globalRendering->supportMSAAFrameBuffer) {
				glDeleteTextures(1, &texID);
				glBindTexture(tex.target, currentBinding);
				return "";
			}

			glTexImage2DMultisample(tex.target, tex.samples, tex.format, tex.xsize, tex.ysize, GL_TRUE);
		} break;

		default: {
			assert(false);
		} break;
	}

	if (glGetError() != GL_NO_ERROR) {
		glDeleteTextures(1, &texID);
		glBindTexture(tex.target, currentBinding);
		return "";
	}

	ApplyParams(tex);

	glBindTexture(tex.target, currentBinding); // revert the current binding

	GLuint fbo = 0;
	GLuint fboDepth = 0;

	if (tex.fbo != 0) {
		if (!FBO::IsSupported()) {
			glDeleteTextures(1, &texID);
			return "";
		}

		GLint currentFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);

		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		if (tex.fboDepth != 0) {
			glGenRenderbuffersEXT(1, &fboDepth);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fboDepth);
			GLenum depthFormat = static_cast<GLenum>(CGlobalRendering::DepthBitsToFormat(globalRendering->supportDepthBufferBitDepth));
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthFormat, tex.xsize, tex.ysize);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fboDepth);
		}

		bool attachFailure = false;
		try {
			LuaFBOs::AttachObjectTexTarget(__func__, GL_FRAMEBUFFER_EXT, tex.target, texID, GL_COLOR_ATTACHMENT0_EXT, 0);
		}
		catch (const opengl_error& e) {
			attachFailure = true;
			LOG_L(L_ERROR, "[LuaTextures::%s] %s", __func__, e.what());
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT || attachFailure) {
			glDeleteTextures(1, &texID);
			glDeleteFramebuffersEXT(1, &fbo);
			glDeleteRenderbuffersEXT(1, &fboDepth);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
			return "";
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
	}

	std::string str = fmt::format("{}{}", prefix, ++lastCode);

	Texture newTex = tex;
	newTex.id = texID;
	newTex.fbo = fbo;
	newTex.fboDepth = fboDepth;

	if (freeIndices.empty()) {
		textureMap.emplace(str, textureVec.size());
		textureVec.emplace_back(newTex);
		return str;
	}

	// recycle
	textureMap[str] = freeIndices.back();
	textureVec[freeIndices.back()] = newTex;
	freeIndices.pop_back();
	return str;
}


bool LuaTextures::Bind(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		glBindTexture(tex.target, tex.id);
		return true;
	}

	return false;
}


bool LuaTextures::Free(const std::string& name)
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		glDeleteTextures(1, &tex.id);

		if (FBO::IsSupported()) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}

		freeIndices.push_back(it->second);
		textureMap.erase(it);
		return true;
	}

	return false;
}


bool LuaTextures::FreeFBO(const std::string& name)
{
	if (!FBO::IsSupported())
		return false;

	const auto it = textureMap.find(name);

	if (it == textureMap.end())
		return false;

	Texture& tex = textureVec[it->second];

	glDeleteFramebuffersEXT(1, &tex.fbo);
	glDeleteRenderbuffersEXT(1, &tex.fboDepth);

	tex.fbo = 0;
	tex.fboDepth = 0;
	return true;
}


void LuaTextures::FreeAll()
{
	for (const auto& item: textureMap) {
		const Texture& tex = textureVec[item.second];
		glDeleteTextures(1, &tex.id);

		if (FBO::IsSupported()) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}
	}

	textureMap.clear();
	textureVec.clear();
	freeIndices.clear();
}


void LuaTextures::ApplyParams(const Texture& tex) const
{
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_S, tex.wrap_s);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_T, tex.wrap_t);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_R, tex.wrap_r);
	glTexParameteri(tex.target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
	glTexParameteri(tex.target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);
	if (tex.cmpFunc != GL_NONE) {
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_FUNC, tex.cmpFunc);
	}
	else {
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); //sensible default
	}

	if (tex.lodBias != 0.0f)
		glTexParameterf(tex.target, GL_TEXTURE_LOD_BIAS, tex.lodBias);

	if (tex.aniso != 0.0f && GLAD_GL_EXT_texture_filter_anisotropic)
		glTexParameterf(tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::clamp(tex.aniso, 1.0f, globalRendering->maxTexAnisoLvl));
}

void LuaTextures::ChangeParams(const Texture& tex)  const
{
	auto texBind = GL::TexBind(tex.target, tex.id);
	ApplyParams(tex);
}


size_t LuaTextures::GetIdx(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end())
		return (it->second);

	return (size_t(-1));
}

