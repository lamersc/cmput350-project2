#pragma once

// =============================================================================
// EntityManager.h
//
// AI DISCLOSURE (CMPUT 350 AI policy):
//  - Skeleton of the class and the API signatures: provided by instructor/TAs.
//  - ComponentStorage<T> (Component.h): provided by instructor/TAs.
//  - Everything marked "AI-ASSISTED (Claude)" below was written by Claude and
//    then read, typed in, and debugged by the team. This includes: the private
//    data members, CreateEntity, RemoveEntity, IsValid, Add/Remove/Has/Get
//    Component, GetComponentStorage, TypeIndex, ReleaseIfPresent, and the
//    documentation comments themselves.
//  - See AI_LOG.txt for the prompts used.
// =============================================================================

#include <cassert>
#include <iostream>
#include <limits>
#include <type_traits>

//AI Assisted for additional includes
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
#include <array>
#include <tuple>
#include <utility>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {


/**
 * Owns all entities and all component data for the game.
 *
 * DATA LAYOUT
 *  - An entity is an EntityID: its index into mEntities.
 *  - Component data lives in one ComponentStorage<T> per component type
 *    (struct-of-arrays), so a system looping over one component type reads
 *    contiguous memory.
 *  - A table (mComponentIndex) maps (entity, component type) -> slot in that
 *    type's storage, or kNullComponent if the entity does not have it.
 *
 * OWNERSHIP AND LIFETIME RULES (read before using references)
 *  - EntityManager owns everything it returns. Callers never delete anything.
 *  - Entity IDs are stable: an ID never changes while the entity is alive.
 *  - A removed entity's ID is reused by a later CreateEntity. Do not keep an
 *    EntityID after calling RemoveEntity on it unless you re-check IsValid
 *    (note: a reused ID passes IsValid, so also clear your copy).
 *  - A T& from GetComponent, and a ComponentStorage<T>& from
 *    GetComponentStorage, are valid until the next AddComponent<T>() call
 *    for that same T (the underlying vector may reallocate), or until the
 *    EntityManager is destroyed. Do not store them across frames or across
 *    adds; re-fetch each time.
 *  - Removing a component or entity does NOT erase data. The storage slot is
 *    marked invalid (mValid) and reused later. Code that loops directly over
 *    a ComponentStorage must skip slots where mValid[i] is false.
 *
 * Not thread-safe. Debug builds assert on misuse; release builds do not.
 *
 * @tparam Components every component type this game uses, e.g.
 *         EntityManager<LocationComponent, MovementComponent>.
 */
template <typename... Components>
class EntityManager {
public:
    // Marker stored in the index table meaning "entity does not have this component".
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // AI-ASSISTED (Claude)
    /**
     * Creates a new, empty entity (no components).
     * Reuses the ID of a previously removed entity if one exists,
     * otherwise appends a new ID.
     *
     * @param name debugging label only; not used by the engine.
     * @return the new entity's ID. Valid until RemoveEntity(id).
     */
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        if (!mFreeIDs.empty()) {
            EntityID id = mFreeIDs.back();
            mFreeIDs.pop_back();
            mEntities[id] = Entity(id, name, ++mNextGeneration);
            mValid[id] = true;
            mComponentIndex[id].fill(kNullComponent);
            return id;
        }
        EntityID id = mEntities.size();
        mEntities.push_back(Entity(id, name, ++mNextGeneration));
        mValid.push_back(true);

        tIndexRow row;
        row.fill(kNullComponent);
        mComponentIndex.push_back(row);

        return id;
    }

    // AI-ASSISTED (Claude)
    /**
     * Removes an entity and all of its components.
     * The slot in mEntities is kept (IDs never shift) and the ID goes on a
     * free list for reuse.
     *
     * @pre IsValid(entity) (asserted in debug builds).
     * @post IsValid(entity) is false until the ID is reused by CreateEntity.
     * Do not call while iterating a ComponentStorage that the entity uses if
     * you are relying on slot order staying the same.
     */
    void RemoveEntity(EntityID entity) {
        assert(IsValid(entity));
        (ReleaseIfPresent<Components>(entity), ...);  // runs once per component type
        mValid[entity] = false;
        mFreeIDs.push_back(entity);
    }

    // AI-ASSISTED (Claude)
    /**
     * Is this ID a live entity?
     * Systems iterating with begin()/end() must call this and skip entities
     * that return false, because removed entities leave gaps.
     *
     * @return true if the ID is in range and the entity has not been removed.
     */
    bool IsValid (EntityID entity) const {
        return entity < mEntities.size() && mValid[entity];
    }

    size_t GetGeneration(EntityID entity) const {
        assert(IsValid(entity));
        return mEntities[entity].mGeneration;
    }

    // AI-ASSISTED (Claude)
    /**
     * Attaches a component to an entity. The component is copied into storage.
     *
     * @pre IsValid(entity) and the entity does not already have a T.
     * @warning May reallocate T's storage: any T& or ComponentStorage<T>& you
     *          obtained earlier becomes invalid.
     */
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(IsValid(entity));
        assert(!HasComponent<T>(entity));

        ComponentStorage<T>& storage = GetComponentStorage<T>();
        size_t slot = 0;
        
        if (!storage.mFreeList.empty()) {          // reuse a freed slot
            slot = storage.mFreeList.back();
            storage.mFreeList.pop_back();
            storage.mStorage[slot] = std::move(component);
            storage.mValid[slot] = true;
        } else {                                   // grow the packed list
            slot = storage.mStorage.size();
            storage.mStorage.push_back(std::move(component));
            storage.mValid.push_back(true);
        }
        mComponentIndex[entity][TypeIndex<T>()] = slot;  // leave the note
    }

    // AI-ASSISTED (Claude)
    /**
     * Detaches a component from an entity.
     * The data is not erased: the storage slot is marked invalid and reused
     * by a later AddComponent<T>.
     *
     * @pre IsValid(entity) and the entity has a T.
     * @post Any T& previously returned for this entity must no longer be used.
     */
    template <typename T>
    void RemoveComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        size_t& slot = mComponentIndex[entity][TypeIndex<T>()];
        ComponentStorage<T>& storage = GetComponentStorage<T>();
        storage.mStorage[slot] = T{};
        storage.mValid[slot] = false;              // data stays, but marked dead
        storage.mFreeList.push_back(slot);
        slot = kNullComponent;                     // entity no longer has T
    }

    // AI-ASSISTED (Claude)
    /**
     * Does this entity have a component of type T?
     * O(1): one table lookup.
     *
     * @pre IsValid(entity).
     */
    template <typename T>
    bool HasComponent(EntityID entity) const {
        assert(IsValid(entity));
        return mComponentIndex[entity][TypeIndex<T>()] != kNullComponent;
    }

    // AI-ASSISTED (Claude)
    /**
     * Returns the entity's T component by reference, so it can be modified.
     *
     * @pre IsValid(entity) and HasComponent<T>(entity).
     * @return reference owned by the EntityManager. Valid until the next
     *         AddComponent<T>(), RemoveComponent<T>() on this entity, or
     *         RemoveEntity(entity). Do not store it; re-fetch when needed.
     */
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        size_t slot = mComponentIndex[entity][TypeIndex<T>()];
        return GetComponentStorage<T>().mStorage[slot];
    }

    // AI-ASSISTED (Claude)
    /**
     * Returns all components of type T, for systems that loop over every T.
     * Slots where mValid[i] is false are dead and must be skipped.
     * Note: slot index i is NOT an EntityID. Use GetComponent<T>(entity) to
     * look up by entity, or store the owning EntityID inside the component.
     *
     * @return reference owned by the EntityManager. Valid until the manager is
     *         destroyed, but its contents (and the vectors inside) change when
     *         AddComponent<T> reallocates, so do not hold element references
     *         or iterators across adds.
     */
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    //FROM SLIDES "Lecture 14: Project #2 (Part 1) - Entity Manager"
    // These iterate ALL slots including removed ones. Skip dead entities with
    //     if (!em.IsValid(e.GetID())) continue;
    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.begin(); }
    const_iterator cend() const { return mEntities.end(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); } 


private:
    size_t mNextGeneration = 0;
    // AI-ASSISTED (Claude): these three members
    std::vector<Entity> mEntities;      // slot i holds entity i (even if removed)
    std::vector<bool> mValid;           // mValid[i] == true means entity i is alive
    std::vector<EntityID> mFreeIDs;     // removed IDs we can hand out again

    // One storage per component type, e.g. EntityManager<Pos, Vel> holds
    // (ComponentStorage<Pos>, ComponentStorage<Vel>)
    std::tuple<ComponentStorage<Components>...> mStorages;

    // Row i = entity i. Column = component type. Value = slot in that type's storage.
    using tIndexRow = std::array<size_t, sizeof...(Components)>;
    std::vector<tIndexRow> mComponentIndex;

    // Which column belongs to T? Position of T in the template list.
    template <typename T>
    static constexpr size_t TypeIndex() {
        constexpr std::array<bool, sizeof...(Components)> matches{std::is_same_v<T, Components>...};
        for (size_t i = 0; i < matches.size(); ++i) {
            if (matches[i]) return i;
        }
        // AI-ASSISTED (Claude): fails at compile time if T is not in Components
static_assert((std::is_same_v<T, Components> || ...),
              "T is not one of this EntityManager's component types");
        return kNullComponent;  // not reached for a valid T
    }

    // Used by RemoveEntity: free T's component only if the entity has one
    template <typename T>
    void ReleaseIfPresent(EntityID entity) {
        if (HasComponent<T>(entity)) RemoveComponent<T>(entity);
    }
};

}  // namespace ECSEngine
