#pragma once

#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <optional>

namespace ECSEngine {

template <typename... Components>
class EngineView {
public:
    // AI-ASSISTED (Claude)
    virtual ~EngineView() = default;

    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
void ProcessInputEvent(EntityManager<Components...>& entities, const sf::Event& event) {
    if constexpr ((std::is_same_v<InputComponent, Components> || ...)) {
        auto& inputs = entities.template GetComponentStorage<InputComponent>();
        const auto* pressed = event.getIf<sf::Event::KeyPressed>();
        const auto* released = event.getIf<sf::Event::KeyReleased>();
        const bool lostFocus = event.is<sf::Event::FocusLost>();
        if (!lostFocus && pressed == nullptr && released == nullptr) return;

        size_t bit = Key::ScanCount;
        if (!lostFocus) {
            const auto code = pressed != nullptr ? pressed->scancode : released->scancode;
            if (code == sf::Keyboard::Scancode::Unknown) return;
            bit = static_cast<size_t>(code);
            if (bit >= Key::ScanCount) return;
        }
        for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (!inputs.mValid[i]) continue;
            if (lostFocus) inputs.mStorage[i].keydown.reset();
            else inputs.mStorage[i].keydown.set(bit, pressed != nullptr);
        }
    }
}

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    // For debugging - pause and step the engine
    // AI-ASSISTED (Claude)
    bool running = true;   // false once the engine should stop
    bool step = false;     // true = advance one frame while paused
    int debug = 0;         // debug draw level, 0 = off
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() { mSystemManager.ClearSystems(); }

// AI-ASSISTED (Claude): Run
/**
 * The main game loop. Each frame: handle events, clear the window, run all
 * systems in order. Returns when the window is closed or a system returns kExit.
 * SpriteSystem is responsible for calling display() (per the spec).
 */
template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    window->setFramerateLimit(60);  // cap the frame rate so we don't spin the CPU
    running = true;

    while (running && window->isOpen()) {
        if (!ProcessEvents()) break;               // window closed
        window->clear();                           // start a fresh frame
        if (!mSystemManager.Run(this)) break;      // a system asked to exit
    }
    window->close();
}

// AI-ASSISTED (Claude): ProcessEvents
/**
 * Reads all pending SFML events and updates every InputComponent.
 * @return true to keep running, false if the window was closed.
 * The InputComponent storage is fetched fresh each call; the reference is not kept.
 */
template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    // "template" is needed because mEntityManager's type depends on Components.
    // 1. Remember last frame's keys, skipping dead slots.
    if constexpr ((std::is_same_v<InputComponent, Components> || ...)) {
        auto& inputs = mEntityManager.template GetComponentStorage<InputComponent>();
        for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (inputs.mValid[i]) inputs.mStorage[i].lastKeydown = inputs.mStorage[i].keydown;
        }
    }

    // 2. Handle events.
    auto window = mWindowManager.GetWindow();
    while (const std::optional<sf::Event> event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) return false;

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            if (resized->size.x > 0 && resized->size.y > 0) {
                window->setView(sf::View(sf::FloatRect(
                    {0.f, 0.f}, {static_cast<float>(resized->size.x),
                                static_cast<float>(resized->size.y)})));
            }
        }
        const auto* pressed = event->getIf<sf::Event::KeyPressed>();
        const auto* released = event->getIf<sf::Event::KeyReleased>();
        if (pressed == nullptr && released == nullptr) {
            ProcessInputEvent(mEntityManager, *event);
            continue;
        }

        sf::Keyboard::Scancode code = (pressed != nullptr) ? pressed->scancode : released->scancode;
        if (code == sf::Keyboard::Scancode::Unknown) continue;  // -1 is not a valid bit

        // 3. Set or clear that key on every live input component.
        ProcessInputEvent(mEntityManager, *event);
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {}

}  // namespace ECSEngine
