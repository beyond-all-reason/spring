/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "GroundMoveSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

using namespace MoveTypes;

void GroundMoveSystem::Init() {}

template<typename T, typename F>
void issue_events(F func)
{
    auto view = Sim::registry.view<T>();
    view.each([&](T& comp){
        std::for_each(comp.value.begin(), comp.value.end(), func);
        comp.value.clear();
    });
}

void GroundMoveSystem::Update() {
    // TODO: GroundMove could become a component (or series of components) and then the extra indirection wouldn't be
    // needed. Though that will be a bigger change.
	{
		SCOPED_TIMER("Sim::Unit::MoveType::1::UpdateTraversalPlan");
        auto view = Sim::registry.view<GroundMoveType>();
        for_mt(0, view.size(), [&view](const int i){
            auto entity = view.storage<GroundMoveType>()[i];
            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
			CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            #ifndef NDEBUG
			unit->SanityCheck();
            #endif

			moveType->UpdateTraversalPlan();
		});
	}
	{
		SCOPED_TIMER("Sim::Unit::MoveType::2::UpdatePreCollisions");

        // These two sections are ST due to the numerous synced vars being changed.
        {
            auto view = Sim::registry.view<ChangeHeadingEvent>();
            view.each([](ChangeHeadingEvent& event){
                if (event.changed) {
                    CUnit* unit = unitHandler.GetUnit(event.unitId);
                    CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
                    moveType->ChangeHeading(event.deltaHeading);
                    event.changed = false;
                }
            });
        }
        {
            auto view = Sim::registry.view<ChangeMainHeadingEvent>();
            view.each([](ChangeMainHeadingEvent& event){
                if (event.changed) {
                    CUnit* unit = unitHandler.GetUnit(event.unitId);
                    CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
                    moveType->SetMainHeading();
                    event.changed = false;
                }
            });
        }
    }
	{
        auto view = Sim::registry.view<GroundMoveType>();
        for_mt(0, view.size(), [&view](const int i){
            auto entity = view.storage<GroundMoveType>()[i];
            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
			CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

			moveType->UpdateUnitPosition();
		});

		view.each([](GroundMoveType& unitId){
			CUnit* unit = unitHandler.GetUnit(unitId.value);
			CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

			moveType->UpdatePreCollisions();

            // this unit is not coming back, kill it now without any death
            // sequence (s.t. deathScriptFinished becomes true immediately)
            if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
                unit->ForcedKillUnit(nullptr, false, true, -CSolidObject::DAMAGE_KILLED_OOB);
		});
	}
    {
        SCOPED_TIMER("Sim::Unit::MoveType::3::CollisionDetection");
        auto view = Sim::registry.view<GroundMoveType>();
        //size_t count = view.storage<GroundMoveType>().size();
        for_mt(0, view.size(), [&view](const int i){
            auto entity = view.storage<GroundMoveType>()[i];
            assert( Sim::registry.valid(entity) );
            assert( Sim::registry.all_of<GroundMoveType>(entity) );
            assert( !Sim::registry.all_of<GeneralMoveType>(entity) );

            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            moveType->SetMtJobId(i);
            moveType->UpdateCollisionDetections();
        });
    }
	{
        SCOPED_TIMER("Sim::Unit::MoveType::4::ProcessCollisionEvents");

        issue_events<UnitCrushEvents>([](const UnitCrushEvent& event) {
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        issue_events<FeatureCrushEvents>([](const FeatureCrushEvent& event) {
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        issue_events<UnitCollisionEvents>([&](const UnitCollisionEvent& event) {
            eventHandler.UnitUnitCollision(event.collider, event.collidee);
        });
        issue_events<FeatureCollisionEvents>([](const FeatureCollisionEvent& event) {
            eventHandler.UnitFeatureCollision(event.collider, event.collidee);
        });
        issue_events<FeatureMoveEvents>([](const FeatureMoveEvent& event) {
            quadField.RemoveFeature(event.collidee);
            event.collidee->Move(event.moveImpulse, true);
            quadField.AddFeature(event.collidee);
        });
	}
	{
        // TODO: the vars are synced and that's what is stopping this being MT'ed.
        // Need an alternative method to support sync values that doesn't stop MT.
        // Same for change heading above as well.
        SCOPED_TIMER("Sim::Unit::MoveType::5::Update");
        auto view = Sim::registry.view<GroundMoveType>();
        view.each([&view](GroundMoveType& unitId){
            CUnit* unit = unitHandler.GetUnit(unitId.value);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);
            if (moveType->Update()) 
                eventHandler.UnitMoved(unit);

            #ifndef NDEBUG
            unit->SanityCheck();
            #endif
        });
    }
}

void GroundMoveSystem::Shutdown() {}
