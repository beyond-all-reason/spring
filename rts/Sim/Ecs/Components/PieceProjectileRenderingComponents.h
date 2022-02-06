#ifndef PROJECTILE_RENDERING_COMPONENTS_H__
#define PROJECTILE_RENDERING_COMPONENTS_H__

#include "Sim/Projectiles/PieceProjectile.h"

struct SimBasedPieceProjectileRendering {
    unsigned int explFlags;
    unsigned int cegID;
    float3 spinVec;
    float spinAngle;
    float spinSpeed;
    CPieceProjectile::FireTrailPoint fireTrailPoints[CPieceProjectile::NUM_TRAIL_PARTS];
    const S3DModelPiece* omp; 
    CSmokeTrailProjectile* smokeTrail;
	float3 oldSmokePos;
	float3 oldSmokeDir;
};

#endif