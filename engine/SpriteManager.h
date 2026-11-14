#pragma once

// AI-ASSISTED (Claude): includes, private members, deleted copy operations,
// and doc comments. API signatures are from the spec/TA code.

#include <unordered_map>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

using SpriteID = size_t;


/**
 * Loads textures and hands out sprites that draw a rectangle of a texture.
 *
 * OWNERSHIP AND LIFETIME RULES
 *  - The manager owns every texture and sprite. Callers never delete them.
 *  - A texture is loaded once per file path and keeps a fixed address until
 *    the manager is destroyed.
 *  - A SpriteID is valid for the manager's whole life (sprites are never removed).
 *  - The sf::Sprite& from GetSprite is valid only until the next RegisterTexture
 *    call (the sprite vector may reallocate). Re-fetch it each time you draw.
 *  - Not copyable: a copied sprite would still point at the original's textures.
 *  - Relative paths are resolved from the program's working directory.
 */
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    
    /**
     * Registers a sprite that draws sourceRect of the image at texturePath.
     * The image is loaded only if this path was not registered before.
     *
     * @param sourceRect region of the texture, in texture pixels.
     * @return ID to pass to GetSprite.
     * @throws std::runtime_error if loading fails in a build with assertions disabled.
     *         With assertions enabled, a load failure triggers an assertion first.
     */
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    /**
     * Return the manager-owned sprite for a registered ID. The ID must be valid;
     * debug builds assert otherwise. The reference may be invalidated by the
     * next RegisterTexture call and must not outlive this manager.
     */
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::unordered_map<std::string, sf::Texture> mTextures;  ///< path -> texture (stable addresses)
    std::vector<sf::Sprite> mSprites;                        ///< index == SpriteID
};

}  // namespace ECSEngine
