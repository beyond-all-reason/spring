#include "MissileParticleGenerator.h"

CR_BIND(MissileParticleData, )
CR_REG_METADATA(MissileParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(fsize),
	CR_MEMBER(speed),
	CR_MEMBER(drawOrder),
	CR_MEMBER(texCoord)
))