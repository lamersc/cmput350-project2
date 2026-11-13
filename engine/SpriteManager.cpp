#include "SpriteManager.h"

#include <stdexcept>

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    // AI-ASSISTED (Claude): RegisterTexture
    // try_emplace = "find this path, or insert an empty texture for it".
    // 'inserted' is true only the first time we see this path.
    auto [it, inserted] = mTextures.try_emplace(texturePath);
    if (inserted) {
        bool ok = it->second.loadFromFile(texturePath);
        assert(ok && "Failed to load texture");
        (void)ok;  // keeps release builds (assert compiled out) warning-free
        if (!ok) {
            mTextures.erase(it);
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }
    }

    // Rect stores floats, but SFML's texture rectangle wants ints.
    sf::IntRect source(
        {static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
        {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)});

    SpriteID id = mSprites.size();
    mSprites.emplace_back(it->second, source);  // sprite points at the texture stored in the map
    return id;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    // AI-ASSISTED (Claude): GetSprite
    assert(id < mSprites.size());  // catches IDs that never came from RegisterTexture
    return mSprites[id];
}

}  // namespace ECSEngine
