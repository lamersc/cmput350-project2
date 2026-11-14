#pragma once

#include <algorithm>
#include <string>

#include "ECSEngine.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

/** Updates the three digit sprites referenced by each score component. */
template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    /**
     * Set each live digit sprite to the corresponding decimal texture. Negative
     * scores display as zero, and only the lowest three digits are displayed.
     * Invalid digit entities or entities without a sprite are skipped.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& scores = em.template GetComponentStorage<ScoreComponent>();
        for (size_t i = 0; i < scores.mStorage.size(); ++i) {
            if (!scores.mValid[i]) continue;
            const auto& score = scores.mStorage[i];
            int value = std::max(0, score.score);
            for (int digit = 2; digit >= 0; --digit) {
                EntityID id = score.digitID[digit];
                if (em.IsValid(id) && em.template HasComponent<SpriteComponent>(id)) {
                    em.template GetComponent<SpriteComponent>(id).textureID =
                        score.digitTextureID[value % 10];
                }
                value /= 10;
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
