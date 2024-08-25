#include "BeamLaserParticleGenerator.h"

CR_BIND(BeamLaserParticleData, )
CR_REG_METADATA(BeamLaserParticleData,
(
	CR_MEMBER(startPos),
	CR_MEMBER(coreColStart),
	CR_MEMBER(targetPos),
	CR_MEMBER(coreColEnd),
	CR_MEMBER(animParams1),
	CR_MEMBER(edgeColStart),
	CR_MEMBER(animParams2),
	CR_MEMBER(edgeColEnd),
	CR_MEMBER(animParams3),
	CR_MEMBER(drawOrder),
	CR_MEMBER(texCoord1),
	CR_MEMBER(texCoord2),
	CR_MEMBER(texCoord3),
	CR_MEMBER(thickness),
	CR_MEMBER(coreThickness),
	CR_MEMBER(flareSize),
	CR_MEMBER(midTexX2)
))