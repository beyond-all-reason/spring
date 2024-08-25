#include "TorpedoParticleGenerator.h"

CR_BIND(TorpedoParticleData, )
CR_REG_METADATA(TorpedoParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(drawOrder),
	CR_MEMBER(speed),
	CR_IGNORED(unused),
	CR_MEMBER(texCoord)
))