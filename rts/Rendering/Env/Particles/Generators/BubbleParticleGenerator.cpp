#include "BubbleParticleGenerator.h"

CR_BIND(BubbleParticleData, )
CR_REG_METADATA(BubbleParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(alpha),
	CR_MEMBER(size),
	CR_MEMBER(sizeExpansion),
	CR_MEMBER(drawOrder),
	CR_MEMBER(unused),
	CR_MEMBER(texCoord)
))