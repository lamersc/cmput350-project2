#pragma once

#include <memory>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

/**
 * Owns an SFML render window and the world-to-window camera transform.
 * Coordinates are expressed as world units or window pixels as indicated below;
 * the scale is world units per pixel.
 */
class WindowManager {
public:
    /** Open a window of the given pixel dimensions and title. */
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    ~WindowManager() = default;

    /**
     * Return shared ownership of the window. A copied shared_ptr can outlive the
     * manager, although ECSEngine::Run closes the window when its loop ends.
     */
    std::shared_ptr<sf::RenderWindow> GetWindow() const;

    /** Return the current window rectangle in pixels, with origin (0, 0). */
    Rect GetWindowBounds() const;

    /** Position the camera so worldPt appears at screenPt, in window pixels. */
    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);

    /** Set the world-space point at the center of the window. */
    void SetWorldCenter(const Point2D& worldCenter);

    /**
     * Center on a positive-size world rectangle and use a uniform scale large
     * enough to contain it. The window must also have positive dimensions.
     */
    void SetWorldRect(const Rect& worldCameraRect);

    /** Set a positive world-units-per-pixel scale; debug builds assert if not positive. */
    void SetWorldScale(float worldUnitsPerPixel);

    /** Return the camera center in world coordinates. */
    Point2D GetWorldCenter() const;

    /** Return the current world-units-per-pixel scale. */
    float GetWorldScale() const;

    /** Convert a pixel distance to a world-space distance, without translation. */
    float WindowToWorld(float x) const;

    /** Convert a world-space distance to a pixel distance, without translation. */
    float WorldToWindow(float x) const;

    /** Convert a window-space rectangle, including its position, to world space. */
    Rect WindowToWorld(const Rect& rect) const;

    /** Convert a world-space rectangle, including its position, to window space. */
    Rect WorldToWindow(const Rect& rect) const;

    /** Convert a world-space point to a window-space point in pixels. */
    Point2D WorldToWindow(const Point2D& pt) const;

    /** Convert a window-space point in pixels to a world-space point. */
    Point2D WindowToWorld(const Point2D& pt) const;

    /** Draw a filled world-space rectangle using RGBA color components (0-255). */
    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);

    /** Draw the outline of a world-space rectangle using RGBA components (0-255). */
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    Point2D mWorldCenter;
    float mWorldUnitsPerPixel;
};

}  // namespace ECSEngine
