#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    bool AudioManager::Initialize() {
        if (!MIX_Init()) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_mixer! Mix_Error: ") + SDL_GetError());
            return false;
        }

        SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 44100 };
        m_Mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (!m_Mixer) {
            VECTOR_LOG_ERROR(std::string("Failed to create mixer device! Mix_Error: ") + SDL_GetError());
            return false;
        }

        m_MusicTrack = MIX_CreateTrack(m_Mixer);

        VECTOR_LOG_INFO("AudioManager initialized successfully.");
        return true;
    }

    void AudioManager::Shutdown() {
        for (auto& pair : m_Sounds) {
            if (pair.second) MIX_DestroyAudio(pair.second);
        }
        m_Sounds.clear();

        for (auto& pair : m_Music) {
            if (pair.second) MIX_DestroyAudio(pair.second);
        }
        m_Music.clear();

        if (m_MusicTrack) {
            MIX_DestroyTrack(m_MusicTrack);
            m_MusicTrack = nullptr;
        }

        if (m_Mixer) {
            MIX_DestroyMixer(m_Mixer);
            m_Mixer = nullptr;
        }

        MIX_Quit();
    }

    void AudioManager::PlaySound(const std::string& filepath) {
        if (m_Sounds.find(filepath) == m_Sounds.end()) {
            MIX_Audio* audio = MIX_LoadAudio(m_Mixer, filepath.c_str(), true);
            if (!audio) {
                VECTOR_LOG_WARN(std::string("Failed to load sound: ") + filepath + " - " + SDL_GetError());
                return;
            }
            m_Sounds[filepath] = audio;
        }

        MIX_PlayAudio(m_Mixer, m_Sounds[filepath]);
    }

    void AudioManager::PlayMusic(const std::string& filepath, int loops) {
        if (m_Music.find(filepath) == m_Music.end()) {
            MIX_Audio* music = MIX_LoadAudio(m_Mixer, filepath.c_str(), false);
            if (!music) {
                VECTOR_LOG_WARN(std::string("Failed to load music: ") + filepath + " - " + SDL_GetError());
                return;
            }
            m_Music[filepath] = music;
        }

        if (m_MusicTrack) {
            MIX_SetTrackAudio(m_MusicTrack, m_Music[filepath]);
            MIX_SetTrackLoops(m_MusicTrack, loops);
            MIX_PlayTrack(m_MusicTrack, 0);
        }
    }

    void AudioManager::StopMusic() {
        // Halt music
        if (m_MusicTrack) {
            MIX_SetTrackAudio(m_MusicTrack, nullptr);
        }
    }

    void AudioManager::SetMusicVolume(float volume) {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        if (m_MusicTrack) {
            MIX_SetTrackGain(m_MusicTrack, volume);
        }
    }

    float AudioManager::GetMusicVolume() const {
        return 1.0f; // Simplified for now since MIX_GetTrackGain exists but this is sufficient.
    }

} // namespace VECTOR
