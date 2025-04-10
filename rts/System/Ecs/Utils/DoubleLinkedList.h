/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ECS_DOUBLE_LINKED_LIST_H_
#define ECS_DOUBLE_LINKED_LIST_H_

#include "System/Ecs/EcsMain.h"

namespace ecs_dll {

class DoubleLinkList {
public:
	DoubleLinkList(entt::registry& reg)
	    : registry(reg)
	{
	}

	template<class T> void InsertChain(entt::entity head, entt::entity newLink)
	{
		auto& nextChain = registry.get<T>(head);
		auto& prevChain = registry.get<T>(nextChain.prev);

		// LOG("%s: added chain link with %x <-> [%x] <-> %x.", __func__
		//         , entt::to_integral(nextChain.prev)
		//         , entt::to_integral(newLink)
		//         , entt::to_integral(prevChain.next));

		registry.emplace_or_replace<T>(newLink, nextChain.prev, prevChain.next);
		prevChain.next = newLink;
		nextChain.prev = newLink;
	}

	template<class T> void RemoveChain(entt::entity removeLink)
	{
		auto& curChain = registry.get<T>(removeLink);

		auto& nextChain = registry.get<T>(curChain.next);
		auto& prevChain = registry.get<T>(curChain.prev);

		// LOG("%s: removed chain link from %x. Linking %x <-> %x.", __func__
		//         , entt::to_integral(removeLink)
		//         , entt::to_integral(curChain.prev)
		//         , entt::to_integral(curChain.next));

		prevChain.next = curChain.next;
		nextChain.prev = curChain.prev;

		registry.remove<T>(removeLink);
	}

	// walk the chain list
	template<class T, typename F> void ForEachInChain(entt::entity head, F&& func)
	{
		// for (auto* chainLink = &registry.get<T>(head);
		//     chainLink->next != head;
		//     chainLink = &registry.get<T>(chainLink->next)
		// ) {
		//     LOG("%s: walking chain link %x [head %x]", __func__
		//             , entt::to_integral(chainLink->next)
		//             , entt::to_integral(head));
		//     func(chainLink->next);
		// }

		auto curEntity = head;
		do {
			// LOG("%s: walking chain link %x [head %x]", __func__
			//         , entt::to_integral(curEntity)
			//         , entt::to_integral(head));
			func(curEntity);

			auto* chainLink = &registry.get<T>(curEntity);
			curEntity = chainLink->next;
		} while (curEntity != head);
	}

	template<class T, typename F> void BackWalkWithEarlyExit(entt::entity head, F&& func)
	{
		auto curEntity = head;
		do {
			// LOG("%s: walking chain link %x [head %x]", __func__
			//         , entt::to_integral(curEntity)
			//         , entt::to_integral(head));
			if (!func(curEntity)) {
				break;
			}

			auto* chainLink = &registry.get<T>(curEntity);
			curEntity = chainLink->prev;
		} while (curEntity != head);
	}

private:
	entt::registry& registry;
};

} // namespace ecs_dll

#endif
