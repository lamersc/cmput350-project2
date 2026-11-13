#pragma once

#include <string>

#include "CollisionComponent.h"
#include "InputComponent.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "SoundManager.h"
#include "SystemManager.h"
#include "Timing.h"

namespace ECSEngine {

// Constants are the instructor-provided template values unless noted.
// Velocities are px/s, accelerations px/s^2, timers are frames (60 = 1 s).
// All values are provisional and must be tuned against the spike-wall tests.
const float kMaxRunVelocity = 400;          // template: 15
const float kDashVelocity = 1500;           // template: 50
const float kJumpVelocity = 700;            // template: 25 (AI guess, tune)
const float kGroundRunAcceleration = 1500;  // template: 5
const float kAirRunAcceleration = 1000;     // template: 4
const float kDashInterval = 60;             // template: 25. Spec: once per second

// AI-ASSISTED (Claude): new tuning constants, all guesses
const float kWallJumpVelocity = 400;        // horizontal push away from the wall
const float kJumpCutFactor = 0.4f;          // upward speed kept when Z is released early
const float kFastFallAcceleration = 2000;   // extra downward acceleration while Down is held
const int kDashFrames = 8;                  // frames of dash (gravity and control paused)

// Input reads keys by Key:: index, ProcessEvents sets them by SFML scancode.
// This fails to compile if the numbering ever differs.
static_assert(Key::Z == static_cast<unsigned int>(sf::Keyboard::Scancode::Z) &&
                Key::C == static_cast<unsigned int>(sf::Keyboard::Scancode::C) &&
                Key::Down == static_cast<unsigned int>(sf::Keyboard::Scancode::Down),
                "Key:: constants must match sf::Keyboard::Scancode");

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    // AI-ASSISTED (Claude): Run
    /**
     * Turns keys into velocity changes. Runs after CollisionLateSystem and before
     * Gravity and Movement. Reads last frame's collision flags (onGround, onLeftWall,
     * onRightWall) set by CollisionSystem.
     *  - Left/Right: accelerate (ground or air rate), clamp to kMaxRunVelocity,
     *    decelerate to a stop when no key or both keys are held.
     *  - Z: jump on the ground, or wall jump (up and away) when touching a wall in
     *    the air. Releasing Z while rising cuts the jump short (kJumpCutFactor).
     *  - Down: extra downward acceleration while falling.
     *  - C: dash, at most once per kDashInterval frames. On a wall in the air it
     *    dashes away from the wall. Pauses gravity via GravityComponent::disabledTimer
     *    and ignores run input until the dash ends.
     * Wall contact only exists while the player presses into the wall, because
     * CollisionSystem sets the flag from an actual overlap.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& inputs = em.template GetComponentStorage<InputComponent>();
        bool wallSliding = false;

        for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (!inputs.mValid[i]) continue;  // skip dead slots
            InputComponent& ic = inputs.mStorage[i];
            EntityID id = ic.entity;
            if (!em.IsValid(id)) continue;
            if (!em.template HasComponent<MovementComponent>(id)) continue;

            MovementComponent& mc = em.template GetComponent<MovementComponent>(id);

            bool onGround = false;
            bool onLeftWall = false;
            bool onRightWall = false;
            if (em.template HasComponent<CollisionComponent>(id)) {
                const CollisionComponent& cc = em.template GetComponent<CollisionComponent>(id);
                onGround = cc.onGround != 0;
                onLeftWall = cc.onLeftWall != 0;
                onRightWall = cc.onRightWall != 0;
            }
            GravityComponent* gc = nullptr;
            if (em.template HasComponent<GravityComponent>(id)) {
                gc = &em.template GetComponent<GravityComponent>(id);
            }

            const bool right = ic.keydown[Key::Right];
            const bool left = ic.keydown[Key::Left];
            const bool down = ic.keydown[Key::Down];
            const bool jumpHeld = ic.keydown[Key::Z];
            const bool jumpPressed = jumpHeld && !ic.lastKeydown[Key::Z];
            const bool jumpReleased = !jumpHeld && ic.lastKeydown[Key::Z];
            const bool dashPressed = ic.keydown[Key::C] && !ic.lastKeydown[Key::C];

            // lastDash counts frames since the last dash (stops counting at the interval).
            if (ic.lastDash < static_cast<int>(kDashInterval)) ++ic.lastDash;

            // While dashing, ignore run input (Gravity counts the timer down).
            if (gc != nullptr && gc->disabledTimer > 0) continue;

            // ---- Dash ----
            if (dashPressed && ic.lastDash >= static_cast<int>(kDashInterval)) {
                float dir = 1.0f;
                if (!onGround && onLeftWall) dir = 1.0f;         // dash away from the wall
                else if (!onGround && onRightWall) dir = -1.0f;
                else if (right != left) dir = right ? 1.0f : -1.0f;
                else if (mc.velocity.x < 0) dir = -1.0f;         // keep current direction
                mc.velocity.x = dir * kDashVelocity;
                mc.velocity.y = 0;
                if (gc != nullptr) gc->disabledTimer = kDashFrames;
                ic.lastDash = 0;
                continue;
            }

            // ---- Jump and wall jump ----
            if (jumpPressed) {
                if (onGround) {
                    mc.velocity.y = -kJumpVelocity;  // screen y points down
                    engine->GetSoundManager().GetSound("jump").play();
                } else if (onLeftWall || onRightWall) {
                    mc.velocity.y = -kJumpVelocity;
                    mc.velocity.x = onLeftWall ? kWallJumpVelocity : -kWallJumpVelocity;
                    engine->GetSoundManager().GetSound("jump").play();
                }
            }

            // ---- Variable jump height: releasing Z while rising cuts the jump ----
            if (jumpReleased && mc.velocity.y < 0) mc.velocity.y *= kJumpCutFactor;
            if (!onGround && mc.velocity.y > 0 &&
                ((left && !right && onLeftWall) || (right && !left && onRightWall))) {
                wallSliding = true;
            }

            // ---- Fast fall ----
            if (down && mc.velocity.y > 0) mc.velocity.y += kFastFallAcceleration * kFixedDt;

            // AI-ASSISTED (Claude): a finished dash falls back to normal run speed
            if (mc.velocity.x > kMaxRunVelocity) mc.velocity.x = kMaxRunVelocity;
            if (mc.velocity.x < -kMaxRunVelocity) mc.velocity.x = -kMaxRunVelocity;

            // ---- Left/right ----
            const float accel = onGround ? kGroundRunAcceleration : kAirRunAcceleration;
            if (right == left) {  // neither key, or both: slow down to a stop
                const float drop = accel * kFixedDt;
                if (mc.velocity.x > drop) mc.velocity.x -= drop;
                else if (mc.velocity.x < -drop) mc.velocity.x += drop;
                else mc.velocity.x = 0;
                continue;
            }
            const float dir = right ? 1.0f : -1.0f;
            mc.velocity.x += dir * accel * kFixedDt;
            if (mc.velocity.x > kMaxRunVelocity) mc.velocity.x = kMaxRunVelocity;
            if (mc.velocity.x < -kMaxRunVelocity) mc.velocity.x = -kMaxRunVelocity;
        }
        if (wallSliding) {
            auto& sound = engine->GetSoundManager().GetSound("wall");
            sound.setLooping(true);
            if (sound.getStatus() != sf::Sound::Status::Playing) sound.play();
            mWallPlaying = true;
        } else if (mWallPlaying) {
            engine->GetSoundManager().GetSound("wall").stop();
            mWallPlaying = false;
        }
        return kContinue;
    }

private:
    bool mWallPlaying = false;
};

}  // namespace ECSEngine