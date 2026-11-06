#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// The sprite rectangle is relative to the entity location in the world
// This is only defined if an entity also has a LocationComponent.
class SpriteComponent {
public:
    // AI-ASSISTED (Claude): default member initializers
    SpriteID textureID = 0;   // index from SpriteManager::RegisterTexture
    Rect bounds{};            // where to draw, relative to the entity's location (world units)
    bool worldSpace = true;  // If false, this sprite is drawn in screen space instead of world space
private:
};

}  // namespace ECSEngine
