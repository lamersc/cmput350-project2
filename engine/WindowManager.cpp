#include "WindowManager.h"

#include <algorithm>
#include <cassert>

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)),
      mWorldCenter(width / 2.f, height / 2.f), mWorldUnitsPerPixel(1.f) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    auto size = mWindow->getSize();
    return Rect{{0, 0}, static_cast<float>(size.x), static_cast<float>(size.y)};
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screen = WorldToWindow(r);
    sf::RectangleShape shape({screen.width, screen.height});
    shape.setPosition({screen.topLeft.x, screen.topLeft.y});
    shape.setFillColor(sf::Color(red, green, blue, alpha));
    mWindow->draw(shape);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect screen = WorldToWindow(r);
    sf::RectangleShape shape({screen.width, screen.height});
    shape.setPosition({screen.topLeft.x, screen.topLeft.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(red, green, blue, alpha));
    shape.setOutlineThickness(-1.f);
    mWindow->draw(shape);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const {
    return Rect{WindowToWorld(rect.topLeft), WindowToWorld(rect.width), WindowToWorld(rect.height)};
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    Rect bounds = GetWindowBounds();
    return Point2D(mWorldCenter.x + WindowToWorld(pt.x - bounds.width / 2.f),
                   mWorldCenter.y + WindowToWorld(pt.y - bounds.height / 2.f));
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    return Rect{WorldToWindow(rect.topLeft), WorldToWindow(rect.width), WorldToWindow(rect.height)};
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    Rect bounds = GetWindowBounds();
    return Point2D(bounds.width / 2.f + WorldToWindow(pt.x - mWorldCenter.x),
                   bounds.height / 2.f + WorldToWindow(pt.y - mWorldCenter.y));
}

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    Rect bounds = GetWindowBounds();
    mWorldCenter = Point2D(worldPt.x - WindowToWorld(screenPt.x - bounds.width / 2.f),
                           worldPt.y - WindowToWorld(screenPt.y - bounds.height / 2.f));
}

Point2D WindowManager::GetWorldCenter() const { return mWorldCenter; }

float WindowManager::GetWorldScale() const { return mWorldUnitsPerPixel; }

void WindowManager::SetWorldCenter(const Point2D& worldCenter) { mWorldCenter = worldCenter; }

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    assert(worldUnitsPerPixel > 0);
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    Rect bounds = GetWindowBounds();
    assert(bounds.width > 0 && bounds.height > 0);
    assert(worldCameraRect.width > 0 && worldCameraRect.height > 0);
    SetWorldScale(std::max(worldCameraRect.width / bounds.width,
                          worldCameraRect.height / bounds.height));
    SetWorldCenter(Point2D(worldCameraRect.topLeft.x + worldCameraRect.width / 2.f,
                           worldCameraRect.topLeft.y + worldCameraRect.height / 2.f));
}

float WindowManager::WindowToWorld(float x) const { return x * mWorldUnitsPerPixel; }

float WindowManager::WorldToWindow(float x) const { return x / mWorldUnitsPerPixel; }

}  // namespace ECSEngine
