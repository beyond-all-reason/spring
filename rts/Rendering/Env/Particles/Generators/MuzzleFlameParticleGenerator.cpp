#include "MuzzleFlameParticleGenerator.h"

CR_BIND(MuzzleFlameParticleData, )
CR_REG_METADATA(MuzzleFlameParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(age),
	CR_MEMBER(randDir),
	CR_MEMBER(size),
	CR_MEMBER(aIndex),
	CR_MEMBER(drawOrder),
	CR_MEMBER(unused1),
	CR_MEMBER(unused2),
	CR_MEMBER(texCoord1),
	CR_MEMBER(texCoord2)
))