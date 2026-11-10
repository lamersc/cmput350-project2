#pragma once

#include <string>

#include "LocationComponent.h"
#include "SystemManager.h"
#include "Timing.h"

namespace ECSEngine
{

// AI-ASSISTED (Claude): starting value, tune by feel
const float kFallGravityMultiplier = 3.0f;  // gravity is this much stronger while falling

template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }
	
	// AI-ASSISTED (Claude): Run
    /**
     * Applies acceleration to velocity for every entity that has both a
     * GravityComponent and a MovementComponent: velocity += acceleration * kFixedDt.
     * While disabledTimer > 0 (counted in frames), gravity is skipped and the
     * timer counts down. Runs after InputSystem and before MovementSystem.
     * Resources are reached only through the EngineView API.
     * Falling (velocity.y > 0) uses gravity * kFallGravityMultiplier, so falls are
 	 * faster than jumps (Lecture 17). Screen y points down, so positive y is falling.
 	 * TODO: wall-slide speed limit (needs CollisionSystem to set the wall flags).
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const auto& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id)) continue;  // skip removed entities
            if (!em.template HasComponent<GravityComponent>(id)) continue;
            if (!em.template HasComponent<MovementComponent>(id)) continue;

            GravityComponent& gc = em.template GetComponent<GravityComponent>(id);
            MovementComponent& mc = em.template GetComponent<MovementComponent>(id);

            if (gc.disabledTimer > 0) {  // gravity paused this frame
                --gc.disabledTimer;
                continue;
            }
			float gravityY = gc.acceleration.y;
			if (mc.velocity.y > 0) gravityY *= kFallGravityMultiplier;  // falling: stronger gravity
            mc.velocity.x += gc.acceleration.x * kFixedDt;
            mc.velocity.y += gravityY * kFixedDt;
        }
        return kContinue;
    }
};

}



