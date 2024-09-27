#include "BitmapMuzzleFlameParticleGenerator.h"

CR_BIND(BitmapMuzzleFlameData, )
CR_REG_METADATA(BitmapMuzzleFlameData,
(
	CR_MEMBER(pos),
	CR_MEMBER(invttl),
	CR_MEMBER(dir),
	CR_MEMBER(createFrame),
	CR_MEMBER(rotParams),
	CR_MEMBER(drawOrder),
	CR_MEMBER(animParams),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(size),
	CR_MEMBER(len),
	CR_MEMBER(frontOffset),
	CR_IGNORED(unused),
	CR_MEMBER(color0),
	CR_MEMBER(color1),
	CR_MEMBER(edge0),
	CR_MEMBER(edge1),
	CR_MEMBER(sideTexture),
	CR_MEMBER(frontTexture)
))