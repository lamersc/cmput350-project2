#include <fstream>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds
    auto& sounds = engine.GetSoundManager();
    sounds.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    sounds.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
    sounds.RegisterSound(gResourcePath + "sfx_gem.ogg", "star");
    sounds.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall");

    // Load maps

    // Place player

    // Setup score component

    // Setup Camera
    auto& em = engine.GetEntityManager();
    auto& wm = engine.GetWindowManager();
    auto bounds = wm.GetWindowBounds();
    auto camera = em.CreateEntity("camera");
    em.AddComponent(camera, ECSEngine::CameraComponent{
        wm.GetWorldCenter(), wm.GetWorldScale(),
        {{bounds.width * 0.1f, 0.f}, bounds.width * 0.8f, bounds.height},
        {{bounds.width * 0.3f, 0.f}, bounds.width * 0.4f, bounds.height}, 0.1f});
    for (const auto& entity : em) {
        if (em.IsValid(entity.GetID()) &&
            em.HasComponent<ECSEngine::InputComponent>(entity.GetID())) {
            em.AddComponent(camera, ECSEngine::CameraFollower{entity.GetID()});
            break;
        }
    }

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
