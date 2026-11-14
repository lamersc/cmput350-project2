#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"

namespace ECSEngine {

/** Texture path, source rectangle and optional relative collision rectangle. */
struct TileDefinition {
    std::string texturePath;
    Rect sourceRect{};
    Rect collisionBox{};
};

/** Parsed dictionary, world origin, tile size and character rows of a map. */
struct TileMap {
    int dictionaryType = 0;
    std::unordered_map<char, TileDefinition> dictionary;
    Point2D origin;
    Point2D tileSize;
    size_t width = 0;
    size_t height = 0;
    std::vector<std::string> rows;
};

/**
 * Read a dictionary 1 or 2 tile map from the borrowed stream. Dictionary 1
 * entries have sprite rectangles; dictionary 2 entries also have collision
 * rectangles. The returned map owns its data and does not retain the stream.
 * Malformed headers, entries, dimensions or rows throw std::runtime_error.
 */
TileMap ParseTileMap(std::istream& input);

}
