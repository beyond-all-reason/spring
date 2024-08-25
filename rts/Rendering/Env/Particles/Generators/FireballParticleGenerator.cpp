#include "FireballParticleGenerator.h"

CR_BIND(FireballParticleData, )
CR_REG_METADATA(FireballParticleData,
(
	CR_MEMBER(sparkPosSize),
	CR_MEMBER(dgunPos),
	CR_MEMBER(dgunSize),
	CR_MEMBER(animParams1),
	CR_MEMBER(numSparks),
	CR_MEMBER(animParams2),
	CR_MEMBER(drawOrder),
	CR_MEMBER(speed),
	CR_MEMBER(checkCol),
	CR_MEMBER(texCoord1),
	CR_MEMBER(texCoord2)
))