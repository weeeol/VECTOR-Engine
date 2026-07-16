#pragma once

#include "Engine/Audio/AudioAPI.hpp"

#ifdef _WIN32
#include <xaudio2.h>
#undef PlaySound
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace VECTOR {

    struct XAudio2Sound {
        WAVEFORMATEXTENSIBLE wfx;
        std::vector<uint8_t> audioData;
    };

    class XAudio2Backend : public AudioAPI {
    public:
        XAudio2Backend();
        ~XAudio2Backend() override;

        bool Initialize() override;
        void Shutdown() override;

        void PlaySound(const std::string& filepath) override;
        void PlayMusic(const std::string& filepath, int loops = -1) override;
        void StopMusic() override;
        void SetMusicVolume(float volume) override;

    private:
        bool LoadWAV(const std::string& filepath, XAudio2Sound& outSound);

        Microsoft::WRL::ComPtr<IXAudio2> m_XAudio2;
        IXAudio2MasteringVoice* m_MasterVoice = nullptr;
        
        IXAudio2SourceVoice* m_MusicVoice = nullptr;

        std::unordered_map<std::string, XAudio2Sound> m_Sounds;
        
        float m_MusicVolume = 1.0f;
    };

} // namespace VECTOR
#endif
