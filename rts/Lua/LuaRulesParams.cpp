/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaRulesParams.h"
#include "System/creg/STL_Variant.h"

using namespace LuaRulesParams;

CR_BIND(Param,)
CR_REG_METADATA(Param, (
	CR_MEMBER(los),
	CR_MEMBER(value)
))
