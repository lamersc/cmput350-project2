#pragma once

#include <bitset>
#include <limits>
#include <unordered_map>

#include "MathUtil.h"
#include "Entity.h"
#include "Keyboard.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

static_assert(Key::ScanCount == sf::Keyboard::ScancodeCount);

/**
 * Input state for an entity. keydown and lastKeydown use Key scancode indices;
 * the event loop saves the previous state before processing new events.
 * lastDash counts frames since the last dash, up to the configured interval.
 */
struct InputComponent {
    InputComponent(EntityID whichEntity = std::numeric_limits<EntityID>::max())
        : entity(whichEntity), lastDash(0) {}
    EntityID entity;
    int lastDash;
    std::bitset<Key::ScanCount> keydown;
    std::bitset<Key::ScanCount> lastKeydown;
};

}  // namespace ECSEngine
