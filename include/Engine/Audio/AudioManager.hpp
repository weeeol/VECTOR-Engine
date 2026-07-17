#pragma once

#include <SDL3_mixer/SDL_mixer.h>
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
        void SetMusicVolume(float volume);

    private:
        AudioManager() = default;
        ~AudioManager() = default;

        MIX_Mixer* m_Mixer = nullptr;
        MIX_Track* m_MusicTrack = nullptr;
        std::unordered_map<std::string, MIX_Audio*> m_Sounds;
        std::unordered_map<std::string, MIX_Audio*> m_Music;
    };

} // namespace VECTOR
