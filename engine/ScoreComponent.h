#pragma once

#include <unordered_map>
#include <limits>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

// ScoreSystem uses the ten digit sprite IDs and three digit entity IDs to draw
// a three-digit score, ordered from hundreds to ones.
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
