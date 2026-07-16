#pragma once

#include "Engine/Audio/AudioAPI.hpp"
#include <string>
#include <memory>

namespace VECTOR {

    class AudioManager {
    public:
        static AudioManager& Get() {
            static AudioManager instance;
            return instance;
        }

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        bool Initialize();
        void Shutdown();

        void PlaySound(const std::string& filepath);
        void PlayMusic(const std::string& filepath, int loops = -1);
        void StopMusic();
        void SetMusicVolume(float volume);

    private:
        AudioManager() = default;
        ~AudioManager() = default;

        std::unique_ptr<AudioAPI> m_API;
    };

} // namespace VECTOR
