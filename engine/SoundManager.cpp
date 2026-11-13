#include "SoundManager.h"

#include <stdexcept>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    sf::SoundBuffer loaded;
    if (!loaded.loadFromFile(soundPath)) {
        throw std::runtime_error("Failed to load sound: " + soundPath);
    }
    auto existing = mSounds.find(soundName);
    if (existing != mSounds.end()) existing->second.stop();
    auto [buffer, inserted] = mBuffers.try_emplace(soundName, loaded);
    if (!inserted) buffer->second = loaded;
    auto [sound, created] = mSounds.try_emplace(soundName, buffer->second);
    if (!created) sound->second.setBuffer(buffer->second);
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    return mSounds.at(soundName);
}

}  // namespace ECSEngine
