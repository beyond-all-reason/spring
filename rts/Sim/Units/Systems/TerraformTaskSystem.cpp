/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#undef NDEBUG

#include "TerraformTaskSystem.h"

#include "Map/MapDamage.h"
#include "Map/ReadMap.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Components/UnitComponents.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

using namespace Sim;
using namespace Unit;



constexpr int SMOOTH_BORDER = 3;

void OnTerraformBuildTaskDestroy(entt::registry &reg, entt::entity entity) {
    auto& task = reg.get<TerraformBuildTask>(entity);
    if (reg.valid(task.buildee))
        reg.remove<TerraformTaskReference>(task.buildee);
}


struct JobUpdateDetails {
    float terraformPower = 0.f;
    bool complete = false;

    JobUpdateDetails(entt::entity buildTaskEntity) {
        AggregateWorkerData(buildTaskEntity);
    }

    void AggregateWorkerData(entt::entity buildTaskEntity) {
        doubleLinkedListUtils.ForEachInChain<TerraformTaskChain>(buildTaskEntity, [this](entt::entity workerEntity)
        {
            if (!registry.all_of<InBuildStance>(workerEntity)) { return; }

            auto buildPowerComponent = registry.try_get<TerraformTaskBuildPower>(workerEntity);
            if (buildPowerComponent != nullptr)
                terraformPower += buildPowerComponent->value;
        });
    }
};

void RemoveOrphanedTasks(){
    auto view = registry.view<TerraformBuildTask, TerraformTaskChain>();

    // Remove tasks without workers.
    // view.each([&view](entt::entity buildTaskEntity){
    for (entt::entity buildTaskEntity : view) {
        auto& chain = view.get<TerraformTaskChain>(buildTaskEntity);
        bool taskHasNoWorkers = (chain.next == chain.prev);
        if (taskHasNoWorkers) {
            registry.destroy(buildTaskEntity);
        }
    };
}

void SmoothBorders(TerraformBuildTask& task, float terraformScale) {
    const float* heightmap = readMap->GetCornerHeightMapSynced();
    constexpr int b = SMOOTH_BORDER;

    // smooth the x-borders
    for (int z = task.tz1; z <= task.tz2; z++) {
        for (int x = 1; x <= b; x++) {
            if (task.tx1 - b >= 0) {
                const float ch3 = heightmap[z * mapDims.mapxp1 + task.tx1];
                const float ch = heightmap[z * mapDims.mapxp1 + task.tx1 - x];
                const float ch2 = heightmap[z * mapDims.mapxp1 + task.tx1 - 3];
                const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

                readMap->AddHeight(z * mapDims.mapxp1 + task.tx1 - x, amount);
            }
            if (task.tx2 + b < mapDims.mapx) {
                const float ch3 = heightmap[z * mapDims.mapxp1 + task.tx2];
                const float ch = heightmap[z * mapDims.mapxp1 + task.tx2 + x];
                const float ch2 = heightmap[z * mapDims.mapxp1 + task.tx2 + 3];
                const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

                readMap->AddHeight(z * mapDims.mapxp1 + task.tx2 + x, amount);
            }
        }
    }

    // smooth the z-borders
    for (int z = 1; z <= 3; z++) {
        for (int x = task.tx1; x <= task.tx2; x++) {
            if ((task.tz1 - b) >= 0) {
                const float ch3 = heightmap[(task.tz1)*mapDims.mapxp1 + x];
                const float ch = heightmap[(task.tz1 - z) * mapDims.mapxp1 + x];
                const float ch2 = heightmap[(task.tz1 - 3) * mapDims.mapxp1 + x];
                const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

                readMap->AddHeight((task.tz1 - z) * mapDims.mapxp1 + x, adjust);
            }
            if ((task.tz2 + b) < mapDims.mapy) {
                const float ch3 = heightmap[(task.tz2)*mapDims.mapxp1 + x];
                const float ch = heightmap[(task.tz2 + z) * mapDims.mapxp1 + x];
                const float ch2 = heightmap[(task.tz2 + 3) * mapDims.mapxp1 + x];
                const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

                readMap->AddHeight((task.tz2 + z) * mapDims.mapxp1 + x, adjust);
            }
        }
    }
}

void StepTerraformFlattenHeight(const TerraformBuildTask& task, float terraformScale) {
    const float* heightmap = readMap->GetCornerHeightMapSynced();

    auto unitId = registry.get<UnitId>(task.buildee).value;
    auto curBuildee = unitHandler.GetUnit(unitId);

    for (int z = task.tz1; z <= task.tz2; z++) {
        for (int x = task.tx1; x <= task.tx2; x++) {
            const int idx = z * mapDims.mapxp1 + x;
            const float addHeight = (curBuildee->pos.y - heightmap[idx]);

            readMap->AddHeight(idx, addHeight * terraformScale);
        }
    }
}

void StepTerraformRestoreHeight(const TerraformBuildTask& task, float terraformScale) {
    const float* heightmap = readMap->GetCornerHeightMapSynced();

    for (int z = task.tz1; z <= task.tz2; z++) {
        for (int x = task.tx1; x <= task.tx2; x++) {
            const int idx = z * mapDims.mapxp1 + x;
            float ch = heightmap[idx];
            float oh = readMap->GetOriginalHeightMapSynced()[idx];
            const float addHeight = (oh - ch);

            readMap->AddHeight(idx, addHeight * terraformScale);
        }
    }
}

void StepTerraform(TerraformBuildTask& task, JobUpdateDetails& update) {
    const float scale = (task.remaining <= 0.f) ? 0.f : update.terraformPower / task.remaining;
    const float terraformScale = std::min(scale, 1.0f);

    if (task.buildee != entt::null) {
        StepTerraformFlattenHeight(task, terraformScale);
    } else {
        StepTerraformRestoreHeight(task, terraformScale);
    }
    SmoothBorders(task, terraformScale);

    constexpr int b = SMOOTH_BORDER;
    mapDamage->RecalcArea(task.tx1 - b, task.tx2 + b, task.tz1 - b, task.tz2 + b);

    task.remaining -= update.terraformPower;
    update.complete = (task.remaining <= 0.0f);
}

void StepAllTerrafromTasks() {
    spring::spinlock lock;
    auto view = registry.group<TerraformBuildTask, TerraformTaskChain>();

    for_mt(0, view.size(), [&view, &lock](int taskIdx){
        entt::entity buildTaskEntity = view.begin()[taskIdx];
        JobUpdateDetails jobDetails(buildTaskEntity);

        auto& buildTask = view.get<TerraformBuildTask>(buildTaskEntity);
        StepTerraform(buildTask, jobDetails);

        if (jobDetails.complete) {
            lock.lock();
            registry.emplace_or_replace<TerraformTaskComplete>(buildTaskEntity);
            lock.unlock();
        }
    });
}

void CleanFinishedTasks() {
    auto view = registry.view<TerraformBuildTask, TerraformTaskComplete>();
    for (entt::entity buildTaskEntity : view) {
        // Remove workers from the job to signal to them the job is complete.
        auto& chainHead = registry.get<TerraformTaskChain>(buildTaskEntity);
        while (chainHead.prev != chainHead.next){
            registry.remove<TerraformTaskReference>(chainHead.next);
            doubleLinkedListUtils.RemoveChain<TerraformTaskChain>(chainHead.next);
        }

        // remove to signal that a future event needs not be sent?
        // registry.remove<TerraformTaskComplete>(buildTaskEntity);
    };
}

void TerraformTaskSystem::Init() {
    systemUtils.OnUpdate().connect<&TerraformTaskSystem::Update>();
    registry.on_destroy<TerraformBuildTask>().connect<&OnTerraformBuildTaskDestroy>();
}

void TerraformTaskSystem::Update() {
    SCOPED_TIMER("ECS::TerraformTaskSystem");

    RemoveOrphanedTasks();
    StepAllTerrafromTasks();
    CleanFinishedTasks();

    // This could be deferred, but it is here to ensure that it is called.
    // Note this doesn't seem to be used by games. Makes sense to remove it.
    SendEvents();
}

void TerraformTaskSystem::Shutdown() {
    systemUtils.OnUpdate().disconnect<&TerraformTaskSystem::Update>();
    registry.on_destroy<TerraformBuildTask>().disconnect<&OnTerraformBuildTaskDestroy>();
}

void TerraformTaskSystem::SendEvents() {
    // {
    // auto view = registry.view<TerraformBuildTask, TerraformTaskComplete>();
    // view.each([&view](entt::entity buildTaskEntity){
    //     auto& buildTask = view.get<TerraformBuildTask>(buildTaskEntity);
    //     if (buildTask.buildee != UNIT_ID_NULL) {
    //         eventHandler.TerraformComplete(this, buildTask.buildee);
    //         registry.remove<TerraformTaskComplete>(buildTaskEntity);
    //     }
    // });
    // }
}
