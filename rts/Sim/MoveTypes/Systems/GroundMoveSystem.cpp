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

void GroundMoveSystem::Init() {
    auto& comp = Sim::systemGlobals.CreateSystemComponent<GroundMoveSystemComponent>();
}


template<typename T>
struct ThreadEventWalker {
    std::array<std::vector<T>, ThreadPool::MAX_THREADS>& threadEventQueues;
    std::array<int, ThreadPool::MAX_THREADS> indicies;

    ThreadEventWalker(std::array<std::vector<T>, ThreadPool::MAX_THREADS>& _threadEventQueues)
        : threadEventQueues(_threadEventQueues)
    {
        std::for_each(indicies.begin(), indicies.end(), [](int &v){ v=0; });
    }

    T* getNextEventFromThreads() {
        int chosenThread = -1;
        int chosenId = std::numeric_limits<int>::max();
        T* result;

        for(int i=0; i<threadEventQueues.size(); ++i){
            if (indicies[i] >= threadEventQueues[i].size()) { continue; }

            int curId = threadEventQueues[i][indicies[i]].id;
            if (curId < chosenId) {
                chosenId = curId;
                chosenThread = i;
                result = &threadEventQueues[i][indicies[i]];
            }
        }

        if (chosenThread == -1)
            return nullptr;

        indicies[chosenThread]++;
        return result;
    }

    template<typename F>
    void forEach(F func) {
        auto event = getNextEventFromThreads();
        while (event != nullptr) {
            func(*event);
            event = getNextEventFromThreads();
        }
    }

    void clearThreadData() {
        for (int i=0; i < threadEventQueues.size(); ++i){
            threadEventQueues[i].clear();
        }
    }
};

template<typename T, typename Container, typename F>
void process_thread_data(Container& threadDataLists, F func)
{
    ThreadEventWalker<T> threadEventWalker(threadDataLists);
    threadEventWalker.forEach(func);
    threadEventWalker.clearThreadData();
}


void GroundMoveSystem::Update() {
    auto& comp = Sim::systemGlobals.GetSystemComponent<GroundMoveSystemComponent>();

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

        process_thread_data<UnitCrushEvent>(comp.killUnits, [](UnitCrushEvent& event){
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        process_thread_data<FeatureCrushEvent>(comp.killFeatures, [](FeatureCrushEvent& event) {
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        process_thread_data<UnitCollisionEvent>(comp.collidedUnits, [](UnitCollisionEvent& event) {
            eventHandler.UnitUnitCollision(event.collider, event.collidee);
        });
        process_thread_data<FeatureCollisionEvent>(comp.collidedFeatures, [](FeatureCollisionEvent& event) {
            eventHandler.UnitFeatureCollision(event.collider, event.collidee);
        });
        process_thread_data<FeatureMoveEvent>(comp.moveFeatures, [](FeatureMoveEvent& event) {
            quadField.RemoveFeature(event.collidee);
            event.collidee->Move(event.moveImpulse, true);
            quadField.AddFeature(event.collidee);
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
