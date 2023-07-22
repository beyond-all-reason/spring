/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "Rendering/Textures/TextureFormat.h"


inline GLint glTempBindTexture(GLenum target, GLuint textureID);



inline GLint glTempBindTexture(GLenum target, GLuint textureID) {
	GLenum query = GL::GetBindingQueryFromTarget(target);
	assert(query);
	GLint currentBinding;
	glGetIntegerv(query, &currentBinding);
	glBindTexture(target, textureID);
	return currentBinding;
}
