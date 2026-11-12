#pragma once


#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    // AI-ASSISTED (Claude): Run
    /**
     * Runs first each frame, before any system moves entities. For every entity with
     * a LocationComponent and a CollisionComponent, stores the collision box in
     * WORLD coordinates (box + location) as previousCollisionBox. CollisionSystem
     * uses it later to tell which side an overlap came from.
     * Does not touch the onGround/onWall/onCeiling flags: they still hold last
     * frame's result, which Input and Gravity read.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const auto& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id)) continue;  // skip removed entities
            if (!em.template HasComponent<LocationComponent>(id)) continue;
            if (!em.template HasComponent<CollisionComponent>(id)) continue;

            const LocationComponent& lc = em.template GetComponent<LocationComponent>(id);
            CollisionComponent& cc = em.template GetComponent<CollisionComponent>(id);

            cc.previousCollisionBox = Rect{
                {lc.loc.x + cc.collisionBox.topLeft.x, lc.loc.y + cc.collisionBox.topLeft.y},
                cc.collisionBox.width,
                cc.collisionBox.height};
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
