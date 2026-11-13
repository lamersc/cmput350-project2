#pragma once

#include <string>

#include "LocationComponent.h"
#include "SystemManager.h"
#include "CollisionComponent.h"
#include "Timing.h"
#include "InputComponent.h"

namespace ECSEngine
{

// AI-ASSISTED (Claude): starting values, tune by feel
const float kFallGravityMultiplier = 3.0f;  // gravity is this much stronger while falling
const float kWallSlideSpeed = 150;          // AI guess: max fall speed while sliding on a wall

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
 	 * Wall slide: while touching a wall in the air, fall speed is capped at kWallSlideSpeed
     * (flags come from last frame's CollisionSystem).
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

			// AI-ASSISTED (Claude): player-only fall multiplier and wall slide
            const bool isPlayer = em.template HasComponent<InputComponent>(id);
            float gravityY = gc.acceleration.y;
            if (isPlayer && mc.velocity.y > 0) gravityY *= kFallGravityMultiplier;


            // AI-ASSISTED (Claude): wall slide cap
            mc.velocity.x += gc.acceleration.x * kFixedDt;
            mc.velocity.y += gravityY * kFixedDt;

            if (em.template HasComponent<CollisionComponent>(id)) {
                const CollisionComponent& cc = em.template GetComponent<CollisionComponent>(id);
                const bool onWall = cc.onLeftWall != 0 || cc.onRightWall != 0;
                                if (isPlayer && onWall && cc.onGround == 0 && mc.velocity.y > kWallSlideSpeed) {
                    mc.velocity.y = kWallSlideSpeed;
                }
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine