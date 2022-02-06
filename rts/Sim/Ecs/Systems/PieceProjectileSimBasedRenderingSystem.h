#ifndef PIECE_PROJECTILE_SYSTEM_H__
#define PIECE_PROJECTILE_SYSTEM_H__

#include "lib/entt/entt.hpp"

#include "Sim/Projectiles/Projectile.h"

class PieceProjectileSimBasedRenderingSystem {
public:
    void Init();
    void Update();
    void AddProjectile(CProjectile* projectile);
    void RemoveProjectile(CProjectile* projectile);

private:
    float3 RandomVertexPos(SimBasedPieceProjectileRendering& sbr) const;
};

extern PieceProjectileSimBasedRenderingSystem pieceProjectileSimBasedRenderingSystem;

#endif