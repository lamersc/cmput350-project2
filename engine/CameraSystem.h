#pragma once

#include <string>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

/** Updates the window transform from camera, follower and optional shake data. */
template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    /**
     * Track a valid follower horizontally using the camera's hard and soft
     * screen-space bounds, then apply and advance any temporary shake offset.
     * Camera position and scale are read from the camera component.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& wm = engine->GetWindowManager();
        for (const auto& entity : em) {
            EntityID id = entity.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<CameraComponent>(id)) continue;
            auto& camera = em.template GetComponent<CameraComponent>(id);
            wm.SetWorldScale(camera.worldUnitsPerPixel);
            wm.SetWorldCenter(camera.position);
            Point2D trackedPosition;
            bool following = false;
            float hardLeft = camera.hardExclusion.topLeft.x;
            float hardRight = hardLeft + camera.hardExclusion.width;
            if (em.template HasComponent<CameraFollower>(id)) {
                EntityID target = em.template GetComponent<CameraFollower>(id).entityID;
                if (em.IsValid(target) && em.template HasComponent<LocationComponent>(target)) {
                    Point2D position = em.template GetComponent<LocationComponent>(target).loc;
                    if (em.template HasComponent<CollisionComponent>(target)) {
                        const auto& box =
                            em.template GetComponent<CollisionComponent>(target).collisionBox;
                        position.x += box.topLeft.x + box.width / 2.f;
                    } else if (em.template HasComponent<SpriteComponent>(target)) {
                        const auto& box = em.template GetComponent<SpriteComponent>(target).bounds;
                        position.x += box.topLeft.x + box.width / 2.f;
                    }
                    trackedPosition = position;
                    following = true;
                    float x = wm.WorldToWindow(position).x;
                    float softLeft = camera.softExclusion.topLeft.x;
                    float softRight = softLeft + camera.softExclusion.width;
                    float adjustment = 0;
                    if (x < hardLeft) adjustment = x - hardLeft;
                    else if (x > hardRight) adjustment = x - hardRight;
                    else if (x < softLeft) {
                        adjustment = x - softLeft;
                        if (adjustment < -0.01f) adjustment *= camera.speed;
                    } else if (x > softRight) {
                        adjustment = x - softRight;
                        if (adjustment > 0.01f) adjustment *= camera.speed;
                    }
                    camera.position.x += wm.WindowToWorld(adjustment);
                }
            }
            Point2D center = camera.position;
            if (em.template HasComponent<CameraShake>(id)) {
                auto& shake = em.template GetComponent<CameraShake>(id);
                if (shake.frameLimit > 0) {
                    float direction = shake.frameLimit % 2 == 0 ? 1.f : -1.f;
                    center.x += direction * shake.offset.x;
                    center.y += direction * shake.offset.y;
                    --shake.frameLimit;
                }
            }
            wm.SetWorldCenter(center);
            if (following) {
                float x = wm.WorldToWindow(trackedPosition).x;
                if (x < hardLeft) center.x += wm.WindowToWorld(x - hardLeft);
                else if (x > hardRight) center.x += wm.WindowToWorld(x - hardRight);
                wm.SetWorldCenter(center);
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
