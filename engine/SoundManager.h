#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

/**
 * Owns named sound buffers and sounds. A sound keeps a reference to its buffer,
 * so the manager cannot be copied and must outlive references to its sounds.
 */
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /**
     * Load a sound file and register it under soundName. Reusing a name stops
     * that sound and replaces its buffer. Relative paths use the working
     * directory; a load failure throws std::runtime_error.
     */
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    /**
     * Return the named, manager-owned sound; an unknown name throws
     * std::out_of_range. The reference remains valid until manager destruction,
     * including across subsequent registrations, but is not owned by the caller.
     */
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, sf::SoundBuffer> mBuffers;
    std::unordered_map<std::string, sf::Sound> mSounds;
};

}  // namespace ECSEngine
