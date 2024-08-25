#include "BeamLaserParticleGenerator.h"

CR_BIND(BeamLaserParticleData, )
CR_REG_METADATA(BeamLaserParticleData,
(
	CR_MEMBER(startPos),
	CR_MEMBER(ccsColor),
	CR_MEMBER(targetPos),
	CR_MEMBER(cceColor),
	CR_MEMBER(animParams1),
	CR_MEMBER(ecsColor),
	CR_MEMBER(animParams2),
	CR_MEMBER(eceColor),
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