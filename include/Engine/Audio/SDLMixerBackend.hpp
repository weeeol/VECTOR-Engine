#pragma once

#include "Engine/Audio/AudioAPI.hpp"
#include <SDL_mixer.h>
#include <unordered_map>

namespace VECTOR {

    class SDLMixerBackend : public AudioAPI {
    public:
        SDLMixerBackend() = default;
        ~SDLMixerBackend() override = default;

        bool Initialize() override;
        void Shutdown() override;

        void PlaySound(const std::string& filepath) override;
        void PlayMusic(const std::string& filepath, int loops = -1) override;
        void StopMusic() override;
        void SetMusicVolume(float volume) override;

    private:
        std::unordered_map<std::string, Mix_Chunk*> m_Sounds;
        std::unordered_map<std::string, Mix_Music*> m_Music;
    };

} // namespace VECTOR
