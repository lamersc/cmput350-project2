#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <utility>

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
#include "TileMap.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

// AI-ASSISTED (Claude): template, structure and fill-ins.
/**
 * Loads one map file and creates one entity per non-empty tile.
 * path = resource folder (with trailing /), file = map file name.
 * Dictionary 1 = sprites only. Dictionary 2 = sprites plus static collision boxes.
 * Entities are created row by row, so call this for the background map first
 * (sprites draw in entity-ID order).
 */
void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream in(path + file);
    if (!in) throw std::runtime_error("Cannot open map: " + path + file);
    ECSEngine::TileMap map = ECSEngine::ParseTileMap(in);  // throws on a bad file

    auto& em = engine.GetEntityManager();
    auto& sprites = engine.GetSpriteManager();

    // 1. Register each dictionary character's sprite once.
    std::unordered_map<char, ECSEngine::SpriteID> ids;
    for (const auto& [ch, def] : map.dictionary) {
        ids[ch] = sprites.RegisterTexture(path + def.texturePath, def.sourceRect);
    }

    // 2. One entity per tile.
    for (size_t row = 0; row < map.height; ++row) {
        for (size_t col = 0; col < map.width; ++col) {
            const char ch = map.rows[row][col];
            auto found = ids.find(ch);
            if (found == ids.end()) continue;  // '.' has no sprite, so it stays empty

            ECSEngine::Point2D pos(map.origin.x + static_cast<float>(col) * map.tileSize.x,
                                    map.origin.y + static_cast<float>(row) * map.tileSize.y);

            auto tile = em.CreateEntity("tile");
            em.AddComponent(tile, ECSEngine::LocationComponent{pos});
            em.AddComponent(tile, ECSEngine::SpriteComponent{
                found->second, {{0, 0}, map.tileSize.x, map.tileSize.y}, true
            });

            if (map.dictionaryType == 2) {
                ECSEngine::CollisionComponent cc{};
                cc.staticEntity = 1;
                cc.collisionBox = map.dictionary.at(ch).collisionBox;
                if (ch == 'x') cc.doesDamage = 1;   // spike tile
                em.AddComponent(tile, cc);
            }

            if (ch == 'S') {
                ECSEngine::SpawnComponent sp;
                sp.spawnType = "star";
                sp.timeToNextSpawn = 120;       // frames
                sp.timeBetweenSpawns = 180;     // frames
                sp.totalSpawns = 1000;
                em.AddComponent(tile, sp);      // entityID is filled in by SpawnSystem
            }
        }
    }
}

// AI-ASSISTED (Claude): skeleton
class StarSpawner : public ECSEngine::SpawnHandler<COMPONENTS> {
public:
    void Spawn(ECSEngine::EngineView<COMPONENTS>* engine, const ECSEngine::SpawnComponent& request) override {
        auto& em = engine->GetEntityManager();
        const ECSEngine::EntityID src = request.entityID;
        if (!em.IsValid(src) || !em.HasComponent<ECSEngine::LocationComponent>(src)) return;
        const ECSEngine::Point2D origin = em.GetComponent<ECSEngine::LocationComponent>(src).loc;  // copy, not a reference
        const float angle = static_cast<float>(std::rand() % 360) * 3.14159f / 180.f;

        auto star = em.CreateEntity("star");
        em.AddComponent(star, ECSEngine::LocationComponent{{origin.x, origin.y - 44.f}});
        em.AddComponent(star, ECSEngine::MovementComponent{{std::cos(angle) * 250.f, std::sin(angle) * 250.f}});
        em.AddComponent(star, ECSEngine::GravityComponent{{0.f, 1000.f}});
        em.AddComponent(star, ECSEngine::SpriteComponent{request.spawnTextureID, {{-12, -12}, 64, 64}, true});
        ECSEngine::CollisionComponent cc{};
        cc.collisionBox = ECSEngine::Rect{{0, 0}, 40, 40};
        em.AddComponent(star, cc);
    }
};

class StarCollisionHandler : public ECSEngine::DynamicContactHandler<COMPONENTS> {
public:
    void OnContact(ECSEngine::EngineView<COMPONENTS>* engine,
                   ECSEngine::EntityID first, ECSEngine::EntityID second) override {
        auto& em = engine->GetEntityManager();
        if (!em.HasComponent<ECSEngine::MovementComponent>(first) ||
            !em.HasComponent<ECSEngine::MovementComponent>(second) ||
            em.HasComponent<ECSEngine::InputComponent>(first) ||
            em.HasComponent<ECSEngine::InputComponent>(second)) return;

        auto& firstLocation = em.GetComponent<ECSEngine::LocationComponent>(first).loc;
        auto& secondLocation = em.GetComponent<ECSEngine::LocationComponent>(second).loc;
        const auto& firstBox = em.GetComponent<ECSEngine::CollisionComponent>(first).collisionBox;
        const auto& secondBox = em.GetComponent<ECSEngine::CollisionComponent>(second).collisionBox;
        const float firstX = firstLocation.x + firstBox.topLeft.x;
        const float firstY = firstLocation.y + firstBox.topLeft.y;
        const float secondX = secondLocation.x + secondBox.topLeft.x;
        const float secondY = secondLocation.y + secondBox.topLeft.y;
        const float overlapX = std::min(firstX + firstBox.width, secondX + secondBox.width) -
                               std::max(firstX, secondX);
        const float overlapY = std::min(firstY + firstBox.height, secondY + secondBox.height) -
                               std::max(firstY, secondY);
        if (overlapX <= 0 || overlapY <= 0) return;

        auto& firstVelocity = em.GetComponent<ECSEngine::MovementComponent>(first).velocity;
        auto& secondVelocity = em.GetComponent<ECSEngine::MovementComponent>(second).velocity;
        if (overlapX < overlapY) {
            const float direction = firstX + firstBox.width * 0.5f <=
                                    secondX + secondBox.width * 0.5f ? -1.f : 1.f;
            firstLocation.x += direction * overlapX * 0.5f;
            secondLocation.x -= direction * overlapX * 0.5f;
            if ((firstVelocity.x - secondVelocity.x) * direction < 0)
                std::swap(firstVelocity.x, secondVelocity.x);
        } else {
            const float direction = firstY + firstBox.height * 0.5f <=
                                    secondY + secondBox.height * 0.5f ? -1.f : 1.f;
            firstLocation.y += direction * overlapY * 0.5f;
            secondLocation.y -= direction * overlapY * 0.5f;
            if ((firstVelocity.y - secondVelocity.y) * direction < 0)
                std::swap(firstVelocity.y, secondVelocity.y);
        }
    }
};

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

    auto& em = engine.GetEntityManager();
    auto& sprites = engine.GetSpriteManager();
    ECSEngine::SpriteID starSprite = sprites.RegisterTexture(gResourcePath + "spritesheet-tiles-default.png", {{640, 320}, 64, 64});


    // Load maps
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    for (const auto& entity : em) {
        const ECSEngine::EntityID id = entity.GetID();
        if (em.IsValid(id) && em.HasComponent<ECSEngine::SpawnComponent>(id)) {
            em.GetComponent<ECSEngine::SpawnComponent>(id).spawnTextureID = starSprite;
        }
    }

    // Place player
    ECSEngine::SpriteID playerSprite = sprites.RegisterTexture(gResourcePath + "spritesheet-characters-default.png", {{0, 0}, 128, 128});

    auto player = em.CreateEntity("player");
    em.AddComponent(player, ECSEngine::LocationComponent{{800, 500}});   // above the floor
    em.AddComponent(player, ECSEngine::MovementComponent{});
    em.AddComponent(player, ECSEngine::GravityComponent{{0, 1000}});
    em.AddComponent(player, ECSEngine::SpriteComponent{playerSprite, {{-20, -38}, 96, 96}, true});
    ECSEngine::CollisionComponent pc{};
    pc.collisionBox = ECSEngine::Rect{{0, 0}, 56, 56};
    em.AddComponent(player, pc);
    em.AddComponent(player, ECSEngine::InputComponent{player});

    // Setup score component
    ECSEngine::ScoreComponent score;
    for (int d = 0; d < 10; ++d) {
        score.digitTextureID[d] = sprites.RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            {{832.f, static_cast<float>(13 - d) * 64.f}, 64, 64});
    }
    for (int i = 0; i < 3; ++i) {
        auto digit = em.CreateEntity("digit");
        em.AddComponent(digit, ECSEngine::LocationComponent{{10.f + 40.f * static_cast<float>(i), 10.f}});
        em.AddComponent(digit, ECSEngine::SpriteComponent{score.digitTextureID[0], {{0, 0}, 32, 32}, false});
        score.digitID[i] = digit;
    }
    em.AddComponent(player, score);

    // AI-ASSISTED (Claude): bonus star above the tall wall, to test the wall jump (spec)
    auto bonus = em.CreateEntity("bonusStar");
    em.AddComponent(bonus, ECSEngine::LocationComponent{{652.f, 196.f}});
    em.AddComponent(bonus, ECSEngine::SpriteComponent{starSprite, {{-12, -12}, 64, 64}, true});
    ECSEngine::CollisionComponent bc{};
    bc.collisionBox = ECSEngine::Rect{{0, 0}, 40, 40};
    em.AddComponent(bonus, bc);

    // Setup Camera
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

    em.AddComponent(player, ECSEngine::RespawnComponent{{800, 500}});

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>(
        std::make_unique<StarCollisionHandler>()));
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>(std::make_unique<StarSpawner>()));

    engine.Run();
    return 0;
}
