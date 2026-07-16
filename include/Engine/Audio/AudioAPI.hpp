#pragma once

#include <string>

namespace VECTOR {

    class AudioAPI {
    public:
        enum class API {
            None = 0, SDL_Mixer = 1, XAudio2 = 2
        };

        virtual ~AudioAPI() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void PlaySound(const std::string& filepath) = 0;
        virtual void PlayMusic(const std::string& filepath, int loops = -1) = 0;
        virtual void StopMusic() = 0;
        virtual void SetMusicVolume(float volume) = 0;

        inline static API GetAPI() { return s_API; }
        static void SetAPI(API api) { s_API = api; }

        static AudioAPI* Create();

    private:
        static API s_API;
    };

} // namespace VECTOR
