/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTURE_FORMAT_H
#define TEXTURE_FORMAT_H

#include "Rendering/GL/myGL.h"
#include "System/UnorderedMap.hpp"

namespace GL
{
	GLenum GetInternalFormatDataFormat(GLenum internalFormat);

	GLenum GetInternalFormatDataType(GLenum internalFormat);

	inline GLenum GetBindingQueryFromTarget(GLenum target);
}



namespace GL::Impl { extern const spring::unordered_map<GLenum, GLenum> TargetToBindingQuery; }

inline GLenum GL::GetBindingQueryFromTarget(GLenum target)
{
	const auto it = Impl::TargetToBindingQuery.find(target);
	return (it != Impl::TargetToBindingQuery.end())? it->second : 0;
}

#endif // TEXTURE_FORMAT_H
