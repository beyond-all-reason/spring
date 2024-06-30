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
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

#include "System/Misc/TracyDefs.h"

using namespace MoveTypes;

void SystemInit() {
    auto& comp = Sim::systemGlobals.CreateSystemComponent<YardmapTrapCheckSystemComponent>();
}

void UnitTrapCheckSystem::Init() {
    RECOIL_DETAILED_TRACY_ZONE;

    // std::for_each(comp.trappedUnitLists.begin(), comp.trappedUnitLists.end(), [](auto& list){
    //     list.reserve(YardmapTrapCheckSystemComponent::INITIAL_TRAP_UNIT_LIST_ALLOC_SIZE);
    // });

    SystemInit();

    Sim::systemUtils.OnPostLoad().connect<&SystemInit>();

    // Sim::systemUtils.OnUpdate().connect<&UnitTrapCheckSystem::Update>();
}

void TagUnitsThatMayBeStuck(std::vector<CUnit*> &curList, const CSolidObject* collidee, int curThread) {
    RECOIL_DETAILED_TRACY_ZONE;
    const int largestMoveTypSizeH = moveDefHandler.GetLargestFootPrintSizeH() + 1;
    const int bufferSize = SQUARE_SIZE * modInfo.unitQuadPositionUpdateRate * 2 + largestMoveTypSizeH + 1;

    const int2& pos = collidee->mapPos;
    const int xmin = pos.x;
	const int zmin = pos.y;
	const int xmax = pos.x + collidee->xsize;
	const int zmax = pos.y + collidee->zsize;

    const float3 min((xmin - bufferSize) * SQUARE_SIZE, 0.f, (zmin - bufferSize) * SQUARE_SIZE);
    const float3 max((xmax + bufferSize) * SQUARE_SIZE, 0.f, (zmax + bufferSize) * SQUARE_SIZE);

    QuadFieldQuery qfQuery;
    qfQuery.threadOwner = curThread;
    quadField.GetUnitsExact(qfQuery, min.cClampInMap(), max.cClampInMap());
    for (const CUnit* unit: *qfQuery.units) {
        MoveDef *moveDef = unit->moveDef;
        if (moveDef == nullptr || unit->moveType == nullptr)
            continue;

        auto unitMoveType = dynamic_cast<CGroundMoveType*>(unit->moveType);
        if (unitMoveType == nullptr)
            continue;

        // Filter out units that will not overlap the collision area.
        int squareX = int(unit->pos.x) / SQUARE_SIZE;
        int squareZ = int(unit->pos.z) / SQUARE_SIZE;

        if ((squareX - moveDef->xsizeh) > xmax) continue;
        if ((squareZ - moveDef->zsizeh) > zmax) continue;
        if ((squareX + moveDef->xsizeh) < xmin) continue;
        if ((squareZ + moveDef->zsizeh) < zmin) continue;
        
        // curList.emplace_back(unit);

        // This is okay for multithreading because the value will only be set one way.
        unitMoveType->OwnerMayBeStuck();
    }
}

void UnitTrapCheckSystem::Update() {
    SCOPED_TIMER("ECS::UnitTrapCheckSystem::Update");

    auto& comp = Sim::systemGlobals.GetSystemComponent<YardmapTrapCheckSystemComponent>();

    // std::for_each(comp.trappedUnitLists.begin(), comp.trappedUnitLists.end(), [](auto& list){
    //     list.clear();
    // });

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

    view.each([](entt::entity entity){ Sim::registry.remove<UnitTrapCheck>(entity); });
}

void UnitTrapCheckSystem::Shutdown() {
    Sim::systemUtils.OnPostLoad().disconnect<&SystemInit>();
    // systemUtils.OnUpdate().disconnect<&UnitTrapCheckSystem::Update>();
}
