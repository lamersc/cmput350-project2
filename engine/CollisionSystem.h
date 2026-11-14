#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SystemManager.h"
#include "RespawnComponent.h"
namespace ECSEngine {

template <typename... Components>
class DynamicContactHandler {
public:
    virtual ~DynamicContactHandler() = default;
    virtual void OnContact(EngineView<Components...>* engine, EntityID first, EntityID second) = 0;
};

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    using ContactHandler = DynamicContactHandler<Components...>;

    explicit CollisionSystem(std::unique_ptr<ContactHandler> handler = nullptr)
        : mDynamicContactHandler(std::move(handler)) {}

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
            if (dc.collisionBox.width <= 0 || dc.collisionBox.height <= 0) continue;

            const Box prev = ToBox(dc.previousCollisionBox);

            // AI-ASSISTED (Claude): touching a damaging static box sends the player to its respawn point
            if (em.template HasComponent<InputComponent>(did) &&
                em.template HasComponent<RespawnComponent>(did)) {
                const Box now = ToBox(dl.loc, dc.collisionBox);
                bool hurt = false;
                for (const auto& s : em) {
                    Box sb;
                    if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                    if (!em.template GetComponent<CollisionComponent>(s.GetID()).doesDamage) continue;
                    if (Overlaps(now, sb)) { hurt = true; break; }
                }
                if (hurt) {
                    dl.loc = em.template GetComponent<RespawnComponent>(did).loc;
                    if (mc != nullptr) mc->velocity = Point2D();
                    continue;
                }
            }

            const bool isStar = IsStar(em, did);

            // ---- Vertical pass ----
            for (const auto& s : em) {
                Box sb;
                if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                Box cur = ToBox(dl.loc, dc.collisionBox);
                if (prev.left >= sb.right || prev.right <= sb.left) continue;

                if (prev.bottom <= sb.top + kEps && cur.bottom >= sb.top - kEps &&
                    cur.bottom >= prev.bottom && velocity.y >= 0) {  // came from above: land
                    dl.loc.y += sb.top - cur.bottom;
                    dc.onGround = 1;
                    if (mc != nullptr && mc->velocity.y > 0) mc->velocity.y = Bounce(mc->velocity.y, isStar);
                } else if (prev.top >= sb.bottom - kEps && cur.top <= sb.bottom + kEps &&
                           cur.top <= prev.top && velocity.y <= 0) {  // came from below: bonk head
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
                if (cur.top >= sb.bottom || cur.bottom <= sb.top) continue;

                if (prev.right <= sb.left + kEps && cur.right >= sb.left - kEps &&
                    cur.right >= prev.right && velocity.x >= 0) {  // came from the left: wall on our right
                    dl.loc.x += sb.left - cur.right;
                    dc.onRightWall = 1;
                    if (mc != nullptr && mc->velocity.x > 0) mc->velocity.x = Bounce(mc->velocity.x, isStar);
                } else if (prev.left >= sb.right - kEps && cur.left <= sb.right + kEps &&
                           cur.left <= prev.left && velocity.x <= 0) {  // wall on our left
                    dl.loc.x += sb.right - cur.left;
                    dc.onLeftWall = 1;
                    if (mc != nullptr && mc->velocity.x < 0) mc->velocity.x = Bounce(mc->velocity.x, isStar);
                }
            }

            for (const auto& s : em) {
                Box sb;
                if (!GetStaticBox(em, s.GetID(), did, sb)) continue;
                Box cur = ToBox(dl.loc, dc.collisionBox);
                if (!Overlaps(cur, sb)) continue;
                const float left = cur.right - sb.left;
                const float right = sb.right - cur.left;
                const float top = cur.bottom - sb.top;
                const float bottom = sb.bottom - cur.top;
                const float overlap = std::min({left, right, top, bottom});
                if (overlap == top) {
                    dl.loc.y -= top;
                    dc.onGround = 1;
                    if (mc != nullptr && mc->velocity.y > 0)
                        mc->velocity.y = Bounce(mc->velocity.y, isStar);
                } else if (overlap == bottom) {
                    dl.loc.y += bottom;
                    dc.onCeiling = 1;
                    if (mc != nullptr && mc->velocity.y < 0)
                        mc->velocity.y = Bounce(mc->velocity.y, isStar);
                } else if (overlap == left) {
                    dl.loc.x -= left;
                    dc.onRightWall = 1;
                    if (mc != nullptr && mc->velocity.x > 0)
                        mc->velocity.x = Bounce(mc->velocity.x, isStar);
                } else {
                    dl.loc.x += right;
                    dc.onLeftWall = 1;
                    if (mc != nullptr && mc->velocity.x < 0)
                        mc->velocity.x = Bounce(mc->velocity.x, isStar);
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
        if (mDynamicContactHandler) {
            std::vector<Contact> contacts;
            for (auto first = em.cbegin(); first != em.cend(); ++first) {
                Box a;
                if (!GetDynamicBox(em, first->GetID(), a)) continue;
                for (auto second = std::next(first); second != em.cend(); ++second) {
                    Box b;
                    if (GetDynamicBox(em, second->GetID(), b) && Overlaps(a, b)) {
                        contacts.push_back({first->GetID(), second->GetID(),
                            em.GetGeneration(first->GetID()), em.GetGeneration(second->GetID())});
                    }
                }
            }
            for (const auto& [first, second, firstGeneration, secondGeneration] : contacts) {
                Box a, b;
                if (GetDynamicBox(em, first, a) && GetDynamicBox(em, second, b) &&
                    em.GetGeneration(first) == firstGeneration &&
                    em.GetGeneration(second) == secondGeneration &&
                    Overlaps(a, b)) {
                    mDynamicContactHandler->OnContact(engine, first, second);
                }
            }
        }
        return kContinue;
    }

private:
    std::unique_ptr<ContactHandler> mDynamicContactHandler;
    struct Contact {
        EntityID first, second;
        size_t firstGeneration, secondGeneration;
    };
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
        if (!sc.staticEntity || sc.collisionBox.width <= 0 || sc.collisionBox.height <= 0)
            return false;
        const LocationComponent& sl = em.template GetComponent<LocationComponent>(sid);
        out = ToBox(sl.loc, sc.collisionBox);
        return true;
    }

    static bool GetDynamicBox(EntityManager<Components...>& em, EntityID id, Box& out) {
        if (!em.IsValid(id) || !em.template HasComponent<LocationComponent>(id) ||
            !em.template HasComponent<CollisionComponent>(id)) return false;
        const auto& collision = em.template GetComponent<CollisionComponent>(id);
        if (collision.staticEntity || collision.collisionBox.width <= 0 ||
            collision.collisionBox.height <= 0) return false;
        out = ToBox(em.template GetComponent<LocationComponent>(id).loc, collision.collisionBox);
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
