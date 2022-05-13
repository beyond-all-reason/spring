#include "UnitSystem.h"

#include "SolidObjectSystem.h"
#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/SolidObjectComponent.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Utils/EconomyTask.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"

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
    auto entity = unit->entityReference;

    EcsMain::registry.emplace<UnitId>(entity, unit->id);
    EcsMain::registry.emplace<Team>(entity, unit->team);
    EcsMain::registry.emplace<UnitDefRef>(entity, unit->unitDef);
    EcsMain::registry.get<SolidObject::MaxHealth>(entity).value = unit->unitDef->health;

    unit->entityReference = entity;

    LOG("%s: added unit %d (%d)", __func__, unit->id, (int)entity);
}

void UnitSystem::RemoveUnit(CUnit* unit)
{
    EconomyTaskUtil::DeleteAllUnitEconomyTasks(unit->entityReference);

    solidObjectSystem.RemoveObject(unit);
}
