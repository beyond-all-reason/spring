#include "SimpleParticleGenerator.h"

CR_BIND(SimpleParticleData, )
CR_REG_METADATA(SimpleParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(size),
	CR_MEMBER(speed),
	CR_MEMBER(createFrame),
	CR_MEMBER(gravity),
	CR_MEMBER(airDrag),
	CR_MEMBER(lifeDecayRate),
	CR_MEMBER(sizeMod),
	CR_MEMBER(sizeGrowth),
	CR_IGNORED(unused),
	CR_MEMBER(animParams),
	CR_MEMBER(directional),
	CR_MEMBER(rotParams),
	CR_MEMBER(drawOrder),
	CR_MEMBER(color0),
	CR_MEMBER(color1),
	CR_MEMBER(colEdge0),
	CR_MEMBER(colEdge1),
	CR_MEMBER(texCoord)
))