#include "ExploSpikeParticleGenerator.h"

CR_BIND(ExploSpikeParticleData, )
CR_REG_METADATA(ExploSpikeParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(alpha),
	CR_MEMBER(speed),
	CR_MEMBER(alphaDecay),
	CR_MEMBER(dir),
	CR_MEMBER(color),
	CR_MEMBER(length),
	CR_MEMBER(lengthGrowth),
	CR_MEMBER(width),
	CR_MEMBER(drawOrder),
	CR_MEMBER(texCoord)
))