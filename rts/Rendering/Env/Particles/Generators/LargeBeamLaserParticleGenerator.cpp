#include "LargeBeamLaserParticleGenerator.h"

CR_BIND(LargeBeamLaserParticleData, )
CR_REG_METADATA(LargeBeamLaserParticleData,
(
	CR_MEMBER(startPos),
	CR_MEMBER(drawOrder),
	CR_MEMBER(targetPos),
	CR_IGNORED(unused),
	CR_MEMBER(thickness),
	CR_MEMBER(coreThickness),
	CR_MEMBER(flareSize),
	CR_MEMBER(tileLength),
	CR_MEMBER(scrollSpeed),
	CR_MEMBER(pulseSpeed),
	CR_MEMBER(coreColStart),
	CR_MEMBER(edgeColStart),
	CR_MEMBER(texCoord1),
	CR_MEMBER(texCoord2),
	CR_MEMBER(texCoord3),
	CR_MEMBER(texCoord4)
))