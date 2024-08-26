#include "FlameParticleGenerator.h"

CR_BIND(FlameParticleData, )
CR_REG_METADATA(FlameParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(drawRadius),
	CR_MEMBER(speed),
	CR_MEMBER(createFrame),
	CR_MEMBER(animParams),
	CR_MEMBER(drawOrder),
	CR_MEMBER(rotParams),
	CR_MEMBER(curTime),
	CR_MEMBER(color0),
	CR_MEMBER(color1),
	CR_MEMBER(colEdge0),
	CR_MEMBER(colEdge1),
	CR_MEMBER(texCoord)
))