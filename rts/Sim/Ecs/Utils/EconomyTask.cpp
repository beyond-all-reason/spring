#include "EconomyTask.h"

#include "Sim/Ecs/Components/UnitComponents.h"
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
}

template<class ChainHeadComp, class T>
void AddToChain(entt::entity head, entt::entity newLink) {
    T& chainHeadLinkComp = GetChainHeadLink<T>(head);
    InsertAfterChainLink<T>(chainHeadLinkComp.prev, newLink);

    auto& chainHeadComp = EcsMain::registry.get_or_emplace<ChainHeadComp>(head);
    chainHeadComp.size++;
}

template <class T>
void DisconnectChainLink(entt::entity linkToRemove) {
    auto removedLinkComp = EcsMain::registry.get<T>(linkToRemove);

    auto& beforeRemovedLinkComp = EcsMain::registry.get<T>(removedLinkComp.prev);
    auto& afterRemovedLinkComp = EcsMain::registry.get<T>(removedLinkComp.next);

    beforeRemovedLinkComp.next = removedLinkComp.next;
    afterRemovedLinkComp.prev = removedLinkComp.prev;
}

template<class ChainHeadComp, class T>
void RemoveFromChain(entt::entity head, entt::entity linkToRemove) {
    if (head != linkToRemove) {
        DisconnectChainLink<T>(linkToRemove);
        auto& chainHeadComp = EcsMain::registry.get<ChainHeadComp>(head);
        chainHeadComp.size--;
    }
}

entt::entity EconomyTaskUtil::CreateUnitEconomyTask(entt::entity unit) {
    auto economyTask = EcsMain::registry.create();
    EcsMain::registry.emplace<Units::OwningEntity>(economyTask, unit);

    AddToChain<Units::EconomyTasks, Units::ChainEntity>(unit, economyTask);

    return economyTask;
}

bool EconomyTaskUtil::DeleteUnitEconomyTask(entt::entity economyTask) {
    if (!EcsMain::registry.valid(economyTask)) return false;

    auto unit = EcsMain::registry.get<Units::OwningEntity>(economyTask).value;

    RemoveFromChain<Units::EconomyTasks, Units::ChainEntity>(unit, economyTask);
    EcsMain::registry.destroy(economyTask); // FIXME: mark for deletion rather than delete due to frame delays?

    return true;
}
