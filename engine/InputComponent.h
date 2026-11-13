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

struct InputComponent {
    InputComponent(EntityID whichEntity = std::numeric_limits<EntityID>::max())
        : entity(whichEntity), lastDash(0) {}
    EntityID entity;
    int lastDash;
    std::bitset<Key::ScanCount> keydown;
    std::bitset<Key::ScanCount> lastKeydown;
};

}  // namespace ECSEngine
