/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Resource.h"

#include <float.h>

CR_BIND(CResourceDescription, )
CR_REG_METADATA(CResourceDescription, (
	CR_MEMBER(name),
	CR_MEMBER(description),
	CR_MEMBER(optimum)
))


CR_BIND(SResourcePack,)
CR_REG_METADATA(SResourcePack,(
	CR_MEMBER(res)
))


CR_BIND(SResourceOrder,)
CR_REG_METADATA(SResourceOrder,(
	CR_MEMBER(use),
	CR_MEMBER(add),
	CR_MEMBER(quantum),
	CR_MEMBER(overflow)
))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResourceDescription::CResourceDescription()
: name("UNNAMED_RESOURCE")
, optimum(FLT_MAX)
{
}

CResourceDescription::~CResourceDescription()
{
}
