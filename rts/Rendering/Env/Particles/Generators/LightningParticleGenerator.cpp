#include "LightningParticleGenerator.h"

CR_BIND(LightningParticleData, )
CR_REG_METADATA(LightningParticleData,
(
	CR_MEMBER(startPos),
	CR_MEMBER(thickness),
	CR_MEMBER(targetPos),
	CR_MEMBER(unused),
	CR_MEMBER(displacements),
	CR_MEMBER(texCoord),
	CR_MEMBER(col),
	CR_MEMBER(drawOrder),
	CR_MEMBER(unused2)
))