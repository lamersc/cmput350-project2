#pragma once

#include <algorithm>
#include <cmath>
#include <string>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
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
            Point2D velocity = mc != nullptr ? mc->velocity : Point2D();
            bool wasGround = dc.onGround != 0;
            bool wasCeiling = dc.onCeiling != 0;
            bool wasLeftWall = dc.onLeftWall != 0;
            bool wasRightWall = dc.onRightWall != 0;

            dc.onGround = 0;
            dc.onCeiling = 0;
            dc.onLeftWall = 0;
            dc.onRightWall = 0;

            const Box prev = ToBox(dc.previousCollisionBox);

            const bool isStar = IsStar(em, did);

            // ---- Vertical pass ----
            for (const auto& s : em) {
                Box sb;
                if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                Box cur = ToBox(dl.loc, dc.collisionBox);
                if (!Overlaps(cur, sb)) continue;

                if (prev.bottom <= sb.top + kEps) {  // came from above: land
                    dl.loc.y += sb.top - cur.bottom;
                    dc.onGround = 1;
                    if (mc != nullptr && mc->velocity.y > 0) mc->velocity.y = Bounce(mc->velocity.y, isStar);
                } else if (prev.top >= sb.bottom - kEps) {  // came from below: bonk head
                    dl.loc.y += sb.bottom - cur.top;
                    dc.onCeiling = 1;
                    if (mc != nullptr && mc->velocity.y < 0) mc->velocity.y = Bounce(mc->velocity.y, isStar);
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
                    if (mc != nullptr && mc->velocity.x > 0) mc->velocity.x = Bounce(mc->velocity.x, isStar);
                } else if (prev.left >= sb.right - kEps) {  // wall on our left
                    dl.loc.x += sb.right - cur.left;
                    dc.onLeftWall = 1;
                    if (mc != nullptr && mc->velocity.x < 0) mc->velocity.x = Bounce(mc->velocity.x, isStar);
                }
            }
            if (!em.template HasComponent<InputComponent>(did)) continue;
            if (dc.onGround && !wasGround && velocity.y > 0) {
                engine->GetSoundManager().GetSound("land").play();
            }
            bool hitX = (dc.onLeftWall && !wasLeftWall) || (dc.onRightWall && !wasRightWall);
            bool hitY = (dc.onGround && !wasGround) || (dc.onCeiling && !wasCeiling);
            if (hitX || hitY) {
                for (const auto& cameraEntity : em) {
                    EntityID cameraID = cameraEntity.GetID();
                    if (!em.IsValid(cameraID) ||
                        !em.template HasComponent<CameraComponent>(cameraID) ||
                        !em.template HasComponent<CameraFollower>(cameraID)) continue;
                    if (em.template GetComponent<CameraFollower>(cameraID).entityID != did)
                        continue;
                    CameraShake shake{
                        6, {hitX ? std::min(8.f, std::abs(velocity.x) * 0.01f) : 0.f,
                            hitY ? std::min(8.f, std::abs(velocity.y) * 0.01f) : 0.f}};
                    if (em.template HasComponent<CameraShake>(cameraID)) {
                        em.template GetComponent<CameraShake>(cameraID) = shake;
                    } else {
                        em.template AddComponent<CameraShake>(cameraID, shake);
                    }
                }
            }
            if (!em.template HasComponent<ScoreComponent>(did)) continue;
            for (const auto& star : em) {
                EntityID sid = star.GetID();
                if (sid == did || !em.IsValid(sid) ||
                    !em.template HasComponent<LocationComponent>(sid) ||
                    !em.template HasComponent<CollisionComponent>(sid) ||
                    em.template HasComponent<InputComponent>(sid)) continue;
                const auto& sc = em.template GetComponent<CollisionComponent>(sid);
                const auto& sl = em.template GetComponent<LocationComponent>(sid);
                if (sc.staticEntity ||
                    !Overlaps(ToBox(dl.loc, dc.collisionBox), ToBox(sl.loc, sc.collisionBox)))
                    continue;
                ++em.template GetComponent<ScoreComponent>(did).score;
                engine->GetSoundManager().GetSound("star").play();
                em.RemoveEntity(sid);
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

    // AI-ASSISTED (Claude): kStarBounceKeep, kStarRestSpeed, IsStar
    /// Fraction of the normal-direction speed a star keeps after a bounce (unitless).
    static constexpr float kStarBounceKeep = 0.7f;
    /// Below this normal-direction speed (px/s) a star stops bouncing and rests.
    /// Gravity adds about 1000 * kFixedDt = 16.7 px/s per frame, so 50 sits clearly above that.
    static constexpr float kStarRestSpeed = 50.f;

    /// A "star" is a valid dynamic collider that can move and has no InputComponent.
    static bool IsStar(EntityManager<Components...>& em, EntityID id) {
        if (!em.IsValid(id)) return false;
        if (!em.template HasComponent<CollisionComponent>(id)) return false;
        if (!em.template HasComponent<MovementComponent>(id)) return false;
        if (em.template HasComponent<InputComponent>(id)) return false;
        return em.template GetComponent<CollisionComponent>(id).staticEntity == 0;
    }

        // AI-ASSISTED (Claude): Bounce
    /// New velocity along the collision axis. Non-stars stop (0). Stars reflect and keep
    /// kStarBounceKeep of their speed, or stop if slower than kStarRestSpeed (px/s).
    static float Bounce(float v, bool isStar) {
        if (!isStar || std::abs(v) <= kStarRestSpeed) return 0.f;
        return -v * kStarBounceKeep;
    }
};

}  // namespace ECSEngine
