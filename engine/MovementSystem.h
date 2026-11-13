#pragma once

#include <string>

#include "ECSEngine.h"
#include "LocationComponent.h"
#include "SystemManager.h"
#include "Timing.h"
namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

     // AI-ASSISTED (Claude): Run
    /**
     * Moves every entity that has both a LocationComponent and a MovementComponent
     * by velocity * kFixedDt. Runs after Input and Gravity, before Collision.
     * Resources are reached only through the EngineView API.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const auto& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id)) continue;  // skip removed entities
            if (!em.template HasComponent<LocationComponent>(id)) continue;
            if (!em.template HasComponent<MovementComponent>(id)) continue;

            LocationComponent& lc = em.template GetComponent<LocationComponent>(id);
            const MovementComponent& mc = em.template GetComponent<MovementComponent>(id);
            lc.loc.x += mc.velocity.x * kFixedDt;
            lc.loc.y += mc.velocity.y * kFixedDt;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
