#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"

namespace ECSEngine {

struct TileDefinition {
    std::string texturePath;
    Rect sourceRect{};
    Rect collisionBox{};
};

struct TileMap {
    int dictionaryType = 0;
    std::unordered_map<char, TileDefinition> dictionary;
    Point2D origin;
    Point2D tileSize;
    size_t width = 0;
    size_t height = 0;
    std::vector<std::string> rows;
};

TileMap ParseTileMap(std::istream& input);

}
