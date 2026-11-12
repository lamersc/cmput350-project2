#pragma once

#include <string>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SystemManager.h"
namespace ECSEngine {

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    // AI-ASSISTED (Claude): Run, Box helpers
    /**
     * Resolves dynamic-vs-static collisions. Runs after MovementSystem.
     * For each dynamic entity: clears the onGround/onCeiling/onLeftWall/onRightWall
     * flags, resolves vertical overlaps first (so falling onto a floor does not
     * register as a wall hit), then horizontal ones. Which side the entity came
     * from is decided with previousCollisionBox (world coordinates, set by
     * CollisionLateSystem). Flags set here are read next frame by Input and Gravity.
     * TODO: dynamic-vs-dynamic (star pickup, bouncing), screen shake, sounds.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        for (const auto& d : em) {
            EntityID did = d.GetID();
            if (!em.IsValid(did)) continue;
            if (!em.template HasComponent<LocationComponent>(did)) continue;
            if (!em.template HasComponent<CollisionComponent>(did)) continue;

            CollisionComponent& dc = em.template GetComponent<CollisionComponent>(did);
            if (dc.staticEntity) continue;  // only dynamic entities are moved
            LocationComponent& dl = em.template GetComponent<LocationComponent>(did);
            MovementComponent* mc = nullptr;
            if (em.template HasComponent<MovementComponent>(did)) {
                mc = &em.template GetComponent<MovementComponent>(did);
            }

            dc.onGround = 0;
            dc.onCeiling = 0;
            dc.onLeftWall = 0;
            dc.onRightWall = 0;

            const Box prev = ToBox(dc.previousCollisionBox);

            // ---- Vertical pass ----
            for (const auto& s : em) {
                Box sb;
                if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                Box cur = ToBox(dl.loc, dc.collisionBox);
                if (!Overlaps(cur, sb)) continue;

                if (prev.bottom <= sb.top + kEps) {  // came from above: land
                    dl.loc.y += sb.top - cur.bottom;
                    dc.onGround = 1;
                    if (mc != nullptr && mc->velocity.y > 0) mc->velocity.y = 0;
                } else if (prev.top >= sb.bottom - kEps) {  // came from below: bonk head
                    dl.loc.y += sb.bottom - cur.top;
                    dc.onCeiling = 1;
                    if (mc != nullptr && mc->velocity.y < 0) mc->velocity.y = 0;
                }
            }

            // ---- Horizontal pass ----
            for (const auto& s : em) {
                Box sb;
                if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                Box cur = ToBox(dl.loc, dc.collisionBox);
                if (!Overlaps(cur, sb)) continue;

                if (prev.right <= sb.left + kEps) {  // came from the left: wall on our right
                    dl.loc.x += sb.left - cur.right;
                    dc.onRightWall = 1;
                    if (mc != nullptr && mc->velocity.x > 0) mc->velocity.x = 0;
                } else if (prev.left >= sb.right - kEps) {  // wall on our left
                    dl.loc.x += sb.right - cur.left;
                    dc.onLeftWall = 1;
                    if (mc != nullptr && mc->velocity.x < 0) mc->velocity.x = 0;
                }
            }
        }
        return kContinue;
    }

private:
    struct Box {
        float left = 0, top = 0, right = 0, bottom = 0;
    };
    static constexpr float kEps = 0.01f;  // tolerance for float comparison

    /// Box of a rect that is already in world coordinates.
    static Box ToBox(const Rect& r) {
        return Box{r.topLeft.x, r.topLeft.y, r.topLeft.x + r.width, r.topLeft.y + r.height};
    }
    /// Box of a rect stored relative to an entity location.
    static Box ToBox(const Point2D& loc, const Rect& r) {
        return Box{loc.x + r.topLeft.x, loc.y + r.topLeft.y, loc.x + r.topLeft.x + r.width,
                    loc.y + r.topLeft.y + r.height};
    }
    /// Strict overlap: boxes that only touch do not overlap.
    static bool Overlaps(const Box& a, const Box& b) {
        return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
    }
    /// Fills out with the world box of entity sid if it is a valid static collider
    /// other than 'self'. Returns false otherwise.
    static bool GetStaticBox(EntityManager<Components...>& em, EntityID sid, EntityID self,
                            Box& out) {
        if (sid == self || !em.IsValid(sid)) return false;
        if (!em.template HasComponent<LocationComponent>(sid)) return false;
        if (!em.template HasComponent<CollisionComponent>(sid)) return false;
        const CollisionComponent& sc = em.template GetComponent<CollisionComponent>(sid);
        if (!sc.staticEntity) return false;
        const LocationComponent& sl = em.template GetComponent<LocationComponent>(sid);
        out = ToBox(sl.loc, sc.collisionBox);
        return true;
    }
};

}  // namespace ECSEngine
