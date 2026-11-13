// AI-ASSISTED (Claude): whole file. Headless unit tests for finished engine parts.
#include <cmath>
#include <cstdio>
#include <memory>

#include "ECSEngine.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "GravitySystem.h"
#include "MovementSystem.h"
#include "Timing.h"

namespace E = ECSEngine;

#define COMPONENTS                                                                     \
    E::InputComponent, E::SpriteComponent, E::LocationComponent, E::MovementComponent, \
        E::CollisionComponent, E::GravityComponent, E::CameraComponent,                \
        E::CameraFollower, E::CameraShake, E::ScoreComponent

using Comps = E::ECSEngine<COMPONENTS>;

static int gFailed = 0;
static int gPassed = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++gPassed; }                                           \
        else { ++gFailed; std::printf("FAIL line %d: %s\n", __LINE__, #cond); } \
    } while (0)

static bool Near(float a, float b, float eps = 0.05f) { return std::abs(a - b) < eps; }

// Dynamic 64x64 entity with location, movement and collision box.
template <typename EM>
static E::EntityID MakeDynamic(EM& em, const char* name, float x, float y, float vx, float vy) {
    E::EntityID id = em.CreateEntity(name);
    em.AddComponent(id, E::LocationComponent{{x, y}});
    em.AddComponent(id, E::MovementComponent{{vx, vy}});
    E::CollisionComponent c{};
    c.collisionBox = E::Rect{{0, 0}, 64, 64};
    em.AddComponent(id, c);
    return id;
}

// Static box at (x, y) with size w x h.
template <typename EM>
static E::EntityID MakeStatic(EM& em, const char* name, float x, float y, float w, float h) {
    E::EntityID id = em.CreateEntity(name);
    em.AddComponent(id, E::LocationComponent{{x, y}});
    E::CollisionComponent c{};
    c.staticEntity = 1;
    c.collisionBox = E::Rect{{0, 0}, w, h};
    em.AddComponent(id, c);
    return id;
}

// Runs CollisionLate, Movement, Collision once, in the spec order (no Gravity).
static void Step(Comps& engine) {
    E::CollisionLateSystem<COMPONENTS> late;
    E::MovementSystem<COMPONENTS> move;
    E::CollisionSystem<COMPONENTS> coll;
    late.Run(&engine);
    move.Run(&engine);
    coll.Run(&engine);
}

static void TestEntityManager(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID a = em.CreateEntity("a");
    E::EntityID b = em.CreateEntity("b");
    CHECK(a != b);
    CHECK(em.IsValid(a) && em.IsValid(b));

    CHECK(!em.HasComponent<E::LocationComponent>(a));
    em.AddComponent(a, E::LocationComponent{{5, 6}});
    CHECK(em.HasComponent<E::LocationComponent>(a));
    CHECK(!em.HasComponent<E::LocationComponent>(b));
    CHECK(Near(em.GetComponent<E::LocationComponent>(a).loc.x, 5));
    em.GetComponent<E::LocationComponent>(a).loc.x = 9;
    CHECK(Near(em.GetComponent<E::LocationComponent>(a).loc.x, 9));
    em.RemoveComponent<E::LocationComponent>(a);
    CHECK(!em.HasComponent<E::LocationComponent>(a));

    em.AddComponent(b, E::MovementComponent{{1, 2}});
    em.RemoveEntity(b);
    CHECK(!em.IsValid(b));
    CHECK(em.IsValid(a));

    // Iteration includes gaps; valid ones must be countable and skip removed ones.
    int valid = 0;
    for (const auto& ent : em) {
        if (em.IsValid(ent.GetID())) ++valid;
    }
    CHECK(valid == 1);

    // A new entity must come back valid with no leftover components.
    E::EntityID c = em.CreateEntity("c");
    CHECK(em.IsValid(c));
    CHECK(!em.HasComponent<E::MovementComponent>(c));
    em.RemoveEntity(a);
    em.RemoveEntity(c);
}

static void TestSpriteManager(Comps& engine) {
    auto& sm = engine.GetSpriteManager();
    E::SpriteID s1 = sm.RegisterTexture("assets/spritesheet-tiles-default.png", {{0, 0}, 64, 64});
    E::SpriteID s2 = sm.RegisterTexture("assets/spritesheet-tiles-default.png", {{64, 0}, 32, 32});
    CHECK(s1 != s2);
    CHECK(sm.GetSprite(s1).getTextureRect().size.x == 64);
    CHECK(sm.GetSprite(s2).getTextureRect().size.x == 32);
    // Same address both times: sprites must stay stable.
    CHECK(&sm.GetSprite(s1) == &sm.GetSprite(s1));
}

static void TestMovement(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID id = em.CreateEntity("mover");
    em.AddComponent(id, E::LocationComponent{{0, 0}});
    em.AddComponent(id, E::MovementComponent{{60, -120}});
    E::MovementSystem<COMPONENTS> move;
    move.Run(&engine);
    const auto& l = em.GetComponent<E::LocationComponent>(id).loc;
    CHECK(Near(l.x, 60 * E::kFixedDt));
    CHECK(Near(l.y, -120 * E::kFixedDt));
    em.RemoveEntity(id);
}

static void TestGravity(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::GravitySystem<COMPONENTS> grav;

    // Plain entity (star-like): v += a * dt, no fall multiplier.
    E::EntityID star = MakeDynamic(em, "star", 0, 0, 0, 100);
    em.AddComponent(star, E::GravityComponent{{0, 1000}});
    grav.Run(&engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(star).velocity.y,
               100 + 1000 * E::kFixedDt));

    // Player (InputComponent): falling gets the multiplier.
    E::EntityID p = MakeDynamic(em, "player", 0, 0, 0, 100);
    em.AddComponent(p, E::InputComponent{p});
    em.AddComponent(p, E::GravityComponent{{0, 1000}});
    grav.Run(&engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y,
               100 + 1000 * E::kFallGravityMultiplier * E::kFixedDt));

    // Rising player uses plain gravity.
    em.GetComponent<E::MovementComponent>(p).velocity.y = -300;
    grav.Run(&engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, -300 + 1000 * E::kFixedDt));

    // Wall slide: on a wall, in the air, fall speed is capped.
    em.GetComponent<E::MovementComponent>(p).velocity.y = 500;
    em.GetComponent<E::CollisionComponent>(p).onRightWall = 1;
    grav.Run(&engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, E::kWallSlideSpeed));

    // disabledTimer pauses gravity for that frame.
    E::EntityID d = MakeDynamic(em, "paused", 0, 0, 0, 0);
    E::GravityComponent g{{0, 1000}};
    g.disabledTimer = 2;
    em.AddComponent(d, g);
    grav.Run(&engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(d).velocity.y, 0));
    CHECK(em.GetComponent<E::GravityComponent>(d).disabledTimer == 1);

    em.RemoveEntity(star);
    em.RemoveEntity(p);
    em.RemoveEntity(d);
}

static void TestCollisionLate(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID id = MakeDynamic(em, "late", 10, 20, 0, 0);
    E::CollisionLateSystem<COMPONENTS> late;
    late.Run(&engine);
    const auto& prev = em.GetComponent<E::CollisionComponent>(id).previousCollisionBox;
    CHECK(Near(prev.topLeft.x, 10) && Near(prev.topLeft.y, 20));
    CHECK(Near(prev.width, 64) && Near(prev.height, 64));
    em.RemoveEntity(id);
}

static void TestCollision(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID floor = MakeStatic(em, "floor", 0, 400, 800, 64);
    E::EntityID wall = MakeStatic(em, "wall", 500, 100, 64, 300);
    E::EntityID ceil = MakeStatic(em, "ceil", 0, 0, 800, 50);

    // Player lands: snapped to the floor, onGround set, velocity.y zeroed.
    E::EntityID p = MakeDynamic(em, "player", 100, 330, 0, 600);
    em.AddComponent(p, E::InputComponent{p});
    Step(engine);
    CHECK(Near(em.GetComponent<E::LocationComponent>(p).loc.y, 336));
    CHECK(em.GetComponent<E::CollisionComponent>(p).onGround == 1);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, 0));
    em.RemoveEntity(p);

    // Player hits the wall from the left: snapped, onRightWall set, vx zeroed.
    p = MakeDynamic(em, "player2", 430, 200, 600, 0);
    em.AddComponent(p, E::InputComponent{p});
    Step(engine);
    CHECK(Near(em.GetComponent<E::LocationComponent>(p).loc.x, 436));
    CHECK(em.GetComponent<E::CollisionComponent>(p).onRightWall == 1);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, 0));
    em.RemoveEntity(p);

    // Player bonks the ceiling: snapped below it, onCeiling set, vy zeroed.
    p = MakeDynamic(em, "player3", 100, 55, 0, -600);
    em.AddComponent(p, E::InputComponent{p});
    Step(engine);
    CHECK(Near(em.GetComponent<E::LocationComponent>(p).loc.y, 50));
    CHECK(em.GetComponent<E::CollisionComponent>(p).onCeiling == 1);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, 0));
    em.RemoveEntity(p);

    // Star (no InputComponent) bounces off the floor keeping kStarBounceKeep (0.7).
    E::EntityID s = MakeDynamic(em, "star", 300, 330, 0, 400);
    Step(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(s).velocity.y, -280));
    em.RemoveEntity(s);

    // Slow star (below rest speed 50) stops instead of bouncing.
    s = MakeDynamic(em, "slowstar", 300, 335, 0, 40);
    Step(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(s).velocity.y, 0));
    em.RemoveEntity(s);

    // Star reflects off the wall and loses speed.
    s = MakeDynamic(em, "wallstar", 430, 200, 600, 0);
    Step(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(s).velocity.x, -420));
    em.RemoveEntity(s);

    em.RemoveEntity(floor);
    em.RemoveEntity(wall);
    em.RemoveEntity(ceil);
}

int main() {
    Comps engine(800, 600, "engine tests");
    auto& snd = engine.GetSoundManager();
    snd.RegisterSound("assets/footstep_grass_003.ogg", "land");
    snd.RegisterSound("assets/sfx_gem.ogg", "star");
    snd.RegisterSound("assets/sfx_jump.ogg", "jump");
    snd.RegisterSound("assets/footstep_snow_001.ogg", "wall");

    TestEntityManager(engine);
    TestSpriteManager(engine);
    TestMovement(engine);
    TestGravity(engine);
    TestCollisionLate(engine);
    TestCollision(engine);

    std::printf("%d passed, %d failed\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}