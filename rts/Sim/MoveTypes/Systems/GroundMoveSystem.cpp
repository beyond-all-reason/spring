/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "GroundMoveSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

using namespace MoveTypes;

void GroundMoveSystem::Init() {}

void GroundMoveSystem::Update() {
	{
		SCOPED_TIMER("Sim::Unit::MoveType::1::UpdatePreCollisionsMT");
        auto view = Sim::registry.view<GroundMoveType>();
        for_mt(0, view.size(), [&view](const int i){
            auto entity = view.storage<GroundMoveType>()[i];
            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
			AMoveType* moveType = unit->moveType;

            #ifndef NDEBUG
			unit->SanityCheck();
            #endif
    
			unit->PreUpdate();
			moveType->UpdatePreCollisionsMt();
		});
	}
	{
		SCOPED_TIMER("Sim::Unit::MoveType::2::UpdatePreCollisionsST");
        auto view = Sim::registry.view<GroundMoveType>();
		view.each([](GroundMoveType& unitId){
			CUnit* unit = unitHandler.GetUnit(unitId.value);
			AMoveType* moveType = unit->moveType;

			moveType->UpdatePreCollisions();
		});
	}
    {
        SCOPED_TIMER("Sim::Unit::MoveType::3::CollisionDetectionMT");
        auto view = Sim::registry.view<GroundMoveType>();
        //size_t count = view.storage<GroundMoveType>().size();
        for_mt(0, view.size(), [&view](const int i){
            auto entity = view.storage<GroundMoveType>()[i];
            assert( Sim::registry.valid(entity) );
            assert( Sim::registry.all_of<GroundMoveType>(entity) );
            assert( !Sim::registry.all_of<GeneralMoveType>(entity) );

            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
            AMoveType* moveType = unit->moveType;

            moveType->UpdateCollisionDetections();
        });
    }
	{
        SCOPED_TIMER("Sim::Unit::MoveType::4::ProcessCollisionEvents");
        auto view = Sim::registry.view<GroundMoveType>();
        view.each([](GroundMoveType& unitId){
            CUnit* unit = unitHandler.GetUnit(unitId.value);
            AMoveType* moveType = unit->moveType;
            moveType->ProcessCollisionEvents();
        });
	}
	{
        SCOPED_TIMER("Sim::Unit::MoveType::5::UpdateST");
        auto view = Sim::registry.view<GroundMoveType>();
        view.each([](GroundMoveType& unitId){
            CUnit* unit = unitHandler.GetUnit(unitId.value);
            AMoveType* moveType = unit->moveType;

            if (moveType->Update())
                eventHandler.UnitMoved(unit);

            // this unit is not coming back, kill it now without any death
            // sequence (s.t. deathScriptFinished becomes true immediately)
            if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
                unit->ForcedKillUnit(nullptr, false, true);

            #ifndef NDEBUG
            unit->SanityCheck();
            #endif
        });
	}
}

void GroundMoveSystem::Shutdown() {}
