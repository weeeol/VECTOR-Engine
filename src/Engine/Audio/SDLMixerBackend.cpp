#include "Engine/Audio/SDLMixerBackend.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    bool SDLMixerBackend::Initialize() {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_mixer! Mix_Error: ") + Mix_GetError());
            return false;
        }
        VECTOR_LOG_INFO("SDLMixerBackend initialized successfully.");
        return true;
    }

    void SDLMixerBackend::Shutdown() {
        for (auto& pair : m_Sounds) {
            if (pair.second) Mix_FreeChunk(pair.second);
        }
        m_Sounds.clear();

        for (auto& pair : m_Music) {
            if (pair.second) Mix_FreeMusic(pair.second);
        }
        m_Music.clear();

        Mix_Quit();
    }

    void SDLMixerBackend::PlaySound(const std::string& filepath) {
        if (m_Sounds.find(filepath) == m_Sounds.end()) {
            Mix_Chunk* chunk = Mix_LoadWAV(filepath.c_str());
            if (!chunk) {
                VECTOR_LOG_WARN(std::string("Failed to load sound: ") + filepath + " - " + Mix_GetError());
                return;
            }
            m_Sounds[filepath] = chunk;
        }

        Mix_PlayChannel(-1, m_Sounds[filepath], 0);
    }

    void SDLMixerBackend::PlayMusic(const std::string& filepath, int loops) {
        if (m_Music.find(filepath) == m_Music.end()) {
            Mix_Music* music = Mix_LoadMUS(filepath.c_str());
            if (!music) {
                VECTOR_LOG_WARN(std::string("Failed to load music: ") + filepath + " - " + Mix_GetError());
                return;
            }
            m_Music[filepath] = music;
        }

        Mix_PlayMusic(m_Music[filepath], loops);
    }

    void SDLMixerBackend::StopMusic() {
        Mix_HaltMusic();
    }

    void SDLMixerBackend::SetMusicVolume(float volume) {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        Mix_VolumeMusic((int)(volume * MIX_MAX_VOLUME));
    }

} // namespace VECTOR
