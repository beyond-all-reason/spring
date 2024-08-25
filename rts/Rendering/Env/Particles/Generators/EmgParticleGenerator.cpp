#include "EmgParticleGenerator.h"

CR_BIND(EmgParticleData, )
CR_REG_METADATA(EmgParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(radius),
	CR_MEMBER(animParams),
	CR_MEMBER(color),
	CR_MEMBER(rotParams),
	CR_MEMBER(drawOrder),
	CR_MEMBER(texCoord)
))