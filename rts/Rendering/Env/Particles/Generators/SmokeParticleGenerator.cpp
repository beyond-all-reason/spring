#include "SmokeParticleGenerator.h"

CR_BIND(SmokeParticleData, )
CR_REG_METADATA(SmokeParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(size),
	CR_MEMBER(startSize),
	CR_MEMBER(sizeExpansion),
	CR_MEMBER(ageRate),
	CR_IGNORED(unused),
	CR_MEMBER(speed),
	CR_MEMBER(createFrame),
	CR_MEMBER(animParams),
	CR_MEMBER(color),
	CR_MEMBER(rotParams),
	CR_MEMBER(drawOrder),
	CR_MEMBER(texCoord)
))