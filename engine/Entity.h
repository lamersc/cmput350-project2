#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <tuple>

// An entity can have any component it is templated with,
// but does not have these installed by default.
// This is a base class for all entities in the ECS system.

namespace ECSEngine {

using EntityID = size_t;

class Entity {
public:
    /** Return this entity's ID; it may be reused after the entity is removed. */
    EntityID GetID() const { return mID; }

private:
    Entity() = default;
    Entity(EntityID eid, const std::string& name, size_t generation)
        : mID(eid), mType(name), mGeneration(generation) {}
    EntityID mID;
    std::string mType;
    size_t mGeneration = 0;

    template <typename... T>
    friend class EntityManager;
};

}  // namespace ECSEngine
