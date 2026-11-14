#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ECSEngine.h"
#include "SpawnComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

/** Creates an entity or performs another game-defined action for a spawn request. */
template <typename... Components>
class SpawnHandler {
public:
    virtual ~SpawnHandler() = default;

    /**
     * Handle a copied request using a borrowed engine view. The request
     * reference is only valid for the call; copy it if it must be kept.
     * Adding components may invalidate previously acquired component references.
     */
    virtual void Spawn(EngineView<Components...>* engine, const SpawnComponent& request) = 0;
};

/** Advances spawn timers and dispatches valid requests after scanning entities. */
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    using Handler = SpawnHandler<Components...>;

    /** Take ownership of the handler; without one, updates leave timers unchanged. */
    explicit SpawnSystem(std::unique_ptr<Handler> handler = nullptr)
        : mHandler(std::move(handler)) {}

    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    /**
     * Count down live spawners in frames, decrement remaining spawns and queue
     * requests. Call the handler after the scan only if the source entity and
     * component are still live and its ID has not been reused.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        if (!mHandler) return kContinue;
        auto& em = engine->GetEntityManager();
        std::vector<std::pair<SpawnComponent, size_t>> requests;
        for (const auto& entity : em) {
            EntityID id = entity.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<SpawnComponent>(id)) continue;
            auto& spawn = em.template GetComponent<SpawnComponent>(id);
            if (spawn.totalSpawns <= 0) continue;
            if (spawn.timeToNextSpawn > 0) --spawn.timeToNextSpawn;
            if (spawn.timeToNextSpawn > 0) continue;
            SpawnComponent request = spawn;
            request.entityID = id;
            requests.emplace_back(std::move(request), em.GetGeneration(id));
            --spawn.totalSpawns;
            spawn.timeToNextSpawn = std::max(1, spawn.timeBetweenSpawns);
        }
        for (const auto& [request, generation] : requests) {
            if (em.IsValid(request.entityID) &&
                em.GetGeneration(request.entityID) == generation &&
                em.template HasComponent<SpawnComponent>(request.entityID)) {
                mHandler->Spawn(engine, request);
            }
        }
        return kContinue;
    }

private:
    std::unique_ptr<Handler> mHandler;
};

}  // namespace ECSEngine
