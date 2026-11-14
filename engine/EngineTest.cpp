// AI-ASSISTED (Claude): Headless unit tests for finished engine parts template given, tweaked some ases.
#include <cmath>
#include <cstdio>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "InputSystem.h"
#include "ScoreSystem.h"
#include "SpawnSystem.h"
#include "TileMap.h"

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
        E::CameraFollower, E::CameraShake, E::ScoreComponent, E::RespawnComponent,     \
        E::SpawnComponent

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

// ---------- helpers for the new tests ----------
template <typename EM>
static E::EntityID MakePlayer(EM& em, bool onGround) {
    E::EntityID p = MakeDynamic(em, "player", 0, 0, 0, 0);
    em.AddComponent(p, E::InputComponent{p});
    em.AddComponent(p, E::GravityComponent{{0, 1000}});
    if (onGround) em.template GetComponent<E::CollisionComponent>(p).onGround = 1;
    return p;
}

static void RunInput(Comps& engine) {
    E::InputSystem<COMPONENTS> in;
    in.Run(&engine);
}

static void TestInput(Comps& engine) {
    auto& em = engine.GetEntityManager();
    const float groundStep = E::kGroundRunAcceleration * E::kFixedDt;
    const float airStep = E::kAirRunAcceleration * E::kFixedDt;

    // Run right on the ground uses the ground acceleration.
    E::EntityID p = MakePlayer(em, true);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Right);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, groundStep));
    em.RemoveEntity(p);

    // Run left in the air uses the (smaller) air acceleration.
    p = MakePlayer(em, false);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Left);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, -airStep));
    em.RemoveEntity(p);

    // Speed is clamped to kMaxRunVelocity.
    p = MakePlayer(em, true);
    em.GetComponent<E::MovementComponent>(p).velocity.x = 1000;
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Right);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, E::kMaxRunVelocity));
    em.RemoveEntity(p);

    // No key: decelerate toward zero. Both keys: same.
    p = MakePlayer(em, true);
    em.GetComponent<E::MovementComponent>(p).velocity.x = 100;
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, 100 - groundStep));
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Left);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Right);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, 100 - 2 * groundStep));
    em.RemoveEntity(p);

    // Deceleration stops at zero instead of overshooting.
    p = MakePlayer(em, true);
    em.GetComponent<E::MovementComponent>(p).velocity.x = 10;
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, 0));
    em.RemoveEntity(p);

    // Jump: only on the ground, only on the press (not while held).
    p = MakePlayer(em, true);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Z);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, -E::kJumpVelocity));
    em.RemoveEntity(p);

    p = MakePlayer(em, true);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Z);
    em.GetComponent<E::InputComponent>(p).lastKeydown.set(E::Key::Z);  // held, not pressed
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, 0));
    em.RemoveEntity(p);

    // No double jump in mid-air away from walls.
    p = MakePlayer(em, false);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Z);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, 0));
    em.RemoveEntity(p);

    // Wall jump: up and away from a wall on the left (then air deceleration applies).
    p = MakePlayer(em, false);
    em.GetComponent<E::CollisionComponent>(p).onLeftWall = 1;
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Z);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, -E::kJumpVelocity));
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x,
               E::kWallJumpVelocity - airStep));
    em.RemoveEntity(p);

    // Releasing jump while rising cuts the jump short.
    p = MakePlayer(em, false);
    em.GetComponent<E::MovementComponent>(p).velocity.y = -500;
    em.GetComponent<E::InputComponent>(p).lastKeydown.set(E::Key::Z);  // was down, now up
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, -500 * E::kJumpCutFactor));
    em.RemoveEntity(p);

    // Down key adds extra fall speed.
    p = MakePlayer(em, false);
    em.GetComponent<E::MovementComponent>(p).velocity.y = 100;
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Down);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y,
               100 + E::kFastFallAcceleration * E::kFixedDt));
    em.RemoveEntity(p);

    // Dash: allowed once the interval has passed; sets speed, pauses gravity, resets timer.
    p = MakePlayer(em, false);
    em.GetComponent<E::InputComponent>(p).lastDash = static_cast<int>(E::kDashInterval);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::C);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Right);
    RunInput(engine);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, E::kDashVelocity));
    CHECK(em.GetComponent<E::GravityComponent>(p).disabledTimer == E::kDashFrames);
    CHECK(em.GetComponent<E::InputComponent>(p).lastDash == 0);
    em.RemoveEntity(p);

    // Dash is blocked during the cooldown (lastDash just counts up).
    p = MakePlayer(em, false);
    em.GetComponent<E::InputComponent>(p).lastDash = 10;
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::C);
    em.GetComponent<E::InputComponent>(p).keydown.set(E::Key::Right);
    RunInput(engine);
    CHECK(em.GetComponent<E::MovementComponent>(p).velocity.x < 100);
    CHECK(em.GetComponent<E::InputComponent>(p).lastDash == 11);
    em.RemoveEntity(p);
}

static void TestCollisionExtras(Comps& engine) {
    auto& em = engine.GetEntityManager();

    // Resting on a floor stays grounded and does not sink.
    E::EntityID floor = MakeStatic(em, "floor", 0, 400, 800, 64);
    E::EntityID p = MakeDynamic(em, "rest", 100, 336, 0, 0);
    Step(engine);
    Step(engine);
    CHECK(em.GetComponent<E::CollisionComponent>(p).onGround == 1);
    CHECK(Near(em.GetComponent<E::LocationComponent>(p).loc.y, 336));
    em.RemoveEntity(p);
    em.RemoveEntity(floor);

    // Wall on the player's left side.
    E::EntityID wall = MakeStatic(em, "wall", 500, 100, 64, 300);
    p = MakeDynamic(em, "leftwall", 570, 200, -600, 0);
    Step(engine);
    CHECK(Near(em.GetComponent<E::LocationComponent>(p).loc.x, 564));
    CHECK(em.GetComponent<E::CollisionComponent>(p).onLeftWall == 1);
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.x, 0));
    em.RemoveEntity(p);
    em.RemoveEntity(wall);
}

static void TestSpikes(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID spike = MakeStatic(em, "spike", 200, 400, 64, 20);
    em.GetComponent<E::CollisionComponent>(spike).doesDamage = 1;

    E::EntityID p = MakeDynamic(em, "player", 200, 350, 0, 30);
    em.AddComponent(p, E::InputComponent{p});
    em.AddComponent(p, E::RespawnComponent{{50, 50}});
    Step(engine);
    const auto& loc = em.GetComponent<E::LocationComponent>(p).loc;
    CHECK(Near(loc.x, 50) && Near(loc.y, 50));
    CHECK(Near(em.GetComponent<E::MovementComponent>(p).velocity.y, 0));
    em.RemoveEntity(p);
    em.RemoveEntity(spike);
}

static void TestStarPickup(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID p = MakeDynamic(em, "player", 100, 100, 0, 0);
    em.AddComponent(p, E::InputComponent{p});
    em.AddComponent(p, E::ScoreComponent{});
    E::EntityID near = MakeDynamic(em, "nearstar", 120, 120, 0, 0);
    E::EntityID far = MakeDynamic(em, "farstar", 500, 500, 0, 0);
    Step(engine);
    CHECK(em.GetComponent<E::ScoreComponent>(p).score == 1);
    CHECK(!em.IsValid(near));
    CHECK(em.IsValid(far));
    em.RemoveEntity(far);
    em.RemoveEntity(p);
}

static void TestScoreSystem(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID owner = em.CreateEntity("scorer");
    E::ScoreComponent score;
    score.score = 42;
    for (int d = 0; d < 10; ++d) score.digitTextureID[d] = 100 + static_cast<E::SpriteID>(d);
    E::EntityID digits[3];
    for (int i = 0; i < 3; ++i) {
        digits[i] = em.CreateEntity("digit");
        em.AddComponent(digits[i], E::SpriteComponent{0, {{0, 0}, 32, 32}, false});
        score.digitID[i] = digits[i];
    }
    em.AddComponent(owner, score);

    E::ScoreSystem<COMPONENTS> sys;
    sys.Run(&engine);
    // 42 is shown as 0 4 2 (hundreds, tens, ones).
    CHECK(em.GetComponent<E::SpriteComponent>(digits[0]).textureID == 100);
    CHECK(em.GetComponent<E::SpriteComponent>(digits[1]).textureID == 104);
    CHECK(em.GetComponent<E::SpriteComponent>(digits[2]).textureID == 102);
    for (auto id : digits) em.RemoveEntity(id);
    em.RemoveEntity(owner);
}

class CountingHandler : public E::SpawnHandler<COMPONENTS> {
public:
    int count = 0;
    E::SpriteID lastTexture = 0;
    E::EntityID lastSource = 0;
    void Spawn(E::EngineView<COMPONENTS>*, const E::SpawnComponent& r) override {
        ++count;
        lastTexture = r.spawnTextureID;
        lastSource = r.entityID;
    }
};

static void TestSpawn(Comps& engine) {
    auto& em = engine.GetEntityManager();
    auto handler = std::make_unique<CountingHandler>();
    CountingHandler* h = handler.get();
    E::SpawnSystem<COMPONENTS> spawn(std::move(handler));

    E::EntityID s = em.CreateEntity("spawner");
    E::SpawnComponent sp;
    sp.spawnTextureID = 7;
    sp.timeToNextSpawn = 2;
    sp.timeBetweenSpawns = 3;
    sp.totalSpawns = 2;
    em.AddComponent(s, sp);

    spawn.Run(&engine);  // timer 2 -> 1
    CHECK(h->count == 0);
    spawn.Run(&engine);  // timer reaches 0: first spawn
    CHECK(h->count == 1);
    CHECK(h->lastSource == s && h->lastTexture == 7);
    spawn.Run(&engine);
    spawn.Run(&engine);
    CHECK(h->count == 1);
    spawn.Run(&engine);  // second spawn after the 3-frame gap
    CHECK(h->count == 2);
    spawn.Run(&engine);
    spawn.Run(&engine);  // totalSpawns used up: nothing more
    CHECK(h->count == 2);
    CHECK(em.GetComponent<E::SpawnComponent>(s).totalSpawns == 0);
    em.RemoveEntity(s);
}

class RecordSystem : public E::System<COMPONENTS> {
public:
    RecordSystem(int id, std::vector<int>* log, E::tSystemReturn result)
        : mId(id), mLog(log), mResult(result), mName("Record" + std::to_string(id)) {}
    E::tSystemReturn Run(E::EngineView<COMPONENTS>*) override {
        mLog->push_back(mId);
        return mResult;
    }
    const std::string& GetName() const override { return mName; }

private:
    int mId;
    std::vector<int>* mLog;
    E::tSystemReturn mResult;
    std::string mName;
};

static void TestSystemManager(Comps& engine) {
    std::vector<int> log;
    E::SystemManager<COMPONENTS> sm;
    sm.AddSystem(std::make_unique<RecordSystem>(1, &log, E::kContinue));
    sm.AddSystem(std::make_unique<RecordSystem>(2, &log, E::kContinue));
    CHECK(sm.Run(&engine));
    CHECK(log.size() == 2 && log[0] == 1 && log[1] == 2);  // insertion order

    sm.AddSystem(std::make_unique<RecordSystem>(3, &log, E::kExit));
    sm.AddSystem(std::make_unique<RecordSystem>(4, &log, E::kContinue));
    log.clear();
    CHECK(!sm.Run(&engine));  // system 3 asks to exit
    CHECK(log.size() == 3 && log[2] == 3);  // system 4 never ran

    sm.ClearSystems();
    log.clear();
    CHECK(sm.Run(&engine));
    CHECK(log.empty());
}

static bool ParseThrows(const char* text) {
    std::istringstream in(text);
    try {
        E::ParseTileMap(in);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static void TestTileMap() {
    std::istringstream in(
        "dictionary 2\n"
        "n sheet.png 128 64 64 64 0 0 64 64\n"
        "map origin 10 20 tile 64 64 size 3 2\n"
        ".n.\n"
        "nn.\n");
    E::TileMap m = E::ParseTileMap(in);
    CHECK(m.dictionaryType == 2);
    CHECK(m.width == 3 && m.height == 2);
    CHECK(m.rows.size() == 2 && m.rows[0] == ".n.");
    CHECK(m.dictionary.size() == 1);
    CHECK(Near(m.origin.x, 10) && Near(m.origin.y, 20));
    CHECK(Near(m.dictionary.at('n').collisionBox.width, 64));
    CHECK(Near(m.dictionary.at('n').sourceRect.topLeft.x, 128));

    // Bad files must throw instead of producing a half-built map.
    CHECK(ParseThrows("dictionary 1\nn s.png 0 0 64 64\nmap origin 0 0 tile 64 64 size 3 1\n..\n"));
    CHECK(ParseThrows("dictionary 3\n"));
    CHECK(ParseThrows("dictionary 1\nn s.png 0 0 64 64\nn s.png 0 0 64 64\n"));
    CHECK(ParseThrows(""));
}

static void TestEntityManagerReuse(Comps& engine) {
    auto& em = engine.GetEntityManager();
    E::EntityID a = em.CreateEntity("a");
    em.AddComponent(a, E::LocationComponent{{1, 1}});
    size_t genA = em.GetGeneration(a);
    em.RemoveEntity(a);

    E::EntityID b = em.CreateEntity("b");
    CHECK(b == a);                                   // ID reused
    CHECK(em.GetGeneration(b) != genA);              // but generation changed
    CHECK(!em.HasComponent<E::LocationComponent>(b));  // no leftover data

    // Freed component slots are reused, so the storage does not grow.
    size_t before = em.GetComponentStorage<E::LocationComponent>().mStorage.size();
    em.AddComponent(b, E::LocationComponent{{2, 2}});
    em.RemoveComponent<E::LocationComponent>(b);
    em.AddComponent(b, E::LocationComponent{{3, 3}});
    CHECK(em.GetComponentStorage<E::LocationComponent>().mStorage.size() <= before + 1);
    CHECK(Near(em.GetComponent<E::LocationComponent>(b).loc.x, 3));
    em.RemoveEntity(b);
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

    TestInput(engine);
    TestCollisionExtras(engine);
    TestSpikes(engine);
    TestStarPickup(engine);
    TestScoreSystem(engine);
    TestSpawn(engine);
    TestSystemManager(engine);
    TestTileMap();
    TestEntityManagerReuse(engine);

    std::printf("%d passed, %d failed\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}