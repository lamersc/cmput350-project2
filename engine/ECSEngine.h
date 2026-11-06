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
ECSEngine<Components...>::~ECSEngine() {}

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
    auto& inputs = mEntityManager.template GetComponentStorage<InputComponent>();

    // 1. Remember last frame's keys, skipping dead slots.
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) inputs.mStorage[i].lastKeydown = inputs.mStorage[i].keydown;
    }

    // 2. Handle events.
    auto window = mWindowManager.GetWindow();
    while (const std::optional<sf::Event> event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) return false;

        const auto* pressed = event->getIf<sf::Event::KeyPressed>();
        const auto* released = event->getIf<sf::Event::KeyReleased>();
        if (pressed == nullptr && released == nullptr) continue;

        sf::Keyboard::Scancode code = (pressed != nullptr) ? pressed->scancode : released->scancode;
        if (code == sf::Keyboard::Scancode::Unknown) continue;  // -1 is not a valid bit
        size_t bit = static_cast<size_t>(code);

        // 3. Set or clear that key on every live input component.
        for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (inputs.mValid[i]) inputs.mStorage[i].keydown.set(bit, pressed != nullptr);
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {}

}  // namespace ECSEngine
