#pragma once

#include <unordered_map>
#include <limits>
#include <string>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

// Describes the type and texture to spawn, the remaining spawn count, and
// frame-based timers consumed by SpawnSystem.
class SpawnComponent {
public:
    EntityID entityID = std::numeric_limits<EntityID>::max();  // Owner ID filled in the dispatched request.
    std::string spawnType;
    SpriteID spawnTextureID = std::numeric_limits<SpriteID>::max();
    int timeToNextSpawn = 0;
    int timeBetweenSpawns = 0;  // in frames
    int totalSpawns = 0;

private:
};

}  // namespace ECSEngine
