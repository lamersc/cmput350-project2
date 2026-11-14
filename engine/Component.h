#pragma once

#include <cstddef>
#include <vector>

namespace ECSEngine {

/**
 * EntityManager's storage for one component type. mStorage and mValid use the
 * same slot indices; a false mValid entry is a removed component. mFreeList
 * contains reusable slots. Slot indices are not entity IDs. Adding components
 * may grow the vectors and invalidate element references and iterators.
 */
template <typename T>
class ComponentStorage {
public:
    std::vector<T> mStorage;
    std::vector<bool> mValid;       // Quickly track if a component is valid
    std::vector<size_t> mFreeList;  // Avoid scanning for free slots
};

}  // namespace ECSEngine
