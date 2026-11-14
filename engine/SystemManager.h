#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// #include "ECSEngine.h"

namespace ECSEngine {

enum tSystemReturn { kContinue, kExit };

template <typename... Components>
class EngineView;

/** Interface for an ordered engine system. */
template <typename... Components>
class System {
public:
    virtual ~System() = default;

    /** Run one update using the borrowed engine view; return kExit to stop the loop. */
    virtual tSystemReturn Run(EngineView<Components...>* scene) = 0;

    /** Return a borrowed system name, with lifetime defined by the implementation. */
    virtual const std::string& GetName() const = 0;
};

/** Owns systems and runs them in the order they were added. */
template <typename... Components>
class SystemManager {
public:
    /**
     * Run each system until one returns kExit. Return true if all continue,
     * or false on kExit. scene must point to a valid engine view. Do not add
     * or clear systems while this function is iterating over them.
     */
    bool Run(EngineView<Components...>* scene) {
        for (auto& system : mSystems) {
            if (system->Run(scene) == kExit)  // system wants to exit
            {
                std::cout << system->GetName() << " requested exit from system manager\n";
                return false;
            }
        }
        return true;
    }
    /** Transfer ownership of a non-null system to the manager. */
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        assert(system != nullptr);
        mSystems.push_back(std::move(system));
    }
    /** Destroy all owned systems and invalidate references or pointers to them. */
    void ClearSystems() { mSystems.clear(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
