/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "glHelpers.h"

namespace GL
{

// A texture bind control object, that automatically unbinds texture and restores active tex unit upon destruction
// When constructed without exact slot, will also restore the previously bound texture to the contemporary slot
class TexBind {
public:
	inline TexBind(unsigned slot, GLenum target, GLuint textureID)
	:	stateTexUnit(GL::FetchEffectualStateAttribValue<GLenum>(GL_ACTIVE_TEXTURE)),
		texUnit(GL_TEXTURE0+slot),
		target(target),
		restoredTextureID(0)
	{
		if (stateTexUnit != texUnit) glActiveTexture(texUnit);
		glBindTexture(target, textureID);
	}
	inline TexBind(GLenum target, GLuint textureID)
	:	stateTexUnit(GL::FetchEffectualStateAttribValue<GLenum>(GL_ACTIVE_TEXTURE)),
		texUnit(stateTexUnit),
		target(target),
		restoredTextureID(GL::FetchCurrentSlotTextureID(target))
	{
		glBindTexture(target, textureID);
	}

	inline ~TexBind()
	{
		glActiveTexture(texUnit);
		glBindTexture(target, restoredTextureID);
		if (stateTexUnit != texUnit) glActiveTexture(stateTexUnit);
	}

private:
	const GLenum stateTexUnit, texUnit, target;
	const GLuint restoredTextureID;
};

}
