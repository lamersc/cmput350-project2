#pragma once

#include <string>
#include <iostream>

#include "Keyboard.h"
#include "SystemManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "Timing.h"

namespace ECSEngine {

// Constants below are the instructor-provided template values, unchanged except
// kDashInterval (spec: at most one dash per second = 60 frames at 60 FPS).
// UNITS ARE UNCONFIRMED: our Movement/Gravity systems use px/s and px/s^2 with
// kFixedDt. If the player barely moves, these need rescaling; see AI_LOG.
// Timers are in frames.
const float kMaxRunVelocity = 400; //15;
const float kDashVelocity = 1500; //50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 1500; //5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 60;  // template: 25. Changed to match the spec (once per second)

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    // AI-ASSISTED (Claude): Run (left/right only)
    /**
        * Accelerates left/right from the arrow keys (clamped to kMaxRunVelocity) and
        * decelerates to a stop when no key, or both keys, are held.
        * TODO: air acceleration (needs onGround), jump, variable jump height, dash.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& inputs = em.template GetComponentStorage<InputComponent>();

        for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (!inputs.mValid[i]) continue;  // skip dead slots
            const InputComponent& ic = inputs.mStorage[i];
            if (ic.keydown.any()) std::cout << "key down\n";
            EntityID id = ic.entity;
            if (!em.IsValid(id)) continue;
            if (!em.template HasComponent<MovementComponent>(id)) continue;

            MovementComponent& mc = em.template GetComponent<MovementComponent>(id);
            // AI-ASSISTED (Claude): deceleration when no key (or both keys) is held
            bool right = ic.keydown[Key::Right];
            bool left = ic.keydown[Key::Left];

            if (right == left) {  // neither key, or both: slow down to a stop
                float drop = kGroundRunAcceleration * kFixedDt;
                if (mc.velocity.x > drop) mc.velocity.x -= drop;
                else if (mc.velocity.x < -drop) mc.velocity.x += drop;
                else mc.velocity.x = 0;
                continue;
            }

            float dir = right ? 1.0f : -1.0f;
            mc.velocity.x += dir * kGroundRunAcceleration * kFixedDt;
            if (mc.velocity.x > kMaxRunVelocity) mc.velocity.x = kMaxRunVelocity;
            if (mc.velocity.x < -kMaxRunVelocity) mc.velocity.x = -kMaxRunVelocity;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
