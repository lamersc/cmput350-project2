#include "TileMap.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace ECSEngine {
namespace {

bool ReadLine(std::istream& input, std::string& line) {
    if (!std::getline(input, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}

bool AtEnd(std::istringstream& line) {
    line >> std::ws;
    return line.eof();
}

bool ReadRect(std::istringstream& line, Rect& rect) {
    return static_cast<bool>(line >> rect.topLeft.x >> rect.topLeft.y >> rect.width >> rect.height)
        && std::isfinite(rect.topLeft.x) && std::isfinite(rect.topLeft.y)
        && std::isfinite(rect.width) && std::isfinite(rect.height)
        && rect.width >= 0 && rect.height >= 0;
}

}

TileMap ParseTileMap(std::istream& input) {
    TileMap result;
    std::string line;
    do {
        if (!ReadLine(input, line)) throw std::runtime_error("Missing tile dictionary");
    } while (line.find_first_not_of(" \t") == std::string::npos);
    std::istringstream header(line);
    std::string word;
    if (!(header >> word >> result.dictionaryType) || word != "dictionary" ||
        (result.dictionaryType != 1 && result.dictionaryType != 2) || !AtEnd(header)) {
        throw std::runtime_error("Expected dictionary 1 or dictionary 2");
    }

    bool foundMap = false;
    while (ReadLine(input, line)) {
        if (line.find_first_not_of(" \t") == std::string::npos) continue;
        std::istringstream entry(line);
        entry >> word;
        if (word == "map") {
            std::string origin, tile, size;
            long long width, height;
            if (!(entry >> origin >> result.origin.x >> result.origin.y >> tile
                        >> result.tileSize.x >> result.tileSize.y >> size >> width >> height) ||
                origin != "origin" || tile != "tile" || size != "size" ||
                !std::isfinite(result.origin.x) || !std::isfinite(result.origin.y) ||
                !std::isfinite(result.tileSize.x) || !std::isfinite(result.tileSize.y) ||
                result.tileSize.x <= 0 || result.tileSize.y <= 0 ||
                width <= 0 || height <= 0 || !AtEnd(entry)) {
                throw std::runtime_error("Invalid tile map dimensions or origin");
            }
            result.width = static_cast<size_t>(width);
            result.height = static_cast<size_t>(height);
            foundMap = true;
            break;
        }
        TileDefinition definition;
        if (word.size() != 1 || !(entry >> definition.texturePath) ||
            !ReadRect(entry, definition.sourceRect) ||
            definition.sourceRect.width <= 0 || definition.sourceRect.height <= 0 ||
            (result.dictionaryType == 2 && !ReadRect(entry, definition.collisionBox)) ||
            !AtEnd(entry)) {
            throw std::runtime_error("Invalid tile dictionary entry");
        }
        if (!result.dictionary.emplace(word[0], std::move(definition)).second) {
            throw std::runtime_error("Duplicate tile dictionary character");
        }
    }
    if (!foundMap) throw std::runtime_error("Missing tile map header");
    for (size_t row = 0; row < result.height; ++row) {
        if (!ReadLine(input, line) || line.size() != result.width) {
            throw std::runtime_error("Tile map row does not match declared size");
        }
        result.rows.push_back(line);
    }
    while (ReadLine(input, line)) {
        if (line.find_first_not_of(" \t") != std::string::npos) {
            throw std::runtime_error("Unexpected data after tile map");
        }
    }
    if (input.bad()) throw std::runtime_error("Could not read tile map");
    return result;
}

}
