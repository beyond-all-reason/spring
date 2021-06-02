#pragma once

#include <cstdint>
#include <utility>

#if       defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
	#include <GL/glew.h>
#endif // defined(HEADLESS)

namespace GL {
	static void UnbindTexture() { BindTexture(0u, 0u); }
	static void UnbindTexture(GLuint texUnitIdx) { BindTexture(texUnitIdx, 0u); }
	static void UnbindTexture(GLuint texUnitIdx, GLenum target) { BindTexture(texUnitIdx, target, 0u); }

	static void UnbindTextures(GLuint texUnitIdxStart, GLsizei count, GLenum target);

	static void BindTexture(GLuint texture) { BindTexture(0u, texture); }
	static void BindTexture(GLuint texUnitIdx, GLuint texture) { BindTexture(texUnitIdx, GL_TEXTURE_2D, texture); }
	static void BindTexture(GLuint texUnitIdx, GLenum target, const GLuint texture);

	static void BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLenum target, const GLuint* textures);
	static void BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLenum* targets, const GLuint* textures);
	static void BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLuint* textures) { BindTextures(texUnitIdxStart, GL_TEXTURE_2D, textures); }

	static void TexStorage2D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height);

	template <typename TargFunc>
	static void BindTexturesCommon(GLuint texUnitIdxStart, GLsizei count, const TargFunc& targFunc, const GLuint* textures);
};