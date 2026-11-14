#pragma once

#include <bitset>
#include <cstdint>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

/**
 * Collision state for an entity. collisionBox is relative to its location;
 * CollisionLateSystem records previousCollisionBox in world coordinates before
 * movement. CollisionSystem updates contact flags for the next frame.
 * staticEntity marks an obstacle; doesDamage marks a damaging obstacle.
 */
struct CollisionComponent {
    uint8_t staticEntity : 1 = 0;
    uint8_t doesDamage : 1 = 0;
    uint8_t onGround : 1 = 0;
    uint8_t onCeiling : 1 = 0;
    uint8_t onLeftWall : 1 = 0;
    uint8_t onRightWall : 1 = 0;
    Rect collisionBox{};          // The bounding box for collision detection
    Rect previousCollisionBox{};  // The previous bounding box for collision detection
};

}  // namespace ECSEngine
