#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Units/Unit.h"

#include "UnitSystem.h"

#include "System/TimeProfiler.h"


#include "System/Log/ILog.h"

UnitSystem unitSystem;

using namespace Units;

void UnitSystem::Init()
{
}

void UnitSystem::Update()
{
}

void UnitSystem::AddUnit(CUnit* unit)
{
    auto entity = EcsMain::registry.create();

    EcsMain::registry.emplace<UnitId>(entity, unit->id);
    EcsMain::registry.emplace<Team>(entity, unit->team);
    EcsMain::registry.emplace<UnitDefRef>(entity, unit->unitDef);

    unit->entityReference = entity;

    LOG("%s: added unit %d", __func__, unit->id);
}

void UnitSystem::RemoveUnit(CUnit* unit)
{
    auto view = EcsMain::registry.view<const UnitId>();
    entt::entity entity = unit->entityReference;

    unit->entityReference = entt::null;
    
    if (EcsMain::registry.valid(entity))
        EcsMain::registry.destroy(entity);
}
