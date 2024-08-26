#include "ExplosiveParticleGenerator.h"

CR_BIND(ExplosiveParticleData, )
CR_REG_METADATA(ExplosiveParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(size),
	CR_MEMBER(speed),
	CR_MEMBER(createFrame),
	CR_MEMBER(dir),
	CR_MEMBER(drawOrder),
	CR_MEMBER(color0),
	CR_MEMBER(color1),
	CR_MEMBER(numStages),
	CR_MEMBER(noGap),
	CR_MEMBER(animParams),
	CR_MEMBER(alphaDecay),
	CR_MEMBER(rotParams),
	CR_MEMBER(sizeDecay),
	CR_MEMBER(separation),
	CR_MEMBER(colEdge0),
	CR_MEMBER(colEdge1),
	CR_MEMBER(curTime),
	CR_MEMBER(texCoord)
))