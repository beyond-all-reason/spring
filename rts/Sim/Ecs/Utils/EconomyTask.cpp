#include "EconomyTask.h"

#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Units/Unit.h"

template <class T>
T& GetChainHeadLink(entt::entity head) {
    return EcsMain::registry.get_or_emplace<T>(head, head, head);
}

template <class T>
void InsertAfterChainLink(entt::entity insertAfter, entt::entity newLink) {
    auto& insertAfterLinkComp = EcsMain::registry.get<T>(insertAfter);
    auto insertBefore = insertAfterLinkComp.next;

    auto& insertBeforeLinkComp = EcsMain::registry.get<T>(insertBefore);

    T newChainLinkComp;
    newChainLinkComp.prev = insertAfter;
    newChainLinkComp.next = insertBefore;
    EcsMain::registry.emplace<T>(newLink, newChainLinkComp);

    insertAfterLinkComp.next = newLink;
    insertBeforeLinkComp.prev = newLink;

    LOG("%s: new link %d <-> (%d) <-> %d", __func__, (int)insertAfter, (int)newLink, (int)insertBefore);
}

template<class ChainHeadComp, class T>
void AddToChain(entt::entity head, entt::entity newLink) {
    T& chainHeadLinkComp = GetChainHeadLink<T>(head);
    InsertAfterChainLink<T>(chainHeadLinkComp.prev, newLink);

    auto& chainHeadComp = EcsMain::registry.get_or_emplace<ChainHeadComp>(head);
    chainHeadComp.size++;

    LOG("%s: %d chain links now on %d", __func__, (int)chainHeadComp.size, (int)head);
}

template <class T>
void DisconnectChainLink(entt::entity linkToRemove) {
    auto removedLinkComp = EcsMain::registry.get<T>(linkToRemove);

    auto& beforeRemovedLinkComp = EcsMain::registry.get<T>(removedLinkComp.prev);
    auto& afterRemovedLinkComp = EcsMain::registry.get<T>(removedLinkComp.next);

    beforeRemovedLinkComp.next = removedLinkComp.next;
    afterRemovedLinkComp.prev = removedLinkComp.prev;

    LOG("%s: new link %d <-x (%d) x-> %d", __func__, (int)removedLinkComp.prev, (int)linkToRemove, (int)removedLinkComp.next);
}

template<class ChainHeadComp, class T>
void RemoveFromChain(entt::entity head, entt::entity linkToRemove) {
    if (head != linkToRemove) {
        DisconnectChainLink<T>(linkToRemove);
        auto& chainHeadComp = EcsMain::registry.get<ChainHeadComp>(head);
        chainHeadComp.size--;

        LOG("%s: %d chain links now on %d", __func__, (int)chainHeadComp.size, (int)head);
    }
}

template<typename TF, typename... TR>
void AddComponentsIfNotExist(entt::entity entity) {
    if (!EcsMain::registry.all_of<TF>(entity)){
        EcsMain::registry.emplace<TF>(entity);
    }
    if constexpr (sizeof...(TR) > 0) {
        AddComponentsIfNotExist<TR...>(entity);
    }
}

entt::entity EconomyTaskUtil::CreateUnitEconomyTask(entt::entity unit) {
    auto economyTask = EcsMain::registry.create();
    EcsMain::registry.emplace<Units::OwningEntity>(economyTask, unit);

    AddComponentsIfNotExist
        < UnitEconomy::MetalCurrentMake
        , UnitEconomy::EnergyCurrentMake
        , UnitEconomy::MetalCurrentUsage
        , UnitEconomy::EnergyCurrentUsage
        , UnitEconomyReport::SnapshotMetalMake
        , UnitEconomyReport::SnapshotEnergyMake
        , UnitEconomyReport::SnapshotMetalUsage
        , UnitEconomyReport::SnapshotEnergyUsage
        >(unit);

    auto team = EcsMain::registry.get<Units::Team>(unit).value;
    EcsMain::registry.emplace<Units::Team>(economyTask, team);

    AddToChain<Units::EconomyTasks, Units::ChainEntity>(unit, economyTask);

    LOG("%s: Eco Task %d owned by %d", __func__, (int)economyTask, (int)unit);

    return economyTask;
}

bool EconomyTaskUtil::DeleteUnitEconomyTask(entt::entity economyTask) {
    if (!EcsMain::registry.valid(economyTask)) return false;

    auto unit = EcsMain::registry.get<Units::OwningEntity>(economyTask).value;

    RemoveFromChain<Units::EconomyTasks, Units::ChainEntity>(unit, economyTask);
    EcsMain::registry.destroy(economyTask); // FIXME: mark for deletion rather than delete due to frame delays?

    auto& chainHead = EcsMain::registry.get<Units::EconomyTasks>(unit);
    if (chainHead.size <= 0) {
        EcsMain::registry.remove
                < UnitEconomy::MetalCurrentMake
                , UnitEconomy::EnergyCurrentMake
                , UnitEconomy::MetalCurrentUsage
                , UnitEconomy::EnergyCurrentUsage
                , UnitEconomyReport::SnapshotMetalMake
                , UnitEconomyReport::SnapshotEnergyMake
                , UnitEconomyReport::SnapshotMetalUsage
                , UnitEconomyReport::SnapshotEnergyUsage
                >(unit);
    }

    LOG("%s: Eco Task %d removed from %d", __func__, (int)economyTask, (int)unit);

    return true;
}

void EconomyTaskUtil::DeleteAllUnitEconomyTasks(entt::entity unit) {

    LOG("%s:Checking %d to remove economy tasks", __func__, (int)unit);

    auto ChainEntityComp = EcsMain::registry.try_get<Units::ChainEntity>(unit);
    if (ChainEntityComp == nullptr)
        return;

    for (auto nextInChain = ChainEntityComp->next; nextInChain != unit; ) {
        auto currentInChain = nextInChain;
        auto ChainEntityComp = EcsMain::registry.try_get<Units::ChainEntity>(nextInChain);
        if (ChainEntityComp == nullptr)
            nextInChain = unit;
        else
            nextInChain = ChainEntityComp->next;
        
        EcsMain::registry.destroy(currentInChain);

        LOG("%s: Eco Task %d removed from %d", __func__, (int)currentInChain, (int)unit);
    }
}
