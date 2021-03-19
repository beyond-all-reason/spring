#ifndef COMMON_GL_H
#define COMMON_GL_H

#include <cstdint>
#include <utility>

#if       defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
	#include <GL/glew.h>
#endif // defined(HEADLESS)


class CommonGL {
public:
	static void UnbindTexture() { CommonGL::BindTexture(0u, 0u); }
	static void UnbindTexture(const GLuint texUnitIdx) { CommonGL::BindTexture(texUnitIdx, 0u); }
	static void UnbindTexture(const GLuint texUnitIdx, const GLenum target) { CommonGL::BindTexture(texUnitIdx, target, 0u); }

	static void UnbindTextures(const GLuint texUnitIdxStart, const GLsizei count, const GLenum target);

	static void BindTexture(const GLuint texture) { CommonGL::BindTexture(0u, texture); }
	static void BindTexture(const GLuint texUnitIdx, const GLuint texture) { CommonGL::BindTexture(texUnitIdx, GL_TEXTURE_2D, texture); }
	static void BindTexture(const GLuint texUnitIdx, const GLenum target, const GLuint texture);

	static void BindTextures(const GLuint texUnitIdxStart, const GLsizei count, const GLenum target, const GLuint* textures);
	static void BindTextures(const GLuint texUnitIdxStart, const GLsizei count, const GLenum* targets, const GLuint* textures);
	static void BindTextures(const GLuint texUnitIdxStart, const GLsizei count, const GLuint* textures) { CommonGL::BindTextures(texUnitIdxStart, GL_TEXTURE_2D, textures); }

	static void TexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height);
private:
	template <typename TargFunc>
	static void BindTexturesCommon(const GLuint texUnitIdxStart, const GLsizei count, const TargFunc& targFunc, const GLuint* textures);
};

#define ALIAS_TEMPLATE_FUNCTION(FBase, FAlias) \
template<typename... Args> \
inline auto FBase(Args&&... args) -> decltype(FAlias(std::forward<Args>(args)...)) \
{ \
    return FAlias(std::forward<Args>(args)...); \
}

#define ALIAS_TEMPLATE_FUNCTION_HERE(BaseFunc) ALIAS_TEMPLATE_FUNCTION(BaseFunc, CommonGL::BaseFunc)

namespace GL {
	ALIAS_TEMPLATE_FUNCTION_HERE(UnbindTexture);
	ALIAS_TEMPLATE_FUNCTION_HERE(BindTexture);
	ALIAS_TEMPLATE_FUNCTION_HERE(BindTextures);
	ALIAS_TEMPLATE_FUNCTION_HERE(TexStorage2D);
}

#undef ALIAS_TEMPLATE_FUNCTION_HERE
#undef ALIAS_TEMPLATE_FUNCTION

#endif //COMMON_GL_H