#ifndef SYSTEM_UTILS_H__
#define SYSTEM_UTILS_H__

#include "Sim/Ecs/EcsMain.h"

namespace SystemUtils {

class SystemUtils {
public:

    void InitSystems();

    void NotifyUpdate() {
        update.publish();
    }

    void NotifyPreLoad() {
        preLoad.publish();
    }

    void NotifyPostLoad() {
        postLoad.publish();
    }

    [[nodiscard]] auto OnUpdate() ENTT_NOEXCEPT {
        return entt::sink{update};
    }

    [[nodiscard]] auto OnPreLoad() ENTT_NOEXCEPT {
        return entt::sink{preLoad};
    }

    [[nodiscard]] auto OnPostLoad() ENTT_NOEXCEPT {
        return entt::sink{postLoad};
    }

private:
    entt::sigh<void()> update{};
    entt::sigh<void()> preLoad{};
    entt::sigh<void()> postLoad{};
};

extern SystemUtils systemUtils;

}

#endif