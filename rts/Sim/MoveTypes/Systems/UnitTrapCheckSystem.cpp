/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "UnitTrapCheckSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/Ecs/Utils/SystemGlobalUtils.h"
// #include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"
// #include "Sim/Units/UnitDef.h"

using namespace MoveTypes;

void UnitTrapCheckSystem::Init() {
    auto& comp = Sim::systemGlobals.CreateSystemComponent<YardmapTrapCheckSystemSystemComponent>();

    std::for_each(comp.trappedUnitLists.begin(), comp.trappedUnitLists.end(), [](auto& list){
        list.reserve(YardmapTrapCheckSystemSystemComponent::INITIAL_TRAP_UNIT_LIST_ALLOC_SIZE);
    });

    Sim::systemUtils.OnUpdate().connect<&UnitTrapCheckSystem::Update>();
}

void TagUnitsThatMayBeStuck(std::vector<CUnit*> &curList, const CSolidObject* collidee, int curThread) {
    const int largestMoveTypSizeH = moveDefHandler.GetLargestFootPrintSizeH() + 1;
    const int bufferSize = SQUARE_SIZE * modInfo.unitQuadPositionUpdateRate * 2 + largestMoveTypSizeH + 1;

    const int2& pos = collidee->mapPos;
    const int xmin = pos.x                   + (-bufferSize);
	const int zmin = pos.y                   + (-bufferSize);
	const int xmax = pos.x + collidee->xsize +   bufferSize;
	const int zmax = pos.y + collidee->zsize +   bufferSize;

    const float3 min(xmin * SQUARE_SIZE, 0.f, zmin * SQUARE_SIZE);
    const float3 max(xmax * SQUARE_SIZE, 0.f, zmax * SQUARE_SIZE);

    QuadFieldQuery qfQuery;
    qfQuery.threadOwner = curThread;
    quadField.GetUnitsExact(qfQuery, min, max);
    for (const CUnit* unit: *qfQuery.units) {
        if (unit->moveDef == nullptr || unit->moveType == nullptr)
            continue;
        
        auto unitMoveType = dynamic_cast<CGroundMoveType*>(unit->moveType);
        if (unitMoveType == nullptr)
            continue;
        
        // curList.emplace_back(unit);

        // This is okay for multithrading because the value will only be set one way.
        unitMoveType->OwnerMayBeStuck();
    }
}

void UnitTrapCheckSystem::Update() {
    SCOPED_TIMER("ECS::RemoveDeadPathsSystem::Update");

    auto& comp = Sim::systemGlobals.GetSystemComponent<YardmapTrapCheckSystemSystemComponent>();

    std::for_each(comp.trappedUnitLists.begin(), comp.trappedUnitLists.end(), [](auto& list){
        list.clear();
    });

    auto view = Sim::registry.view<UnitTrapCheck>();
    for_mt(0, view.size(), [&comp, &view](int index){
        int curThread = ThreadPool::GetThreadNum();
        auto& curList = comp.trappedUnitLists[curThread];

        entt::entity entity = view.begin()[index];
        auto& unitToCheckComp = view.get<UnitTrapCheck>(entity);

        CSolidObject* object = (unitToCheckComp.type == UnitTrapCheckType::TRAPPER_IS_UNIT)
                ? static_cast<CSolidObject*>( unitHandler.GetUnit(unitToCheckComp.id) )
                : static_cast<CSolidObject*>( featureHandler.GetFeature(unitToCheckComp.id) );

        TagUnitsThatMayBeStuck(curList, object, curThread);
    });
}

void UnitTrapCheckSystem::Shutdown() {}
