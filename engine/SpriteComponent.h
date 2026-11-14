#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// The sprite rectangle is relative to the entity's LocationComponent. Its
// coordinates and dimensions are in world units or screen pixels according to
// worldSpace; textureID is a SpriteManager ID, not a texture reference.
class SpriteComponent {
public:
    // AI-ASSISTED (Claude): default member initializers
    SpriteID textureID = 0;   // index from SpriteManager::RegisterTexture
    Rect bounds{};            // where to draw, relative to the entity's location
    bool worldSpace = true;  // If false, this sprite is drawn in screen space instead of world space
private:
};

}  // namespace ECSEngine
