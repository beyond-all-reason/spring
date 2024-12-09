/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTURE_FORMAT_H
#define TEXTURE_FORMAT_H

#include "Rendering/GL/myGL.h"

namespace GL
{
	GLenum GetInternalFormatDataFormat(GLenum internalFormat);

	GLenum GetInternalFormatDataType(GLenum internalFormat);

	// a convenient data type to store pixel component
	GLenum GetInternalFormatUserType(GLenum internalFormat);

	GLenum GetBindingQueryFromTarget(GLenum target);

	GLsizei GetPixelFormatSize(GLenum format);
}

#endif // TEXTURE_FORMAT_H
