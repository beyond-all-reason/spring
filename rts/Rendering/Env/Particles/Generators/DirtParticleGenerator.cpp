#include "DirtParticleGenerator.h"

CR_BIND(DirtParticleData, )
CR_REG_METADATA(DirtParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(alpha),
	CR_MEMBER(speed),
	CR_MEMBER(color),
	CR_MEMBER(size),
	CR_MEMBER(sizeExpansion),
	CR_MEMBER(drawOrder),
	CR_MEMBER(unused),
	CR_MEMBER(texCoord)
))