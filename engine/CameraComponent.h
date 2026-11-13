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

struct CameraComponent {
    Point2D position;  // [world coordinates] position in world coordinates
    float worldUnitsPerPixel = 1.f;
    Rect hardExclusion{};  // [screen coordinates] Area where the camera is not allowed to go
                         // (negative of this rect)
    Rect softExclusion{};  // [screen coordinates] Area where the camera is allowed to go (but will
                         // scroll out)
    float speed = 0.1f;
};

struct CameraFollower {
    EntityID entityID = std::numeric_limits<EntityID>::max();  // The entity that this camera follows
};

struct CameraShake {
    int frameLimit = 0;
    Point2D offset;
};

}  // namespace ECSEngine
