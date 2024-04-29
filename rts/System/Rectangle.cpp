/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rectangle.h"

CR_BIND(SRectangle, )
CR_REG_METADATA(SRectangle, (
	CR_MEMBER(x1),
	CR_MEMBER(y1),
	CR_MEMBER(x2),
	CR_MEMBER(y2)
))

int SRectangle::OverlapArea(SRectangle&& r) const
{
	return
		std::max(0, std::min(x2, r.x2) - std::max(x1, r.x1)) *
		std::max(0, std::min(y2, r.y2) - std::max(y1, r.y1));
}

int SRectangle::OverlapArea(const SRectangle& r) const
{
	return
		std::max(0, std::min(x2, r.x2) - std::max(x1, r.x1)) *
		std::max(0, std::min(y2, r.y2) - std::max(y1, r.y1));
}