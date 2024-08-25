#include "HeatCloudParticleGenerator.h"

CR_BIND(HeatCloudParticleData, )
CR_REG_METADATA(HeatCloudParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(maxHeat),
	CR_MEMBER(speed),
	CR_MEMBER(heat),
	CR_MEMBER(animParams),
	CR_MEMBER(size),
	CR_MEMBER(rotParams),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_MEMBER(drawOrder),
	CR_MEMBER(createFrame),
	CR_MEMBER(unused),
	CR_MEMBER(texCoord)
))