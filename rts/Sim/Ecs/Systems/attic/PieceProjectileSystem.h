#ifndef PIECE_PROJECTILE_SYSTEM_H__
#define PIECE_PROJECTILE_SYSTEM_H__

#include "lib/entt/entt.hpp"

#include "Sim/Projectiles/PieceProjectile.h"

class PieceProjectileSystem {
public:
    void Init();
    void Update();
    void AddProjectile(CProjectile* projectile);
    void RemoveProjectile(CProjectile* projectile);

private:
    void UpdatePhysics();
    void UpdateState();
};

extern PieceProjectileSystem pieceProjectileSystem;

#endif