#pragma once

#include <unordered_map>
#include <limits>

#include "Entity.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

/** Camera position, scale and horizontal follower bounds. */
struct CameraComponent {
    Point2D position;  // [world coordinates] position in world coordinates
    float worldUnitsPerPixel = 1.f;
    Rect hardExclusion{};  // Screen-space bounds enforced for the follower's tracked point.
    Rect softExclusion{};  // Screen-space region before gradual tracking begins.
    float speed = 0.1f;    // Fraction of the soft-bound correction applied per update.
};

/** Entity whose location the camera follows. */
struct CameraFollower {
    EntityID entityID = std::numeric_limits<EntityID>::max();  // The entity that this camera follows
};

/** Alternating world-space offset and remaining update frames for camera shake. */
struct CameraShake {
    int frameLimit = 0;
    Point2D offset;
};

}  // namespace ECSEngine
