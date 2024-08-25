#include "TorpedoParticleGenerator.h"

CR_BIND(TorpedoParticleData, )
CR_REG_METADATA(TorpedoParticleData,
(
	CR_MEMBER(partPos),
	CR_MEMBER(drawOrder),
	CR_MEMBER(partSpeed),
	CR_MEMBER(unused),
	CR_MEMBER(texCoord)
))