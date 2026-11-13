#pragma once

#include <bitset>
#include <cmath>
#include <unordered_map>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"

namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    // AI-ASSISTED (Claude): Run
    /**
     * Draws every entity that has a SpriteComponent and a LocationComponent,
     * in entity-ID order, then calls display() once.
     * Resources are reached only through the EngineView API.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& sprites = engine->GetSpriteManager();
        auto& wm = engine->GetWindowManager();
        auto window = wm.GetWindow();

        for (const auto& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id)) continue;                                // skip removed entities
            if (!em.template HasComponent<SpriteComponent>(id)) continue;
            if (!em.template HasComponent<LocationComponent>(id)) continue;

            const SpriteComponent& sc = em.template GetComponent<SpriteComponent>(id);
            const LocationComponent& lc = em.template GetComponent<LocationComponent>(id);

            // Sprite rectangle is relative to the entity's location.
            Rect world{{lc.loc.x + sc.bounds.topLeft.x, lc.loc.y + sc.bounds.topLeft.y}, sc.bounds.width, sc.bounds.height};
            Rect screen = sc.worldSpace ? wm.WorldToWindow(world) : world;

            sf::Sprite& sprite = sprites.GetSprite(sc.textureID);
            sprite.setPosition({screen.topLeft.x, screen.topLeft.y});
            // scale texture rect to the destination size
            sf::IntRect src = sprite.getTextureRect();
            if (src.size.x == 0 || src.size.y == 0) continue;
            sprite.setScale({screen.width / std::abs(static_cast<float>(src.size.x)),
                            screen.height / std::abs(static_cast<float>(src.size.y))});
            window->draw(sprite);
        }
        window->display();
        return kContinue;
    }
};

}  // namespace ECSEngine
