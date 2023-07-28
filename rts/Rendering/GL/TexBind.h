/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "Rendering/GlobalRendering.h"

namespace GL
{

class TexBind {
public:
	inline TexBind(int slot, GLenum target, GLuint textureID)
	:	texUnit(GL_TEXTURE0 +GLenum(slot >= 0? slot : globalRendering->maxCombShSlots+slot)),
		target(target)
	{
		GLint iStateTexUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &iStateTexUnit);
		stateTexUnit = (GLenum)iStateTexUnit;
		if (stateTexUnit != texUnit) glActiveTexture(texUnit);
		glBindTexture(target, textureID);
	}
	inline TexBind(GLenum target, GLuint textureID)
	:	TexBind(-1, target, textureID)
	{}

	inline ~TexBind()
	{
		glActiveTexture(texUnit);
		glBindTexture(target, 0);
		if (stateTexUnit != texUnit) glActiveTexture(stateTexUnit);
	}

private:
	GLenum stateTexUnit, texUnit, target;
};

}
