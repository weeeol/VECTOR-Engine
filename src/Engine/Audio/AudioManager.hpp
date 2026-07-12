#pragma once

#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

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

    private:
        AudioManager() = default;
        ~AudioManager() = default;

        std::unordered_map<std::string, Mix_Chunk*> m_Sounds;
        std::unordered_map<std::string, Mix_Music*> m_Music;
    };

} // namespace VECTOR
