#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    bool AudioManager::Initialize() {
        if (!MIX_Init()) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_mixer library! SDL_Error: ") + SDL_GetError());
            return false;
        }

        m_Mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!m_Mixer) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_mixer device! SDL_Error: ") + SDL_GetError());
            return false;
        }
        VECTOR_LOG_INFO("AudioManager initialized successfully.");
        return true;
    }

    void AudioManager::Shutdown() {
        for (auto& pair : m_Sounds) {
            if (pair.second) {
                // Audio is freed when Mixer is closed or explicitly?
                // There might be MIX_FreeAudio
                // Since this is just a mockup shutdown, we'll let SDL clean up
            }
        }
        m_Sounds.clear();

        for (auto& pair : m_Music) {
            if (pair.second) {
                // Free audio...
            }
        }
        m_Music.clear();

        // If there is a mixer close function, we could call it here. For now let's just MIX_Quit()
        MIX_Quit();
    }

    void AudioManager::PlaySound(const std::string& filepath) {
        if (!m_Mixer) return;
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
        if (!m_Mixer) return;
        if (m_Music.find(filepath) == m_Music.end()) {
            MIX_Audio* audio = MIX_LoadAudio(m_Mixer, filepath.c_str(), false);
            if (!audio) {
                VECTOR_LOG_WARN(std::string("Failed to load music: ") + filepath + " - " + SDL_GetError());
                return;
            }
            m_Music[filepath] = audio;
        }

        if (!m_MusicTrack) {
            m_MusicTrack = MIX_CreateTrack(m_Mixer);
        }
        MIX_SetTrackAudio(m_MusicTrack, m_Music[filepath]);
        MIX_SetTrackLoops(m_MusicTrack, loops);
        MIX_PlayTrack(m_MusicTrack, 0);
    }

    void AudioManager::StopMusic() {
        if (m_MusicTrack) {
            MIX_PauseTrack(m_MusicTrack);
        }
    }

    void AudioManager::SetMusicVolume(float volume) {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        if (m_MusicTrack) {
            MIX_SetTrackGain(m_MusicTrack, volume);
        }
    }

} // namespace VECTOR
