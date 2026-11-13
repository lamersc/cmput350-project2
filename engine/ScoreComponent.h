#pragma once

#include <unordered_map>
#include <limits>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

// The sprite rectangle is relative to the entity location in the world
// This is only defined if an entity also has a LocationComponent.
class ScoreComponent {
public:
    int score = 0;
    SpriteID digitTextureID[10]{};
    EntityID digitID[3]{std::numeric_limits<EntityID>::max(),
                        std::numeric_limits<EntityID>::max(),
                        std::numeric_limits<EntityID>::max()};

private:
};

}  // namespace ECSEngine
