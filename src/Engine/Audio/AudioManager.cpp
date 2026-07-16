#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    bool AudioManager::Initialize() {
        m_API.reset(AudioAPI::Create());
        if (m_API) {
            return m_API->Initialize();
        }
        return false;
    }

    void AudioManager::Shutdown() {
        if (m_API) {
            m_API->Shutdown();
            m_API.reset();
        }
    }

    void AudioManager::PlaySound(const std::string& filepath) {
        if (m_API) m_API->PlaySound(filepath);
    }

    void AudioManager::PlayMusic(const std::string& filepath, int loops) {
        if (m_API) m_API->PlayMusic(filepath, loops);
    }

    void AudioManager::StopMusic() {
        if (m_API) m_API->StopMusic();
    }

    void AudioManager::SetMusicVolume(float volume) {
        if (m_API) m_API->SetMusicVolume(volume);
    }

} // namespace VECTOR
