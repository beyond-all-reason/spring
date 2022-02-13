#ifndef UNIT_SYSTEM_H__
#define UNIT_SYSTEM_H__

#include "lib/entt/entt.hpp"

class UnitSystem {
public:
    void Init();
    void Update();
    void AddUnit(CUnit* projectile);
    void RemoveUnit(CUnit* projectile);

private:

};

extern UnitSystem unitSystem;

#endif