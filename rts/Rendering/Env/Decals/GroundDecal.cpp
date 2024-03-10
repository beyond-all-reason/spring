#include "GroundDecal.h"

#define VA_ATTR_DEF(T, idx, count, type, member, normalized, name) AttributeDef(idx, count, type, sizeof(T), VA_TYPE_OFFSET(T, member), normalized, name)

decltype(GroundDecal::attributeDefs) GroundDecal::attributeDefs = {
	VA_ATTR_DEF(GroundDecal,  0, 4, GL_FLOAT, refHeight     , false, "forcedHeight"),
	VA_ATTR_DEF(GroundDecal,  1, 4, GL_FLOAT, posTL         , false, "posT"),
	VA_ATTR_DEF(GroundDecal,  2, 4, GL_FLOAT, posBR         , false, "posB"),
	VA_ATTR_DEF(GroundDecal,  3, 4, GL_FLOAT, texMainOffsets, false, "uvMain"),
	VA_ATTR_DEF(GroundDecal,  4, 4, GL_FLOAT, texNormOffsets, false, "uvNorm"),
	VA_ATTR_DEF(GroundDecal,  5, 4, GL_FLOAT, alpha         , false, "createParams1"),
	VA_ATTR_DEF(GroundDecal,  6, 4, GL_FLOAT, rot           , false, "createParams2"),
	VA_ATTR_DEF(GroundDecal,  7, 4, GL_FLOAT, createFrameMin, false, "createParams3"),
	VA_ATTR_DEF(GroundDecal,  8, 4, GL_FLOAT, forcedNormal  , false, "createParams4"),
	VA_ATTR_DEF(GroundDecal,  9, 4, GL_UNSIGNED_INT, info   , false, "createParams5")
};

#undef VA_ATTR_DEF