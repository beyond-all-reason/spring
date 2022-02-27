#ifndef BUILD_SYSTEM_H__
#define BUILD_SYSTEM_H__

class CUnit;

class BuildSystem {

public:
    void Init();
    void Update();

    void AddUnitEconomy(CUnit *unit);

    bool IsSystemActive() { return active; }

private:
    bool active = false;
};

extern BuildSystem buildSystem;

#endif