#pragma once

#include <unordered_map>
#include <limits>
#include <string>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

// The sprite rectangle is relative to the entity location in the world
// This is only defined if an entity also has a LocationComponent.
class SpawnComponent {
public:
    EntityID entityID = std::numeric_limits<EntityID>::max();  // The entity that this spawn component is attached to
    std::string spawnType;
    SpriteID spawnTextureID = std::numeric_limits<SpriteID>::max();
    int timeToNextSpawn = 0;
    int timeBetweenSpawns = 0;  // in frames
    int totalSpawns = 0;

private:
};

}  // namespace ECSEngine
