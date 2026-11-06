#include <cassert>
#include "EntityManager.h"
#include "SpriteManager.h"

struct Pos { int x = 0; int y = 0; };
struct Vel { int dx = 0; };

int main(/*int argc, char* argv[]*/) { 
    ECSEngine::EntityManager<Pos, Vel> em;
    auto e = em.CreateEntity("p");
    assert(!em.HasComponent<Pos>(e));
    em.AddComponent(e, Pos{3, 4});
    assert(em.HasComponent<Pos>(e) && !em.HasComponent<Vel>(e));
    assert(em.GetComponent<Pos>(e).x == 3);
    em.GetComponent<Pos>(e).x = 10;                  // edit through the reference
    assert(em.GetComponent<Pos>(e).x == 10);

    em.AddComponent(e, Vel{7});
    em.RemoveComponent<Pos>(e);
    assert(!em.HasComponent<Pos>(e));
    auto f = em.CreateEntity("q");
    em.AddComponent(f, Pos{1, 1});                   // should reuse Pos slot 0
    assert(em.GetComponentStorage<Pos>().mStorage.size() == 1);

    em.RemoveEntity(e);                              // frees its Vel too
    auto g = em.CreateEntity("r");                   // reuses e's ID
    assert(g == e && !em.HasComponent<Vel>(g));

    int alive = 0;
    for (const auto& ent : em) {
        if (em.IsValid(ent.GetID())) ++alive;
    }
    assert(alive == 2);   // e (reused by g) and f are alive
    return 0;


    //SpriteManager Tests
    ECSEngine::SpriteManager sm;
    auto a = sm.RegisterTexture("spritesheet-tiles-default.png", {{0, 0}, 64, 64});
    auto b = sm.RegisterTexture("spritesheet-tiles-default.png", {{64, 0}, 64, 64});
    assert(a != b);
    assert(sm.GetSprite(b).getTextureRect().position.x == 64);
    assert(&sm.GetSprite(a).getTexture() == &sm.GetSprite(b).getTexture());  // one shared texture
}
